import os, argparse, time
from ClusterSubmission.Utils import CreateDirectory, ResolvePath, IsROOTFile, ReadListFromFile
from ClusterSubmission.ClusterEngine import ATLASPROJECT
from XAMPPbase.Utils import IsArgumentDefault
if not ATLASPROJECT:
    print("ERROR: This module is only available in an Athena based release")
    exit(1)


def BringToAthenaStyle(Argument):
    """
    @brief      Modifies an argument to satisfy the athena conventions.
    
    @param      Argument  The argument to be modified
    
    @return     The modified argument
    """
    Arg = Argument.replace("\"", "\\\"")
    Arg = Arg.replace("/share/", "/")
    Arg = Arg.replace("/data/", "/")
    return Arg


def AssembleRemoteRunCmd(RunOptions, Parser=None):
    """
    @brief      Assemble athena options from run options and argument parser
                for remote running on grid sites.
    
    @param      RunOptions  The run options
    @param      Parser      The parser
    
    @return     String with athena command line options
    """
    return AssembleAthenaOptions(RunOptions, Parser, True)


def AssembleAthenaOptions(RunOptions, Parser=None, IsRemote=False):
    """
    @brief      Assemble athena options from run options and argument parser.
                The athena arguments work like this (as documented here:
                https://gitlab.cern.ch/atlas/athena/blob/21.2/Control/AthenaCommon/python/AthArgumentParser.py#L2)
    
                The command line arguments in the athena call are first passed
                to athena. Every argument that should be passed to the user code
                needs to be prepended by a single additional `-`.
    
                Example:
    
                athena.py XAMPPbase/runXAMPPbase.py  --maxEvt 100 - --noSys
                -----------------------------------------------------------
                         | job option              | athena arg  | user arg
    
    @param      RunOptions  The run options
    @param      Parser      The parser
    @param      IsRemote    Flag to toggle option parsing for pathena instead of
                            athena for running on the grid
    
    @return     List with athena command line options
    """
    Options = []
    if not IsRemote and RunOptions.testJob:
        RunOptions.noSyst = True
        RunOptions.parseFilesForPRW = True
    athena_args = ["skipEvents", "evtMax", "filesInput"]
    local_only = ["outFile", "parseFilesForPRW"] + athena_args
    from XAMPPbase.SubmitToBatch import exclusiveBatchOpt
    from XAMPPbase.SubmitToGrid import exclusiveGridOpts

    black_listed = ["jobOptions", "valgrind"] + [x.dest
                                                 for x in exclusiveBatchOpt()._actions] + [x.dest for x in exclusiveGridOpts()._actions]
    attributes = [att for att in dir(RunOptions) if not att.startswith("_") and att not in black_listed]
    attributes.sort(key=lambda x: x not in athena_args)
    ath_delimiter = False
    l_delim = -1
    for att in attributes:
        if ath_delimiter and att in athena_args: ath_delimiter = False
        if not ath_delimiter and not att in athena_args:
            ath_delimiter = True
            Options += ["-"]
            l_delim = len(Options)
        ### Skip all arguments which are default from the parser
        if IsArgumentDefault(getattr(RunOptions, att), att, Parser): continue
        if IsRemote and att in local_only: continue
        ### Attributed
        if att == "filesInput" and (os.path.isfile(RunOptions.filesInput) and not IsROOTFile(RunOptions.filesInput)
                                    or os.path.isdir(RunOptions.filesInput)):
            Options += [
                "--%s '%s'" % (att, ",".join(
                    ReadListFromFile(RunOptions.filesInput) if not os.path.isdir(RunOptions.filesInput) else
                    ["%s/%s" % (RunOptions.filesInput, item) for item in os.listdir(RunOptions.filesInput) if IsROOTFile(item)]))
            ]
        elif isinstance(getattr(RunOptions, att), bool):
            Options += ["--%s" % (att)]
        elif isinstance(getattr(RunOptions, att), list):
            Options += ["--%s %s" % (att, " ".join(getattr(RunOptions, att)))]
        else:
            Options += ["--%s %s" % (att, getattr(RunOptions, att))]
    ### No extra options were parsed. Get rid of the trailing -
    if len(Options) == l_delim:
        Options.pop()
    return Options


def ExecuteAthena(RunOptions, AthenaArgs):
    """
    @brief      Execute athena with options specified in run options and athena arguments
    
    @param      RunOptions  The run options (these are modified to satisfy athena style)
    @param      AthenaArgs  The athena arguments (are directly joined to the athena command)
    """
    ExeCmd = "athena.py %s %s" % (BringToAthenaStyle(RunOptions.jobOptions), " ".join(AthenaArgs))
    if RunOptions.outFile.find("/") != -1:
        print("INFO: Will execute Athena in directory " + RunOptions.outFile.rsplit("/", 1)[0])
        CreateDirectory(RunOptions.outFile.rsplit("/", 1)[0], False)
        os.chdir(RunOptions.outFile.rsplit("/", 1)[0])
    if RunOptions.outFile.find("/") == len(RunOptions.outFile) - 1 or not IsROOTFile(RunOptions.outFile):
        print("ERROR: Please give a file to save not only the directory")
        exit(1)

    # options to run with valgrind
    # ----------------------------------------------------------------------------------------------------
    if RunOptions.valgrind:
        if not any(os.access(os.path.join(path, 'valgrind'), os.X_OK) for path in os.environ["PATH"].split(os.pathsep)):
            print("ERROR: valgrind not avaliable - you should set up an ATLAS release that contains it or install it manually")
            exit(1)
    if RunOptions.valgrind == "callgrind":
        ExeCmd = "valgrind --suppressions=${ROOTSYS}/etc/valgrind-root.supp  --tool=callgrind --smc-check=all --num-callers=50  --trace-children=yes " + ExeCmd
        print "INFO: You are running with valgrind's callgrind! Execute command modified to:"
        print ExeCmd
    elif RunOptions.valgrind == "memcheck":
        ExeCmd += " --config-only=rec.pkl --stdcmalloc"
        print "INFO: You are running with valgrind's memcheck. First will create picke file with Athena configuration, then execute valgrind."
        print "Creating pickle file ..."
        print ExeCmd
        if os.system(ExeCmd):
            print("ERROR: Creating python pickle file with Athena has failed")
        print("Running valgrind and storing output in valgrind.log ...")
        print ExeCmd
        ExeCmd = "valgrind --suppressions=${ROOTSYS}/etc/valgrind-root.supp --leak-check=yes --trace-children=yes --num-callers=50 --show-reachable=yes --track-origins=yes --smc-check=all `which python` `which athena.py` --stdcmalloc rec.pkl 2>&1 | tee valgrind.log"
        print("Explanation of output: https://twiki.cern.ch/twiki/bin/view/AtlasComputing/UsingValgrind")
    # ----------------------------------------------------------------------------------------------------
    if os.system(ExeCmd):
        print("ERROR: Athena execeution failed")
        os.system("rm %s" % (RunOptions.outFile))
        exit(1)


def applyZnunuSampleFix(RunOptionsOrFilename, AthenaArgs=None):
    """
    Metadata information in Sherpa 2.2.1 Znunu PTVMJJ sliced samples is wrong
    and must be re-assigned. This function tries to detect from the input file
    which kind of sample is processed and configures the fix in XAMPPbase.
    
    @param      RunOptionsOrFilename  Can either be the run options object from
                                      which the input file or input dataset will
                                      be extracted or a string specifying the
                                      file name
    @param      AthenaArgs            The athena arguments (will be appended
                                      in-place) if provided
    
    @return     If sample not affected, return empty string, else heavy flavour
                filter name
    """
    try:
        import re
        if type(RunOptionsOrFilename) is not str:
            try:  # first try naming convention from local submit
                RunOptionsOrFilename = RunOptionsOrFilename.filesInput
            except:  # if that fails try naming convention for grid/batch submit
                RunOptionsOrFilename = RunOptionsOrFilename.inputDS
        # regular expression: assume file name has structure "mc16_13TeV.{dsid}.{samplename}.{rest}"
        dsid = int(re.search('mc16_13TeV\.(.+?)\.(.+?)\.', RunOptionsOrFilename).group(1))
        name = re.search('mc16_13TeV\.(.+?)\.(.+?)\.', RunOptionsOrFilename).group(2)
    except Exception:
        return ''
    if dsid >= 366010 and dsid <= 366035:
        print('Detected buggy Znunu sample! Metadata mc channel number will be re-assigned')
        HFFilter = ''
        if 'BFilter' in name: HFFilter = 'BFilter'
        if 'CFilterBVeto' in name: HFFilter = 'CFilterBVeto'
        if 'CVetoBVeto' in name: HFFilter = 'CVetoBVeto'
        if HFFilter and AthenaArgs: AthenaArgs.append("- --dsidBugFix='{hffilter}'".format(hffilter=HFFilter))
        return HFFilter
    return ''


if __name__ == "__main__":
    from XAMPPbase.AthArgParserSetup import SetupArgParser
    parser = SetupArgParser()
    RunOptions = parser.parse_args()
    AthenaArgs = AssembleAthenaOptions(RunOptions, parser)
    ExecuteAthena(RunOptions, AthenaArgs)
