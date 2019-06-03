import os, sys, argparse, commands, ROOT, threading, time
from ClusterSubmission.Utils import CheckRemainingProxyTime, IsROOTFile, WriteList, FillWhiteSpaces, ClearFromDuplicates, CheckRucioSetup, CheckPandaSetup, CreateDirectory, ExecuteCommands, ReadListFromFile, ExecuteThreads, ResolvePath, id_generator
from ClusterSubmission.AMIDataBase import getAMIDataBase
from ClusterSubmission.PeriodRunConverter import getGRL
from XAMPPbase.Utils import IsTextFile, IsListIn
from XAMPPbase.CreateAODFromDAODList import convertToAOD
from XAMPPbase.SubmitToGrid import DoesDSExists, getTarBallOptions, GetNFiles
from pprint import pprint
from array import array


class PrwMerging(threading.Thread):
    def __init__(self, campaign, stype, rtag, datasets, temp_dir, outdir, check_consistency=False, notDownloadAgain=True):
        threading.Thread.__init__(self)
        self.__campaign = campaign
        self.__stype = stype
        self.__rtag = rtag
        self.__datasets = datasets
        self.__dsids = ClearFromDuplicates([GetPRW_datasetID(ds) for ds in self.__datasets])
        self.__purged = []
        self.__tempdir = temp_dir
        self.__outdir = outdir
        self.__check_consistency = check_consistency
        self.__to_black_list = []
        self.__ds_to_submit = []
        self.__inconsistent_log = []
        self.__already_on_disk = [] if not notDownloadAgain or not os.path.exists("%s/Finished.txt" %
                                                                                  (self.download_dir())) else ReadListFromFile(
                                                                                      "%s/Finished.txt" % (self.download_dir()))
        if check_consistency:
            getAMIDataBase().getMCDataSets(channels=self.dsids(), campaign="%s" % (self.campaign()), derivations=[])

    def events_in_prwFile(self, directory, ds):
        prw_configs = ["%s/%s/%s" % (directory, ds, f) for f in os.listdir("%s/%s" % (directory, ds)) if IsROOTFile(f)]
        prw_helper = self.__setup_prw_helper(config_files=prw_configs)
        prw_period = prw_helper.getPRWperiods_fullsim()[0]
        return prw_helper.nEventsPerPRWperiod_full(GetPRW_datasetID(ds), prw_period)

    def final_directory(self):
        return self.__outdir

    def final_file(self):
        return "%s/%s_%s_%s_NTUP_PILEUP.root" % (self.__outdir, self.__campaign, self.__stype, self.__rtag)

    def campaign(self):
        return self.__campaign

    def rtag(self):
        return self.__rtag

    def stype(self):
        return self.__stype

    def isAFII(self):
        return self.stype() == "AF2"

    def dsids(self):
        return self.__dsids

    def datasets(self):
        return self.__datasets

    def purged(self):
        return self.__purged

    def hasDataset(self, to_test):
        for known in self.datasets():
            if known.find(":") != -1: known = known[known.find(":") + 1:]
            if known == to_test: return True
        return False

    def clearFromDuplicates(self, directory=""):
        print "INFO: Clear input of %s from duplicates and empty datasets" % (self.final_file())
        samples = []
        #### After downloading everything we need to clean it from the duplicates and remove
        #### all dataset containers which are empty
        #### To remove the empty datasets the fastest way is to check if the rucio download directory contains
        #### ROOT files
        for i in range(len(self.datasets())):
            ### We know persuade a now approach. It seems that extensions of a dataset are assigned
            ### to different NTUP_PILEUP p-tags so we need to download them all?
            ds_list = []
            dsid_to_check = GetPRW_datasetID(self.__datasets[i])
            tag = GetAMITagsMC(self.__datasets[i], SkimPTag=True, SkimSTag=False, SkimETag=False)
            while i < len(self.datasets()) and (GetPRW_datasetID(self.__datasets[i]) == dsid_to_check
                                                and tag == GetAMITagsMC(self.__datasets[i], SkimPTag=True, SkimSTag=False, SkimETag=False)):
                ds = self.__datasets[i]
                smp_dir = "%s/%s" % (directory, ds)
                if os.path.isdir(smp_dir) and len([f for f in os.listdir(smp_dir) if IsROOTFile(f)]) > 0: ds_list += [ds]
                i += 1

            if len(ds_list) == 0: continue
            if len(ds_list) > 1:
                ds_pairs = [(x, self.events_in_prwFile(directory, x)) for x in ds_list]
                ds_list = [d[0] for d in sorted(ds_pairs, key=lambda x: x[1], reverse=True)]
            if self.__check_consistency:
                ### Setup the PileupHelper instance to read the prw config files
                ami_lookup = getAMIDataBase().getMCchannel(dsid_to_check, "%s" % (self.campaign()))
                if not ami_lookup:
                    print "WARNING: The dataset %s does not exist in AMI at all. Interesting that we made prw files out of it" % (ds)
                    continue

                config_file_tag = GetAMITagsMC(DS=ds_list[0], SkimPTag=True, SkimETag=False, SkimSTag=False)
                ev_in_ami = ami_lookup.getEvents(tag=config_file_tag)

                if ev_in_ami == -1:
                    print "WARNING: no AMI tag could be found for dataset %s " % (ds)
                    for T in ami_lookup.getTags():
                        print "        --- %s: %d" % (T, ami_lookup.getEvents(tag=T))
                    continue

                ds_to_add = []
                ev_in_prw = 0
                for ds in ds_list:
                    ev_in_ds = self.events_in_prwFile(directory, ds)
                    ev_in_prw += ev_in_ds
                    if ev_in_ds == ev_in_ami:
                        ds_to_add = [ds]
                        break
                    ### We still can add datasets
                    if ev_in_ami >= ev_in_prw:
                        ds_to_add += [ds]
                        if ev_in_ami == ev_in_prw: break

                if ev_in_prw != ev_in_ami:
                    print "WARNING: %s has different number of events in AMI (%d) vs. NTUP_PILEUP (%d)" % (ds, ev_in_ami, ev_in_prw)
                    self.__inconsistent_log += ["%s    %d  %d" % (ds, ev_in_ami, ev_in_prw) for ds in ds_list]
                    ds_to_add = []
                ### Somehow we've more events in the config file than in AMI... Definetly a candidte to blacklist
                if ev_in_ami < ev_in_prw: self.__to_black_list += ds_to_add

            samples += ds_list

        if self.__check_consistency:
            WriteList(samples, "%s/Finished.txt" % (self.download_dir()))
        new_dsids = ClearFromDuplicates([GetPRW_datasetID(ds) for ds in samples])
        if len(self.dsids()) != len(new_dsids):
            self.__dsids = sorted(new_dsids)
            self.__purged = sorted([GetPRW_datasetID(ds) for ds in self.__datasets if GetPRW_datasetID(ds) not in self.dsids()])
            print "INFO: %d dsids have been eliminated since all input files are invalid." % (len(self.purged()))

        #### for the removal the sorting is important
        #### 1) official vs. privately produced
        #### 2) Newer ptag vs old
        ##samples = sorted(samples, cmp=lambda x,y: PRWdatasetSorter(x,y))
        AOD_Samples = []
        for s in samples:
            AOD = "%d.%s" % (GetPRW_datasetID(s), GetAMITagsMC(s, SkimPTag=True, SkimETag=False, SkimSTag=False))
            if not AOD in AOD_Samples:
                self.__datasets += [s]
                AOD_Samples.append(AOD)

        print "INFO: Will merge %d files to %s" % (len(self.datasets()), self.final_file())

    def download_dir(self):
        return "%s/%s_%s_%s" % (self.__tempdir, self.__campaign, self.__stype, self.__rtag)

    def run(self):
        CreateDirectory(self.download_dir(), False)
        CreateDirectory(self.final_directory(), False)
        #self.__datasets = sorted(self.__datasets, key=lambda x: GetPRW_datasetID(x))
        DownloadList = [
            'rucio download --ndownloader 5 --dir %s %s' % (self.download_dir(), ds) for ds in self.__datasets
            if ds not in self.__already_on_disk
        ]
        ExecuteCommands(ListOfCmds=DownloadList, MaxCurrent=16)
        self.clearFromDuplicates(self.download_dir())
        Files = []
        for dir in os.listdir(self.download_dir()):
            dir_path = "%s/%s" % (self.download_dir(), dir)
            if not os.path.isdir(dir_path): continue
            if not self.hasDataset(dir): continue
            Files += ["%s/%s" % (dir_path, F) for F in os.listdir(dir_path) if IsROOTFile(F)]

        WriteList(sorted(Files), "%s/temp_in.txt" % (self.download_dir()))
        #   only 1 entry in the MCPileupReweighting tree per Channel/RunNumber combination is actually needed
        #   thus, remove all others but one in order to significantly reduce the files size!
        #   This is done by the SlimPRWFile macro in XAMPPbase/utils/
        MergeCmd = "SlimPRWFile --InList %s/temp_in.txt --outFile %s" % (self.download_dir(), self.final_file())
        print MergeCmd
        os.system(MergeCmd)
        print "INFO: Clean up the temporary file"
        os.system("rm %s/temp_in.txt " % (self.download_dir()))

        self.standaloneCheck()

        print "INFO: Done"

    def __setup_prw_helper(self, config_files=[]):
        prw_helper = ROOT.XAMPP.PileupHelper(id_generator(24))
        prw_config_files = ROOT.std.vector(str)()
        for f in config_files:
            if IsROOTFile(f): prw_config_files.push_back(f)

        prw_helper.loadPRWperiod_fullsim(prw_config_files)

        if len(prw_helper.getPRWperiods_fullsim()) != 1:
            print "WARNING: More than one period..."
            exit(1)
        return prw_helper

    def standaloneCheck(self):
        if not self.__check_consistency: return True
        prw_helper = self.__setup_prw_helper(config_files=[self.final_file()])
        prw_period = prw_helper.getPRWperiods_fullsim()[0]
        missing_dsids = [ds for ds in self.dsids() if prw_helper.nEventsPerPRWperiod_full(ds, prw_period) <= 0]
        ### Submit another prw job on the purged prw files
        if len(self.purged()) > 0 or len(missing_dsids) > 0:
            print "WARNING: %d datasets were purged from the prw file %s" % (len(self.purged()) + len(missing_dsids), self.final_file())
            purged_AODs = []
            for ds in ClearFromDuplicates(self.purged() + missing_dsids):
                for A in getAODsFromRucio(self.campaign(), ds, self.isAFII()):
                    all_tags = GetAMITagsMC(A, SkimETag=False, SkimSTag=False, SkimPTag=True)
                    ### Reject second e-tag
                    if all_tags.find("_e") != -1: continue
                    ### Reject second r-tag
                    if all_tags.find("_r") != all_tags.rfind("_r"): continue
                    ### Reject second s-tag
                    if all_tags.find("_s") != all_tags.rfind("_s"): continue
                    ## Or a tag
                    if all_tags.find("_a") != all_tags.rfind("_a"): continue
                    rtag = all_tags.split("_")[-1]
                    if rtag != self.rtag() or A in self.__ds_to_submit: continue
                    self.__ds_to_submit.append(A)

            os.system("rm %s" % (self.final_file()))
            return False
        return True

    def black_listed(self):
        return self.__to_black_list

    def missing_log(self):
        return self.__inconsistent_log

    def missing_ds(self):
        return self.__ds_to_submit

    def print_datasets(self):
        print "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-"
        print "INFO: The following datasets were used to merge %s" % (self.final_file())
        for ds in self.datasets():
            print "    --- %s " % (ds)
        print "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-"


#####################
#       GLOBALS     #
#####################
PatternsToExclude = ['Sherpa_CT10', 'ParticleGun', "BeamHaloGenerator", "ParticleGenerator"]
USERNAME = os.getenv("USER")
TEMP = "/ptmp/mpp/%s/TEMP/PRW_Merge/" % (USERNAME)
XAMPP_GIT = "atlas-mpp-xampp"
ILUMICALCFILES_1516 = getGRL([15, 16], flavour='lumiCalc')
ILUMICALCFILES_17 = getGRL(17, flavour='lumiCalc')
ILUMICALCFILES_18 = getGRL(18, flavour='lumiCalc')


def readPRWchannels(File):
    print "INFO: Open prw config file %s to read in all channels." % (File)
    if not os.path.exists(File):
        print "ERROR: The file %s does not exist" % (File)
        return []
    prw_File = ROOT.TFile.Open(File, "READ")
    if not prw_File or not prw_File.IsOpen():
        print "ERROR: Failed to open file %s" % (File)
        return []
    prw_Tree = prw_File.Get("PileupReweighting/MCPileupReweighting")
    channels = []
    for i in range(prw_Tree.GetEntries()):
        prw_Tree.GetEntry(i)
        channels += [prw_Tree.Channel]
    return channels


def GetAMITagsMC(DS, SkimPTag=False, SkimETag=True, SkimSTag=True):
    Tag = DS[DS.rfind(".") + 1:]
    if SkimPTag:
        while SkimETag and Tag.find("e") != -1:
            Tag = Tag[Tag.find("_") + 1:]
        while SkimSTag and Tag.find("s") != -1:
            Tag = Tag[Tag.find("_") + 1:]
        while SkimSTag and Tag.find("a") != -1:
            Tag = Tag[Tag.find("_") + 1:]
        while Tag.rfind("_p") != -1:
            Tag = Tag[:Tag.rfind("_p")]
        return Tag
    else:
        if Tag.rfind("_p") != -1:
            Tag = Tag[Tag.find("_p"):]
            while Tag.rfind("_p") != Tag.rfind("_"):
                Tag = Tag[:Tag.rfind("_")]
            if Tag.startswith("_"): Tag = Tag[1:]
            return GetAMITagsMC(DS, SkimPTag=True, SkimETag=SkimETag, SkimSTag=SkimSTag) + "_" + Tag
        else:
            return GetAMITagsMC(DS, SkimPTag=True, SkimETag=SkimETag, SkimSTag=SkimSTag)
    return Tag


def GetDatasets(campaign, skip_empty=False, requested_ds=[], restrictToRequestedDatasets=False):
    ### For mc16 there are several prw files merge.NTUP_PILEUP and deriv.NTUP_PILEUP
    Cmd = "rucio list-dids %s:%s.*deriv.NTUP_PILEUP* --filter type=CONTAINER --short" % (campaign, campaign)
    Datasets = [entry.strip() for entry in commands.getoutput(Cmd).split("\n")]
    Cmd = "rucio list-dids %s:%s.*merge.NTUP_PILEUP* --filter type=CONTAINER --short" % (campaign, campaign)
    Datasets += [entry.strip() for entry in commands.getoutput(Cmd).split("\n")]

    print "################################################################################"
    print "INFO: Found %d NTUP_PILEUP datasets in rucio for campaign %s" % (len(Datasets), campaign)
    DatasetList = []
    black_list = getPRWblackList()
    for i, entry in enumerate(Datasets):
        #### Filter the official datasets from the beginning
        if entry in black_list: continue
        if not IsNeededDataset(entry, requestedDSs=requested_ds, restrictToRequestedDatasets=restrictToRequestedDatasets):
            continue
        elif not skip_empty:
            DatasetList.append(entry)
        elif GetNFiles(entry) > 0:
            DatasetList.append(entry)
        else:
            print "WARNING: The dataset %s has no files " % (entry)
        if i % int(min([len(Datasets) / 100, 100])) == 0:
            print "Checked successfully %d / %d datasets" % (i, len(Datasets))

    print "INFO: %d NTUP_PILEUP are non-empty" % (len(DatasetList))
    for user in getUsersSubmittedPRW():
        print "INFO: Look for privately produced NTUP_PILEUP datsets submitted by %s." % (user)
        rucio_cmd = "rucio list-dids %s:%s.%s.*.XAMPP_PILEUP*_NTUP_PILEUP --filter type=CONTAINER --short" % (user, user, campaign)
        Datasets = commands.getoutput(rucio_cmd).split()
        DatasetList += [
            entry.strip() for entry in Datasets if entry.strip() not in black_list
            and IsNeededDataset(entry.strip(), requestedDSs=requested_ds, restrictToRequestedDatasets=restrictToRequestedDatasets)
        ]

    print "################################################################################"
    return sorted(DatasetList)


def getUsersSubmittedPRW():
    FileName = ResolvePath("XAMPPbase/UsersWhoSubmittedPRW.txt")
    if not FileName:
        print "ERROR: The file XAMPPbase/data/UsersWhoSubmittedPRW.txt could not be found in the repository"
        print "ERROR: Did you delete it by accident? Please check!!!!"
        sys.exit(1)
    return sorted(ReadListFromFile(FileName))


def getPRWblackList():
    FileName = ResolvePath("XAMPPbase/BlackListedPRWdatasets.txt")
    if not FileName:
        print "ERROR: The file XAMPPbase/data/BlackListedPRWdatasets.txt could not be found in the repository"
        print "ERROR: Did you delete it by accident? Please check!!!!"
        sys.exit(1)
    return sorted(ReadListFromFile(FileName))


def getGITremotes():
    remotes = {}
    current_dir = os.getcwd()
    Pkg_Dir = os.path.realpath(ResolvePath("XAMPPbase"))
    os.chdir(Pkg_Dir)
    for line in commands.getoutput("git remote --verbose").split("\n"):
        remote_name = line.split()[0]
        remote_url = line.split()[1]
        remotes[remote_name] = remote_url
    os.chdir(current_dir)
    return remotes


def setupGITupstream(upstream="upstream"):
    current_dir = os.getcwd()
    Pkg_Dir = os.path.realpath(ResolvePath("XAMPPbase"))
    remotes = getGITremotes()
    if len(remotes) == 0:
        print "ERROR: No remote GIT repository has been found. How did you get the code?"
        exit(1)
    if upstream in remotes.iterkeys():
        if remotes[upstream].split("/")[-2] == XAMPP_GIT:
            os.chdir(Pkg_Dir)
            os.system("git fetch %s" % (upstream))
            os.chdir(current_dir)
            return upstream
    else:
        print "INFO: Add the original XAMPPbase repository to the remote list"
        URLs = [R for R in remotes.itervalues()]
        UP_URL = URLs[0]
        ToReplace = UP_URL.split("/")[-2]
        UP_URL = UP_URL.replace(ToReplace, XAMPP_GIT)
        os.chdir(Pkg_Dir)
        os.system("git remote add %s %s" % (upstream, UP_URL))
        os.system("git fetch %s" % (upstream))
        os.chdir(current_dir)
        return upstream


def getBranch():
    current_dir = os.getcwd()
    Pkg_Dir = os.path.realpath(ResolvePath("XAMPPbase"))
    os.chdir(Pkg_Dir)
    branch = None
    for B in commands.getoutput("git branch").split("\n"):
        if B.startswith("*") and B.find("(no branch)") == -1: branch = B
    os.chdir(current_dir)
    return branch


def insertPRWUser(user):
    Users = getUsersSubmittedPRW()
    if user in Users: return
    Users += [user]
    current_dir = os.getcwd()
    FileName = os.path.realpath(ResolvePath("XAMPPbase/UsersWhoSubmittedPRW.txt"))
    Pkg_Dir = os.path.realpath(ResolvePath("XAMPPbase"))
    ###############################################################################
    #      Find out the current branch to propagage only                          #
    #      the updated List to the main repository. Other changes regarding       #
    #      side developments of the package should not be propagated yet          #
    ###############################################################################
    upstream = setupGITupstream()
    current_branch = getBranch()
    os.chdir(Pkg_Dir)
    new_branch = "PRW_" + user.replace(".", "_")
    if current_branch:
        os.system("git commit -am \"Commit changes of all files in order to push the 'UsersWhoSubmittedPRW.txt'\"")
    print "INFO: Create new branch %s to update the UsersWhoSubmittedPRW " % (new_branch)
    os.system("git checkout -b %s %s/master" % (new_branch, upstream))
    print "INFO: %s submitted to the grid prw_config jobs. Add him to the common list such that others can download his files" % (user)
    WriteList(sorted(Users), FileName)
    os.system("git add UsersWhoSubmittedPRW.txt")
    os.system("git commit UsersWhoSubmittedPRW.txt -m \"Added %s to the list of users who submitted a prw config creation job\"" % (user))
    os.system("git push %s %s" % (upstream, new_branch))
    if current_branch: os.system("git checkout %s" % (current_branch))
    os.chdir(current_dir)


def updateBlackList(black_list):
    current_black = getPRWblackList()
    if IsListIn(black_list, current_black): return
    current_black = ClearFromDuplicates(current_black + black_list)
    current_dir = os.getcwd()
    FileName = os.path.realpath(ResolvePath("XAMPPbase/BlackListedPRWdatasets.txt"))
    Pkg_Dir = os.path.realpath(ResolvePath("XAMPPbase"))
    ###############################################################################
    #      Find out the current branch to propagage only                          #
    #      the updated List to the main repository. Other changes regarding       #
    #      side developments of the package should not be propagated yet          #
    ###############################################################################
    upstream = setupGITupstream()
    current_branch = getBranch()
    os.chdir(Pkg_Dir)
    new_branch = "PRW_%s_%s" % (time.strftime("%Y%m%d"), USERNAME)
    if current_branch:
        os.system("git commit -am \"Commit changes of all files in order to push the 'BlackListedPRWdatasets.txt'\"")
    print "INFO: Create new branch %s to update the BlackListedPRWdatasets " % (new_branch)
    os.system("git checkout -b %s %s/master" % (new_branch, upstream))
    WriteList(sorted(current_black), FileName)
    os.system("git add BlackListedPRWdatasets.txt")
    os.system("git commit BlackListedPRWdatasets.txt -m \"Updated the list of black prw files\"")
    os.system("git push %s %s" % (upstream, new_branch))
    if current_branch: os.system("git checkout %s" % (current_branch))
    os.chdir(current_dir)


def submitPRWFiles(DataSets=[], RUCIO=None, RSE=None, Official=False):
    if RUCIO == None:
        print "ERROR: Please provide a rucio account to use"
        sys.exit(1)
    if RSE == None:
        print "ERROR: The submission of prw samples does not make sense if there is no long-term storage for them"
        sys.exit(1)

    os.environ["RUCIO_ACCOUNT"] = RUCIO
    ### Such that eveyone of the XAMPP community can benefit from more or less XAMPP intern
    ### prwFiles we need to know which user actually submitted a prw job.
    insertPRWUser("user.%s" % (RUCIO) if not Official else "group.%s" % (RUCIO))

    black_files = getPRWblackList()
    for DS in DataSets:
        InDS = convertToAOD(DS)
        if not GetNFiles(InDS): continue
        OutDS = "user.%s.%s.%s.XAMPP_PILEUP.%s" % (RUCIO, InDS.split(".")[0], InDS.split(".")[1], InDS[InDS.find("AOD") + 4:])
        if Official:
            OutDS = "group.%s.%s.%s.XAMPP_PILEUP.%s" % (RUCIO, InDS.split(".")[0], InDS.split(".")[1], InDS[InDS.find("AOD") + 4:])
        version = 1
        while OutDS in black_files:
            OutDS = "user.%s.%s.%s.XAMPP_PILEUP_v%d.%s" % (RUCIO, InDS.split(".")[0], InDS.split(".")[1], version,
                                                           InDS[InDS.find("AOD") + 4:])
            if Official:
                OutDS = "group.%s.%s.%s.XAMPP_PILEUP_v%d.%s" % (RUCIO, InDS.split(".")[0], InDS.split(".")[1], version,
                                                                InDS[InDS.find("AOD") + 4:])
            version += 1

        SubmitCmd = "pathena PileupReweighting/generatePRW_jobOptions.py --mergeOutput \
                    --useShortLivedReplicas --express --forceStaged \
                    --inDS=\"%s\" --outDS=\"%s\" %s %s %s" % (InDS, OutDS, "" if not RSE else "--destSE='%s'" % (RSE),
                                                              "" if not Official else "--official --voms=atlas:/atlas/%s/Role=production" %
                                                              (RUCIO), " ".join(getTarBallOptions()))
        print "INFO: Submit new prw job %s on input AOD: %s." % (OutDS, InDS)
        #os.system(SubmitCmd)


def GetPRW_datasetID(DS):
    try:
        dsid = int(DS.split('.')[1])
        return dsid
    except:
        pass
    ds_copy = DS
    if ds_copy.find(":") != -1: ds_copy = ds_copy[ds_copy.find(":") + 1:]
    for Sub in getUsersSubmittedPRW():
        if ds_copy.startswith(Sub):
            ds_copy = ds_copy[len(Sub) + 1:]
            return GetPRW_datasetID(ds_copy)
    print "WARNING: Could not extract the DSID from %s" % (DS)
    return 0


def GetPRW_campaign(DS):
    if DS.find("XAMPP_PILEUP") != -1:
        for Sub in getUsersSubmittedPRW():
            ###<Scope>.mc16_13TeV.XAMPP_PILEUP.<RTAG>
            if DS.startswith(Sub):
                return DS[len(Sub) + 1:DS.find(".", len(Sub) + 1)]
        print "WARNING: Could not extract the campaign from %s" % (DS)
        return ""
    ### Official dataset mc16_13TeV
    return DS[:DS.find(".")]


# for now, we only want to have mcChannelNumber correspoding to either the SM, Higgs or Top WG
# -> Exotics/SUSY etc. signal samples have to be added by the analyzers via this requestedDSs mechanism
def IsRequestedDataset(dataset, requestedDSs=[]):
    dsid = GetPRW_datasetID(dataset)
    for requested in requestedDSs:
        req_dsid = GetPRW_datasetID(requested)
        if req_dsid == dsid: return True
    return False


def IsNeededDataset(dataset, doFilter=False, requestedDSs=[], restrictToRequestedDatasets=False):
    # do not take every ATLAS DSID, but take SM+Top+Higgs for now
    # (cf. https://svnweb.cern.ch/trac/atlasoff/browser/Generators/MC15JobOptions/trunk/share/Blocks.list)
    dsid = GetPRW_datasetID(dataset)
    amiTag = GetAMITagsMC(dataset, SkimPTag=True, SkimETag=False, SkimSTag=False)
    ### Okay now we need to reject some tags
    ### According to the latest recommendations only 3 tags are allowed
    ### I.e. eXXXX_sXXXX_rXXXX
    if amiTag.find("_e") != -1: return False
    ### Only one rtag is allowed
    if amiTag.rfind("_r") != amiTag.find("_r"): return False
    ### Reject datases with more than one p-tag
    if amiTag.rfind("_p") != amiTag.find("_p"): return False
    if not doFilter: return True

    if not restrictToRequestedDatasets:
        if dsid >= 341000 and dsid <= 360999: return True  # Higgs
        if dsid >= 361000 and dsid <= 369999: return True  # Standard Model
        if dsid >= 410000 and dsid <= 414999: return True  # Top
    if IsRequestedDataset(dataset, requestedDSs): return True
    return False


def passesNamePattern(sample, pattern=[], exclude_pattern=[], requested_DS=[]):
    if sample.find("NTUP_PILEUP") != -1:
        if IsRequestedDataset(sample, requested_DS): return True

        if len([exclude for exclude in exclude_pattern if sample.find(exclude) != -1]) > 0:
            return False
        if len(pattern) == 0: return True
        if len([pat for pat in pattern if sample.find(pat) != -1]) != len(pattern):
            return False

        return True
    if sample.find("XAMPP_PILEUP") != -1:
        if len(pattern) == 0: return True
        if IsRequestedDataset(sample, requested_DS): return True
    return False


def IsMissingDataSet(prw_files=[], DS=""):
    #### Assume that the DS is an AOD
    processed_DS = "%s.%s.XAMPP_PILEUP.%s" % (DS.split(".")[0], GetPRW_datasetID(DS), DS[DS.find("AOD") + 4:])
    for prw in prw_files:
        if convertToAOD(prw) == DS: return False
        if prw.find(processed_DS) != -1: return False
    return True


def IsOfficialPRWdataset(ds):
    if ds.find("XAMPP_PILEUP") != -1: return False
    return True


def PRWdatasetSorter(DS1, DS2):
    dsid_1 = GetPRW_datasetID(DS1)
    dsid_2 = GetPRW_datasetID(DS2)
    rtag_1 = GetAMITagsMC(DS1, SkimPTag=False)
    rtag_2 = GetAMITagsMC(DS2, SkimPTag=False)
    if IsOfficialPRWdataset(DS1) == IsOfficialPRWdataset(DS2):
        if dsid_1 < dsid_2: return -1
        elif dsid_1 > dsid_2: return 1
        ### The ptag must be newer
        if rtag_1 > rtag_2: return -1
        elif rtag_1 < rtag_2: return 1
    elif IsOfficialPRWdataset(DS1): return -1
    else: return 1
    return 0


def setupPRWTool(mc_config_files=[], name="prw_testing_tool", isAF2=False, use1516=True, use17=True):
    import ROOT
    try:
        prw_tool = ROOT.XAMPP.PileupHelper(name + ("_fullsim" if not isAF2 else "_af2"))
    except:
        ROOT.xAOD.Init().ignore()
        prw_tool = ROOT.XAMPP.PileupHelper(name + ("_fullsim" if not isAF2 else "_af2"))

    PRWConfig = ROOT.std.vector(str)()
    LumiCalc = ROOT.std.vector(str)()
    for PRW in mc_config_files:
        PRWConfig.push_back(PRW)

    if use1516:
        for lumi in ILUMICALCFILES_1516:
            LumiCalc.push_back(lumi)
    if use17:
        for lumi in ILUMICALCFILES_17:
            LumiCalc.push_back(lumi)

    prw_tool.setConfigFiles(PRWConfig)
    prw_tool.setLumiCalcFiles(LumiCalc)

    if not prw_tool.initialize(): exit(1)

    return prw_tool


def getAODsFromRucio(campaign, dsid, af2=False):
    rucio_cmd = "rucio list-dids %s.%d*recon.AOD.*%s --filter type=CONTAINER --short" % (campaign, dsid, "_s*" if not af2 else "_a*")
    return [l.strip().replace("\n", "") for l in commands.getoutput(rucio_cmd).split("\n") if len(l.strip().replace("\n", "")) > 0]


def performConsistencyCheck(RunOptions, mc16a_file, mc16d_file, mc16e_file):
    if not mc16a_file or not mc16d_file: return True
    mc16a_file.print_datasets()
    mc16d_file.print_datasets()
    prwTool = setupPRWTool(mc_config_files=[mc16a_file.final_file(), mc16d_file.final_file()], isAF2=mc16a_file.isAFII())
    dsids = ClearFromDuplicates(mc16a_file.dsids() + mc16d_file.dsids())
    missing_dsids = []

    for ds in sorted(dsids):
        if not prwTool.isDSIDvalid(ds): missing_dsids.append(ds)
    if len(missing_dsids) == 0:
        print "########################################################################"
        print "INFO: Consistency check of the two prw files is successful"
        print "INFO: In none of the mc16a(%s) and mc16d(%s) files a dsid has been kicked by the tool" % (mc16a_file.final_file(),
                                                                                                         mc16d_file.final_file())
        print "########################################################################"
        return True

    print "#############################################################################################"
    print "INFO: %d out of the %d DSIDs are kicked by the prwTool " % (len(missing_dsids), len(dsids))
    prw_files = [
        convertToAOD(f) for f in mc16a_file.datasets() + mc16d_file.datasets()
        if GetPRW_datasetID(f) in missing_dsids and IsOfficialPRWdataset(f)
    ]
    #prw_files = sorted(prw_files, cmp=lambda x, y: PRWdatasetSorter(x, y))
    for p in prw_files:
        print "        --- %s" % (p)
    ### Look in AMI for the AODs
    AODs = []
    print "INFO: Start to search for the corresponding AODs in both campaigns mc16a (%s) and mc16d (%s)" % (mc16a_file.rtag(),
                                                                                                            mc16d_file.rtag())
    for i, ds in enumerate(missing_dsids):
        for A in getAODsFromRucio(mc16a_file.campaign(), ds, mc16a_file.isAFII()):
            rtag = GetAMITagsMC(A)
            if rtag != mc16a_file.rtag() and rtag != mc16d_file.rtag(): continue
            if convertToAOD(A) in prw_files: continue
            AODs.append(A)
        if i % int(min([len(missing_dsids) / 10, 100])) == 0:
            print "INFO: Searched %d / %d datasets" % (i, len(missing_dsids))

    if len(AODs) == 0: return True

    ### Delete the prw files since they are inconsistent
    os.system("rm %s" % (mc16a_file.final_file()))
    os.system("rm %s" % (mc16d_file.final_file()))

    print "INFO: Found the following %d AODs to submit prw jobs:" % (len(AODs))
    for A in AODs:
        print "       *** %s" % (A)
    print "#############################################################################################"
    submitPRWFiles(DataSets=AODs, RUCIO=RunOptions.rucio, RSE=RunOptions.destRSE, Official=RunOptions.official)
    return False


if __name__ == "__main__":
    CheckPandaSetup()
    CheckRucioSetup()
    CheckRemainingProxyTime()
    RSE = None if not os.getenv("RUCIO_RSE") else os.getenv("RUCIO_RSE")
    RUCIO_ACCOUNT = os.getenv("RUCIO_ACCOUNT")

    parser = argparse.ArgumentParser(
        prog='CreateMergedNTUP_PILEUP',
        description=
        'This script searches for NTUP_PILEUP derivations in rucio (or takes a given list) and sorts the datasets by their AMI-tags. Then it donwloads and merges them accordingly.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--campaign', '-c', '-C', help='campaign to look for', nargs="+", type=str, default=['mc16_13TeV'])
    parser.add_argument('--outDir',
                        '-o',
                        '-O',
                        help='output directory to merged NTUP_PILEUP files',
                        type=str,
                        default="%s/PRWlists" % (os.getcwd()))
    parser.add_argument('--readFromList', help='read text file with prw dataset list', action='store_true', default=False)
    parser.add_argument('--inputFile', '-i', '-I', help='text file with prw datasets to sort', nargs="+", type=str)
    parser.add_argument('--pattern', '-p', '-P', help='specify a pattern which is part of dataset name', nargs='+', default=[])
    parser.add_argument('--temp',
                        '-t',
                        '-T',
                        help='specify a temporary directory (with enough space!) to download all PRW datasets',
                        default=TEMP)
    parser.add_argument('--printTagsOnly', help='only print the tags found, do nothing else', action='store_true', default=False)
    ####################################################
    #    Options for submission of needed datasets     #
    ####################################################
    parser.add_argument('--requestedDataSets',
                        nargs="*",
                        type=str,
                        help='List of datasets which are needed by the analyzer. Missing datasets will be submitted to the grid',
                        default=[])
    parser.add_argument('--restrictToRequestedDatasets',
                        help='Only process datasets on list for requested datasets',
                        action='store_true',
                        default=False)
    parser.add_argument('--rucio', help="Rucio account to use for the submission", default=RUCIO_ACCOUNT)
    parser.add_argument('--destRSE', help="RSE Element to replicate the files", default=RSE)
    parser.add_argument('--official', help="Use official production rules", action='store_true', default=False)
    #####################################################
    #      Options to check the consistency of the DS   #
    #####################################################
    parser.add_argument(
        '--doConsistencyCheck',
        help=
        'The merged prw files are checked if they are consitent with 2015-2017 data. An instance of the prwTool is setup and foreach dsid which is not recognized by the tool a prw job is submitted',
        action='store_true',
        default=False)
    parser.add_argument('--mc16aTag', help='Which rtag should be used for the mc16a campaign', default='r9364')
    parser.add_argument('--mc16dTag', help='Which rtag should be used for the mc16d campaign', default='r10201')
    parser.add_argument('--mc16eTag', help='Which rtag should be used for the mc16d campaign', default='r10724')
    parser.add_argument("--log_file",
                        help="Define the location of the log-file from the consistency check",
                        default="%s/Merged_NTUP.log" % (os.getcwd()))
    parser.add_argument("--mergeAllTags", help="Merge everything which is available", default=False, action='store_true')

    RunOptions = parser.parse_args()

    Required_DS = []
    if len(RunOptions.requestedDataSets) > 0:
        for requestedDS in RunOptions.requestedDataSets:
            Required_DS.extend([convertToAOD(DS) for DS in ReadListFromFile(requestedDS)])

    Datasets = []
    if RunOptions.readFromList:
        if len(RunOptions.inputFile) == 0:
            print 'ERROR: Please give a file containing PRW files list when using --readFromList option!'
            sys.exit(1)

        for inputFile in RunOptions.inputFile:
            datasetsInList.extend(ReadListFromFile(inputFile))
        Datasets.extend(datasetsInList)

    else:
        print 'INFO: Looking for NTUP_PILEUP datasets in rucio...'
        for c in RunOptions.campaign:
            Datasets += GetDatasets(campaign=c,
                                    requested_ds=Required_DS,
                                    restrictToRequestedDatasets=RunOptions.restrictToRequestedDatasets)

    ##### Find datasets which are required by the user but there is no prw file for them available yet
    MissingDataSets = ClearFromDuplicates([ds for ds in Required_DS if IsMissingDataSet(prw_files=Datasets, DS=ds)])

    if len(MissingDataSets) > 0:
        print "INFO: You've requested a list of datasets, where for the following datasets no associated NTUP_PILEUP could be found:"
        for M in MissingDataSets:
            print "        *** %s" % (M)
        print "INFO: Will submit foreach dataset a job to create prw files"
        submitPRWFiles(DataSets=MissingDataSets, RUCIO=RunOptions.rucio, RSE=RunOptions.destRSE, Official=RunOptions.official)
        print "#################################################################################################################"
        print "INFO: All prwFiles have been submitted. Will omitt to create a new prw file                                     #"
        print "INFO: Please execute the script if the jobs are processed on the grid or remove the samples from the list.      #"
        print "#################################################################################################################"
        sys.exit(0)

    sortedSamples = {}
    print "INFO: Sort the %d samples according to their tag and whether they're fast or full simulation" % (len(Datasets))
    for ds in Datasets:
        if ':' in ds: ds = ds[ds.find(':') + 1:]
        if not passesNamePattern(ds, pattern=RunOptions.pattern, exclude_pattern=PatternsToExclude, requested_DS=Required_DS):
            continue
        if not IsNeededDataset(ds, requestedDSs=Required_DS, restrictToRequestedDatasets=RunOptions.restrictToRequestedDatasets):
            continue
        ##### Datasets have now the form of
        #####   - mc16_13TeV.<DSID>.<Physics>.NTUP_PILEUP.<RTAG>
        #####   - <user>.mc16_13TeV.<DSID>.XAMPP_PILEUP.<RTAG>
        campaign = GetPRW_campaign(ds)
        if not campaign in sortedSamples.iterkeys():
            sortedSamples[campaign] = {"FULLSIM": {}, "AF2": {}}
        rtags = GetAMITagsMC(ds, SkimPTag=True)
        isFullSim = ds.split(".")[-1].find("_s") != -1
        simKey = "FULLSIM" if isFullSim else "AF2"
        try:
            sortedSamples[campaign][simKey][rtags].append(ds)
        except:
            sortedSamples[campaign][simKey][rtags] = [ds]

    ### Assemble the merging tools
    MergingTools = []
    for campaign in sortedSamples.iterkeys():
        for stype in sortedSamples[campaign].iterkeys():
            for rtag, datasets in sortedSamples[campaign][stype].iteritems():
                if not RunOptions.mergeAllTags and rtag != RunOptions.mc16aTag and rtag != RunOptions.mc16dTag and rtag != RunOptions.mc16eTag:
                    continue
                MergingTools.append(
                    PrwMerging(
                        campaign=campaign,
                        stype=stype,
                        rtag=rtag,
                        datasets=datasets,
                        temp_dir=RunOptions.temp,
                        outdir=RunOptions.outDir,
                        check_consistency=RunOptions.doConsistencyCheck,
                    ))

    print 'INFO: Found the following tags:\n'
    for campaign in sortedSamples.iterkeys():
        for stype in sortedSamples[campaign].iterkeys():
            rtagsfound = sorted([r for r in sortedSamples[campaign][stype].iterkeys()])
            print '%s (%s):\t %s' % (campaign, stype, rtagsfound)

    if RunOptions.printTagsOnly: sys.exit(0)

    if len(sortedSamples) == 0:
        print 'INFO: No samples found, exiting...'
        sys.exit(1)

    CreateDirectory(RunOptions.outDir, CleanUpOld=False)  # save the final PRW files here

    #### now, download the NTUP_PILEUPs, merge them and move them to the final output directory
    ExecuteThreads(MergingTools, MaxCurrent=2)
    #### Everything is merged now let's check if a consistency check is demanded
    if not RunOptions.doConsistencyCheck:
        print "#############################################################################################################"
        print "INFO: Merged successfully %d prw Files in %s" % (len(MergingTools), RunOptions.outDir)
        print "INFO: You did not request if all datasets are running in 2015-2017 data"
        print "INFO: Attentiton if you're running with the lumi-calc files from all three years it might be"
        print "      that few of your jobs die because the dsid is only avaialble in mc16a/mc16d"
        print "      We recommend to perform a consistency check of your merged prw files before hand, where it is checked"
        print "      that all of your dsid in the prw files are running with all three lumicalc files. Missing prw ntuples"
        print "      are submitted otherwise, if the dataset is available in both campaigns."
        print "#############################################################################################################"
        sys.exit(0)

    #### Okay let's first find the black_listed samples
    to_black_list = []
    to_submit = []
    black_log = ["#<DataSet>  <AMI>   <prw-File> "]

    for Merging in MergingTools:
        to_black_list += Merging.black_listed()
        to_submit += Merging.missing_ds()
        black_log += Merging.missing_log()
#   updateBlackList(to_black_list)
    to_submit = sorted(ClearFromDuplicates(to_submit))
    if len(black_log) > 1:
        len_ds_name = max([len(l.split()[0]) for l in black_log])
        len_ev_ami = max([len(l.split()[1]) for l in black_log])
        black_log = sorted([
            "%s %s %s %s %s" % (l.split()[0], FillWhiteSpaces(len_ds_name - len(l.split()[0])), l.split()[1],
                                FillWhiteSpaces(len_ev_ami - len(l.split()[1])), l.split()[2]) for l in black_log
        ],
                           key=lambda x: GetPRW_datasetID(x))
        WriteList(black_log, RunOptions.log_file)

    if len(to_submit) > 0:
        print "INFO: There are %d datasets which need to be resubmitted" % (len(to_submit))
        for A in to_submit:
            print "       /// %s" % (A)

        #submitPRWFiles(DataSets=to_submit, RUCIO=RunOptions.rucio, RSE=RunOptions.destRSE, Official=RunOptions.official)
        exit(1)

    #### Find the four merging tools for the consistency check
    AFII_mc16a_file = None
    AFII_mc16d_file = None
    AFII_mc16e_file = None

    FullSim_mc16a_file = None
    FullSim_mc16d_file = None
    FullSim_mc16e_file = None

    for tool in MergingTools:
        if tool.rtag() == RunOptions.mc16aTag:
            if tool.isAFII(): AFII_mc16a_file = tool
            else: FullSim_mc16a_file = tool
        if tool.rtag() == RunOptions.mc16dTag:
            if tool.isAFII(): AFII_mc16d_file = tool
            else: FullSim_mc16d_file = tool
        if tool.rtag() == RunOptions.mc16eTag:
            if tool.isAFII(): AFII_mc16e_file = tool
            else: FullSim_mc16e_file = tool

    performConsistencyCheck(RunOptions, FullSim_mc16a_file, FullSim_mc16d_file, FullSim_mc16e_file)
    performConsistencyCheck(RunOptions, AFII_mc16a_file, AFII_mc16d_file, FullSim_mc16e_file)
    print "#######################"
    print "INFO:    Done         #"
    print "#######################"
