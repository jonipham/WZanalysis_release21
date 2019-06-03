## automatically generated CMakeLists.txt file

# Declare the package
atlas_subdir( MuonAnalysis )

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
   Control/CxxUtils
   Event/xAOD/xAODCore
   Event/xAOD/xAODEgamma
   Event/xAOD/xAODEventInfo
   Event/xAOD/xAODHIEvent
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
   PhysicsAnalysis/HeavyIonPhys/HIEventUtils
   Reconstruction/tauRecTools
   Tools/PathResolver
   Trigger/TrigAnalysis/TriggerMatchingTool
   Trigger/TrigConfiguration/TrigConfInterfaces
   Trigger/TrigConfiguration/TrigConfxAOD
   Reconstruction/RecExample/RecExConfig
   ${extra_dep} )

# Declare external dependencies ... default here is to include ROOT
find_package( ROOT COMPONENTS MathCore RIO Core Tree Hist )

# Declare package as a library
# Note the convention that library names get "Lib" suffix
# Any package you depend on you should add
# to LINK_LIBRARIES line below (see the example)
atlas_add_library( MuonAnalysisLib src/*.cxx
                   PUBLIC_HEADERS MuonAnalysis
                   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
                   LINK_LIBRARIES ${ROOT_LIBRARIES}
                                    AthAnalysisBaseCompsLib xAODEventInfo TrigDecisionToolLib xAODMuon xAODTrigMuon xAODTrigger MuonEfficiencyCorrectionsLib MuonMomentumCorrectionsLib MuonSelectorToolsLib TriggerMatchingToolLib TrigConfxAODLib xAODHIEvent xAODTracking InDetTrackSelectionTool IsolationSelectionLib SUSYToolsLib JetCPInterfaces JetCalibToolsLib JetInterface JetResolutionLib JetUncertaintiesLib  JetMomentToolsLib METInterface METUtilitiesLib TrigConfInterfaces xAODTrigMissingET xAODMetaData ElectronEfficiencyCorrectionLib ElectronPhotonFourMomentumCorrectionLib ElectronPhotonSelectorToolsLib ElectronPhotonShowerShapeFudgeToolLib MCTruthClassifierLib JetJvtEfficiencyLib JetSubStructureUtils IsolationCorrectionsLib PhotonEfficiencyCorrectionLib JetSelectorToolsLib XAMPPbaseLib FourMomUtils xAODPrimitives GoodRunsListsLib PathResolver HIEventUtils CxxUtils



)

# if you add athena components (tools, algorithms) to this package
# these lines are needed so you can configure them in joboptions
atlas_add_component( MuonAnalysis src/components/*.cxx
                      NOCLIDDB
                      LINK_LIBRARIES MuonAnalysisLib InDetTrackSelectionTool IsolationSelectionLib SUSYToolsLib XAMPPbaseLib MuonSelectorToolsLib METUtilitiesLib GoodRunsListsLib PathResolver xAODPrimitives HIEventUtils
)

# if you add an application (exe) to this package
# declare it like this (note convention that apps go in the util dir)
# atlas_add_executable( MyApp util/myApp.cxx
#                       LINK_LIBRARIES MuonAnalysisLib
# )

# Install python modules, joboptions, and share content
atlas_install_python_modules( python/*.py )
atlas_install_joboptions( share/*.py )
atlas_install_data( data/* )
# You can access your data from code using path resolver, e.g.
# PathResolverFindCalibFile("MuonAnalysis/file.txt")
