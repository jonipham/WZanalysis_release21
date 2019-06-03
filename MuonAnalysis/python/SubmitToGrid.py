#! /usr/bin/env python
from __future__ import print_function

from MuonAnalysis.runAthena import setupSUSYArgParser, AssembleRemoteRunCmd, AssembleConfigArgument
from ClusterSubmission.Utils import ResolvePath, CheckPandaSetup, CheckRemainingProxyTime, prettyPrint
from ClusterSubmission.ClusterEngine import TESTAREA, USERNAME, ATLASPROJECT, ATLASVERSION

import argparse, os, time
import ROOT
import sys
from pprint import pprint

import commands
import subprocess
import math
import sys
import re
from datetime import datetime

CheckPandaSetup()

USER = os.getenv("USER")
RUCIO_ACC = os.getenv("RUCIO_ACCOUNT")
TODAY= str(time.strftime("%d-%m-%Y"))
TIME = commands.getoutput("date +'%Y-%m-%d_%H-%M-%S'")
EXCLUDEDSITE='ANALY_VICTORIA'
ROOTCOREBIN = os.getenv("ROOTCOREBIN")

def ReadListFromDSFile(File, Campaign="data16_hip8TeV"):
    List = []
    if os.path.isfile(File) == True:
        for Line in open(File):
            if Line[0] is not "#" and Campaign in Line:
                List.append(Line.replace("\n", ""))
    else:
        print ("Could not find ListFile " + str(File))
    return List


def ConvertListToString(List , Sep=","):
    Str=""
    for E in List:
    	Str=E+Sep+Str
    return Str[0 : len(Str) -1 ]

def AddToAnalysisOption(Name, Opt):
    AnalysisOptions.append("--%s" % Name)
    AnalysisOptions.append(Opt)
    return StringToBool(Opt)


# add additional arguments to the argument parser
def setupGridSubmitArgParser():
    parser = argparse.ArgumentParser(
        description='This script submits the analysis code to the grid',
        prog='SubmitToGrid',
        conflict_handler='resolve',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser = setupSUSYArgParser(parser)
    parser.set_defaults(singlePRWperiodsOnly=True)
    RSE = '-1' if not os.getenv("RUCIO_RSE") else os.getenv("RUCIO_RSE")
    parser.add_argument('--Test', help='run in test modus, i.e. without DS replication', action='store_true', default=False)
    parser.add_argument('--debug',help='run in debug modus',action='store_true',default=False)
    parser.add_argument('--destSE', help='specify a destination for replication apart from to %s (or \"-1\" for no replication)' % RSE, default='%s' % RSE)
    parser.add_argument('--DuplicateTask', help='You can create another job with the same output dataset...', action='store_true', default=False)
    parser.add_argument('-a','--analysis',help='choose the analysis to run', default='HeavyIon')
    parser.add_argument('--FilesPerJob', help= 'You can specify the number of files per job... If you have data then it would be may be better to give a larger number... Default: 10', type=int, default=10)
    parser.add_argument('--nJobs', help='Specifiy the maximum number of jobs you want to run ', type=int, default=0)
    parser.add_argument('--exSite',help='blacklist some specific sites',action='store_true',default=False)
    parser.add_argument('-j','--jobName',help='specify a job name which gets included in the name of the output dataset (default is analysis name)')
    parser.add_argument('-o', '--outputSuffix', help='suffix name of the output file (default is \"AnalysisOutput.root\")', default='AnalysisOutput.root')

    #parser.add_argument('--productionRole', help='specify optionally a production role you have e.g. perf-muons', default='')
    parser.add_argument('--inputDS', '-i', help='input dataset', required=True)
    parser.add_argument('--noAmiCheck', help='disables the check of the existence of the input datasets.', action="store_true", default=False)
    parser.add_argument('--nevents',type=int,help="number of events to process for all the datasets")
    parser.add_argument('--newTarBall', help='The current TarBall gets deleted before submission and a new one will be created', action='store_true', default=False)
    parser.add_argument('--outDS', help='specify a name which overwrites the naming scheme for the outDS used by this script internally (only the \"user.<username>.\" remains, the string given here gets appended and nothing more', default='')
    parser.add_argument('--DSCampaign', help= 'choose campaign of DS production in order to adjust the filepath to the DS list (default is \"InputDataset\")', default='data16_hip8TeV')


    return parser

def DoesDSExists(ds):
    if os.system("rucio list-dids %s --filter type=CONTAINER --short" % (ds.strip().replace("/", ""))) != 0:
        return False
    return GetNFiles(ds) > 0

def GetNFiles(ds):
    Cmd = "rucio list-files %s --csv" % (ds)
    N_Files = commands.getoutput(Cmd)
    if len(N_Files) == 0: return 0
    return len(N_Files.split("\n"))

def getTarBallOptions(renew=False):
    TarList = []
    os.chdir(TESTAREA)
    if renew and os.path.isfile(TESTAREA + "/TarBall.tgz"):
        prettyPrint("Delete exisiting TarBall", TESTAREA + "/TarBall.tgz")
        os.system("rm %s/TarBall.tgz" % (TESTAREA))
    CREATETARBALL = (os.path.isfile(TESTAREA + "/TarBall.tgz") == False)
    if CREATETARBALL:
        TarList = [
            "--extFile=*.root", "--outTarBall=TarBall.tgz",
            "--excludeFile=\"*.svn*\",\"*.git*\",\"*.pyc\",\"*.*~\",\"*.tex\",\"*.tmp\",\"*.pdf\",\"*.png\",\"*.log\",\"*.dat\",\"*.core\",\"*README*\""
        ]
    else:
        TarList = ["--inTarBall=TarBall.tgz"]
    return TarList

def SubmitJobs(RunOptions, AnalysisOptions):
    os.chdir(TESTAREA)
    PRUN_Options = []
    PRUN_Options += getTarBallOptions(renew=RunOptions.newTarBall)
    InDSList = RunOptions.inputDS
    GroupDisk = RunOptions.destSE != '-1'
    Duplicate = RunOptions.DuplicateTask
    InputDataSets = []
    OutFiles = [RunOptions.outputSuffix]
    FilesPerJob = RunOptions.FilesPerJob
    nJobs = RunOptions.nJobs
    Campaign = RunOptions.DSCampaign




    #################################################
    # Assemble the datasets list
    #################################################
    InDSListDir = "/srv01/cgrp/users/jpham/Wanalysis_AthAnalysis21.2.69_withSUSYTools/data/%s/" % (Campaign)
    if os.path.exists(InDSListDir) == False:
        print ("ERROR: Campaign is invalid. Could not find the directory containing the sample lists.")
        exit(1)
    # check if inputDS was a file
    if os.path.exists(InDSListDir + InDSList):
        InputDataSets.extend(ReadListFromDSFile(InDSListDir + InDSList, Campaign))
    # if not, it should be a DS or a comma-separated list of DS
    else:
        if ',' in InDSList:
            for DS in InDSList.split(','):
                InputDataSets.append(DS)
        else:
            InputDataSets.append(InDSList)
    print (InputDataSets)

    #The path is directly given
    """if os.path.exists(InDSList):
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
        if FoundAll: print(' Successful!')"""

    if len(InputDataSets) == 0:
        print('ERROR: There are no valid input DS')
        #exit(1)

    #PRUN_Options.append("--inDS=\"%s\"" % ",".join(InputDataSets))

    #PRUN_Options.append("--inDS=\"%s\""%ConvertListToString(InputDataSets,","))

    # Sets the OutputDataSetName
    JobName = RunOptions.analysis
    if RunOptions.jobName != None and RunOptions.jobName != "":
    	JobName = RunOptions.jobName

    if len(JobName)==0:
        print ("ERROR: Please give a JobName")
        exit(1)

    """for dataset in InputDataSets:
	    DSID = dataset.split(".")[1].split(".")[0]
	    TAG = dataset.split(".")[-1].split("/")[0]
	    print ('submit of single DSID: %s with tag: %s'%(DSID,TAG))"""

    """if RunOptions.outDS == '':
        if len(InputDataSets) == 1:
            DSID = InputDataSets[0].split(".")[1].split(".")[0]
            TAG = InputDataSets[0].split(".")[-1].split("/")[0]
            print( 'submit of single DSID: %s with tag: %s' % (DSID, TAG))
            OutDS = "user.%s.%s.%s.%s.%s" % (RUCIO_ACC, DSID, TAG, TODAY,
                                                       JobName)
        else:
            OutDS = "user.%s.%s.%s" % (RUCIO_ACC, TODAY, JobName)
    else:
        OutDS = "user.%s.%s" % (RUCIO_ACC, RunOptions.outDS)
    PRUN_Options.append("--outDS=\"%s\"" % OutDS)"""


    # Additional Options parsing to the prun Command
    if RunOptions.Test == True:
        GroupDisk = False
        Duplicate = True
        FilesPerJob = 1
        nJobs = 1
        PRUN_Options.append("--nFiles=1")
        PRUN_Options.append("--disableAutoRetry")
    #if GroupDisk: RunOptions.append("--destSE=%s" % RunOptions.destSE)
    #if Duplicate: RunOptions.append("--allowTaskDuplication")
    #if nJobs > 0: RunOptions.append("--nJobs=%d " % nJobs)
    PRUN_Options.append("--mergeOutput")

    if RunOptions.exSite:
                PRUN_Options.append("--excludedSite=%s"%EXCLUDEDSITE)

    AnalysisOptions = []
    FilesPerJob=50
    if RunOptions.debug:
        PRUN_Options.append("--debug")
    if RunOptions.nevents :
        PRUN_Options.append("--evtMax %i"%RunOptions.nevents)
    if RunOptions.skipEvents :
        PRUN_Options.append("--skipEvents %i"%RunOptions.skipEvents)
    #PRUN_Options.append("--runModus grid")
    PRUN_Options.append("--nFilesPerJob=%i"%FilesPerJob)
    #PRUN_Options.append("--outputs=\"%s\""%ConvertListToString(OutFiles ,","))

    print(
        "################################################################################################################################################################"
    )
    print("                                                                        MuonAnalysis on the grid")
    print(
        "################################################################################################################################################################"
    )
    prettyPrint('USER', USERNAME)
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

    #prettyPrint('OutputContainer', OutDS)
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

    AnalysisOptions.reverse()


    #print('\nSubmitting using command:\n%s\n...' % Command)
    #print(str(" ".join(PRUN_Options))
    #print(str(AssembleConfigArgument(AnalysisOptions)))
    #print(str(RunOptions.jobOptions))
    #os.system(Command)
    for dataset in InputDataSets:
        DSID = dataset.split(".")[1].split(".")[0]
        TAG = dataset.split(".")[-1].split("/")[0]
        print ('DSID: %s with tag: %s'%(DSID,TAG))
        #OutDS = "user.%s.%s.%s.%s.%s"%(RUCIO_ACC,DSID,TAG,TODAY,JobName)
        print ("InputContainer:         "+dataset)
        OutDS = "user.%s.%s.%s.%s.%s" % (RUCIO_ACC, DSID, TAG, TODAY,JobName)
        #PRUN_Options.append("--outDS=\"%s\"" % OutDS)

        print ("OutputContainer:        "+OutDS)
	prettyPrint('OutputContainer', OutDS)
        #Command = "prun --exec=\"python XAMPPanalyses/python/runHeavyIon.py %s\" --inDS %s --outDS %s %s"%(ConvertListToString (AnalysisOptions, " ") , dataset, OutDS, ConvertListToString(PRUN_Options , " "))
	Command = ""
        Command = "pathena --inDS=%s --outDS=%s %s %s %s" % (dataset, OutDS, " ".join(PRUN_Options), AssembleConfigArgument(AnalysisOptions), RunOptions.jobOptions)
	#Command = "pathena %s %s %s %s" %(ConvertListToString (AnalysisOptions, " ") , dataset, OutDS, ConvertListToString(PRUN_Options , " "))
        print ('\nSubmitting using command:\n%s\n...'%Command)
        os.system(Command)
    exit(0)


if __name__ == '__main__':
    Parser = setupGridSubmitArgParser()
    RunOptions = setupGridSubmitArgParser().parse_args()
    AnalysisOptions = AssembleRemoteRunCmd(RunOptions, Parser)
    SubmitJobs(RunOptions, AnalysisOptions)
