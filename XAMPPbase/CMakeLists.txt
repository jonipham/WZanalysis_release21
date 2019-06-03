# Declare the package name:
atlas_subdir( XAMPPbase )

# Extra dependencies, based on the environment:
set( extra_dep )
if( XAOD_STANDALONE )
   set( extra_dep Control/xAODRootAccess )
else()
   set( extra_dep Control/AthenaBaseComps 
   				  Control/AthAnalysisBaseComps
   				  GaudiKernel)
endif()

add_definitions( -DISRELEASE21=1 )

# Declare the package's dependencies:
atlas_depends_on_subdirs(
   PUBLIC
   Control/AthToolSupport/AsgTools   
   Event/xAOD/xAODCore
   Event/xAOD/xAODEgamma
   Event/xAOD/xAODEventInfo
   Event/xAOD/xAODJet
   Event/xAOD/xAODMissingET
   Event/xAOD/xAODTrigMissingET
   Event/xAOD/xAODMuon
   Event/xAOD/xAODTau
   Event/xAOD/xAODTracking
   Event/xAOD/xAODTruth
   Event/xAOD/xAODParticleEvent 
   Event/xAOD/xAODLuminosity 
   PhysicsAnalysis/AnalysisCommon/AssociationUtils
   PhysicsAnalysis/AnalysisCommon/PATInterfaces
   PhysicsAnalysis/AnalysisCommon/PMGTools
   PhysicsAnalysis/MCTruthClassifier
   PhysicsAnalysis/SUSYPhys/SUSYTools 
   Reconstruction/Jet/JetJvtEfficiency
   Reconstruction/Jet/JetSubStructureUtils
   Trigger/TrigAnalysis/TrigDecisionTool
   CalcGenericMT2
   PRIVATE
   AsgExternal/Asg_Test
   Control/xAODRootAccess
   Control/AthContainers

   Event/EventPrimitives
   Event/FourMomUtils
   Event/xAOD/xAODBTagging
   Event/xAOD/xAODBase
   Event/xAOD/xAODCaloEvent
   Event/xAOD/xAODCutFlow
   Event/xAOD/xAODPrimitives
   PhysicsAnalysis/AnalysisCommon/IsolationSelection
   PhysicsAnalysis/AnalysisCommon/PileupReweighting
   PhysicsAnalysis/JetMissingEtID/JetSelectorTools
   PhysicsAnalysis/JetTagging/JetTagPerformanceCalibration/xAODBTaggingEfficiency
   
   # include SF tools themselves in order to use the same include in rel. 20.7 and 21
   PhysicsAnalysis/ElectronPhotonID/ElectronEfficiencyCorrection
   PhysicsAnalysis/ElectronPhotonID/PhotonEfficiencyCorrection
   PhysicsAnalysis/MuonID/MuonIDAnalysis/MuonEfficiencyCorrections
   
   PhysicsAnalysis/Interfaces/AsgAnalysisInterfaces
   PhysicsAnalysis/Interfaces/MuonAnalysisInterfaces
   PhysicsAnalysis/Interfaces/EgammaAnalysisInterfaces
   PhysicsAnalysis/TauID/TauAnalysisTools
   Reconstruction/Jet/JetCPInterfaces
   Reconstruction/Jet/JetCalibTools
   Reconstruction/Jet/JetInterface
   Reconstruction/Jet/JetResolution
   Reconstruction/Jet/JetUncertainties
   Reconstruction/Jet/JetMomentTools
   Reconstruction/MET/METInterface
   Reconstruction/MET/METUtilities
   Reconstruction/tauRecTools
   Tools/PathResolver
   Trigger/TrigAnalysis/TriggerMatchingTool
   Trigger/TrigConfiguration/TrigConfInterfaces
   Trigger/TrigConfiguration/TrigConfxAOD
   Reconstruction/RecExample/RecExConfig
   ${extra_dep} )

# External dependencies:
find_package( Boost )
find_package( ROOT COMPONENTS Core Tree RIO Hist Physics )

# Libraries in the package:
atlas_add_library( XAMPPbaseLib
   XAMPPbase/*.h Root/*.cxx
   PUBLIC_HEADERS XAMPPbase
   INCLUDE_DIRS ${Boost_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS}
   LINK_LIBRARIES ${Boost_LIBRARIES} ${ROOT_LIBRARIES} AsgTools xAODCore xAODEgamma xAODEventInfo xAODRootAccess
   xAODJet xAODMissingET xAODMuon xAODTau xAODTracking xAODTruth xAODParticleEvent xAODLuminosity SUSYToolsLib xAODCutFlow CalcGenericMT2Lib
      
   PRIVATE_LINK_LIBRARIES ${ROOT_LIBRARIES} ${BOOST_LIBRARIES}
   AssociationUtilsLib PATInterfaces TrigDecisionToolLib PMGToolsLib
   MCTruthClassifierLib JetJvtEfficiencyLib JetSubStructureUtils
   AthContainers EventPrimitives FourMomUtils xAODBTagging xAODBase
   xAODPrimitives IsolationSelectionLib PileupReweightingLib
   ElectronEfficiencyCorrectionLib ElectronPhotonFourMomentumCorrectionLib
   ElectronPhotonSelectorToolsLib ElectronPhotonShowerShapeFudgeToolLib
   IsolationCorrectionsLib PhotonEfficiencyCorrectionLib JetSelectorToolsLib
   xAODBTaggingEfficiencyLib MuonEfficiencyCorrectionsLib
   MuonMomentumCorrectionsLib MuonSelectorToolsLib TauAnalysisToolsLib
   JetCPInterfaces JetCalibToolsLib JetInterface JetResolutionLib
   JetUncertaintiesLib JetMomentToolsLib METInterface METUtilitiesLib
   PathResolver TriggerMatchingToolLib TrigConfInterfaces TrigConfxAODLib
   xAODTrigMissingET xAODMetaData AsgAnalysisInterfaces
   PRIVATE_LINK_LIBRARIES xAODTrigger PathResolver )

if( NOT XAOD_STANDALONE )
   atlas_add_component( XAMPPbase
      src/*.h src/*.cxx src/components/*.cxx
      INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
      LINK_LIBRARIES ${ROOT_LIBRARIES} AthenaBaseComps GaudiKernel GoodRunsListsLib
      AthenaKernel AthAnalysisBaseCompsLib xAODEventInfo xAODMuon xAODPrimitives xAODCutFlow
      xAODJet xAODBTagging xAODEgamma xAODMissingET xAODTracking xAODTau xAODParticleEvent
      TauAnalysisToolsLib xAODCore AthContainers AsgTools xAODBase xAODCutFlow xAODRootAccess
      xAODMetaData PATInterfaces PathResolver SUSYToolsLib XAMPPbaseLib )
endif()

atlas_add_dictionary( XAMPPbaseDict
   XAMPPbase/XAMPPbaseDict.h
   XAMPPbase/selection.xml
   LINK_LIBRARIES XAMPPbaseLib )

# Executable(s) in the package:
atlas_add_executable( CreateBatchJobSplit
   util/CreateBatchJobSplit.cxx
   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
   LINK_LIBRARIES ${ROOT_LIBRARIES} xAODRootAccess XAMPPbaseLib )

atlas_add_executable( SlimPRWFile
   util/SlimPRWFile.cxx
   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
   LINK_LIBRARIES ${ROOT_LIBRARIES} xAODRootAccess XAMPPbaseLib )



# Install files from the package:
atlas_install_data( data/* )
atlas_install_data( scripts/*.sh )

atlas_install_python_modules( python/*.py )
atlas_install_joboptions( share/*.py )

