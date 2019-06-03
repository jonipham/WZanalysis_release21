#! /usr/bin/env python
from __future__ import print_function
from XAMPPbase.runAthena import AssembleRemoteRunCmd, BringToAthenaStyle
from XAMPPbase.AthArgParserSetup import attachArgs
from ClusterSubmission.Utils import ResolvePath, CheckPandaSetup, CheckRemainingProxyTime, prettyPrint
from ClusterSubmission.ClusterEngine import TESTAREA, ATLASPROJECT, ATLASVERSION
import argparse, os, time, ROOT, sys, commands


def exclusiveGridOpts():
    """
    @brief      Return argument parser with additional arguments for running on
                the grid.
    
    @return     argument parser with additional arguments for grid running, does
                not yet contain the required options of the AthArgParser which
                need to be added separately
    """
    parser = argparse.ArgumentParser(
        description='This script submits the analysis code to the grid. For more help type \"python XAMPPbase/scripts/SubmitToGrid.py -h\"',
        prog='SubmitToGrid',
        conflict_handler='resolve',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    RSE = '-1' if not os.getenv("RUCIO_RSE") else os.getenv("RUCIO_RSE")
    parser.add_argument('--Test', help='run in test modus, i.e. without DS replication', action='store_true', default=False)
    parser.add_argument('--DSCampaign',
                        help='choose campaign of DS production in order to adjust the filepath to the DS list (default is \"mc16_13TeV\")',
                        default='mc16_13TeV')
    parser.add_argument('--destSE',
                        help='specify a destination for replication apart from to %s (or \"-1\" for no replication)' % RSE,
                        default='%s' % RSE)
    parser.add_argument('--DuplicateTask',
                        help='You can create another job with the same output dataset...',
                        action='store_true',
                        default=False)
    parser.add_argument(
        '--nFilesPerJob',
        help=
        'You can specify the number of files per job... If you have data then it would be may be better to give a larger number... Default: 10',
        type=int,
        default=10)
    parser.add_argument('--nJobs', help='Specifiy the maximum number of jobs you want to run ', type=int, default=0)

    parser.add_argument(
        '--outDS',
        help=
        'Specify a name which overwrites the naming scheme for the outDS used by this script internally (only the \"user.<username>.\" remains, the string given here gets appended and nothing more',
        required=True)
    parser.add_argument('--productionRole', help='specify optionally a production role you have e.g. perf-muons', default='')

    parser.add_argument('--inputDS',
                        '-i',
                        help='input dataset or DS list present in \"XAMPPbase/data/<DSCampaign>/SampleLists/\"',
                        required=True)
    parser.add_argument('--noAmiCheck',
                        help='disables the check of the existence of the input datasets.',
                        action="store_true",
                        default=False)
    parser.add_argument('--newTarBall',
                        help='The current TarBall gets deleted before submission and a new one will be created',
                        action='store_true',
                        default=False)
    return parser


def setupGridSubmitArgParser():
    """
    @brief      Set up argument parser with special grid options in addition to
                the default methods of the AthArgParser
    
    @return     Argument parser with extra options for running on the grid
    """
    parser = exclusiveGridOpts()
    attachArgs(parser)
    return parser


def DoesDSExists(ds):
    """
    @brief      Check with rucio if given dataset exists
    
    @param      ds    Dataset of which the existence should be checked using
                      rucio
    
    @return     True if at least one container with the dataset name exists
                according to rucio and is not empty
    """
    if os.system("rucio list-dids %s --filter type=CONTAINER --short" % (ds.strip().replace("/", ""))) != 0:
        return False
    return GetNFiles(ds) > 0


def GetNFiles(ds):
    """
    @brief      Get the number of files associated to a dataset with rucio
    
    @param      ds    Dataset for which the files should be retrieved
    
    @return     The number of files associated to the dataset according to rucio
    """
    Cmd = "rucio list-files %s --csv" % (ds)
    N_Files = commands.getoutput(Cmd)
    if len(N_Files) == 0: return 0
    return len(N_Files.split("\n"))


def getTarBallOptions(renew=False):
    """
    @brief      Gets the tar ball options for creating a TarBall
                (https://en.wikipedia.org/wiki/Tar_(computing)) when submitting
                an athena job over the grid. This option saves a gzipped tarball
                of local files which is used as the build of the code for
                running the job. The TarBall.tgz file is saved in your TestArea.
                If there is no TarBall.tgz file, a new one is built. If there is
                already a TarBall file, the existing TarBall is used for
                submitting the job. This means: if you change your code after
                you built your TarBall (even if you recompile) but there is
                still the old TarBall.tzg file in your TestArea, your changes
                won't have effect since the old TarBall is still used. You
                either have to delete the TarBall manually or use the renew
                option.
    
    @param      renew  Create a new TarBall even if an old one exists and
                       overwrite the old one.
    
    @return     String with command line option for athena specifying the use of
                the TarBall
    """
    TarList = []
    os.chdir(TESTAREA)
    if renew and os.path.isfile(TESTAREA + "/TarBall.tgz"):
        prettyPrint("Delete exisiting TarBall", TESTAREA + "/TarBall.tgz")
        os.system("rm %s/TarBall.tgz" % (TESTAREA))
    CREATETARBALL = (os.path.isfile(TESTAREA + "/TarBall.tgz") == False)
    if CREATETARBALL:
        TarList = [
            "--extFile=*.root", "--outTarBall=TarBall.tgz",
            "--excludeFile=\"*.svn*\",\"*.git*\",\"*.pyc\",\"*.*~\",\"*.tex\",\"*.tmp\",\"*.pdf\",\"*.png\",\"*.log\",\"*.dat\",\"*.core\",\"*README*\",\"XAMPPplotting/data/*\""
        ]
    else:
        TarList = ["--inTarBall=TarBall.tgz"]
    return TarList


def SubmitJobs(RunOptions, AnalysisOptions):
    """
    @brief      Submit grid jobs using pathena with XAMPP
    
    @param      RunOptions       The run options
    @param      AnalysisOptions  The analysis options
    
    """
    CheckPandaSetup()

    RUCIO_ACC = os.getenv("RUCIO_ACCOUNT")
    os.chdir(TESTAREA)

    PRUN_Options = []

    PRUN_Options += getTarBallOptions(renew=RunOptions.newTarBall)
    InDSList = RunOptions.inputDS

    GroupDisk = RunOptions.destSE != '-1'
    Duplicate = RunOptions.DuplicateTask

    InputDataSets = []
    FilesPerJob = RunOptions.nFilesPerJob
    nJobs = RunOptions.nJobs
    OutFiles = [RunOptions.outFile]

    #################################################
    # Assemble the dataset list
    #################################################

    #The path is directly given
    if os.path.exists(InDSList):
        InputDataSets += ReadListFromFile(InDSList)
        # if not, it should be a DS or a comma-separated list of DS
    elif ',' in InDSList:
        InputDataSets += InDSList.split(",")

        #Assume there is only one DS given to the script
    else:
        InputDataSets.append(InDSList)

    ###############################################
    #   No dataset could be extracted from the list
    ###############################################
    if len(InputDataSets) == 0:
        prettyPrint('ERROR', 'No input dataset found')
        exit(1)

    if RunOptions.noAmiCheck:
        print('WARNIG: The check of the existence of the inputDS is disabled, which should be fine.')
    else:
        sys.stdout.write('Checking in rucio if dataset(s) is exist(s)..')
        sys.stdout.flush()
        DSCandidates = InputDataSets
        InputDataSets = []
        FoundAll = True
        for D in DSCandidates:
            sys.stdout.write('.')
            sys.stdout.flush()
            D = D.replace("/", "").strip()
            if not DoesDSExists(D):
                print('ERROR: The input DS %s is not known!' % D)
                FoundAll = False
            else:
                InputDataSets.append(D)
        if FoundAll: print(' Successful!')

    if len(InputDataSets) == 0:
        print('ERROR: There are no valid input DS')
        exit(1)

    PRUN_Options.append("--inDS=\"%s\"" % ",".join(InputDataSets))

    # Sets the OutputDataSetName
    Scope = "user.%s" % (RUCIO_ACC) if len(RunOptions.productionRole) == 0 else "group.%s" % (RunOptions.productionRole)
    OutDS = "%s.%s" % (Scope, RunOptions.outDS)
    if len(RunOptions.productionRole) > 0:
        PRUN_Options += ["--official", "--voms=atlas:/atlas/%s/Role=production" % (RunOptions.productionRole)]
    PRUN_Options.append('--outDS=\"%s\"' % OutDS.replace('\r', ''))
    PRUN_Options.append("--express")
    PRUN_Options.append("--useShortLivedReplicas")

    # Additional Options parsing to the prun Command
    if RunOptions.Test == True:
        GroupDisk = False
        Duplicate = True
        FilesPerJob = 1
        nJobs = 1
        PRUN_Options.append("--nFiles=1")
        PRUN_Options.append("--disableAutoRetry")
    if GroupDisk: PRUN_Options.append("--destSE=%s" % RunOptions.destSE)
    if Duplicate: PRUN_Options.append("--allowTaskDuplication")
    if nJobs > 0: PRUN_Options.append("--nJobs=%d " % nJobs)

    PRUN_Options.append("--nFilesPerJob=%i" % FilesPerJob)
    #PRUN_Options.append("--useNewCode")
    PRUN_Options.append("--mergeOutput")
    print(
        "################################################################################################################################################################"
    )
    print("                                                                        XAMPP on the grid")
    print(
        "################################################################################################################################################################"
    )
    prettyPrint('USER', os.getenv("USER"))
    prettyPrint('RUCIO', RUCIO_ACC)

    prettyPrint('WORKINGDIR', TESTAREA)
    prettyPrint('TODAY', time.strftime("%Y-%m-%d"))

    prettyPrint("ATLASPROJECT", ATLASPROJECT)
    prettyPrint("ATLASVERSION", ATLASVERSION)
    prettyPrint("TESTAREA", TESTAREA)
    print(
        "################################################################################################################################################################"
    )
    print("                                                                        JobOptions")
    print(
        "################################################################################################################################################################"
    )
    prettyPrint('InputDataSets:', '', separator='')
    for ds in InputDataSets:
        prettyPrint('', ds, width=32, separator='-')
    prettyPrint('FilesPerJob', str(FilesPerJob))
    if nJobs > 0: prettyPrint('NumberOfJobs', str(nJobs))

    prettyPrint('OutputContainer', OutDS)
    MakeTarBall = not os.path.isfile(TESTAREA + "/TarBall.tgz")
    prettyPrint('CreateTarBall', ("Yes" if MakeTarBall else "No"))
    if not MakeTarBall:
        prettyPrint('', 'Using already existing tarball located at:', width=32, separator='->')
        prettyPrint('', '%s/TarBall.tgz' % TESTAREA, width=34, separator='')

    prettyPrint('JobOptions', RunOptions.jobOptions)
    prettyPrint('RunWithSystematics', "Yes" if not RunOptions.noSyst else "No")

    print(
        "################################################################################################################################################################"
    )
    ### Tell athena that a grid-job is running -->
    ### each job has a single type of datasets
    Command = "pathena %s %s" % (BringToAthenaStyle(RunOptions.jobOptions), " ".join(PRUN_Options + AnalysisOptions))
    print('\nSubmitting using command:\n%s\n...' % Command)
    os.system(Command)


if __name__ == '__main__':
    Parser = setupGridSubmitArgParser()
    RunOptions = setupGridSubmitArgParser().parse_args()
    AnalysisOptions = AssembleRemoteRunCmd(RunOptions, Parser)
    SubmitJobs(RunOptions, AnalysisOptions)
