# include the common library instead of importing it. So we have full access
# to the globals
# Python you're a strange guy sometimes. Sometimes? Alsmost the entire day
include("XAMPPbase/BaseToolSetup.py")

###############################################################################
# Call the AssembleIO function to load the input files from the globals
#  inputFile = "<MyInFile>"
#  nevents = <MyNumOfEvt>
#  nskip = <MyEvToSkip>
#  outFile = "<MyOutFile>"
###############################################################################
AssembleIO()

###############################################################################
# Optionally:
# Call the SetupSystematicTool() method in order to switch on/off
# specific objects
###############################################################################
SetupSystematicsTool(noJets=False, noBtag=False, noElectrons=False, noMuons=False, noTaus=True, noDiTaus=True, noPhotons=True)

###############################################################################
# Define at some point a trigger list and pass it to the TriggerTool
###############################################################################
SingleLepTriggers = [
    "HLT_e24_lhmedium_iloose_L1EM20VH", "HLT_e24_lhmedium_iloose_L1EM18VH", "HLT_e60_lhmedium", "HLT_e26_lhtight_iloose",
    "HLT_e120_lhloose", "HLT_mu20_iloose_L1MU15", "HLT_mu24_iloose_L1MU15", "HLT_mu26_imedium", "HLT_mu40", "HLT_mu50"
]
DiLepTriggers = [
    "HLT_2e12_lhloose_L12EM10VH", "HLT_2e15_lhloose_L12EM13VH", "HLT_2mu10", "HLT_2mu14", "HLT_e7_lhmedium_mu24", "HLT_e17_lhloose_mu14"
]
TriLepTriggers = ["HLT_e17_lhmedium_2e9_lhmedium", "HLT_2e12_lhloose_mu10", "HLT_e12_lhloose_2mu10", "HLT_3mu6"]

MuonTriggerSF2015 = ["HLT_mu20_iloose_L1MU15_OR_HLT_mu40", "HLT_2mu10", "HLT_mu18_mu8noL1"]

MuonTriggerSF2016 = [
    "HLT_mu24_imedium_OR_HLT_mu40",
    "HLT_mu24_imedium_OR_HLT_mu50",
    #"HLT_mu24_ivarmedium_OR_HLT_mu40", enable this line when
    # HLT_mu24_ivarmedium becomes
    # available in DAOD
    #"HLT_mu24_ivarmedium_OR_HLT_mu50", this as well
    "HLT_mu26_ivarmedium_OR_HLT_mu50",
    "HLT_2mu10",
    "HLT_2mu14",
    "HLT_mu20_mu8noL1",
    "HLT_mu22_mu8noL1",
    "HLT_mu24_imedium_OR_HLT_mu50"
]
SetupSUSYMuonSelector().SFTrigger2015 = MuonTriggerSF2015
SetupSUSYMuonSelector().SFTrigger2016 = MuonTriggerSF2016

from XAMPPbase.Utils import constrainToPeriods, getUnPreScaledTrigger

SetupSUSYTriggerTool().Triggers = SingleLepTriggers + DiLepTriggers + TriLepTriggers
#SetupSUSYTriggerTool().Triggers = constrainToPeriods(SingleLepTriggers + DiLepTriggers + TriLepTriggers)
#SetupSUSYTriggerTool().Triggers = getUnPreScaledTrigger()
SetupSUSYTriggerTool().LowestUnprescaledMetTrigger = True
SetupSUSYTriggerTool().WriteSingleMuonObjTrigger = True
SetupSUSYTriggerTool().WriteSingleElecObjTrigger = True

SetupSUSYMetSelector().IncludeTaus = True
SetupSUSYMetSelector().IncludePhotons = True

#from AthenaCommon.Constants import WARNING, DEBUG, VERBOSE
#SetupSUSYTriggerTool().OutputLevel = DEBUG
#ServiceMgr.MessageSvc.debugLimit = 200000

###############################################################################
# Now setup your analysis helper
###############################################################################
SetupAnalysisHelper()
SetupAnalysisHelper().fillLHEWeights = True
SetupAnalysisHelper().createCommonTree = True
SetupAnalysisHelper().JetSelector = SetupSUSYJetSelector()
SetupAnalysisHelper().MetSelector = SetupSUSYMetSelector()
SetupSUSYMetSelector().WriteMetSignificance = True

###############################################################################
# This function adds all needed selector tools to the AnalysisHelper.
# If you want to initialize other tools instead call their creation methods
# before
###############################################################################
ParseBasicConfigsToHelper(use17Data=True, use18Data=True)
###############################################################################
#  Finally Setup the algorithm to run every thing
###############################################################################
SetupAlgorithm()
