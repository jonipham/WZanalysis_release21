import argparse, os
from ClusterSubmission.PeriodRunConverter import getGRL
include("XAMPPbase/CPToolSetup.py")
m_fileFlags = None
m_athArgs = None


class FileFlags(object):
    def __init__(self):

        self.__isData = False
        self.__isAF2 = False
        self.__isDAOD = False
        self.__isTruth3 = False
        self.__Generators = "Unknown"

        self.__mc_runNumber = -1
        self.__mcChannel = -1
        from AthenaCommon.AppMgr import ServiceMgr
        recoLog = logging.getLogger('XAMPP I/O')
        if len(ServiceMgr.EventSelector.InputCollections) == 0:
            recoLog.warning("No infiles were configured thus far")
            return
        from PyUtils import AthFile
        af = AthFile.fopen(ServiceMgr.EventSelector.InputCollections[0])
        self.__isData = "data" in af.fileinfos['tag_info']['project_name']
        self.__isAF2 = not self.isData() and 'tag_info' in af.fileinfos and len(
            [key for key in af.fileinfos['tag_info'].iterkeys() if 'AtlfastII' in key or 'Fast' in key]) > 0
        self.__mc_runNumber = af.fileinfos["run_number"][0] if len(af.fileinfos["run_number"]) > 0 else -1
        self.__mcChannel = af.fileinfos["mc_channel_number"][0] if not self.isData() and len(af.fileinfos["mc_channel_number"]) > 0 else -1

        self.__isDAOD = "DAOD" in af.fileinfos['stream_names'][0]
        self.__isTruth3 = "TRUTH3" in af.fileinfos['stream_names'][0]
        try:
            self.__Generators = af.fileinfos['det_descr_tags']['generators']
        except (KeyError, AttributeError):
            recoLog.warning("Failed to read the 'generators' metadata field")
            self.__Generators = "Unknown"

    def isData(self):
        return self.__isData

    def isAF2(self):
        return self.__isAF2

    def mcRunNumber(self):
        return self.__mc_runNumber

    def mcChannelNumber(self):
        return self.__mcChannel

    def isDAOD(self):
        return self.__isDAOD

    def isTRUTH3(self):
        return self.__isTruth3

    def generators(self):
        return self.__Generators


def getFlags():
    global m_fileFlags
    if m_fileFlags == None: m_fileFlags = FileFlags()
    return m_fileFlags


def getAthenaArgs():
    global m_athArgs
    if m_athArgs is None:
        from XAMPPbase.AthArgParserSetup import SetupAthArgParser
        theParser = SetupAthArgParser()
        parseResult = theParser.parse_known_args()
        m_athArgs = parseResult[0]
        recoLog = logging.getLogger('XAMPP I/O')
        recoLog.info("Detected the following athena args:")
        recoLog.info(m_athArgs)
        if len(parseResult[1]) > 0:
            recoLog.info("The following args were NOT parsed by the XAMPPbase parser (but possibly in your custom analysis parser: ")
            recoLog.info(parseResult[1])
    return m_athArgs


def AssembleIO(access_mode=0):

    athArgs = getAthenaArgs()

    #--------------------------------------------------------------
    # Reduce the event loop spam a bit
    #--------------------------------------------------------------
    from AthenaCommon.Logging import logging
    recoLog = logging.getLogger('XAMPP I/O')
    recoLog.info('****************** STARTING the job *****************')

    if os.path.exists("%s/athfile-cache.ascii.gz" % (os.getcwd())):
        recoLog.info("Old athfile-cache found. Will delete it otherwise athena just freaks out. This little boy.")
        os.system("rm %s/athfile-cache.ascii.gz" % (os.getcwd()))
    from GaudiSvc.GaudiSvcConf import THistSvc
    from AthenaCommon.JobProperties import jobproperties

    import AthenaRootComps.ReadAthenaxAODHybrid
    svcMgr.EventSelector.AccessMode = access_mode
    ###     https://gitlab.cern.ch/atlas/athena/blob/21.2/PhysicsAnalysis/POOLRootAccess/POOLRootAccess/TEvent.h
    ###     Infromation taken from here...
    recoLog.info("Set the access mode to %d. The access mode determines how athena reads out the xAOD." % (access_mode))
    recoLog.info("Thereby the numbering scheme is as follows")
    recoLog.info(
        "  -2 = kTreeAccess (direct access), -1 = kPoolAccess (horrible slow), 0 = kBranchAccess, 1 = kClassAccess(please try in case of problems)."
    )

    from AthenaCommon.AthenaCommonFlags import athenaCommonFlags as acf
    from AthenaServices.AthenaServicesConf import AthenaEventLoopMgr
    from AthenaCommon.AppMgr import ServiceMgr
    from ClusterSubmission.Utils import ReadListFromFile, ResolvePath, IsROOTFile
    from XAMPPbase.Utils import IsTextFile
    ServiceMgr += AthenaEventLoopMgr(EventPrintoutInterval=1000000)
    ServiceMgr += THistSvc()

    ServiceMgr.THistSvc.Output += ["XAMPP DATAFILE='{}' OPT='RECREATE'".format(athArgs.outFile)]
    recoLog.info("Will save the job's output to " + athArgs.outFile)

    if isData(): recoLog.info("We're running over data today")
    elif isAF2():
        recoLog.info("Please fasten your seatbelt the journey will be on Atlas fast ")
    else:
        recoLog.info("Fullsimulation. Make sure that you wear your augmented reality glasses")


def isData():
    return getFlags().isData()


def isAF2():
    return getFlags().isAF2()


# To get the keys/values below use checkMetaSG.py
# The prwFiles are classified according to their run-number. Let's classify the runs
def getRunNumbersMC():
    return getFlags().mcRunNumber()


def setupEventInfo():
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "EventInfoHandler"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__EventInfo
        EvInfo = CfgMgr.XAMPP__EventInfo("EventInfoHandler")
        EvInfo.SystematicsTool = SetupSystematicsTool()
        ToolSvc += EvInfo
    return getattr(ToolSvc, "EventInfoHandler")


def getMCChannelNumber():
    return getFlags().mcChannelNumber()


def isOnDAOD():
    return getFlags().isDAOD()


def isTRUTH3():
    return getFlags().isTRUTH3()


def SetupSystematicsTool(noJets=False,
                         noBtag=False,
                         noElectrons=False,
                         noMuons=False,
                         noTaus=False,
                         noDiTaus=True,
                         noPhotons=False,
                         noMet=False,
                         noTracks=True):
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr, GlobalFlags
    from AthenaCommon.Logging import logging
    recoLog = logging.getLogger('XAMPP SystTool')
    athArgs = getAthenaArgs()

    if not hasattr(ToolSvc, "SystematicsTool"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__SUSYSystematics
        SystTool = CfgMgr.XAMPP__SUSYSystematics("SystematicsTool")
        SystTool.doNoJets = noJets
        SystTool.doNoBtag = noBtag
        SystTool.doNoElectrons = noElectrons
        SystTool.doNoMuons = noMuons
        SystTool.doNoTaus = noTaus
        SystTool.doNoDiTaus = noDiTaus
        SystTool.doNoPhotons = noPhotons
        SystTool.doNoMet = noMet
        SystTool.doNoTracks = noTracks
        if isData():
            recoLog.info("The input is data. Set the SystematicsTool to data.")
            SystTool.isData = True
            SystTool.doSyst = False
            SystTool.doWeights = False
        else:
            SystTool.isAFII = isAF2()

        if athArgs.noSyst:
            recoLog.info("Switch off the systematics as it is configured by the user.")
            SystTool.doSyst = False
        ToolSvc += SystTool
    return getattr(ToolSvc, "SystematicsTool")


def SetupSUSYElectronSelector():
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "SUSYElectronSelector"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__SUSYElectronSelector
        EleSelector = CfgMgr.XAMPP__SUSYElectronSelector("SUSYElectronSelector")
        ToolSvc += EleSelector

    return getattr(ToolSvc, "SUSYElectronSelector")


def SetupSUSYMuonSelector():
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "SUSYMuonSelector"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__SUSYMuonSelector
        MuoSelector = CfgMgr.XAMPP__SUSYMuonSelector("SUSYMuonSelector")
        ToolSvc += MuoSelector

    return getattr(ToolSvc, "SUSYMuonSelector")


def SetupSUSYJetSelector():
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "SUSYJetSelector"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__SUSYJetSelector
        JetSelector = CfgMgr.XAMPP__SUSYJetSelector("SUSYJetSelector")
        ToolSvc += JetSelector

    return getattr(ToolSvc, "SUSYJetSelector")


def SetupSUSYTauSelector():
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "SUSYTauSelector"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__SUSYTauSelector
        TauSelector = CfgMgr.XAMPP__SUSYTauSelector("SUSYTauSelector")
        ToolSvc += TauSelector

    return getattr(ToolSvc, "SUSYTauSelector")


def SetupSUSYPhotonSelector():
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "SUSYPhotonSelector"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__SUSYPhotonSelector
        PhotonSelector = CfgMgr.XAMPP__SUSYPhotonSelector("SUSYPhotonSelector")
        ToolSvc += PhotonSelector
    return getattr(ToolSvc, "SUSYPhotonSelector")


def SetupSUSYMetSelector():
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "SUSYMetSelector"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__SUSYMetSelector
        MetSelector = CfgMgr.XAMPP__SUSYMetSelector("SUSYMetSelector")
        ToolSvc += MetSelector
    return getattr(ToolSvc, "SUSYMetSelector")


def SetupSUSYTriggerTool():
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "TriggerTool"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__SUSYTriggerTool

        TriggerTool = CfgMgr.XAMPP__SUSYTriggerTool("TriggerTool")
        TriggerTool.SystematicsTool = SetupSystematicsTool()
        TriggerTool.ElectronSelector = SetupSUSYElectronSelector()
        TriggerTool.MuonSelector = SetupSUSYMuonSelector()
        # TriggerTool.JetSelector = SetupSUSYJetSelector()
        TriggerTool.TauSelector = SetupSUSYTauSelector()
        TriggerTool.PhotonSelector = SetupSUSYPhotonSelector()
        ToolSvc += TriggerTool
    return getattr(ToolSvc, "TriggerTool")


def SetupTruthSelector():
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "SUSYTruthSelector"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__SUSYTruthSelector
        TruthSelector = CfgMgr.XAMPP__SUSYTruthSelector("SUSYTruthSelector")
        ToolSvc += TruthSelector
    return getattr(ToolSvc, "SUSYTruthSelector")


def SetupParticleConstuctor():
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "ParticleConstructor"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__ReconstructedParticles
        Constructor = CfgMgr.XAMPP__ReconstructedParticles("ParticleConstructor")
        ToolSvc += Constructor
    return getattr(ToolSvc, "ParticleConstructor")


def SetupTruthAnalysisConfig(Name):
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "TruthAnalysisConfig"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__TruthAnalysisConfig
        AnaConfig = CfgMgr.XAMPP__TruthAnalysisConfig("TruthAnalysisConfig")
        AnaConfig.TreeName = Name
        ToolSvc += AnaConfig

    return getattr(ToolSvc, "TruthAnalysisConfig")


def SetupSUSYTruthAnalysisHelper(TreeName="TruthTree"):
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr

    if not hasattr(ToolSvc, "AnalysisHelper"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__SUSYTruthAnalysisHelper

        TruthHelper = CfgMgr.XAMPP__SUSYTruthAnalysisHelper("AnalysisHelper")
        ToolSvc += TruthHelper
        TruthHelper.SystematicsTool = SetupSystematicsTool()
        TruthHelper.TruthSelector = SetupTruthSelector()
        TruthHelper.AnalysisConfig = SetupTruthAnalysisConfig(TreeName)
    return getattr(ToolSvc, "AnalysisHelper")


def SetupAnalysisConfig(Name):
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "AnalysisConfig"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__AnalysisConfig
        AnaConfig = CfgMgr.XAMPP__AnalysisConfig(name="AnalysisConfig", TreeName=Name)
        ToolSvc += AnaConfig

    return getattr(ToolSvc, "AnalysisConfig")


def SetupAnalysisHelper(TreeName="XAMPPBaseTree"):
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "AnalysisHelper"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__SUSYAnalysisHelper
        BaseHelper = CfgMgr.XAMPP__SUSYAnalysisHelper(name="AnalysisHelper")
        ToolSvc += BaseHelper
        BaseHelper.AnalysisConfig = SetupAnalysisConfig(TreeName)
    return getattr(ToolSvc, "AnalysisHelper")


def SetupAlgorithm():
    from AthenaCommon.AlgSequence import AlgSequence
    from AthenaCommon.AppMgr import ServiceMgr
    job = AlgSequence()
    from AthenaCommon.Logging import logging
    recoLog = logging.getLogger('XAMPP Algortihm')
    ServiceMgr.MessageSvc.Format = "% F%60W%S%7W%R%T %0W%M"
    if not hasattr(job, "XAMPPAlgorithm"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__XAMPPalgorithm
        from PyUtils import AthFile
        thisAlg = XAMPP__XAMPPalgorithm("XAMPPAlgorithm")
        thisAlg.AnalysisHelper = SetupAnalysisHelper()
        thisAlg.SystematicsTool = SetupSystematicsTool()
        thisAlg.nfiles = len(ServiceMgr.EventSelector.InputCollections)
        job += thisAlg
        recoLog.info("Created XAMPP algorithm")
    return getattr(job, "XAMPPAlgorithm")


def getDefaultGRL():
    return getGRL()


def setupGRL(GRL=getDefaultGRL()):
    if isData():
        SetupAnalysisHelper().GoodRunsLists = GRL


def getLumiCalcConfig(use1516Data=True, use17Data=True, use18Data=True):
    LumiCalcFiles = []
    lumiCalcFiles_1516 = getGRL([15, 16], flavour='lumiCalc')
    lumiCalcFiles_17 = getGRL(17, flavour='lumiCalc')
    lumiCalcFiles_18 = getGRL(18, flavour='lumiCalc')
    if use1516Data: LumiCalcFiles += lumiCalcFiles_1516
    if use17Data: LumiCalcFiles += lumiCalcFiles_17
    if use18Data: LumiCalcFiles += lumiCalcFiles_18
    return LumiCalcFiles


def configurePRWtool(offset=0):
    from AthenaCommon.AppMgr import ServiceMgr
    from PyUtils import AthFile
    from ClusterSubmission.Utils import ResolvePath, ClearFromDuplicates
    recoLog = logging.getLogger('XAMPP getPrwConfig')

    use1516Data = isData()
    use17Data = isData()
    use18Data = isData()

    ### The actual mu config file is needed to activate the actual mu reweighting recommended for mc16d & mc16e
    ### https://indico.cern.ch/event/712774/contributions/2928042/attachments/1614637/2565496/prw_mc16d.pdf
    prwConfig_mc16a = []
    prwConfig_mc16d = getGRL(17, flavour='actualMu')
    prwConfig_mc16e = getGRL(18, flavour='actualMu')
    run_channel = [] if isData() else [(getRunNumbersMC(), getMCChannelNumber() + offset)]
    athArgs = getAthenaArgs()
    if not isData() and (len(ServiceMgr.EventSelector.InputCollections) > 1 and athArgs.parseFilesForPRW):
        recoLog.info("Run a local job. Try to find foreach job the prw-config file")
        for i, in_file in enumerate(ServiceMgr.EventSelector.InputCollections):
            recoLog.info("Look up the channel number for %s" % (in_file))
            ### That file is used to read the meta-data we do not need to open it twice
            if i == 0: continue
            af = AthFile.fopen(in_file)
            afII = not isData() and 'tag_info' in af.fileinfos and len(
                [key for key in af.fileinfos['tag_info'].iterkeys() if 'AtlfastII' in key or 'Fast' in key]) > 0
            mc_runNumber = af.fileinfos["run_number"][0] if len(af.fileinfos["run_number"]) > 0 else -1
            mc_channel = af.fileinfos["mc_channel_number"][0] if not isData() and len(af.fileinfos["mc_channel_number"]) > 0 else -1
            ## If the user mixes AFII with fullsim calibration
            ## the resuls are likely to mismatch. We must prevent this and kill
            ## the job
            if afII != isAF2():
                recoLog.error("You are mixing AFII with Fullsim files. Scale-factors and jet calibration are largely affected. Please fix")
                exit(1)
            run_channel += [(mc_runNumber, mc_channel + offset)]
    ## Find the central repo
    for period_num, mc_channel in run_channel:
        if period_num == 284500:
            config_file = ResolvePath("dev/PileupReweighting/share/DSID{dsid_short}xxx/pileup_mc16a_dsid{dsid}_{sim}.root".format(
                dsid_short=str(mc_channel)[0:3], dsid=mc_channel, sim="AFII" if isAF2() else "FS"))
            use1516Data = True
            if not config_file: continue
            prwConfig_mc16a += [config_file]
        elif period_num == 300000:
            config_file = ResolvePath("dev/PileupReweighting/share/DSID{dsid_short}xxx/pileup_mc16d_dsid{dsid}_{sim}.root".format(
                dsid_short=str(mc_channel)[0:3], dsid=mc_channel, sim="AFII" if isAF2() else "FS"))
            use17Data = True
            if not config_file: continue
            prwConfig_mc16d += [config_file]
        elif period_num == 310000:
            config_file = ResolvePath("dev/PileupReweighting/share/DSID{dsid_short}xxx/pileup_mc16e_dsid{dsid}_{sim}.root".format(
                dsid_short=str(mc_channel)[0:3], dsid=mc_channel, sim="AFII" if isAF2() else "FS"))
            use18Data = True
            if not config_file: continue
            prwConfig_mc16e += [config_file]
        else:
            recoLog.warning("Nothing has been found for the sample %d in prw period %d" % (mc_channel, period_num))
            continue

    ConfigFiles = []
    if use1516Data: ConfigFiles += prwConfig_mc16a
    if use17Data: ConfigFiles += prwConfig_mc16d
    if use18Data: ConfigFiles += prwConfig_mc16e
    return sorted(ClearFromDuplicates(ConfigFiles)), getLumiCalcConfig(use1516Data=use1516Data, use17Data=use17Data, use18Data=use18Data)


def ParseBasicConfigsToHelper(STFile="SUSYTools/SUSYTools_Default.conf",
                              xSecDB="SUSYTools/mc15_13TeV/",
                              SeparateSF=False,
                              use1516Data=True,
                              use17Data=True,
                              use18Data=True):
    athArgs = getAthenaArgs()
    BaseHelper = SetupAnalysisHelper()
    BaseHelper.SUSYTools = SetupSUSYTools(ConfigFile=STFile)
    BaseHelper.STConfigFile = STFile if not "STConfigFile" in globals() else STConfigFile
    from AthenaCommon.Logging import logging
    recoLog = logging.getLogger('XAMPP BaseToolSetup')

    if isData():
        setupGRL()
    else:
        BaseHelper.STCrossSectionDB = xSecDB
    #### Some people came up with the idea of assigining the same DSID twice to different
    #### Monte Carlo (Sherpa221_Znunu samples are the first victims of this idea)
    offset = 0
    if not isData() and getMCChannelNumber() >= 366001 and getMCChannelNumber() <= 366008:
        from XAMPPbase.Utils import GetPropertyFromConfFile
        recoLog.info("Detected buggy Znunu sample... Seeking for information in the globals to nail down the thing")
        if athArgs.dsidBugFix != None:
            recoLog.info("Found the following string in 'dsidBugFix': %s" % (athArgs.dsidBugFix))
            if athArgs.dsidBugFix == "BFilter": offset = 9
            elif athArgs.dsidBugFix == "CFilterBVeto": offset = 18
            elif athArgs.dsidBugFix == "CVetoBVeto": offset = 27
            else:
                recoLog.error("Unable to interpret dsidBugFix setting", athArgs.dsidBugFix,
                              ",.... Please check, but the job is going to be killed now... Congratulations!")
                exit(1)
        ### Check whether people made a property in the config file
        elif GetPropertyFromConfFile(SetupSUSYTools().ConfigFile, 'PRW.autoconfigPRWHFFilter'):
            recoLog.info("Interesting found something in the SUSYTools config file.. Let's check whether it's useful")
            meta_property = GetPropertyFromConfFile(SetupSUSYTools().ConfigFile, 'PRW.autoconfigPRWHFFilter')
            if meta_property == "BFilter": offset = 9
            elif meta_property == "CFilterBVeto": offset = 18
            elif meta_property == "CVetoBVeto": offset = 27
            else:
                recoLog.error(
                    "Nope that was not helpful. You need to set up your SUSYTools config file better. Your job will be now terminated ...")
                exit(1)
        BaseHelper.EventInfoHandler = setupEventInfo()
        setupEventInfo().SwitchOnDSIDshift = True
        setupEventInfo().DSIDshift = offset

    ### speed up the initialization of the PRW tool by only passing the required PRW config files for test jobs
    ConfigFiles, LumiCalcFiles = configurePRWtool(offset)

    recoLog.info("The following lumi calc files will be used for pile up reweighting")
    for F in LumiCalcFiles:
        recoLog.info("   ++++ %s" % (F))
    recoLog.info("The following prw config files will be used for pile up reweighting:")
    for F in ConfigFiles:
        recoLog.info("   ++++ %s" % (F))

    SetupSUSYTools().PRWLumiCalcFiles = LumiCalcFiles
    SetupSUSYTools().PRWConfigFiles = ConfigFiles

    BaseHelper.SystematicsTool = SetupSystematicsTool()
    BaseHelper.ElectronSelector = SetupSUSYElectronSelector()
    BaseHelper.MuonSelector = SetupSUSYMuonSelector()
    BaseHelper.JetSelector = SetupSUSYJetSelector()
    BaseHelper.TruthSelector = SetupTruthSelector()
    BaseHelper.TriggerTool = SetupSUSYTriggerTool()
    BaseHelper.TauSelector = SetupSUSYTauSelector()
    BaseHelper.PhotonSelector = SetupSUSYPhotonSelector()
    BaseHelper.MetSelector = SetupSUSYMetSelector()
    if SeparateSF:
        SetupSUSYElectronSelector().SeparateSF = True
        SetupSUSYPhotonSelector().SeparateSF = True
        SetupSUSYMuonSelector().SeparateSF = True
        SetupSUSYJetSelector().SeparateSF = True
        SetupSUSYTauSelector().SeparateSF = True

    from XAMPPbase.Utils import GetKinematicCutFromConfFile, GetPropertyFromConfFile
    SetupSUSYJetSelector().SignalPtCut = GetKinematicCutFromConfFile(STFile, "Jet.Pt")
    SetupSUSYJetSelector().SignalEtaCut = GetKinematicCutFromConfFile(STFile, "Jet.Eta")
    SetupSUSYJetSelector().bJetEtaCut = 2.5

    ### Pipe the trigger tools to the Electron and Tau selector
    SetupSUSYElectronSelector().TriggerTool = SetupSUSYTriggerTool()
    SetupSUSYTauSelector().TriggerTool = SetupSUSYTriggerTool()

    ##########################################################
    #       Load the electron IDs for baseline/signal SFs    #
    ##########################################################
    if GetPropertyFromConfFile(STFile, "EleBaseline.Id") != -1:
        SetupSUSYElectronSelector().BaselineID = GetPropertyFromConfFile(STFile, "EleBaseline.Id")
    if GetPropertyFromConfFile(STFile, "Ele.Id") != -1:
        SetupSUSYElectronSelector().SignalID = GetPropertyFromConfFile(STFile, "Ele.Id")

    try:
        JetType = int(GetPropertyFromConfFile(STFile, "Jet.InputType"))
        if JetType != -1: SetupSUSYJetSelector().JetCollectionType = JetType
    except:
        recoLog.warning("Could not find the Jet.InputType")

    SetupSUSYTauSelector().ORUtilsSelectionFlag = 2
    if not isData():
        SetupTruthSelector().BSMContainer = "TruthBSM"
        SetupTruthSelector().isTRUTH3 = True
        #SetupTruthSelector().doTruthParticles = True

    ### JVT scale-factors need TruthJets which is not stored in the AOD
    if not isData() and not isOnDAOD():
        recoLog.info("Disable TRUTH jets as they are not in AODs")
        SetupTruthSelector().doJets = False
        recoLog.info("Will also disable the JVT efficiencies as well")
        SetupSUSYJetSelector().ApplyJVTSF = False
    if not isData():
        SetupTruthSelector().JetContainer = "AntiKt4TruthJets"


def ParseTruthOptionsToHelper():
    if isData():
        print "Are you sure? You want to run TRUTH on data?!. 46x + 87y / #sqrt{90} ?!"
        exit(1)
    SetupSystematicsTool().doNoJets = True
    SetupSystematicsTool().doNoBtag = True
    SetupSystematicsTool().doNoElectrons = True
    SetupSystematicsTool().doNoMuons = True
    SetupSystematicsTool().doNoTaus = True
    SetupSystematicsTool().doNoDiTaus = True
    SetupSystematicsTool().doNoPhotons = True
    SetupSystematicsTool().doNoTracks = True

    if isTRUTH3():
        print "The input seems to be TRUTH3"
        SetupTruthSelector().doTruthParticles = False
        SetupTruthSelector().isTRUTH3 = True
    SetupTruthSelector().ElectronContainer = "TruthElectrons"
    SetupTruthSelector().MuonContainer = "TruthMuons"
    SetupTruthSelector().TauContainer = "TruthTaus"
    #    SetupTruthSelector().PhotonContainer = "Truth3Photons"
    SetupTruthSelector().NeutrinoContainer = "TruthNeutrinos"
