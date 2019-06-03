import os, argparse, time
from ClusterSubmission.Utils import CreateDirectory
from ClusterSubmission.ClusterEngine import ATLASPROJECT
from MuonAnalysis.Utils import IsArgumentDefault
if not ATLASPROJECT:
    print("ERROR: This module is only available in an Athena based release")
    exit(1)


def setupSUSYArgParser(parser):
    # filesInput is required and should also be indicated as required when calling --help
    requiredNamed = parser.add_argument_group("required named arguments")
    requiredNamed.add_argument('--filesInput', '-i', help='input dataset or file list')
    parser.add_argument('--outFile', '-o', help='name of the output file', default='AnalysisOutput.root')
    parser.add_argument('--analysis', '-a', help='select the analysis you want to run on', default='HeavyIon')
    parser.add_argument('--noSyst', help='run without systematic uncertainties', action='store_true', default=False)
    parser.add_argument('--nevents', type=int, help="number of events to process for all the datasets")
    parser.add_argument('--skipEvents', type=int, help="skip the first n events")
    #parser.add_argument('--STConfig', help='name of custom SUSYTools config file located in data folder', default='SUSYTools/SUSYTools_Default.conf')
    #parser.add_argument("--jobOptions", help="The athena jobOptions file to be executed", default="XAMPPbase/runXAMPPbase.py")
    parser.add_argument("--jobOptions", help="The athena jobOptions file to be executed", default="MuonAnalysis/MuonAnalysisAlgJobOptions.py")
    #parser.add_argument("--testJob", help="If the testjob argument is called then only the mc16a/mc16d prw files are loaded depeending on the input file", default=False, action='store_true')
    #parser.add_argument("--singlePRWperiodsOnly", help="The job only contains mc16a/d/e files. Thus we can only load the corresponding prwFiles", default=False, action='store_true')
    return parser


def BringToAthenaStyle(Argument):
    Arg = Argument.replace("\"", "\\\"")
    Arg = Arg.replace("/share/", "/")
    Arg = Arg.replace("/data/", "/")
    return Arg


def AssembleAthenaOptions(RunOptions, Parser=None, IsRemote=False):
    Options = []
    #if not IsRemote and RunOptions.testJob:
        #RunOptions.noSyst = True
        #RunOptions.singlePRWperiodsOnly = True
    #if RunOptions.singlePRWperiodsOnly:
        #Options.append("loadAssocPRWonly=True")
    if RunOptions.noSyst:
        Options.append("noSyst=True")
    #if RunOptions.analysis and not IsArgumentDefault(RunOptions.analysis, 'analysis', Parser):
        #Options.append("XAMPPanalysis='%s'" % (BringToAthenaStyle(RunOptions.analysis)))
    #if RunOptions.STConfig and not IsArgumentDefault(RunOptions.STConfig, "STConfig", Parser):
        #Options.append("STConfigFile='%s'" % (BringToAthenaStyle(RunOptions.STConfig)))
    if not IsRemote and RunOptions.filesInput and not IsArgumentDefault(RunOptions.filesInput, 'filesInput', Parser):
        Options.append("filesInput='%s'" % (BringToAthenaStyle(RunOptions.filesInput)))
    if not IsRemote and RunOptions.outFile and not IsArgumentDefault(RunOptions.outFile, 'outFile', Parser):
        Options.append("outFile='%s'" % (RunOptions.outFile.replace("\"", "\\\"")))
    if not IsRemote and RunOptions.nevents > 0:
        Options.append("nevents=%i" % (RunOptions.nevents))
    if not IsRemote and RunOptions.skipEvents > 0: Options.append("nskip=%i" % (RunOptions.skipEvents))
    return Options


def AssembleConfigArgument(ListOfArgs):
    if len(ListOfArgs) == 0: return ""
    return "-c \"%s\"" % (";".join(ListOfArgs))


def AssembleRemoteRunCmd(RunOptions, Parser=None):
    return AssembleAthenaOptions(RunOptions, Parser, True)


def ExecuteAthena(RunOptions, AthenaArgs):
    if RunOptions.outFile.find("/") != -1:
        print("INFO: Will execute Athena in directory " + RunOptions.outFile.rsplit("/", 1)[0])
        CreateDirectory(RunOptions.outFile.rsplit("/", 1)[0], False)
        os.chdir(RunOptions.outFile.rsplit("/", 1)[0])
    if RunOptions.outFile.find("/") == len(RunOptions.outFile) - 1 or not RunOptions.outFile.endswith(".root"):
        print("ERROR: Please give a file to save not only the directory")
        exit(1)
    ExeCmd = "athena.py %s %s" % (AssembleConfigArgument(AthenaArgs), BringToAthenaStyle(RunOptions.jobOptions))
    if os.system(ExeCmd):
        print("ERROR: Athena execeution failed")
        os.system("rm %s" % (RunOptions.outFile))
        exit(1)


def SetupAthenaParser():
    parser = argparse.ArgumentParser(
        description='This script starts the analysis code. For more help type \"python MuonAnalysis/python/runAthena.py -h\"',
        prog='runAthena',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    setupSUSYArgParser(parser)
    return parser


if __name__ == "__main__":
    parser = SetupAthenaParser()
    RunOptions = parser.parse_args()
    AthenaArgs = AssembleAthenaOptions(RunOptions, parser)
    ExecuteAthena(RunOptions, AthenaArgs)
