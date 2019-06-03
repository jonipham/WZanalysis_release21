#ifndef MUONANALYSIS_MUONANALYSISALG_H
#define MUONANALYSIS_MUONANALYSISALG_H

#include "AthAnalysisBaseComps/AthAnalysisAlgorithm.h"
#include "AsgTools/AnaToolHandle.h"
#include "TrigDecisionTool/TrigDecisionTool.h"
#include "TriggerMatchingTool/MatchingTool.h"
#include "MuonSelectorTools/MuonSelectionTool.h"
#include "xAODHIEvent/HIEventShapeContainer.h"
//#include <HIEventUtils/IHICentralityTool.h>
//#include <HIEventUtils/HICentralityTool.h>
#include "xAODHIEvent/HIEventShape.h"
#include "xAODTracking/TrackParticleContainer.h"
#include "xAODTracking/VertexContainer.h"
#include "xAODTracking/TrackingPrimitives.h"
#include "InDetTrackSelectionTool/InDetTrackSelectionTool.h"
#include "InDetTrackSelectionTool/IInDetTrackSelectionTool.h"
#include "IsolationSelection/IsolationSelectionTool.h"
#include <SUSYTools/SUSYObjDef_xAOD.h>
#include <SUSYTools/ISUSYObjDef_xAODTool.h>
#include <METInterface/IMETMaker.h>
#include <METInterface/IMETSystematicsTool.h>
#include <xAODMissingET/MissingETAssociationMap.h>
#include <xAODMissingET/MissingETContainer.h>

#include <XAMPPbase/SUSYAnalysisHelper.h>
//#include <XAMPPbase/IEventInfo.h>
#include <XAMPPbase/EventInfo.h>
#include <xAODBase/IParticleHelpers.h>
#include <XAMPPbase/AnalysisConfig.h>

#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/IAnalysisHelper.h>
#include <XAMPPbase/IAnalysisModule.h>

#include <XAMPPbase/HistoBase.h>
#include <XAMPPbase/TreeHelpers.h>

#include <XAMPPbase/IDiTauSelector.h>
#include <XAMPPbase/IElectronSelector.h>
#include <XAMPPbase/IJetSelector.h>
#include <XAMPPbase/IMuonSelector.h>
#include <XAMPPbase/IPhotonSelector.h>
#include <XAMPPbase/ITauSelector.h>
#include <XAMPPbase/ITruthSelector.h>

#include <HIEventUtils/IHICentralityTool.h>
#include <HIEventUtils/HICentralityTool.h>
#include "CxxUtils/make_unique.h"



//Example ROOT Includes
#include "TTree.h"
#include "TH1D.h"

class ITHistSvc;


namespace SUSY {
    class CrossSectionDB;
}
namespace CP {
    class SystematicSet;
    class IJetTileCorrectionTool;
}  // namespace CP
class IGoodRunsListSelectionTool;

namespace XAMPP {
  class TreeBase;
  class IMetaDataTree;
  class IAnalysisConfig;
  class IEventInfo;
  class EventInfo;
  class ReconstructedParticles;
  class IMetSelector;
  class ITriggerTool;
  class ISystematics;

  class MuonAnalysisAlg: public ::AthAnalysisAlgorithm {
   public:
    MuonAnalysisAlg( const std::string& name, ISvcLocator* pSvcLocator );
    virtual ~MuonAnalysisAlg();

    ///uncomment and implement methods as required

                                          //IS EXECUTED:
    virtual StatusCode  initialize();     //once, before any input is loaded
    virtual StatusCode  beginInputFile(); //start of each input file, only metadata loaded
    //virtual StatusCode  firstExecute();   //once, after first eventdata is loaded (not per file)
    virtual StatusCode  execute();        //per event
    //virtual StatusCode  endInputFile();   //end of each input file
    //virtual StatusCode  metaDataStop();   //when outputMetaStore is populated by MetaDataTools
    virtual StatusCode  finalize();       //once, after all events processed
    //virtual StatusCode evtStore();


    ///Other useful methods provided by base class are:
    ///evtStore()        : ServiceHandle to main event data storegate
    ///inputMetaStore()  : ServiceHandle to input metadata storegate
    ///outputMetaStore() : ServiceHandle to output metadata storegate
    ///histSvc()         : ServiceHandle to output ROOT service (writing TObjects)
    ///currentFile()     : TFile* to the currently open input file
    ///retrieveMetadata(...): See twiki.cern.ch/twiki/bin/view/AtlasProtected/AthAnalysisBase#ReadingMetaDataInCpp



   private:

     //Example algorithm property, see constructor for declaration:
     //int m_nProperty = 0;
     float m_runNumber;
     float m_eventNumber;
     float m_lumiBlock;
     bool m_HLT_mu15;
     bool m_HLT_mu15_L1MU6;
     bool m_HLT_mu15_L1MU10;
     bool m_isMC;
     bool eventPassesGRL;

     float m_FCalSumEt;
     float truthmuon;

     float MetTST_mpx;
     float MetTST_mpy;
     float MetTST_met;
     float MetTST_sumet;
     float MetTST_phi;
     float MetTST_et_tst;
     float MetTST_et_el;
     float MetTST_et_ph;
     float MetTST_et_mu;
     float MetTST_et_jet;

     float MetCST_mpx;
     float MetCST_mpy;
     float MetCST_met;
     float MetCST_sumet;
     float MetCST_phi;
     /*float MetCST_et_tst;
     float MetCST_et_el;
     float MetCST_et_ph;
     float MetCST_et_mu;
     float MetCST_et_jet;*/

     float MetTruth_mpx;
     float MetTruth_mpy;
     float MetTruth_met;
     float MetTruth_sumet;

     float MetTrack_mpx;
     float MetTrack_mpy;
     float MetTrack_met;
     float MetTrack_sumet;
     float MetTrack_phi;

     float Centrality;
     float ETsumFCal;
     int nPVx;
     float m_vertex_x, m_vertex_y, m_vertex_z;

     std::vector<float> *m_muon_pt=0;
     std::vector<float> *m_muon_eta=0;
     std::vector<float> *m_muon_phi=0;
     std::vector<float> *m_muon_charge=0;
     std::vector<unsigned int> *m_muon_quality=0;
     std::vector<unsigned int> *m_muon_type=0;

     std::vector<float> *m_Lmuon_pt=0;
     std::vector<float> *m_Lmuon_eta=0;
     std::vector<float> *m_Lmuon_phi=0;
     std::vector<float> *m_Lmuon_charge=0;
     std::vector<unsigned int> *m_Lmuon_quality=0;
     std::vector<unsigned int> *m_Lmuon_type=0;

     std::vector<float> *m_Gmuon_pt=0;
     std::vector<float> *m_Gmuon_eta=0;
     std::vector<float> *m_Gmuon_phi=0;
     std::vector<float> *m_Gmuon_charge=0;
     std::vector<unsigned int> *m_Gmuon_quality=0;
     std::vector<unsigned int> *m_Gmuon_type=0;

     std::vector<float> *m_GLmuon_pt=0;
     std::vector<float> *m_GLmuon_eta=0;
     std::vector<float> *m_GLmuon_phi=0;
     std::vector<float> *m_GLmuon_charge=0;
     std::vector<unsigned int> *m_GLmuon_quality=0;
     std::vector<unsigned int> *m_GLmuon_type=0;

     std::vector<float> *m_FCLmuon_pt=0;
     std::vector<float> *m_FCLmuon_eta=0;
     std::vector<float> *m_FCLmuon_phi=0;
     std::vector<float> *m_FCLmuon_charge=0;
     std::vector<unsigned int> *m_FCLmuon_quality=0;
     std::vector<unsigned int> *m_FCLmuon_type=0;

     std::vector<float> *m_IDtrackLink_pt=0;
     std::vector<float> *m_MStrackLink_pt=0;
     std::vector<float> *m_CBtrackLink_pt=0;
     std::vector<float> *m_IDtrackLink_phi=0;
     std::vector<float> *m_MStrackLink_phi=0;
     std::vector<float> *m_CBtrackLink_phi=0;
     std::vector<float> *m_IDtrackLink_eta=0;
     std::vector<float> *m_MStrackLink_eta=0;
     std::vector<float> *m_CBtrackLink_eta=0;
     std::vector<float> *m_IDtrackLink_d0=0;
     std::vector<float> *m_IDtrackLink_z0=0;
     std::vector<float> *m_IDtrackLink_d0err=0;
     std::vector<float> *m_IDtrackLink_z0err=0;
     std::vector<float> *m_IDtrackLink_d0sig=0;
     std::vector<float> *m_IDtrackLink_z0sig=0;
     std::vector<float> *m_MStrackLink_d0=0;
     std::vector<float> *m_MStrackLink_z0=0;
     std::vector<float> *m_MStrackLink_d0err=0;
     std::vector<float> *m_MStrackLink_z0err=0;
     std::vector<float> *m_MStrackLink_d0sig=0;
     std::vector<float> *m_MStrackLink_z0sig=0;
     std::vector<float> *m_CBtrackLink_d0=0;
     std::vector<float> *m_CBtrackLink_z0=0;
     std::vector<float> *m_CBtrackLink_d0err=0;
     std::vector<float> *m_CBtrackLink_z0err=0;
     std::vector<float> *m_CBtrackLink_d0sig=0;
     std::vector<float> *m_CBtrackLink_z0sig=0;

     std::vector<bool> *match_HLT_mu15=0;
     std::vector<bool> *match_HLT_mu15_L1MU10=0;
     std::vector<bool> *match_HLT_mu15_L1MU6=0;


     //Example histogram, see initialize method for registration to output histSvc
     //TH1D* m_myHist = 0;
     //TTree* m_myTree = 0;
     //TH1D* m_muonHist = 0;
     TTree* m_muonTree = 0;


     asg::AnaToolHandle<Trig::TrigDecisionTool> m_tdt;
     asg::AnaToolHandle<Trig::MatchingTool > m_tmt;
     asg::AnaToolHandle<CP::MuonSelectionTool > m_muonSelection;
     //asg::AnaToolHandle<CP::MuonSelectionTool > m_muonTagSelection;
     asg::AnaToolHandle<InDet::InDetTrackSelectionTool > m_trackSelection;
     asg::AnaToolHandle<CP::IsolationSelectionTool > m_isoSelection;
     asg::AnaToolHandle<CP::IsolationSelectionTool > m_isoSelectionGrad;
     asg::AnaToolHandle<CP::IsolationSelectionTool > m_isoSelectionGradLoose;
     asg::AnaToolHandle<CP::IsolationSelectionTool > m_isoSelectionFCLoose;
     asg::AnaToolHandle<CP::IsolationSelectionTool > m_isoSelectionLoose;
     //asg::AnaToolHandle<CP::IsolationSelectionTool > m_isoIDSelection;

     asg::AnaToolHandle<IMETMaker> m_metutil;
     //asg::AnaToolHandle<XAMPP::IEventInfo> m_XAMPPInfoHandle;
     //asg::AnaToolHandle<XAMPP::IEventInfo> m_XAMPPInfo;
     //XAMPP::EventInfo* m_XAMPPInfo;

     // xAOD containers to build the calibrated MET from the objects

          //  Storages to pipe the final MET's into
          //XAMPP::Storage<XAMPPmet>* m_sto_MetTST;
          //XAMPP::Storage<XAMPPmet>* m_sto_MetCST;
          //XAMPP::Storage<XAMPPmet>* m_sto_MetTrack;

          // xAOD containers used to build the MET from
          const xAOD::MissingETContainer* m_xAODMet;
          const xAOD::MissingETAssociationMap* m_xAODMap;
          const xAOD::MissingETContainer* m_xAODTrackMet;

          asg::AnaToolHandle<XAMPP::IEventInfo> m_InfoHandle;
          //asg::AnaToolHandle<XAMPP::IEventInfo> m_XAMPPInfo;
          //XAMPP::EventInfo* m_XAMPPInfo;



          ToolHandle<XAMPP::IElectronSelector> m_electron_selection;
          ToolHandle<XAMPP::IJetSelector> m_jet_selection;
          ToolHandle<XAMPP::IMetSelector> m_met_selection;
          ToolHandle<XAMPP::IMuonSelector> m_muon_selection;
          ToolHandle<XAMPP::IPhotonSelector> m_photon_selection;
          ToolHandle<XAMPP::ITauSelector> m_tau_selection;
          ToolHandle<XAMPP::IDiTauSelector> m_ditau_selection;
          ToolHandle<XAMPP::ITruthSelector> m_truth_selection;
          ToolHandle<XAMPP::ISystematics> m_systematics;
          ToolHandle<XAMPP::ITriggerTool> m_triggers;

          // SUSYTools as AnaToolHandle because we want to have the possibility in
          // place that SUSYTools is created by the Helper itself


          std::string m_met_key;
          std::string m_met_map_key;
          std::string m_met_track_key;

          std::string m_systName;

          bool m_store_significance;

          // Stuff to calculate the met itself not using SUSYTools
          ToolHandle<IMETMaker> m_metMaker;
          ToolHandle<IMETSystematicsTool> m_metSystTool;

          // Reference terms to get the suff from
          std::string m_EleRefTerm;
          std::string m_MuoRefTerm;
          std::string m_TauRefTerm;
          std::string m_JetRefTerm;
          std::string m_PhoRefTerm;

          std::string m_TrackSoftTerm;
          std::string m_CaloSoftTerm;

          std::string m_FinalMetTerm;
          std::string m_FinalTrackTerm;

          bool m_trkJetsyst;
          bool m_trkMETsyst;



     double get_dR(const double eta1, const double phi1, const double eta2, const double phi2);
     double ReturnFCalEnergy();

  };
}

#endif //> !MUONANALYSIS_MUONANALYSISALG_H
