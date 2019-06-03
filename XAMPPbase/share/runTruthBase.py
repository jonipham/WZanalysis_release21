#include the common library instead of importing it. So we have full access to the globals
# Python you're a strange guy sometimes. Sometimes? Alsmost the entire day
include("XAMPPbase/BaseToolSetup.py")
AssembleIO()
SetupSUSYTruthAnalysisHelper()
SetupSUSYTruthAnalysisHelper().fillLHEWeights = True

ParseTruthOptionsToHelper()
if isData():
    print "You're a son of the sickness. That is a TRUTH AnalysisHelper!!! No TRUTH in data. Truth + Data = baeeh"
    exit(1)

##############################################################################
#        Require that the Basline particles originate from the hard process
##############################################################################
SetupTruthSelector().BaselineHardProcess = True
SetupTruthSelector().JetContainer = "AntiKt4TruthJets"

SetupTruthSelector().ElectronContainer = ""
SetupTruthSelector().MuonContainer = ""
SetupTruthSelector().TauContainer = ""
SetupTruthSelector().NeutrinoContainer = ""

SetupTruthSelector().doJets = False
SetupTruthSelector().doElectrons = True
SetupTruthSelector().doMuons = True
SetupTruthSelector().doTaus = False
SetupTruthSelector().doTruthParticles = True

SetupSystematicsTool().doNoMet = True
SetupAlgorithm()
