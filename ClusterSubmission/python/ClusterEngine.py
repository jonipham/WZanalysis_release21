#! /usr/bin/env python
from ClusterSubmission.Utils import TimeToSeconds, CreateDirectory, WriteList, ResolvePath, prettyPrint, id_generator, getRunningThreads
import os, commands, time, argparse, threading
from random import shuffle
#######################################################################################
#                         environment variables                                       #
#######################################################################################
USERNAME = os.getenv("USER")
ROOTSYS = os.getenv("ROOTSYS").replace("%s/root/" % os.getenv("ATLAS_LOCAL_ROOT"), "")
ATLASPROJECT = os.getenv("AtlasProject") if not os.getenv("AtlasPatch") else os.getenv("AtlasPatch")
ATLASVERSION = os.getenv("AtlasVersion")
TESTAREA = os.getenv("TestArea")


#######################################################################
#           Basic class for cluster submission.                       #
#           It manages the structure of how jobs should be scheduled  #
#######################################################################
class ClusterEngine(object):
    def __init__(self,
                 jobName,
                 baseDir,
                 buildTime="01:59:59",
                 mergeTime="01:59:59",
                 buildCores=2,
                 buildMem=1400,
                 mergeMem=100,
                 maxArraySize=40000,
                 maxCurrentJobs=-1,
                 mail_user="",
                 hold_build=[],
                 submit_build=True):

        self.__jobName = jobName
        self.__baseFolder = baseDir
        self.__Today = time.strftime("%Y-%m-%d")
        self.__buildTime = buildTime

        self.__submit_build = submit_build
        self.__buildMem = buildMem
        self.__mergeMem = mergeMem
        self.__buildCores = buildCores
        self.__buildTime = buildTime
        self.__holdBuild = hold_build
        self.__submitted_build = False
        self.__max_current = maxCurrentJobs
        self.__max_array_size = maxArraySize
        self.__mail_user = mail_user
        self.__merge_time = mergeTime
        #######################################################
        #   Folder structure
        #     BASEFOLDER
        #      --- LOGS/<Date>/<JobName>
        #      --- TMP/<Date>/<JobName>
        #           --- BUILD
        #           --- CONFIG
        #      --- OUTPUT/<Date>/<JobName>
        ########################################################
    def job_name(self):
        return self.__jobName

    def max_array_size(self):
        return self.__max_array_size

    def max_running_per_array(self):
        return self.__max_current

    def merge_time(self):
        return self.__merge_time

    def merge_mem(self):
        return self.__mergeMem

    def base_dir(self):
        return self.__baseFolder

    def log_dir(self):
        return "%s/LOGS/%s/%s" % (self.base_dir(), self.__Today, self.job_name())

    def tmp_dir(self):
        return "%s/TMP/%s/%s" % (self.base_dir(), self.__Today, self.job_name())

    def build_dir(self):
        return "%s/BUILD" % (self.tmp_dir())

    def config_dir(self):
        return "%s/CONFIG" % (self.tmp_dir())

    def out_dir(self):
        return "%s/OUTPUT/%s/%s" % (self.base_dir(), self.__Today, self.job_name())

    def mail_user(self):
        return self.__mail_user

    def link_to_copy_area(self, config_file):
        config_path = ResolvePath(config_file)
        if not config_path: return None
        ### Create the directory
        CreateDirectory(self.config_dir(), False)
        ### Keep the ending of the file but rename it to a random thing
        final_path = "%s/%s.%s" % (self.config_dir(), id_generator(45), config_file[config_file.rfind(".") + 1:])
        os.system("cp %s %s" % (config_path, final_path))
        return final_path

    ### Special jobs which should be handled by any grid engine
    def submit_hook(self):
        if os.path.exists("%s/.job.lck" % (self.tmp_dir())):
            print "ERROR <submit_build_job>: The build job cannot be submitted since there is already a job running %s on the cluster" % (
                self.job_name())
            return False
        #### Do not overwrite existing output
        if os.path.exists("%s/.job.lck" % (self.out_dir())):
            print "ERROR <submit_build_job>: The job has been finished %s and cannot be resubmitted again" % (self.job_name())
            return False
        return True

    def send_build_job(self):
        return self.__submit_build

    def submit_build_job(self):
        if self.__submitted_build:
            print "ERROR <submit_build_job>: Build job is already submitted"
            return False
        if not self.submit_hook(): return False
        if self.send_build_job() and not self.submit_job(
                script="ClusterSubmission/Build.sh",
                sub_job="Build",
                mem=self.__buildMem,
                env_vars=[("CleanOut", self.out_dir()), ("CleanLogs", self.log_dir()), ("CleanTmp", self.tmp_dir()),
                          ("nCoresToUse", self.__buildCores), ("COPYAREA", self.build_dir())],
                run_time=self.__buildTime,
                hold_jobs=self.__holdBuild):
            return False
        elif not self.send_build_job():
            if not CreateDirectory(self.log_dir(), False) or not CreateDirectory(self.out_dir(), False): return False
            Dummy_Job = "%s/%s.sh" % (self.config_dir(), id_generator(35))
            WriteList(["#!/bin/bash", "echo \"I'm a dummy build job. Will wait 60 seconds until everything is scheduled\"", "sleep 120"],
                      Dummy_Job)
            if not self.submit_job(
                    script=Dummy_Job, sub_job="Build", mem=100, env_vars=[], run_time="00:05:00", hold_jobs=self.__holdBuild):
                return False

        self.__submitted_build = True
        self.lock_area()
        return True

    def lock_area(self):
        WriteList(["###Hook file to prevent double submission of the same job"], "%s/.job.lck" % (self.tmp_dir()))

    def submit_clean_job(self, hold_jobs=[], to_clean=[], sub_job=""):
        clean_cfg = "%s/Clean_%s.txt" % (self.config_dir(), id_generator(35))
        WriteList(to_clean, clean_cfg)
        return self.submit_job(
            script="ClusterSubmission/Clean.sh",
            mem=100,
            env_vars=[("ToClean", clean_cfg)],
            hold_jobs=hold_jobs,
            sub_job="Clean%s%s" % ("" if len(sub_job) == 0 else "-", sub_job),
            run_time="01:00:00")

    def submit_copy_job(
            self,
            hold_jobs=[],
            to_copy=[],  ### Give particular files to copy
            destination="",
            source_dir="",  ### Optional
            sub_job=""):
        copy_cfg = ""
        if len(to_copy) > 0:
            copy_cfg = "%s/Copy_%s.txt" % (self.config_dir(), id_generator(35))
            WriteList(to_copy, copy_cfg)
        elif len(source_dir) > 0:
            copy_cfg = source_dir
        else:
            print "<submit_copy_job> Nothing to copy"
            return False
        if len(destination) == 0:
            print "<Submit_copy_job> Where to copy everything?"
            return False
        return self.submit_job(
            script="ClusterSubmission/Copy.sh",
            mem=100,
            env_vars=[
                ("DestinationDir", destination),
                ("FromDir", copy_cfg),
            ],
            hold_jobs=hold_jobs,
            sub_job="Copy%s%s" % ("" if len(sub_job) == 0 else "-", sub_job),
            run_time="01:00:00")

    def submit_clean_all(self, hold_jobs=[]):
        if not self.submit_copy_job(
                hold_jobs=hold_jobs, to_copy=["%s/.job.lck" % (self.tmp_dir())], destination=self.out_dir(), sub_job="LCK"):
            return False
        return self.submit_clean_job(hold_jobs=[self.subjob_name("Copy-LCK")], to_clean=[self.tmp_dir()])

    #### Basic method to submit a job
    def submit_job(self, script, sub_job="", mem=-1, env_vars=[], hold_jobs=[], run_time=""):
        print "ERROR <submit_job>: This is a dummy function. Please replace this by the proper engine command"
        return False

    def submit_array(self, script, sub_job="", mem=-1, env_vars=[], hold_jobs=[], run_time="", array_size=-1):
        print "ERROR <submit_array>: This is a dummy function. Please replace this by the proper engine command"
        return False

    def subjob_name(self, sub_job=""):
        return self.job_name() + ("" if len(sub_job) == 0 else "_") + sub_job

    def common_env_vars(self):
        common_vars = [("OriginalProject", ATLASPROJECT), ("OriginalPatch", ATLASVERSION)]
        ### submit_build_job not yet called
        if not self.__submitted_build:
            common_vars += [("OriginalArea", TESTAREA)]
            ### Build submitted
        elif self.send_build_job():
            common_vars += [("OriginalArea", "%s/source" % (self.build_dir()))]
            ### Testarea is setup in a source dir
        elif TESTAREA.endswith("source"):
            common_vars += [("OriginalArea", TESTAREA)]
            ### Testarea is setup in topdirectory
        else:
            common_vars += [("OriginalArea", "%s/source" % (TESTAREA))]
        return common_vars

    def to_hold(self, hold_jobs):
        return [h for h in hold_jobs] + ([] if not self.__submitted_build else [self.subjob_name("Build")])

    def create_merge_interface(self, out_name="", files_to_merge=[], files_per_job=10, hold_jobs=[], final_split=1, sub_array_begin=-1):
        return MergeSubmit(
            outFileName=out_name,
            files_to_merge=files_to_merge,
            hold_jobs=hold_jobs,
            cluster_engine=self,
            files_per_job=files_per_job,
            final_split=final_split)

    def finish(self):
        return True

    def print_banner(self):
        print "#####################################################################################################"
        print "                        ClusterEngine for job %s " % self.__jobName
        print "#####################################################################################################"
        prettyPrint("JobName", self.job_name())
        prettyPrint("LogIdr", self.log_dir())
        prettyPrint("BuildDir", self.build_dir())
        prettyPrint("TmpDir", self.tmp_dir())
        prettyPrint("outputDir", self.out_dir())


####################################################################
###   Implementation to be used on the SLURM systems               #
####################################################################
class SlurmEngine(ClusterEngine):
    def submit_job(
            self,
            script,
            sub_job="",
            mem=-1,
            env_vars=[],
            hold_jobs=[],
            run_time="",
    ):
        if not CreateDirectory(self.log_dir(), False): return False
        exec_script = self.link_to_copy_area(script)
        pwd = os.getcwd()
        os.chdir(self.log_dir())

        if not exec_script: return False
        if mem < 0:
            print "ERROR: Please give a reasonable memory"
            return False
        submit_cmd = "sbatch --output=%s/%s.log  --mail-type=FAIL --mail-user='%s' --mem=%iM %s %s --job-name='%s' --export=%s %s" % (
            self.log_dir(), sub_job if len(sub_job) > 0 else self.job_name(), self.mail_user(), mem, self.__partition(run_time),
            self.__shedule_jobs(self.to_hold(hold_jobs), sub_job), self.subjob_name(sub_job), ",".join(
                ["%s='%s'" % (var, value) for var, value in env_vars + self.common_env_vars()]), exec_script)
        if os.system(submit_cmd): return False
        os.chdir(pwd)
        return True

    def submit_array(self, script, sub_job="", mem=-1, env_vars=[], hold_jobs=[], run_time="", array_size=-1):
        if not CreateDirectory(self.log_dir(), False): return False
        pwd = os.getcwd()
        os.chdir(self.log_dir())
        exec_script = self.link_to_copy_area(script)
        if not exec_script: return False
        if mem < 0:
            print "ERROR: Please give a reasonable memory"
            return False

        ArrayStart = 0
        ArrayEnd = min(self.max_array_size(), array_size)
        while ArrayEnd > ArrayStart:
            n_jobs_array = min(array_size - ArrayStart, self.max_array_size())
            submit_cmd = "sbatch --output=%s/%s_%%A_%%a.log --array=1-%i%s --mail-type=FAIL --mail-user='%s' --mem=%iM %s %s --job-name='%s' --export=%s %s" % (
                self.log_dir(),
                sub_job if len(sub_job) > 0 else self.job_name(),
                n_jobs_array,
                "" if n_jobs_array < self.max_running_per_array() else "%%%d" % (self.max_running_per_array()),
                self.mail_user(),
                mem,
                self.__partition(run_time),
                self.__shedule_jobs(self.to_hold(hold_jobs), sub_job),
                self.subjob_name(sub_job),
                ",".join(
                    ["%s='%s'" % (var, value) for var, value in ([('IdOffSet', '%i' % (ArrayStart))] + env_vars + self.common_env_vars())]),
                exec_script,
            )
            if os.system(submit_cmd): return False
            ArrayStart = ArrayEnd
            ArrayEnd = min(ArrayEnd + self.max_array_size(), array_size)
        os.chdir(pwd)
        return True

    #### return the partion of the slurm engine
    def __partition(self, RunTime):
        partition = ""
        OldTime = 1.e25
        for L in commands.getoutput("sinfo --format='%%P %%l %%a'").split("\n"):
            name = L.split()[0].replace("*", "")
            Time = L.split()[1]
            Mode = L.split()[2]
            t0 = TimeToSeconds(Time)
            if t0 > 0 and t0 > TimeToSeconds(RunTime) and t0 < OldTime and Mode == "up":
                partition = name
                OldTime = TimeToSeconds(Time)
        if len(partition) == 0:
            print "ERROR: Invalid run-time given %s" % (RunTime)
            exit(1)
        return " --partition %s --time='%s' " % (partition, RunTime)

    ### Convert the job into a slurm job-id
    def __slurm_id(self, job):
        Ids = []
        jobName = ''
        sub_ids = []
        if isinstance(job, str):
            jobName = job
            #### feature to get only sub id's in a certain range
        elif isinstance(job, tuple):
            jobName = job[0]
            #### Users have the possibility to pipe either the string
            #### of job names [ "FirstJobToHold", "SecondJobToHold", "TheCake"]
            #### or to pipe a tuple which can be either of the form
            ####    [ ("MyJobsArray" , [1,2,3,4,5,6,7,8,9,10,11]), "The cake"]
            #### meaning that the job ID's are constructed following the
            #### sequence 1-11. It's important to emphazise that the array
            #### *must* start with a 1. 0's are ignored by the system. There
            #### is also an third option, where the user parses
            ####     ["MyJobArray", -1]
            #### This option is used to indicate a one-by-one dependency of
            #### tasks in 2 consecutive arrays.
            if isinstance(job[1], list):
                sub_ids = [int(i) for i in job[1]]
                if -1 in sub_ids:
                    print "WARNING <__slurm_id>: -1 found in sub ids. If you want to pipe a 1 by 1 dependence of subjobs in two arrays please add [ (%s, -1) ]" % (
                        H[0])
        else:
            print "ERROR: Invalid object " + job
            exit(1)
        for J in commands.getoutput("squeue --format=\"%j %i\"").split("\n"):
            fragments = J.split()
            if fragments[0].strip() == jobName:
                cand_id = fragments[1].strip()
                ### The pending job is an array
                if cand_id.find("_") != -1:
                    main_job = cand_id[:cand_id.find("_")]
                    job_range = cand_id[cand_id.find("_") + 1:]
                    ### We simply do not care about particular subjobs
                    if len(sub_ids) == 0:
                        if main_job not in Ids: Ids += [main_job]
                    elif len(sub_ids) > 0:
                        Ids += ["%s_%d" % (main_job, i) for i in sub_ids if i > 0]

                elif cand_id not in Ids:
                    Ids += [cand_id]
        return Ids

    ### Shedule the job after the following jobs succeeded
    def __shedule_jobs(self, HoldJobs, sub_job="", RequireOk=True):
        prettyPrint("", "#############################################################################")
        if len(sub_job) == 0: prettyPrint("Submit cluster job:", self.job_name())
        else: prettyPrint("Submit job: ", "%s in %s" % (sub_job, self.job_name()))
        prettyPrint("", "#############################################################################")
        info_written = False

        to_hold = []
        dependency_str = ""
        for H in HoldJobs:
            ids = self.__slurm_id(H)
            if len(ids) > 0:
                if not info_written: prettyPrint("Hold %s until" % (sub_job if len(sub_job) else self.job_name()), "")
                info_written = True
                prettyPrint(
                    "",
                    H if isinstance(H, str) else
                    ("%s [%s]" % (H[0], ",".join(str(h) for h in H[1])) if isinstance(H[1], list) else "%s [1 by 1]" % (H[0])),
                    width=32,
                    separator='*')
            ### Usual dependency on entire jobs or certain subjobs in an array
            if isinstance(H, str) or isinstance(H[1], list): to_hold += ids
            elif isinstance(H[1], int) and H[1] == -1:
                dependency_str += " --dependency=aftercorr:%s " % (":".join(ids))
            else:
                print "ERROR <shedule_jobs> Invalid object ", H
                exit(1)
        if len(to_hold) == 0: return ""
        if len(dependency_str) > 0:
            return dependency_str

        return " --dependency=" + ("afterok:" if RequireOk else "after:") + ":".join(to_hold)


######################################################################
##      SUN-GRID ENGINE (SGE)
##  I do not know how widely it's used in ATLAS
#####################################################################
class SGEEngine(ClusterEngine):
    def __shedule_jobs(self, to_hold, sub_job):
        To_Hold = ""
        prettyPrint("", "#############################################################################")
        if len(sub_job) == 0: prettyPrint("Submit cluster job:", self.job_name())
        else: prettyPrint("Submit job: ", "%s in %s" % (sub_job, self.job_name()))
        prettyPrint("", "#############################################################################")
        info_written = False
        for H in To_Hold:
            if not info_written: prettyPrint("Hold %s until" % (sub_job if len(sub_job) else self.job_name()), "")
            if isinstance(H, str): To_Hold += " -hold_jid %s" % (H)
            elif isinstance(H, tuple): To_Hold += " -hold_jid %s" % (H[0])
            else:
                print "ERROR <_shedule_jobs>: Invalid object", H
                exit(1)
            prettyPrint("", H if isinstance(H, str) else H[0])

        return To_Hold

    def submit_job(
            self,
            script,
            sub_job="",
            mem=-1,
            env_vars=[],
            hold_jobs=[],
            run_time="",
    ):
        if not CreateDirectory(self.log_dir(), False): return False
        exec_script = self.link_to_copy_area(script)
        pwd = os.getcwd()
        os.chdir(self.log_dir())

        if not exec_script: return False
        if mem < 0:
            print "ERROR: Please give a reasonable memory"

        submit_cmd = "qsub -o %s -m a -j y -l h_vmem=%dM -l h_rt='%s' %s -N '%s' %s  -cwd %s" % (
            self.log_dir(), self.mail_user(), mem, run_time, self.__shedule_jobs(self.to_hold(hold_jobs), sub_job),
            self.subjob_name(sub_job), " - v ".join(["%s='%s'" % (var, value)
                                                     for var, value in env_vars + self.common_env_vars()]), exec_script)
        if os.system(submit_cmd): return False
        os.chdir(pwd)
        return True

    def submit_array(self, script, sub_job="", mem=-1, env_vars=[], hold_jobs=[], run_time="", array_size=-1):
        if not CreateDirectory(self.log_dir(), False): return False
        pwd = os.getcwd()
        os.chdir(self.log_dir())
        exec_script = self.link_to_copy_area(script)
        if not exec_script: return False
        if mem < 0:
            print "ERROR: Please give a reasonable memory"
            return False
        if array_size < 1:
            print "ERROR: Please give a valid array size"
            return False
        submit_cmd = "qsub -o %s -m a -j y -l h_vmem=%dM -t 1-%d -l h_rt='%s' %s -N '%s' %s  -cwd %s" % (
            self.log_dir(), self.mail_user(), mem, array_size, run_time, self.__shedule_jobs(self.to_hold(hold_jobs), sub_job),
            self.subjob_name(sub_job), " - v ".join(["%s='%s'" % (var, value)
                                                     for var, value in env_vars + self.common_env_vars()]), exec_script)
        os.chdir(pwd)
        return True


###############################################################
##              LocalEngine
## the local engine manages the local submission of the jobs
## i.e. many threads are started in parallel
###############################################################
class LocalEngine(ClusterEngine):
    def __init__(
            self,
            jobName="",
            baseDir="",
            maxCurrentJobs=-1,
    ):
        ClusterEngine.__init__(self, jobName=jobName, baseDir=baseDir, maxCurrentJobs=maxCurrentJobs, submit_build=False)
        self.__threads = []

    def get_threads(self):
        return self.__threads

    def n_threads(self):
        return len(self.get_threads())

    #### Basic method to submit a job
    def submit_job(self, script, sub_job="", mem=-1, env_vars=[], hold_jobs=[], run_time=""):
        ### Memory is senseless in this setup. Do not pipe it further
        return self.submit_array(script=script, sub_job=sub_job, env_vars=env_vars, hold_jobs=hold_jobs, array_size=1)

    def submit_array(self, script, sub_job="", mem=-1, env_vars=[], hold_jobs=[], run_time="", array_size=-1):
        if array_size < 1:
            print "ERROR <submit_array>: Please give a valid array size"
            return False
        pending_threads = []
        direct_pending = []
        for th in self.get_threads():
            for hold in hold_jobs:
                ### Simple name matching
                if isinstance(hold, str):
                    if hold == th.name():
                        pending_threads += [th]
                        break
                elif isinstance(hold, tuple):
                    if hold[0] == th.name() and th.thread_number() > 0:
                        if isinstance(hold[1], list):
                            if th.thread_number() in hold[1]:
                                pending_threads += [th]
                                break
                            elif -1 in hold[1]:
                                direct_pending += [th]
                        elif isinstance(hold[1], int) and -1 == hold[1]:
                            direct_pending += [th]
        for i in range(array_size):
            self.__threads += [
                LocalClusterThread(
                    thread_name=self.subjob_name(sub_job),
                    subthread=i + 1 if array_size > 0 else -1,
                    thread_engine=self,
                    dependencies=pending_threads + [th for th in direct_pending if th.thread_number() == i + 1],
                    script_exec=self.link_to_copy_area(script),
                    env_vars=env_vars,
                )
            ]
        return True

    def finish(self):
        executable = [th for th in self.get_threads() if th.is_launchable()]
        running = []
        runned = 0
        ### There are still some jobs to execute
        while runned < self.n_threads():
            running = [th for th in running if th.isAlive()]
            if len(running) < self.max_running_per_array():
                for i, th in enumerate(executable):
                    if len(running) + i >= self.max_running_per_array(): break
                    th.start()
                    runned += 1
                    running += [th]
                executable = [th for th in self.get_threads() if th.is_launchable()]
            time.sleep(1)

        while getRunningThreads(running) > 0:
            time.sleep(0.5)


class LocalClusterThread(threading.Thread):
    def __init__(self, thread_name="", subthread=-1, thread_engine=None, dependencies=[], script_exec="", env_vars=[]):
        threading.Thread.__init__(self)
        self.__engine = thread_engine
        self.__name = thread_name
        self.__sub_num = subthread

        self.__isSuccess = False
        self.__started = False
        self.__dependencies = [d for d in dependencies]
        self.__script_to_exe = script_exec
        self.__tmp_dir = "%s/%s" % (thread_engine.tmp_dir(), id_generator(50))
        CreateDirectory(self.__tmp_dir, True)
        self.__env_vars = [e for e in env_vars] + [("SGE_TASK_ID", "%d" % (self.thread_number())), ("TMPDIR", self.__tmp_dir)]

    def __del__(self):
        print "<LocalClusterThread>: Clean up %s" % (self.__tmp_dir)
        os.system("rm -rf %s" % (self.__tmp_dir))

    def dependencies(self):
        return self.__dependencies

    def thread_engine(self):
        return self.__engine

    def thread_number(self):
        return self.__sub_num

    def name(self):
        return self.__name

    def is_launchable(self):
        if self.isAlive() or self.__started: return False
        self.__dependencies = [th for th in self.__dependencies if th.isAlive() or not th.is_started()]
        return len(self.__dependencies) == 0

    def is_success(self):
        return self.__isSuccess

    def is_started(self):
        return self.__started

    def run(self):
        self.__started = True
        ###################
        self.__isSuccess = self._cmd_exec()

    def _cmd_exec(self):
        if not os.path.exists(self.__script_to_exe):
            print "ERROR <_cmd_exec>: Could not find %s" % (self.__script_to_exe)
            return False
        ### Threads can set their own enviroment variables without affecting the others
        os.system("chmod 0755 %s" % (self.__script_to_exe))
        print "INFO <_cmd_exec> Start %s to process %s" % (self.name(), self.__script_to_exe)
        return os.system("python %s --Cmd %s --envVars %s > %s/%s%s.log  2>&1" % (
            ResolvePath("ClusterSubmission/exeScript.py"),
            self.__script_to_exe,
            " ".join(["%s %s" % (var, value) for var, value in self.__env_vars]),
            self.thread_engine().log_dir(),
            self.name(),
            "" if self.thread_number() < 1 else "_%d" % (self.thread_number()),
        )) == 0


###############################
### Merging class
###############################
class MergeSubmit(object):
    def __init__(self, outFileName="", files_to_merge=[], hold_jobs=[], cluster_engine=None, files_per_job=5, final_split=1):
        self.__out_name = outFileName
        self.__cluster_engine = cluster_engine
        self.__hold_jobs = [h for h in hold_jobs]
        self.__files_per_job = files_per_job if files_per_job > 1 else 2
        self.__merge_lists = self.__assemble_merge_list(files_to_merge)
        self.__tmp_out_files = []
        self.__child_job = None
        self.__parent_job = None
        self.__submitted = False
        if len(self.__merge_lists) > final_split:
            self.__tmp_out_files = [
                "%s/%s_%s.root" % (self.engine().tmp_dir(), self.outFileName(), id_generator(16)) for d in range(len(self.__merge_lists))
            ]
            self.__child_job = self.create_merge_interface(final_split=final_split)
            self.__child_job.set_parent(self)
        elif final_split == 1:
            CreateDirectory(self.engine().out_dir(), False)
            self.__tmp_out_files = ["%s/%s.root" % (self.engine().out_dir(), self.outFileName())]
        else:
            CreateDirectory(self.engine().out_dir(), False)
            self.__tmp_out_files = [
                "%s/%s_%d.root" % (self.engine().out_dir(), self.outFileName(), i + 1)
                for i in range(min(final_split, len(self.__merge_lists)))
            ]

    def files_per_job(self):
        return self.__files_per_job

    def temporary_files(self):
        return self.__tmp_out_files

    def merge_lists(self):
        return self.__merge_lists

    def hold_jobs(self):
        return self.__hold_jobs if not self.parent() else [self.engine().subjob_name(self.parent().job_name())]

    def create_merge_interface(self, final_split=1):
        return self.engine().create_merge_interface(
            out_name=self.outFileName(),
            files_to_merge=self.temporary_files(),
            files_per_job=int(self.files_per_job() / 2),
            final_split=final_split,
        )

    def engine(self):
        return self.__cluster_engine

    def outFileName(self):
        return self.__out_name

    def childs_in_chain(self):
        if not self.__child_job: return 0
        return 1 + self.__child_job.childs_in_chain()

    def child(self):
        return self.__child_job

    def parent(self):
        return self.__parent_job

    def set_parent(self, parent):
        self.__parent_job = parent

    def job_name(self):
        if self.__child_job:
            return "MergeLvl_%d-%s" % (self.childs_in_chain(), self.outFileName())
        return "merge-%s" % (self.outFileName())

    def __assemble_merge_list(self, files_to_merge):
        copied_in = [x for x in files_to_merge]
        shuffle(copied_in)
        merge_lists = []
        merge_in = []
        for i, fi in enumerate(copied_in):
            if i > 0 and i % self.__files_per_job == 0:
                merge_name = "%s/MergeLists_%s_%s.txt" % (self.engine().config_dir(), self.outFileName(), id_generator(34))
                WriteList(merge_in, merge_name)
                merge_lists += [merge_name]
                merge_in = []
            merge_in += [fi]

        ### Pack the last remenants into a last merge job
        if len(merge_in) > 0:
            merge_name = "%s/MergeLists_%s_%s.txt" % (self.engine().config_dir(), self.outFileName(), id_generator(34))
            WriteList(merge_in, merge_name)
            merge_lists += [merge_name]
        return merge_lists

    def submit_job(self):
        if self.__submitted: return False
        final_merge_name = "%s/%s.txt" % (self.engine().config_dir(), id_generator(30))
        job_array = "%s/%s.txt" % (self.engine().config_dir(), id_generator(31))
        WriteList(self.merge_lists(), job_array)
        WriteList(self.temporary_files(), final_merge_name)
        if not self.engine().submit_array(
                script="ClusterSubmission/Merge.sh",
                sub_job=self.job_name(),
                mem=self.engine().merge_mem(),
                env_vars=[
                    ("JobConfigList", job_array),
                    ("OutFileList", final_merge_name),
                ],
                hold_jobs=self.hold_jobs(),
                run_time=self.engine().merge_time(),
                array_size=len(self.merge_lists())):
            return False
        self.__submitted = True
        if not self.child(): return True
        if not self.child().submit_job(): return False
        return self.engine().submit_clean_job(
            hold_jobs=[self.engine().subjob_name(self.child().job_name())], to_clean=self.temporary_files(), sub_job=self.job_name())


def setupBatchSubmitArgParser():
    BASEFOLDER = os.getenv("SGE_BASEFOLDER")
    MYEMAIL = USERNAME + "@rzg.mpg.de" if not os.getenv("JOB_MAIL") else os.getenv("JOB_MAIL")
    #######################################################################
    #               Basic checks for TestArea and BASEFOLDER              #
    #######################################################################
    if BASEFOLDER == None:
        print "INFO: Could not find the enviroment variable 'SGE_BASEFOLDER'. This variable defines the directory of your output & logs. "
        BASEFOLDER = "/ptmp/mpp/%s/Cluster/" % (USERNAME)
        print "INFO: Will set its default value to " + BASEFOLDER

    if not TESTAREA:
        print "Please Setup AthAnalysis"
        exit(1)

    parser = argparse.ArgumentParser(
        prog='BatchSubmitParser',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="This script provides the basic arguments to setup the cluster engine",
    )
    parser.add_argument('--jobName', '-J', help='Specify the JobName', required=True)
    parser.add_argument(
        '--HoldBuildJob', help='Specifiy job names which should be finished before the build job starts', default=[], nargs="+")
    parser.add_argument('--BaseFolder', help='Changes the BaseFolder where the OutputFiles are saved.', default=BASEFOLDER)
    parser.add_argument('--BuildTime', help='Changes the RunTime of the BuildJob', default='01:59:59')
    parser.add_argument('--MergeTime', help='Changes the RunTime of the merge Jobs', default='01:59:59')
    parser.add_argument("--jobArraySize", help="The maximum size of the slurm job-array", type=int, default=40000)
    parser.add_argument("--maxCurrentJobs", help="The maximum size of the slurm job-array", type=int, default=400)
    parser.add_argument('--Build_vmem', help='Changes the virtual memory needed by the build job', type=int, default=8000)
    parser.add_argument('--Merge_vmem', help='Changes the virtual memory needed by the merge job', type=int, default=100)
    parser.add_argument('--nBuild_Cores', help="How many cores shall be used for the build job", type=int, default=2)
    parser.add_argument(
        "--nMaxCurrentJobs", help="Enable this option to restrict the number of simultaneous occurring jobs", type=int, default=100)
    parser.add_argument("--mailTo", help="Specify a notification E-mail address", default=MYEMAIL)
    parser.add_argument("--engine", help="What is the grid engine to use", choices=["SLURM", "LOCAL", "SGE"], required=True)
    parser.add_argument('--noBuildJob', help='Do not submit the build job', default=True, action="store_false")
    return parser


def setup_engine(RunOptions):
    if not RunOptions.noBuildJob:
        print "WARNING: You are submitting without any schedule of an build job. This is not really recommended"

    if RunOptions.engine == "SLURM":
        return SlurmEngine(
            jobName=RunOptions.jobName,
            baseDir=RunOptions.BaseFolder,
            buildTime=RunOptions.BuildTime,
            mergeTime=RunOptions.MergeTime,
            buildCores=RunOptions.nBuild_Cores,
            buildMem=RunOptions.Build_vmem,
            mergeMem=RunOptions.Merge_vmem,
            maxArraySize=RunOptions.jobArraySize,
            maxCurrentJobs=RunOptions.maxCurrentJobs,
            mail_user=RunOptions.mailTo,
            hold_build=RunOptions.HoldBuildJob,
            submit_build=RunOptions.noBuildJob)
    elif RunOptions.engine == "SGE":
        return SGEEngine(
            jobName=RunOptions.jobName,
            baseDir=RunOptions.BaseFolder,
            buildTime=RunOptions.BuildTime,
            mergeTime=RunOptions.MergeTime,
            buildCores=RunOptions.nBuild_Cores,
            buildMem=RunOptions.Build_vmem,
            mergeMem=RunOptions.Merge_vmem,
            maxArraySize=RunOptions.jobArraySize,
            maxCurrentJobs=RunOptions.maxCurrentJobs,
            mail_user=RunOptions.mailTo,
            hold_build=RunOptions.HoldBuildJob,
            submit_build=RunOptions.noBuildJob)
    elif RunOptions.engine == "LOCAL":
        return LocalEngine(
            jobName=RunOptions.jobName,
            baseDir=RunOptions.BaseFolder,
            maxCurrentJobs=min(max(1, RunOptions.nMaxCurrentJobs), 16),
        )

    ###
    print "ERROR <setup_engine> : How could you parse %s? It's not part of the choices" % (RunOptions.engine)
    exit(1)
