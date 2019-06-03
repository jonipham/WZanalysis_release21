def attachArgs(theParser):
    # requiredNamed = theParser.add_argument_group("required named arguments")
    theParser.add_argument('--outFile', '-o', help='name of the output file', default='AnalysisOutput.root')
    theParser.add_argument('--analysis', '-a', help='select the analysis you want to run on', default='MyCutFlow')
    theParser.add_argument('--noSyst', help='run without systematic uncertainties', action='store_true', default=False)
    theParser.add_argument('--parseFilesForPRW',
                           action='store_true',
                           default=False,
                           help='flag to tune the PRW based on the MC files we encounter - only for local jobs')
    theParser.add_argument('--STConfig',
                           help='name of custom SUSYTools config file located in data folder',
                           default='SUSYTools/SUSYTools_Default.conf')
    theParser.add_argument(
        "--testJob",
        help="If the testjob argument is called then only the mc16a/mc16d prw files are loaded depeending on the input file",
        default=False,
        action='store_true')
    theParser.add_argument("--dsidBugFix",
                           help="Specify a string for the dsid bug fix in case someone messes it up again with the dsid",
                           choices=["", "BFilter", "CFilterBVeto", "CVetoBVeto"],
                           default=None)
    theParser.add_argument("--jobOptions", help="The athena jobOptions file to be executed", default="XAMPPbase/runXAMPPbase.py")
    theParser.add_argument("--valgrind",
                           help="Search for memory leaks/call structure using valgrind",
                           choices=["", "memcheck", "callgrind"],
                           default="")


def SetupArgParser():
    import argparse
    from AthenaCommon import AthOptionsParser
    parser = argparse.ArgumentParser(
        description='This script starts the analysis code. For more help type \"python XAMPPbase/python/runAthena.py -h\"',
        prog='runAthena',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    ### Cann't we just include the main options from atyhena
    parser.add_argument("--evtMax", help='Max number of events to process', default=-1, type=int)
    parser.add_argument("--skipEvents", help='Number of events to skip', default=-1, type=int)
    parser.add_argument("--filesInput", "--inFile", help='Number of events to skip', default="", type=str)
    attachArgs(parser)
    return parser


def SetupAthArgParser():
    from AthenaCommon.AthArgumentParser import AthArgumentParser

    athArgParser = AthArgumentParser(description='Parser for Athena')
    attachArgs(athArgParser)

    return athArgParser
