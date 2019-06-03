from ClusterSubmission.Utils import ResolvePath, CreateDirectory, ReadListFromFile, ExecuteCommands, WriteList, setupBatchSubmitArgParser
import os, argparse


def setupCIparser():
    parser = argparse.ArgumentParser(prog='Update reference cutflows',
                                     conflict_handler='resolve',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--ciDir", help="Directory where the CI files are located.", default=ResolvePath("XAMPPbase/test"))
    parser.add_argument("--joboptions", help="Which job options shall be run", default="XAMPPbase/runXAMPPbase.py")
    parser.add_argument("--regions", help="what are the regions to consider", default=[], nargs="+", required=True)
    parser.add_argument("--serviceAccount", help="what is the name of the service account to use", default="xampp")
    parser.add_argument("--EOSpath",
                        help="Where are all the reference samples located on EOS",
                        default="root://eoshome.cern.ch//eos/user/x/xampp/ci/base/")
    parser.add_argument("--TEMPdir",
                        help="Where to store all of the temporary files",
                        default="%s/CI_temp" % (setupBatchSubmitArgParser().get_default("BaseFolder")))
    parser.add_argument("--athenaArgParser",
                        help="Which athena argument parser shall be used",
                        default=ResolvePath("XAMPPbase/python/runAthena.py"))
    parser.add_argument("--evtMax", help="Limit the number of events to run on", default=-1, type=int)
    parser.add_argument('--noSyst', help='run without systematic uncertainties', action='store_true', default=False)

    return parser


def getEOS_token(options):
    print "INFO: Get the AFS token"
    while os.system("kinit %s@CERN.CH" % (options.serviceAccount)) != 0:
        print "ERROR: Wrong password please try again"

    os.system("aklog -c CERN.CH")


def download_ci_files(options):
    ### Retrieve first the EOS token
    getEOS_token(options)
    ### Check first whether the CI dir actually exits
    smp_dir = "%s/datasamples/" % (options.ciDir)
    if not os.path.isdir(smp_dir):
        print "ERROR: The path to look up for the data samples %s does not exists. Where is my data" % (smp_dir)
        exit(1)

    ### Create first the directory to store the temporary files in there
    ### Clean the old remants
    CreateDirectory(options.TEMPdir, True)
    downloaded_smp = []
    for smp in os.listdir(smp_dir):
        smp_name = smp[:smp.rfind(".")]
        print "INFO: Download the files from sample %s" % (smp_name)
        download_to = "%s/%s" % (options.TEMPdir, smp_name)
        CreateDirectory(download_to, False)
        ### Download the files first
        for file_to_load in ReadListFromFile("%s/%s" % (smp_dir, smp)):
            destination_file = "%s/%s" % (download_to, file_to_load[file_to_load.rfind("/") + 1:])
            CopyCmd = "xrdcp %s/%s %s" % (options.EOSpath, file_to_load, destination_file)
            if os.path.exists(destination_file):
                print "INFO: Omit do download %s" % (file_to_load)
            elif os.system(CopyCmd) != 0:
                print "ERROR: Failed to download %s" % (file_to_load)
                exit(1)
        ### Write the file list for the analysis
        file_list = "%s/FileList_%s.txt" % (options.TEMPdir, smp_name)
        WriteList(["%s/%s" % (download_to, f[f.rfind("/") + 1:]) for f in ReadListFromFile("%s/%s" % (smp_dir, smp))], file_list)
        downloaded_smp += [smp_name]
    return downloaded_smp


def run_athena_cmds(options, job_options="", extra_args=[]):
    Athena_Cmds = []
    Proccessed_Smp = []
    for smp_name in download_ci_files(options):
        file_list = "%s/FileList_%s.txt" % (options.TEMPdir, smp_name)
        ### now start athena
        athena_outfile = "%s/athena_%s/CI.root" % (options.TEMPdir, smp_name)
        athena_logfile = "%s/athena_%s/CI.log" % (options.TEMPdir, smp_name)
        CreateDirectory("%s/athena_%s/" % (options.TEMPdir, smp_name), False)
        athena_cmd = "python %s %s --parseFilesForPRW --jobOptions %s --evtMax %d %s --filesInput %s --outFile %s 2>&1 > %s" % (
            options.athenaArgParser, " ".join(extra_args), job_options, options.evtMax, "--noSyst" if options.noSyst else "", file_list,
            athena_outfile, athena_logfile)
        ### Paralellize the athena commands
        Athena_Cmds += [athena_cmd]
        Proccessed_Smp += [(smp_name, athena_outfile)]

    ExecuteCommands(Athena_Cmds, MaxCurrent=4)
    return Proccessed_Smp


def evaluate_cut_flows(options, Proccessed_Smp=[], analysis="XAMPPbase"):
    for Sample, Out_File in Proccessed_Smp:
        if not os.path.exists(Out_File):
            print "ERROR: No such file or directory %s. Skip sample" % (Out_File)
            continue
        ### Execute the cutflow commands for each region
        cflow_dir = "%s/reference_cutflows/" % (options.ciDir)
        CreateDirectory(cflow_dir, False)
        for region in options.regions:
            CI_file = "%s/%s_%s_%s.txt" % (cflow_dir, Sample, analysis, region)
            Cflow_Cmd = "python %s -i %s -a %s | tee %s" % (ResolvePath("XAMPPbase/python/printCutFlow.py"), Out_File, region, CI_file)
            if os.system(Cflow_Cmd) != 0:
                print "ERROR: Could not process cutflow %s in file %s" % (region, Out_File)
                del_cmd = "rm %s" % (CI_file)
                os.system(del_cmd)
            CI_file_weighted = "%s_weighted.txt" % (CI_file[:CI_file.rfind(".")])
            #### Skip data files to be added to the weighted cutflow
            if Sample.lower().find("data") != -1: continue
            Cflow_Cmd = "python %s -i %s -a %s --weighted | tee %s " % (ResolvePath("XAMPPbase/python/printCutFlow.py"), Out_File, region,
                                                                        CI_file_weighted)
            if os.system(Cflow_Cmd) != 0:
                print "ERROR: Could not process cutflow %s in file %s" % (region, Out_File)
                del_cmd = "rm %s" % (CI_file_weighted)
                os.system(del_cmd)


if __name__ == '__main__':
    options = setupCIparser().parse_args()

    Proccessed_Smp = run_athena_cmds(options, job_options=getAnalyses()[options.analysis])
    evaluate_cut_flows(options, Proccessed_Smp)

    ### Clean up what's left from the previous directories
    CleanCmd = "rm -rf %s" % (options.TEMPdir)
    print "INFO: Clean up the working directory %s" % (CleanCmd)
    os.system(CleanCmd)
