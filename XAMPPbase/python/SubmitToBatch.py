import os
from XAMPPbase.runAthena import AssembleRemoteRunCmd
from XAMPPbase.Utils import IsTextFile, IsListIn
from ClusterSubmission.Utils import TimeToSeconds, prettyPrint, IsROOTFile, ReadListFromFile, WriteList, ResolvePath, CreateDirectory, ClearFromDuplicates, setup_engine, setupBatchSubmitArgParser
from XAMPPbase.AthArgParserSetup import attachArgs


class NtupleMakerSubmit(object):
    def __init__(
            self,
            cluster_engine=None,
            jobOptions="",
            input_ds=[],
            run_time="19:59:59",
            dcache_dir="",
            alg_opt="",  ### Extra options of the algorithm like noSyst... etc
            vmem=2000,
            events_per_job=100000,
            hold_jobs=[],
            files_per_merge=10,
            final_split=1,
    ):
        self.__cluster_engine = cluster_engine
        ### Job splitting configurations
        self.__events_per_job = events_per_job
        self.__dcache_dir = dcache_dir
        self.__dcache_loc = ResolvePath(dcache_dir)

        ### analysis job configurations
        self.__job_options = jobOptions
        self.__alg_opt = alg_opt
        self.__run_time = run_time
        self.__vmem = vmem

        ### Hold jobs
        self.__hold_jobs = [H for H in hold_jobs]
        ### Merging
        self.__merge_interfaces = []
        self.__files_per_merge_itr = files_per_merge
        self.__final_split = final_split
        self.__nsheduled = 0
        for ds in sorted(input_ds):
            if not self.__prepare_input(ds):
                CreateDirectory(self.engine().config_dir(), True)
                self.__nsheduled = 0
                return False

    def __prepare_input(self, in_ds=""):
        print "INFO <_prepare_input>: Assemble configuration for %s" % (in_ds)
        ### Name to be piped to the job
        out_name = in_ds[in_ds.rfind("/") + 1:in_ds.rfind(".")] if IsTextFile(in_ds) or IsROOTFile(in_ds) else in_ds
        split_dir = "%s/Datasets/%s" % (self.split_cfg_dir(), out_name)
        root_files = []
        ### Now we need to find the corresponding ROOT files
        ### 1) The dataset is a root file itself
        if IsROOTFile(in_ds):
            root_files += [in_ds]
        ### 2) The given dataset is a .txt file
        elif IsTextFile(in_ds):
            ### Find the root files from there
            root_files = self.__extract_root_files(in_ds)
            if len(root_files) == 0: return False
        ### 3) The given dataset is a directory
        elif os.path.isdir(in_ds):
            if in_ds.endswith("/"):
                in_ds = in_ds[:in_ds.rfind("/")]
                out_name = in_ds[in_ds.rfind("/") + 1:]
            split_dir = "%s/Directory/%s" % (self.split_cfg_dir(), out_name)
            root_files = ["%s/%s" % (in_ds, F) for F in os.listdir(in_ds) if IsROOTFile(F)]
        ### 4) It's a logical dataset stored on d-cache
        else:
            root_files = self.__find_on_dcache(in_ds)
        if len(root_files) == 0:
            print "ERROR: Could not associate anything to %s" % (in_ds)
            return False
        if len(out_name) == 0:
            print "ERROR: How should the output be called %s" % (in_ds)
            return False

        ### Assemble the splitting of the jobs
        main_list = "%s/AllROOTFiles.main" % (split_dir)
        files_in_main = ReadListFromFile(main_list) if os.path.exists(main_list) else []
        ### The list is unkown or the content of ROOT files has changed
        ### Redo the splitting again ;-)
        if len(files_in_main) != len(root_files) or not IsListIn(files_in_main, root_files):
            print "INFO: Assemble new split for %s" % (in_ds)
            CreateDirectory(split_dir, True)
            WriteList(root_files, main_list)
            os.system("CreateBatchJobSplit -I %s -O %s -EpJ %i" % (main_list, split_dir, self.__events_per_job))
        ### Each of the lists contains the ROOT files to process per each sub job
        split_lists = ["%s/%s" % (split_dir, F) for F in os.listdir(split_dir) if IsTextFile(F)]
        n_jobs = len(split_lists)
        subjob_outs = ["%s/%s_%d.root" % (self.engine().tmp_dir(), out_name, d) for d in range(n_jobs)]

        assembled_in = [] if not os.path.exists(self.job_input()) else ReadListFromFile(self.job_input())
        assembled_out = [] if not os.path.exists(self.job_out_names()) else ReadListFromFile(self.job_out_names())
        start_reg = len(assembled_in)

        ### Write what we've
        WriteList(assembled_in + split_lists, self.job_input())
        WriteList(assembled_out + subjob_outs, self.job_out_names())
        #### Submit the merge jobs
        self.__merge_interfaces += [
            self.engine().create_merge_interface(out_name=out_name,
                                                 files_to_merge=subjob_outs,
                                                 hold_jobs=[(self.engine().job_name(), [start_reg + i + 1 for i in range(n_jobs)])],
                                                 files_per_job=self.__files_per_merge_itr,
                                                 final_split=self.__final_split)
        ]
        self.__nsheduled += n_jobs
        return True

    def __extract_root_files(self, file_list=""):
        content = ReadListFromFile(file_list)
        if len(content) == 0:
            print "ERROR: The file %s is empty" % (in_ds)
            return []
        n_files_in_cont = len(content) - len([c for c in content if IsROOTFile(c)])
        ### The list contains a list of root_files
        if n_files_in_cont == 0:
            return content
        ### It's a mixture
        elif n_files_in_cont != len(content):
            print "ERROR: You've a mixture of ROOT files and other stuff in %s" % (file_list)
            return []
        root_files = []
        for ds in content:
            root_files += self.__find_on_dcache(ds)
        return root_files

    def __find_on_dcache(self, ds):
        if not self.__dcache_loc or not os.path.isdir(self.__dcache_loc):
            print "WARNING %s is not a valid directory" % (self.__dcache_dir)
            return []
        ds_name = ds
        ### Ending slashes must be removed
        if ds_name.endswith("/"): ds_name = ds_name[:ds_name.rfind("/")]
        if ds_name.startswith("/"):
            print "WARNING: Why %s???" % (ds_name)
            ds_name = ds_name[1:]
        ### The dataset has the form
        ### mc16_13TeV:mc16_13TeV.<Blah>
        if ds_name.find(":") != -1: ds_name = ds_name[ds_name.find(":") + 1:]

        ### Try if there is a common list with one of these endings
        txt_endings = ["txt", "conf", "list"]
        for end in txt_endings:
            search_for = "%s/%s.%s" % (self.__dcache_loc, ds_name, end)
            if os.path.isfile(search_for): return self.__extract_root_files(search_for)
        ### Last despaired trial. The user forgot about the ending?
        search_for = "%s/%s" % (self.__dcache_loc, ds_name)
        if os.path.isfile(search_for):
            print "ASKING: Why did you forget about the usual .txt ending? %s" % (search_for)
            return self.__extract_root_files(search_for)
        print "WARNING: Could not find a valid logical dataset for %s" % (ds_name)
        return []

    def engine(self):
        return self.__cluster_engine

    def split_cfg_dir(self):
        return "%s/.SplitConfigs/%d/" % (self.engine().base_dir(), self.__events_per_job)

    ### Location where the input job cfg is stored
    def job_input(self):
        return "%s/InLists.conf" % (self.engine().config_dir())

    ### Location where the temporary files are stored
    def job_out_names(self):
        return "%s/out_fileNames.conf" % (self.engine().config_dir())

    def hold_jobs(self):
        return self.__hold_jobs

    def n_sheduled(self):
        return self.__nsheduled

    def run_time(self):
        return self.__run_time

    def memory(self):
        return self.__vmem

    def submit_job(self):
        if self.n_sheduled() == 0:
            print "ERROR <submit>: Nothing has been scheduled"
            return False
        if not self.engine().submit_build_job(): return False
        if not self.engine().submit_array(script="XAMPPbase/Batch_Analysis.sh",
                                          mem=self.memory(),
                                          run_time=self.run_time(),
                                          hold_jobs=self.hold_jobs(),
                                          env_vars=[
                                              ("Options", " ".join(self.__alg_opt)),
                                              ("Execute", self.__job_options),
                                              ("OutCfg", self.job_out_names()),
                                              ("InCfg", self.job_input()),
                                          ],
                                          array_size=self.n_sheduled()):
            return False
        to_hold = []
        for merge in self.__merge_interfaces:
            if not merge.submit_job(): return False
            to_hold += [self.engine().subjob_name("merge-%s" % (merge.outFileName()))]
        if not self.engine().submit_clean_all(hold_jobs=to_hold): return false
        return self.engine().finish()


def exclusiveBatchOpt():
    parser = setupBatchSubmitArgParser()
    parser.add_argument('--BaseProject',
                        help='choose project containing file lists to adjust the filepath to the DS list (default is \"XAMPPbase\")',
                        default='XAMPPbase')
    parser.set_defaults(Merge_vmem=600)
    parser.add_argument('--inputDS', help='Input datasets', default=[], nargs="+")
    parser.add_argument("--RSE", help='RSE storage element for files located via dcache.', default='MPPMU_LOCALGROUPDISK')
    parser.add_argument('--RunTime', help='Changes the RunTime of the Jobs: default 19:59:59 ', default='19:59:59')
    parser.add_argument('--EventsPerJob', help='Changes the Events per Batch job. Default: 10000 ', default=10000, type=int)
    parser.add_argument('--FilesPerMergeJob', help='Number of files per merge', default=8, type=int)
    parser.add_argument("--FinalSplit", help="How many files should be left after merge", default=1, type=int)
    parser.add_argument('--vmem', help='Virtual memory reserved for each analysis  jobs', default=3500, type=int)
    parser.add_argument('--HoldJob', default=[], nargs="+", help='Specfiy job names which should be finished before your job is starting. ')
    parser.add_argument("--SpareWhatsProcessedIn",
                        help="If the cluster decided to die during production, you can skip the processed files in the directories",
                        default=[],
                        nargs="+")
    return parser


def setupSubmitParser():
    parser = exclusiveBatchOpt()
    attachArgs(parser)
    return parser


def main():
    parser = setupSubmitParser()
    options = parser.parse_args()
    cluster_engine = setup_engine(options)

    Spared_Files = []
    #### The previous round of cluster screwed up. But had some results. There is no
    #### reason to reprocess them. So successful files are not submitted twice
    if len(options.SpareWhatsProcessedIn) > 0:
        print "INFO: Cluster did not perform so well last time? This little.. buttefingered.."
        for dirToSpare in options.SpareWhatsProcessedIn:
            if not os.path.isdir(dirToSpare):
                print "ERROR: I need a directory to look up %s" % (dirToSpare)
                exit(1)
            for finished in os.listdir(dirToSpare):
                if not IsROOTFile(finished): continue
                print "INFO: Yeah... %s has already beeen processed. Let's skip it.." % (finished)
                Spared_Files.append(finished[:finished.rfind(".root")])

    Submit_Class = NtupleMakerSubmit(
        cluster_engine=cluster_engine,
        jobOptions=options.jobOptions.replace("share/", ""),
        input_ds=ClearFromDuplicates([
            ds for ds in options.inputDS if ds[:ds.rfind(".")] not in Spared_Files
            #  or ds not in  Spared_Files
        ]),
        run_time=options.RunTime,
        dcache_dir="%s/GroupDiskLists/%s" % (options.BaseProject, options.RSE),
        alg_opt=AssembleRemoteRunCmd(options, parser),  ### Extra options of the algorithm like noSyst... etc
        vmem=options.vmem,
        events_per_job=options.EventsPerJob,
        hold_jobs=options.HoldJob,
        files_per_merge=options.FilesPerMergeJob,
        final_split=options.FinalSplit,
    )
    Submit_Class.submit_job()


if __name__ == '__main__':
    main()
