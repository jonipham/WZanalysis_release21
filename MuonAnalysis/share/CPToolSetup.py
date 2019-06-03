def GetMETSignificanceTool(SoftTermParam=0, TreatPUJets=True):
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr

    if not hasattr(ToolSvc, "MetSignificanceTool"):
        from METUtilities.METUtilitiesConf import met__METSignificance
        from XAMPPbase.BaseToolSetup import isData, isAF2
        MyTool = CfgMgr.met__METSignificance("MetSignificanceTool")
        MyTool.SoftTermParam = 0
        MyTool.TreatPUJets = True
        MyTool.IsData = isData()
        MyTool.IsAFII = isAF2()
        ToolSvc += MyTool
    return getattr(ToolSvc, "MetSignificanceTool")


def GetBtagSelectionTool(JetCollection="AntiKt4EMTopoJets",
                         WorkingPoint="FixedCutBEff_77",
                         Tagger="MV2c10",
                         CalibPath="xAODBTaggingEfficiency/13TeV/2016-20_7-13TeV-MC15-CDI-2017-01-31_v1.root"):
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    ToolName = "BTagSelTool_%s_%s" % (JetCollection, WorkingPoint)
    if not hasattr(ToolSvc, ToolName):
        from xAODBTaggingEfficiency.xAODBTaggingEfficiencyConf import BTaggingSelectionTool
        MyTool = CfgMgr.BTaggingSelectionTool(ToolName)
        MyTool.TaggerName = Tagger
        MyTool.OperatingPoint = WorkingPoint
        MyTool.JetAuthor = JetCollection
        MyTool.FlvTagCutDefinitionsFileName = CalibPath
        ToolSvc += MyTool
        return MyTool
    return getattr(ToolSvc, ToolName)


def GetBTagShowerDSID(applyMCMCSFs=False):
    MCgen = getFlags().generators()
    if applyMCMCSFs:
        if "Pythia8" in MCgen:
            return 410470  #default
        elif "Sherpa" in MCgen:
            return 410250
        elif "Herwig" in MCgen:
            return 410558
    else:
        return 410470


def SetupBtaggingEfficiencyTool(JetCollection="AntiKt4EMTopoJets",
                                WorkingPoint="FixedCutBEff_77",
                                Tagger="MV2c10",
                                CalibPath="xAODBTaggingEfficiency/13TeV/2016-20_7-13TeV-MC15-CDI-2017-01-31_v1.root",
                                SystStrategy="Envelope",
                                EigenvecRedB="Loose",
                                EigenvecRedC="Loose",
                                EigenvecRedLight="Loose",
                                EffBCalib="default",
                                EffCCalib="default",
                                EffTCalib="default",
                                EffLightCalib="default"):
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    ToolName = "BTagEffTool_%s_%s" % (JetCollection, WorkingPoint)
    from xAODBTaggingEfficiency.xAODBTaggingEfficiencyConf import BTaggingEfficiencyTool
    MyTool = CfgMgr.BTaggingEfficiencyTool(ToolName)
    MyTool.TaggerName = Tagger
    MyTool.OperatingPoint = WorkingPoint
    MyTool.JetAuthor = JetCollection
    MyTool.ScaleFactorFileName = CalibPath
    MyTool.SystematicsStrategy = SystStrategy
    MyTool.EigenvectorReductionB = EigenvecRedB
    MyTool.EigenvectorReductionC = EigenvecRedC
    MyTool.EigenvectorReductionLight = EigenvecRedLight
    MyTool.EfficiencyBCalibrations = EffBCalib
    MyTool.EfficiencyCCalibrations = EffCCalib
    MyTool.EfficiencyTCalibrations = EffTCalib
    MyTool.EfficiencyLightCalibrations = EffLightCalib
    ToolSvc += MyTool
    return MyTool


def AddIsolationSelectionTool(ElectronWP="", MuonWP="", PhotonWP=""):
    from AthenaCommon.AppMgr import ToolSvc
    from IsolationSelection.IsolationSelectionConf import CP__IsolationSelectionTool
    ToolName = "IsolationSelectionTool_Ele_%sMuon_%sPhoton%s" % ("%s_" % (ElectronWP) if len(ElectronWP) > 0 else "", "%s_" %
                                                                 (MuonWP) if len(MuonWP) > 0 else "", "_%s" %
                                                                 (PhotonWP) if len(PhotonWP) > 0 else "")
    if not hasattr(ToolSvc, ToolName):
        isotool = CP__IsolationSelectionTool(ToolName)
        if len(ElectronWP) > 0: isotool.ElectronWP = ElectronWP
        if len(MuonWP) > 0: isotool.MuonWP = MuonWP
        if len(PhotonWP) > 0: isotool.PhotonWP = PhotonWP
        ToolSvc += isotool
    return getattr(ToolSvc, ToolName)


def SetupMuonCalibTool():
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    from MuonMomentumCorrections.MuonMomentumCorrectionsConf import CP__MuonCalibrationPeriodTool
    toolName = "MuonCalibrationAndSmearingTool"
    if hasattr(ToolSvc, toolName): return getattr(ToolSvc, toolName)
    calibTool = CP__MuonCalibrationPeriodTool(toolName)
    try:
        calibTool.useRandomRunNumber = False
    except:
        pass
    ToolSvc += calibTool
    return calibTool


def SetupSUSYTools(ConfigFile="SUSYTools/SUSYTools_Default.conf"):
    from AthenaCommon.AppMgr import ToolSvc
    from AthenaCommon import CfgMgr
    ToolName = "SUSYTools"
    if not hasattr(ToolSvc, ToolName):
        from SUSYTools.SUSYToolsConf import ST__SUSYObjDef_xAOD

        SUSYTools = CfgMgr.ST__SUSYObjDef_xAOD(ToolName)
        SUSYTools.ConfigFile = ConfigFile
        if isData(): SUSYTools.DataSource = 0
        elif isAF2(): SUSYTools.DataSource = 2
        else: SUSYTools.DataSource = 1
        ToolSvc += SUSYTools

    return getattr(ToolSvc, ToolName)
