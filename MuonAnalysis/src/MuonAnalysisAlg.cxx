// MuonAnalysis includes
#include "MuonAnalysisAlg.h"

// System include(s):
#include <memory>
#include <cstdlib>
#include <string>
#include <iostream>

// ROOT include(s):
#include <TFile.h>
#include <TError.h>
#include <TString.h>
#include <TStopwatch.h>
#include <TSystem.h>
#include "TObjArray.h"
#include "TObjString.h"

#include "xAODRootAccess/Init.h"
#include "xAODRootAccess/TStore.h"
//#ifdef ROOTCORE
//#include "xAODRootAccess/TEvent.h"
//#else
//#include "POOLRootAccess/TEvent.h"
//#endif // ROOTCORE

// EDM include(s):
#include "xAODEventInfo/EventInfo.h"
#include "xAODHIEvent/HIEventShape.h"
#include "xAODMuon/MuonContainer.h"
#include "xAODEgamma/ElectronContainer.h"
#include "xAODEgamma/PhotonContainer.h"
#include "xAODTau/TauJetContainer.h"
#include "xAODJet/JetContainer.h"
#include "xAODJet/JetAuxContainer.h"
#include "xAODCaloEvent/CaloCluster.h"
#include "xAODTracking/TrackParticleContainer.h"
#include "xAODTracking/VertexContainer.h"
#include "xAODTracking/TrackingPrimitives.h"
#include "xAODTruth/TruthParticleAuxContainer.h"
#include "xAODTruth/TruthParticleContainer.h"
#include "xAODTruth/TruthEventContainer.h"
#include "xAODTruth/TruthEvent.h"
#include "xAODTruth/TruthParticle.h"
#include "xAODTruth/TruthVertex.h"
#include "xAODCore/ShallowCopy.h"
#include "xAODMissingET/MissingETContainer.h"
#include "xAODMissingET/MissingETAuxContainer.h"
#include "xAODBase/IParticleHelpers.h"
#include "xAODTruth/xAODTruthHelpers.h"
#include "AsgAnalysisInterfaces/IGoodRunsListSelectionTool.h"
#include "xAODCutFlow/CutBookkeeper.h"
#include "xAODCutFlow/CutBookkeeperContainer.h"

// Local include(s):
#include "SUSYTools/SUSYObjDef_xAOD.h"
#include "SUSYTools/SUSYCrossSection.h"

// Other includes
#include "PATInterfaces/SystematicSet.h"
#include "PATInterfaces/SystematicCode.h"
#include "PATInterfaces/CorrectionCode.h"
#include "PathResolver/PathResolver.h"
#include "AsgTools/AnaToolHandle.h"
#include "AsgTools/IAsgTool.h"

#include <HIEventUtils/IHICentralityTool.h>
#include <HIEventUtils/HICentralityTool.h>
#include "CxxUtils/make_unique.h"



#include "TrigDecisionTool/ChainGroup.h"
//#include "METInterface/IMETMaker.h"
//#include <METInterface/IMETSystematicsTool.h>
//#include "IsolationSelection/IsolationSelectionTool.h"
//#include "xAODPrimitives/IsolationType.h"

#include <XAMPPbase/AnalysisConfig.h>
#include <XAMPPbase/EventInfo.h>
//#include <XAMPPbase/IEventInfo.h>
#include <XAMPPbase/HistoBase.h>
#include <XAMPPbase/IDiTauSelector.h>
#include <XAMPPbase/IElectronSelector.h>
#include <XAMPPbase/IJetSelector.h>
#include <XAMPPbase/IMetSelector.h>
#include <XAMPPbase/IMuonSelector.h>
#include <XAMPPbase/IPhotonSelector.h>
#include <XAMPPbase/ITauSelector.h>
#include <XAMPPbase/ITriggerTool.h>
#include <XAMPPbase/ITruthSelector.h>
#include <XAMPPbase/MetaDataTree.h>
#include <XAMPPbase/ReconstructedParticles.h>
#include <XAMPPbase/SUSYAnalysisHelper.h>
#include <XAMPPbase/SUSYSystematics.h>
#include <XAMPPbase/TreeBase.h>

// Tool includes
#include <AsgAnalysisInterfaces/IGoodRunsListSelectionTool.h>
#include <AsgTools/StatusCode.h>
#include <GaudiKernel/ITHistSvc.h>

#include <fstream>
#include <iostream>

using CxxUtils::make_unique;

namespace XAMPP {
  MuonAnalysisAlg::MuonAnalysisAlg( const std::string& name, ISvcLocator* pSvcLocator ) : AthAnalysisAlgorithm( name, pSvcLocator ){

    //declareProperty( "Property", m_nProperty = 0, "My Example Integer Property" ); //example property declaration
    //declareProperty("EventInfoHandler", m_XAMPPInfo, "The XAMPPInfo event Handler");

  }


  MuonAnalysisAlg::~MuonAnalysisAlg() {}


  StatusCode MuonAnalysisAlg::initialize() {
    ATH_MSG_INFO ("Initializing " << name() << "...");
    //
    //This is called once, before the start of the event loop
    //Retrieves of tools you have configured in the joboptions go here
    //

    //HERE IS AN EXAMPLE
    //We will create a histogram and a ttree and register them to the histsvc
    //Remember to configure the histsvc stream in the joboptions
    //
    //m_myHist = new TH1D("myHist","myHist",100,0,100);
    //CHECK( histSvc()->regHist("/MYSTREAM/myHist", m_myHist) ); //registers histogram to output stream
    //m_myTree = new TTree("myTree","myTree");
    //CHECK( histSvc()->regTree("/MYSTREAM/SubDirectory/myTree", m_myTree) ); //registers tree to output stream inside a sub-directory
    //m_muonHist = new TH1D("muonHist","muonHist",100,0,100);
    //CHECK( histSvc()->regHist("/MYSTREAM/muonHist", m_muonHist) ); //registers histogram to output stream


    m_muonTree = new TTree("muonTree","muonTree");
    m_muonTree->Branch("runNumber",   &m_runNumber);
    m_muonTree->Branch("eventNumber",   &m_eventNumber);
    m_muonTree->Branch("lumiBlock",   &m_lumiBlock);
    m_muonTree->Branch("isMC",   &m_isMC);
    m_muonTree->Branch("eventPassesGRL", &eventPassesGRL);

    m_muonTree->Branch("HLT_mu15",          &m_HLT_mu15);
    m_muonTree->Branch("HLT_mu15_L1MU10", &m_HLT_mu15_L1MU10);
    m_muonTree->Branch("HLT_mu15_L1MU6", &m_HLT_mu15_L1MU6);
    m_muonTree->Branch("match_HLT_mu15",          &match_HLT_mu15);
    m_muonTree->Branch("match_HLT_mu15_L1MU10",          &match_HLT_mu15_L1MU10);
    m_muonTree->Branch("match_HLT_mu15_L1MU6",          &match_HLT_mu15_L1MU6);
    m_muonTree->Branch("muon_pt",    &m_muon_pt);
    m_muonTree->Branch("muon_eta",    &m_muon_eta);
    m_muonTree->Branch("muon_phi",    &m_muon_phi);
    m_muonTree->Branch("muon_charge",    &m_muon_charge);
    m_muonTree->Branch("muon_quality",    &m_muon_quality);
    m_muonTree->Branch("muon_type",    &m_muon_type);
    m_muonTree->Branch("truthmuon",        &truthmuon);

    m_muonTree->Branch("MetTST_mpx", &MetTST_mpx);
    m_muonTree->Branch("MetTST_mpy", &MetTST_mpy);
    m_muonTree->Branch("MetTST_met", &MetTST_met);
    m_muonTree->Branch("MetTST_sumet", &MetTST_sumet);
    m_muonTree->Branch("MetTST_phi", &MetTST_met);
    m_muonTree->Branch("MetTST_et_tst", &MetTST_et_tst);
    m_muonTree->Branch("MetTST_et_el", &MetTST_et_el);
    m_muonTree->Branch("MetTST_et_ph", &MetTST_et_ph);
    m_muonTree->Branch("MetTST_et_mu", &MetTST_et_mu);
    m_muonTree->Branch("MetTST_et_jet", &MetTST_et_jet);

    m_muonTree->Branch("MetCST_mpx", &MetCST_mpx);
    m_muonTree->Branch("MetCST_mpy", &MetCST_mpy);
    m_muonTree->Branch("MetCST_met", &MetCST_met);
    m_muonTree->Branch("MetCST_sumet", &MetCST_sumet);
    m_muonTree->Branch("MetCST_phi", &MetCST_met);
    /*m_muonTree->Branch("MetCST_et_cst", &MetCST_et_cst);
    m_muonTree->Branch("MetCST_et_el", &MetCST_et_el);
    m_muonTree->Branch("MetCST_et_ph", &MetCST_et_ph);
    m_muonTree->Branch("MetCST_et_mu", &MetCST_et_mu);
    m_muonTree->Branch("MetCST_et_jet", &MetCST_et_jet);*/

    m_muonTree->Branch("MetTrack_mpx", &MetTrack_mpx);
    m_muonTree->Branch("MetTrack_mpy", &MetTrack_mpy);
    m_muonTree->Branch("MetTrack_met", &MetTrack_met);
    m_muonTree->Branch("MetTrack_sumet", &MetTrack_sumet);
    m_muonTree->Branch("MetTrack_phi", &MetTrack_phi);

    m_muonTree->Branch("MetTruth_mpx", &MetTruth_mpx);
    m_muonTree->Branch("MetTruth_mpy", &MetTruth_mpy);
    m_muonTree->Branch("MetTruth_met", &MetTruth_met);
    m_muonTree->Branch("MetTruth_sumet", &MetTruth_sumet);

    m_muonTree->Branch("Centrality", &Centrality);
    m_muonTree->Branch("ETsumFCal", &ETsumFCal);
    m_muonTree->Branch("nPVx", &nPVx);


    m_tdt.setTypeAndName("Trig::TrigDecisionTool/TrigDecisionTool");
    CHECK( m_tdt.initialize() );

    m_tmt.setTypeAndName("Trig::MatchingTool/MyMatchingTool");
    CHECK( m_tmt.initialize() );

    m_muonSelection.setTypeAndName("CP::MuonSelectionTool/MyMuonSelectionTool");
    CHECK( m_muonSelection.initialize() );

    //m_muonTagSelection.setTypeAndName("CP::MuonSelectionTool/MyMuonTagSelectionTool");
    //CHECK( m_muonTagSelection.initialize() );

    //CP::IsolationSelectionTool m_isoSelection( "MyIsoSelectionTool" );
    m_isoSelection.setTypeAndName("CP::IsolationSelectionTool/MyIsoSelectionTool");
    CHECK( m_isoSelection.setProperty("muonWP","FixedCutTight"));
    CHECK( m_isoSelection.initialize() );


    CHECK( histSvc()->regTree("/MuonAnalysis/muonTree", m_muonTree) ); //registers tree to output stream inside a sub-directory



    return StatusCode::SUCCESS;
  }

  StatusCode MuonAnalysisAlg::finalize() {
    ATH_MSG_INFO ("Finalizing " << name() << "...");
    //
    //Things that happen once at the end of the event loop go here
    //


    return StatusCode::SUCCESS;
  }

  StatusCode MuonAnalysisAlg::execute() {
    ATH_MSG_DEBUG ("Executing " << name() << "...");
    setFilterPassed(false); //optional: start with algorithm not passed



    //
    //Your main analysis code goes here
    //If you will use this algorithm to perform event skimming, you
    //should ensure the setFilterPassed method is called
    //If never called, the algorithm is assumed to have 'passed' by default
    //


    //HERE IS AN EXAMPLE
    bool isData = true;
    bool NoSyst = true;

    const xAOD::EventInfo* ei = 0;
    CHECK( evtStore()->retrieve( ei , "EventInfo" ) );
    //ATH_MSG_INFO("eventNumber=" << ei->eventNumber() );
    m_runNumber = ei->runNumber();
    m_eventNumber = ei->eventNumber();
    m_lumiBlock = ei->lumiBlock();

    // check if the event is data or MC
    //bool isMC = false;
    if(ei->eventType( xAOD::EventInfo::IS_SIMULATION ) ) m_isMC = true;
    else {
        const xAOD::TruthParticleContainer * particles = 0;
        if( evtStore()->retrieve(particles, "TruthParticles") ) {
          m_isMC = true; // this is overlay
          std::cout << "This is overlay" << std::endl;
        }
    }
    //if (!isMC) std::cout << "This is data" << std::endl;
    if (m_isMC) isData = false;



    m_HLT_mu15 = m_tdt->isPassed("HLT_mu15");
    m_HLT_mu15_L1MU10 = m_tdt->isPassed("HLT_mu15_L1MU10");
    m_HLT_mu15_L1MU6 = m_tdt->isPassed("HLT_mu15_L1MU6");

    // loop over muons and save them in vectors
    const xAOD::MuonContainer* CalibMuons = 0;
    CHECK( evtStore()->retrieve(CalibMuons,"CalibratedMuons") );

    //const xAOD::MuonRoIContainer* muonrois = 0;
    //CHECK( evtStore()->retrieve(muonrois, "LVL1MuonRoIs") );

    const xAOD::TrackParticleContainer* tracks = 0;
    ANA_CHECK(evtStore()->retrieve( tracks, "InDetTrackParticles" ));

    const xAOD::TrackParticleContainer* msextracks = 0;
    ANA_CHECK(evtStore()->retrieve( msextracks, "ExtrapolatedMuonTrackParticles" ));


    int n_idtracks = 0;
    int n_mstracks = 0;

    using namespace asg::msgUserCode;
    ANA_CHECK_SET_TYPE (int);

    //StatusCode::enableFailure();
    CP::SystematicCode::enableFailure();
    CP::CorrectionCode::enableFailure();


    // GRL tool
    asg::AnaToolHandle<IGoodRunsListSelectionTool> m_grl;
    if (isData) {
      m_grl.setTypeAndName("GoodRunsListSelectionTool/grl");
      m_grl.isUserConfigured();
      std::vector<std::string> myGRLs;

      myGRLs.push_back(PathResolverFindCalibFile("GoodRunsLists/data15_13TeV/20170619/data15_13TeV.periodAllYear_DetStatus-v89-pro21-02_Unknown_PHYS_StandardGRL_All_Good_25ns.xml"));
      myGRLs.push_back(PathResolverFindCalibFile("GoodRunsLists/data16_13TeV/20180129/data16_13TeV.periodAllYear_DetStatus-v89-pro21-01_DQDefects-00-02-04_PHYS_StandardGRL_All_Good_25ns.xml"));
      myGRLs.push_back(PathResolverFindCalibFile("GoodRunsLists/data16_hip/20161216/data16_hip8TeV.periodAllYear_DetStatus-v86-pro20-19_DQDefects-00-02-04_PHYS_HeavyIonP_All_Good.xml"));
      myGRLs.push_back(PathResolverFindCalibFile("GoodRunsLists/data17_13TeV/20180619/data17_13TeV.periodAllYear_DetStatus-v99-pro22-01_Unknown_PHYS_StandardGRL_All_Good_25ns_Triggerno17e33prim.xml"));
      myGRLs.push_back(PathResolverFindCalibFile("GoodRunsLists/data18_13TeV/20190318/data18_13TeV.periodAllYear_DetStatus-v102-pro22-04_Unknown_PHYS_StandardGRL_All_Good_25ns_Triggerno17e33prim.xml"));

      ANA_CHECK( m_grl.setProperty("GoodRunsListVec", myGRLs) );
      ANA_CHECK( m_grl.setProperty("PassThrough", false) );
      ANA_CHECK( m_grl.retrieve() );

      ATH_MSG_INFO( "GRL tool retrieve & initialized... " );
    }

    bool eventPassesGRL(true);
    if (isData) eventPassesGRL = m_grl->passRunLB(ei->runNumber(), ei->lumiBlock());

    std::string config_file = "/srv01/cgrp/users/jpham/Wanalysis_AthAnalysis21.2.69_withSUSYTools/source/MuonAnalysis/data/MySUSYTools.conf";

    // Create the tool(s) to test:
    ST::SUSYObjDef_xAOD objTool("SUSYObjDef_xAOD");
    ANA_MSG_INFO(" ABOUT TO INITIALIZE SUSYTOOLS " );

    if(!config_file.empty()) ANA_CHECK( objTool.setProperty("ConfigFile", config_file) );

    if ( objTool.initialize() != StatusCode::SUCCESS) {
      ANA_MSG_ERROR( "Cannot initialize SUSYObjDef_xAOD..." );
      ANA_MSG_ERROR( "Exiting... " );
      exit(-1);
    } else {
      ANA_MSG_INFO( "SUSYObjDef_xAOD initialized... " );
    }
    ANA_MSG_INFO(" INITIALIZED SUSYTOOLS " );

    std::vector<std::string> mu_triggers {"HLT_mu15", "HLT_mu15_L1MU10", "HLT_mu15_L1MU6", "HLT_mu26_ivarmedium", "HLT_mu20_iloose_L1MU15"};

    // // Photons
    xAOD::PhotonContainer* photons_nominal(0);
    xAOD::ShallowAuxContainer* photons_nominal_aux(0);
    //if( !xStream.Contains("SUSY12") )//&& !xStream.Contains("SUSY8") ) // Martin : TBC
    ANA_CHECK( objTool.GetPhotons(photons_nominal,photons_nominal_aux) );

    // Muons
    xAOD::MuonContainer* muons_nominal(0);
    xAOD::ShallowAuxContainer* muons_nominal_aux(0);
    ANA_CHECK( objTool.GetMuons(muons_nominal, muons_nominal_aux) );

    // Electrons
    xAOD::ElectronContainer* electrons_nominal(0);
    xAOD::ShallowAuxContainer* electrons_nominal_aux(0);
    //if( !xStream.Contains("SUSY8") ) //SMP derivation, no electrons, no photons // Martin : TBC
    ANA_CHECK( objTool.GetElectrons(electrons_nominal, electrons_nominal_aux) );

    // Jets
    xAOD::JetContainer* jets_nominal(0);
    xAOD::ShallowAuxContainer* jets_nominal_aux(0);
    ANA_CHECK( objTool.GetJets(jets_nominal, jets_nominal_aux) );

    // TrackJets
    xAOD::JetContainer* trkjets_nominal(0);
    xAOD::ShallowAuxContainer* trkjets_nominal_aux(0);
    //ANA_CHECK( objTool.GetTrackJets(trkjets_nominal, trkjets_nominal_aux) );

    //Taus
    //xAOD::TauJetContainer* taus_nominal(0);
    //xAOD::ShallowAuxContainer* taus_nominal_aux(0);
    ////if(xStream.Contains("SUSY3")){
      //ANA_CHECK( objTool.GetTaus(taus_nominal,taus_nominal_aux) );
    ////}


    // MET
    xAOD::MissingETContainer* metcst_nominal = new xAOD::MissingETContainer;
    xAOD::MissingETAuxContainer* metcst_nominal_aux = new xAOD::MissingETAuxContainer;
    metcst_nominal->setStore(metcst_nominal_aux);
    metcst_nominal->reserve(10);

    double metsig_cst (0.);

    xAOD::MissingETContainer* mettst_nominal = new xAOD::MissingETContainer;
    xAOD::MissingETAuxContainer* mettst_nominal_aux = new xAOD::MissingETAuxContainer;
    mettst_nominal->setStore(mettst_nominal_aux);
    mettst_nominal->reserve(10);

    double metsig_tst (0.);

    xAOD::MissingETContainer* mettrack_nominal = new xAOD::MissingETContainer;
    xAOD::MissingETAuxContainer* mettrack_nominal_aux = new xAOD::MissingETAuxContainer;
    mettrack_nominal->setStore(mettrack_nominal_aux);
    mettrack_nominal->reserve(10);

    double metsig_track (0.);

    // Generic pointers for either nominal or systematics copy
    xAOD::ElectronContainer* electrons(electrons_nominal);
    xAOD::PhotonContainer* photons(photons_nominal);
    xAOD::MuonContainer* muons(muons_nominal);
    xAOD::JetContainer* jets(jets_nominal);
    xAOD::JetContainer* trkjets(trkjets_nominal);
    //xAOD::TauJetContainer* taus(taus_nominal);
    xAOD::MissingETContainer* metcst(metcst_nominal);
    xAOD::MissingETContainer* mettst(mettst_nominal);
    xAOD::MissingETContainer* mettrack(mettrack_nominal);
    // Aux containers too
    xAOD::MissingETAuxContainer* metcst_aux(metcst_nominal_aux);
    xAOD::MissingETAuxContainer* mettst_aux(mettst_nominal_aux);
    xAOD::MissingETAuxContainer* mettrack_aux(mettrack_nominal_aux);

    xAOD::JetContainer* goodJets = new xAOD::JetContainer(SG::VIEW_ELEMENTS);

    /*// If necessary (kinematics affected), make a shallow copy with the variation applied
    bool syst_affectsElectrons = ST::testAffectsObject(xAOD::Type::Electron, sysInfo.affectsType);
    bool syst_affectsMuons = ST::testAffectsObject(xAOD::Type::Muon, sysInfo.affectsType);
    bool syst_affectsTaus = ST::testAffectsObject(xAOD::Type::Tau, sysInfo.affectsType);
    bool syst_affectsPhotons = ST::testAffectsObject(xAOD::Type::Photon, sysInfo.affectsType);
    bool syst_affectsJets = ST::testAffectsObject(xAOD::Type::Jet, sysInfo.affectsType);
    bool syst_affectsBTag = ST::testAffectsObject(xAOD::Type::BTag, sysInfo.affectsType);
    //      bool syst_affectsMET = ST::testAffectsObject(xAOD::Type::MissingET, sysInfo.affectsType);

    if (sysInfo.affectsKinematics) {
        if (syst_affectsElectrons) {
          xAOD::ElectronContainer* electrons_syst(0);
          xAOD::ShallowAuxContainer* electrons_syst_aux(0);
          ANA_CHECK( objTool.GetElectrons(electrons_syst, electrons_syst_aux) );
          electrons = electrons_syst;
        }

        if (syst_affectsMuons) {
          xAOD::MuonContainer* muons_syst(0);
          xAOD::ShallowAuxContainer* muons_syst_aux(0);
          ANA_CHECK( objTool.GetMuons(muons_syst, muons_syst_aux) );
          muons = muons_syst;
        }

      	//if(syst_affectsTaus) {
      	  //xAOD::TauJetContainer* taus_syst(0);
      	  //xAOD::ShallowAuxContainer* taus_syst_aux(0);
      	  //if(xStream.Contains("SUSY3")){
      	    //ANA_CHECK( objTool.GetTaus(taus_syst,taus_syst_aux) );
      	  //}
      	  //taus = taus_syst;
      	//}

        if(syst_affectsPhotons) {
          xAOD::PhotonContainer* photons_syst(0);
          xAOD::ShallowAuxContainer* photons_syst_aux(0);
          ANA_CHECK( objTool.GetPhotons(photons_syst,photons_syst_aux) );
          photons = photons_syst;
        }

        if (syst_affectsJets) {
          xAOD::JetContainer* jets_syst(0);
          xAOD::ShallowAuxContainer* jets_syst_aux(0);
          ANA_CHECK( objTool.GetJetsSyst(*jets_nominal, jets_syst, jets_syst_aux) );
          jets = jets_syst;
        }

        //if (syst_affectsBTag) {
          //xAOD::JetContainer* trkjets_syst(0);
          //xAOD::ShallowAuxContainer* trkjets_syst_aux(0);
          //ANA_CHECK( objTool.GetTrackJets(trkjets_syst, trkjets_syst_aux) );
          //trkjets = trkjets_syst;
        //}

        xAOD::MissingETContainer* metcst_syst = new xAOD::MissingETContainer;
        xAOD::MissingETAuxContainer* metcst_syst_aux = new xAOD::MissingETAuxContainer;
        xAOD::MissingETContainer* mettst_syst = new xAOD::MissingETContainer;
        xAOD::MissingETAuxContainer* mettst_syst_aux = new xAOD::MissingETAuxContainer;
        metcst_syst->setStore(metcst_syst_aux);
        mettst_syst->setStore(mettst_syst_aux);
        metcst_nominal->reserve(10);
        metcst_nominal->reserve(10);

        metcst = metcst_syst;
        mettst = mettst_syst;
        metcst_aux = metcst_syst_aux;
        mettst_aux = mettst_syst_aux;
      } */

      ANA_CHECK( objTool.GetMET(*metcst,jets,electrons,muons,photons,0,false,false) );  // 0 for taus, false(1) for CST and false(2) No JVT if you use CST

      ANA_CHECK( objTool.GetMET(*mettst,jets,electrons,muons,photons,0,true,true) );    // 0 for taus, true(1) for TST and true(2)  JVT if you use TST

      ANA_CHECK( objTool.GetTrackMET(*mettrack,jets,electrons,muons) );

      MetTST_mpx = (*mettst_nominal)["Final"]->mpx()/1000.;
      MetTST_mpy = (*mettst_nominal)["Final"]->mpy()/1000.;
      MetTST_met = (*mettst_nominal)["Final"]->met()/1000.;
      MetTST_sumet = (*mettst_nominal)["Final"]->sumet()/1000.;
      MetTST_phi = (*mettst_nominal)["Final"]->phi();
      MetTST_et_tst = (*mettst_nominal)["PVSoftTrk"]->met()/1000.;
      MetTST_et_el = (*mettst_nominal)["RefEle"]->met()/1000.;
      MetTST_et_ph = (*mettst_nominal)["RefGamma"]->met()/1000.;
      MetTST_et_mu = (*mettst_nominal)["Muons"]->met()/1000.;
      MetTST_et_jet = (*mettst_nominal)["RefJet"]->met()/1000.;

      MetCST_mpx = (*metcst_nominal)["Final"]->mpx()/1000.;
      MetCST_mpy = (*metcst_nominal)["Final"]->mpy()/1000.;
      MetCST_met = (*metcst_nominal)["Final"]->met()/1000.;
      MetCST_sumet = (*metcst_nominal)["Final"]->sumet()/1000.;
      MetCST_phi = (*metcst_nominal)["Final"]->phi();
      /*MetCST_et_cst = (*metcst_nominal)["PVSoftTrk"]->met()/1000.;
      MetCST_et_el = (*metcst_nominal)["RefEle"]->met()/1000.;
      MetCST_et_ph = (*metcst_nominal)["RefGamma"]->met()/1000.;
      MetCST_et_mu = (*metcst_nominal)["Muons"]->met()/1000.;
      MetCST_et_jet = (*metcst_nominal)["RefJet"]->met()/1000.;*/

      MetTrack_mpx = (*mettrack_nominal)["Track"]->mpx()/1000.;
      MetTrack_mpy = (*mettrack_nominal)["Track"]->mpy()/1000.;
      MetTrack_met = (*mettrack_nominal)["Track"]->met()/1000.;
      MetTrack_sumet = (*mettrack_nominal)["Track"]->sumet()/1000.;
      MetTrack_phi = (*mettrack_nominal)["Track"]->phi();

      if (m_isMC) {
        const xAOD::MissingETContainer* met_truth(0);
        ATH_CHECK( evtStore()->retrieve(met_truth, "MET_Truth") );

        MetTruth_mpx = (*met_truth)["NonInt"]->mpx();
        MetTruth_mpy = (*met_truth)["NonInt"]->mpy();
        MetTruth_met = (*met_truth)["NonInt"]->met();
        MetTruth_sumet = (*met_truth)["NonInt"]->sumet();
      }


























    for(const auto *muon_itr: *CalibMuons) {
        bool m_match_HLT_mu15=m_tmt->match( *muon_itr, "HLT_mu15", 0.05, false);
        match_HLT_mu15->push_back(m_match_HLT_mu15);

        bool m_match_HLT_mu15_L1MU10=m_tmt->match( *muon_itr, "HLT_mu15_L1MU10", 0.05, false);
        match_HLT_mu15_L1MU10->push_back(m_match_HLT_mu15_L1MU10);

        bool m_match_HLT_mu15_L1MU6=m_tmt->match( *muon_itr, "HLT_mu15_L1MU6", 0.05, false);
        match_HLT_mu15_L1MU6->push_back(m_match_HLT_mu15_L1MU6);

        xAOD::Muon::Quality my_quality;
        my_quality = m_muonSelection->getQuality(*muon_itr);

        if ((m_isMC || (!m_isMC && m_HLT_mu15 && m_match_HLT_mu15)) && muon_itr->pt()/1000.>=15. && m_muonSelection->accept(*muon_itr) && m_isoSelection->accept(*muon_itr) ) {
          if (m_isoSelection->accept( *muon_itr )) printf(" Muon passes Isolation - ");
          m_muon_pt->push_back(muon_itr->pt()/1000);
          m_muon_quality->push_back(my_quality);
          m_muon_eta->push_back(muon_itr->eta());
          m_muon_phi->push_back(muon_itr->phi());
          m_muon_charge->push_back(muon_itr->charge());
          m_muon_type->push_back(muon_itr->muonType());




      } // close if ((isMC || (!isMC && m_HLT_mu15 && m_match_HLT_mu15)) && muon_itr->pt()/1000.>=25. && m_muonSelection->accept(*muon_itr) && m_isoSelection->accept(*muon_itr) && fabs(muon_itr->eta())<=2.4 && my_quality == 0 && muon_itr->muonType() == 0) {
    } //close main muon loop



    const xAOD::VertexContainer * vertices = 0;
    ATH_CHECK( evtStore()->retrieve (vertices, "PrimaryVertices"));
    nPVx = vertices->size();
    if(vertices->size()<2) return StatusCode::SUCCESS;
    xAOD::VertexContainer::const_iterator vtx_itr = vertices->begin();
    xAOD::VertexContainer::const_iterator vtx_end = vertices->end();
    // find primary vertex
    const xAOD::Vertex* primaryVertex = 0;
    for(;vtx_itr!=vtx_end;++vtx_itr) {
          if((*vtx_itr)->vertexType()==xAOD::VxType::PriVtx) {
                primaryVertex = (*vtx_itr);
                m_vertex_x = primaryVertex->x();
                m_vertex_y = primaryVertex->y();
                m_vertex_z = primaryVertex->z();
               break;
          }
     }

    const xAOD::Vertex* pileup = 0;
    int pile_up_vertices=0;
    int trackn = 0;
    for(;vtx_itr!=vtx_end;++vtx_itr) {
          if((*vtx_itr)->vertexType()==xAOD::VxType::PileUp) {
                pileup = (*vtx_itr);
                trackn = pileup->nTrackParticles();
                if(trackn>6) pile_up_vertices=1;
            }
       }//if the number of tracks>6 then this is a pile up vertex
     auto centTool = make_unique<HI::HICentralityTool>("CentralityTool");
     ANA_CHECK( centTool->setProperty("RunSpecies","pPb2016") );
     ANA_CHECK( centTool->initialize() );
     ETsumFCal = centTool->getCentralityEstimator();
     if (!pile_up_vertices) Centrality = centTool->getCentralityPercentile();



    ////////////////////


    m_muonTree->Fill();
    match_HLT_mu15->clear();
    match_HLT_mu15_L1MU10->clear();
    match_HLT_mu15_L1MU6->clear();
    m_muon_pt->clear();
    m_muon_eta->clear();
    m_muon_phi->clear();
    m_muon_charge->clear();
    m_muon_quality->clear();
    m_muon_type->clear();






    setFilterPassed(true); //if got here, assume that means algorithm passed
    return StatusCode::SUCCESS;
  }

  StatusCode MuonAnalysisAlg::beginInputFile() {
    //
    //This method is called at the start of each input file, even if
    //the input file contains no events. Accumulate metadata information here
    //

    //example of retrieval of CutBookkeepers: (remember you will need to include the necessary header files and use statements in requirements file)
     //const xAOD::CutBookkeeperContainer* bks = 0;
     //CHECK( inputMetaStore()->retrieve(bks, "CutBookkeepers") );

    //example of IOVMetaData retrieval (see https://twiki.cern.ch/twiki/bin/viewauth/AtlasProtected/AthAnalysisBase#How_to_access_file_metadata_in_C)
    //float beamEnergy(0); CHECK( retrieveMetadata("/TagInfo","beam_energy",beamEnergy) );
    //std::vector<float> bunchPattern; CHECK( retrieveMetadata("/Digitiation/Parameters","BeamIntensityPattern",bunchPattern) );



    return StatusCode::SUCCESS;
  }

  double MuonAnalysisAlg::get_dR(const double eta1, const double phi1, const double eta2, const double phi2) {
      double deta = fabs(eta1 - eta2);
      double dphi = fabs(phi1 - phi2) < TMath::Pi() ? fabs(phi1 - phi2) : 2*TMath:: \
        Pi() - fabs(phi1 - phi2);
      return sqrt(deta*deta + dphi*dphi);
    }

  /*double MuonAnalysisAlg::ReturnFCalEnergy() {
        const xAOD::VertexContainer * vertices = 0;
        ATH_CHECK( evtStore()->retrieve (vertices, "PrimaryVertices"));
        xAOD::VertexContainer::const_iterator vtx_itr = vertices->begin();
        xAOD::VertexContainer::const_iterator vtx_end = vertices->end();
        const xAOD::Vertex* pileup = 0;
        int pile_up_vertices=0;
        int trackn = 0;
        for(;vtx_itr!=vtx_end;++vtx_itr) {
              if((*vtx_itr)->vertexType()==xAOD::VxType::PileUp) {
                    pileup = (*vtx_itr);
                    trackn = pileup->nTrackParticles();
                    if(trackn>6) pile_up_vertices=1;
                }
           }//if the number of tracks>6 then this is a pile up vertex
       auto centTool = make_unique<HI::HICentralityTool>("CentralityTool");
       ANA_CHECK( centTool->setProperty("RunSpecies","pPb2016") );
       ANA_CHECK( centTool->initialize() );
    	double ETsumFCal = 0;
      ETsumFCal = centTool->getCentralityEstimator();
      return ETsumFCal;
    	float Centrality = 0;
    	if (!pile_up_vertices) Centrality = centTool->getCentralityPercentile();

  	  const xAOD::HIEventShapeContainer *hiue(0);
      CHECK( evtStore()->retrieve(hiue,"CaloSums") );
  	  xAOD::HIEventShapeContainer::const_iterator es_itr = hiue->begin();
  	  xAOD::HIEventShapeContainer::const_iterator es_end = hiue->end();
  	  double m_fcalEt = 0;
  	  for (;es_itr!=es_end;es_itr++ ) {
  	      float et = (*es_itr)->et();
  	      const std::string name = (*es_itr)->auxdataConst<std::string>("Summary");
  	      if (name=="FCal") m_fcalEt = et*1e-6;
  	    }
  	  return m_fcalEt;
  	}*/
}
