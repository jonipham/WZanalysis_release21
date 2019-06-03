#Skeleton joboption for a simple analysis job
include("MuonAnalysis/BaseToolSetup.py")
AssembleIO()

#---- Minimal job options -----

#jps.AthenaCommonFlags.AccessMode = "ClassAccess"              #Choose from TreeAccess,BranchAccess,ClassAccess,AthenaAccess,POOLAccess
#jps.AthenaCommonFlags.TreeName = "MyTree"                    #when using TreeAccess, must specify the input tree name

jps.AthenaCommonFlags.HistOutputs = ["MuonAnalysis:AnalysisOutput.root"]  #register output files like this. MYSTREAM is used in the code

athAlgSeq += CfgMgr.CP__CalibratedMuonsProvider(Input="Muons",Output="CalibratedMuons")
athAlgSeq += CfgMgr.XAMPP__MuonAnalysisAlg()                               #adds an instance of your alg to the main alg sequence

# Create a MuonSelectionTool if we do not yet have one
from AthenaCommon.AppMgr import ToolSvc
#ToolSvc += CfgMgr.Trig__MatchingTool("MyMatchingTool",OutputLevel=DEBUG)
ToolSvc += CfgMgr.Trig__MatchingTool("MyMatchingTool")
if not hasattr(ToolSvc,"MyMuonSelectionTool"):
	from MuonSelectorTools.MuonSelectorToolsConf import CP__MuonSelectionTool
	ToolSvc += CP__MuonSelectionTool("MyMuonSelectionTool")
	ToolSvc.MyMuonSelectionTool.MaxEta = 2.5
	ToolSvc.MyMuonSelectionTool.MuQuality = 1
	ToolSvc.MyMuonSelectionTool.TrtCutOff = True

"""if not hasattr(ToolSvc,"MyMuonTagSelectionTool"):
	from MuonSelectorTools.MuonSelectorToolsConf import CP__MuonSelectionTool
	ToolSvc += CP__MuonSelectionTool("MyMuonTagSelectionTool")
	ToolSvc.MyMuonTagSelectionTool.MaxEta = 2.4
	ToolSvc.MyMuonTagSelectionTool.MuQuality = 0
	ToolSvc.MyMuonTagSelectionTool.TrtCutOff = True """

if not hasattr(ToolSvc,"MyInDetSelectionTool"):
	from InDetTrackSelectionTool.InDetTrackSelectionToolConf import InDet__InDetTrackSelectionTool
	ToolSvc += InDet__InDetTrackSelectionTool("MyInDetSelectionTool")
	ToolSvc.MyInDetSelectionTool.CutLevel = "TightPrimary"
	ToolSvc.MyInDetSelectionTool.maxZ0 = 10.
	ToolSvc.MyInDetSelectionTool.maxZ0SinTheta = 0.5
	ToolSvc.MyInDetSelectionTool.maxD0 = 1.5
	ToolSvc.MyInDetSelectionTool.minPt = 15000.
	ToolSvc.MyInDetSelectionTool.maxAbsEta = 2.5
	ToolSvc.MyInDetSelectionTool.maxD0overSigmaD0 = 3.

#ToolSvc += CfgMgr.IMETMaker("METTool")
def setupEventInfo():
    #from AthenaCommon.AppMgr import ToolSvc
    #from AthenaCommon import CfgMgr
    if not hasattr(ToolSvc, "EventInfoHandler"):
        from XAMPPbase.XAMPPbaseConf import XAMPP__EventInfo
        EvInfo = CfgMgr.XAMPP__EventInfo("EventInfoHandler")
        EvInfo.SystematicsTool = SetupSystematicsTool()
        ToolSvc += EvInfo
    return getattr(ToolSvc, "EventInfoHandler")



#if not hasattr(ToolSvc,"MyIsoSelectionTool"):
#	from IsolationSelectionTool.IsolationSelectionToolConf import CP__IsolationSelectionTool
#	ToolSvc += CP__IsolationSelectionTool("MyIsoSelectionTool")
#	ToolSvc.MyIsoSelectionTool.muonWP = "GradientLoose"





#---- Options you could specify on command line -----
#jps.AthenaCommonFlags.EvtMax=-1                          #set on command-line with: --evtMax=-1
#jps.AthenaCommonFlags.SkipEvents=0                       #set on command-line with: --skipEvents=0
#jps.AthenaCommonFlags.FilesInput = ["/cvmfs/atlas-nightlies.cern.ch/repo/data/data-art/CommonInputs/DAOD_PHYSVAL/mc16_13TeV.410501.PowhegPythia8EvtGen_A14_ttbar_hdamp258p75_nonallhad.DAOD_PHYSVAL.e5458_s3126_r9364_r9315_AthDerivation-21.2.1.0.root"]        #set on command-line with: --filesInput=...


include("AthAnalysisBaseComps/SuppressLogging.py")              #Optional include to suppress as much athena output as possible. Keep at bottom of joboptions so that it doesn't suppress the logging of the things you have configured above
