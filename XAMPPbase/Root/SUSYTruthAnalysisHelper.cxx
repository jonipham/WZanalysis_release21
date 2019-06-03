#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/ITruthSelector.h>
#include <XAMPPbase/ReconstructedParticles.h>
#include <XAMPPbase/SUSYTruthAnalysisHelper.h>

namespace XAMPP {
    SUSYTruthAnalysisHelper::SUSYTruthAnalysisHelper(const std::string& myname) : SUSYAnalysisHelper(myname) {}
    StatusCode SUSYTruthAnalysisHelper::initialize() {
        ATH_CHECK(setProperty("doTruth", true));
        ATH_CHECK(setProperty("doPRW", false));
        DisableRecoFlags();
        ATH_CHECK(SUSYAnalysisHelper::initialize());
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTruthAnalysisHelper::initializeObjectTools() {
        ATH_CHECK(m_truth_selection.retrieve());
        return StatusCode::SUCCESS;
    }

    StatusCode SUSYTruthAnalysisHelper::LoadContainers() {
        ATH_CHECK(initializeOuputFormat());
        ATH_MSG_DEBUG("Call m_XAMPPInfo->LoadInfo()");
        ATH_CHECK(m_XAMPPInfo->LoadInfo());
        if (isData() == m_XAMPPInfo->isMC()) {
            ATH_MSG_ERROR("This is a Truth selector. Not running on data");
            return StatusCode::FAILURE;
        }
        ATH_MSG_DEBUG("Call m_truth_selection->LoadContainers()");
        ATH_CHECK(m_truth_selection->LoadContainers());
        ATH_MSG_DEBUG("Call SaveCrosSection");
        ATH_CHECK(SaveCrossSection());
        ATH_MSG_DEBUG("Exit LoadContainers()");
        return StatusCode::SUCCESS;
    }

    StatusCode SUSYTruthAnalysisHelper::FillInitialObjects(const CP::SystematicSet* systset) {
        ATH_MSG_DEBUG("FillInitialObjects...");
        ATH_CHECK(m_ParticleConstructor->PrepareContainer(systset));
        ATH_CHECK(m_truth_selection->InitialFill(*systset));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTruthAnalysisHelper::RemoveOverlap() {
        ATH_MSG_DEBUG("OverlapRemoval...");
        ATH_CHECK(XAMPP::RemoveOverlap(m_truth_selection->GetTruthPreElectrons(), m_truth_selection->GetTruthPreElectrons(), 0.05));
        ATH_CHECK(XAMPP::RemoveOverlap(m_truth_selection->GetTruthPreJets(), m_truth_selection->GetTruthPreElectrons(), 0.2));

        ATH_CHECK(XAMPP::RemoveOverlap(m_truth_selection->GetTruthPreElectrons(), m_truth_selection->GetTruthPreJets(), 0.4));
        ATH_CHECK(XAMPP::RemoveOverlap(m_truth_selection->GetTruthPreMuons(), m_truth_selection->GetTruthPreJets(), 0.4));

        ATH_CHECK(XAMPP::RemoveOverlap(m_truth_selection->GetTruthPreTaus(), m_truth_selection->GetTruthPreElectrons(), 0.2));
        ATH_CHECK(XAMPP::RemoveOverlap(m_truth_selection->GetTruthPreTaus(), m_truth_selection->GetTruthPreMuons(), 0.2));
        ATH_CHECK(XAMPP::RemoveOverlap(m_truth_selection->GetTruthPreTaus(), m_truth_selection->GetTruthPreTaus(), 0.2));
        ATH_CHECK(XAMPP::RemoveOverlap(m_truth_selection->GetTruthPreJets(), m_truth_selection->GetTruthPreTaus(), 0.4));

        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTruthAnalysisHelper::FillObjects(const CP::SystematicSet* systset) {
        ATH_MSG_DEBUG("FillObjects...");
        ATH_CHECK(m_XAMPPInfo->SetSystematic(systset));
        ATH_CHECK(m_truth_selection->FillTruth(*systset));
        return StatusCode::SUCCESS;
    }

    bool SUSYTruthAnalysisHelper::PassObjectCleaning(const CP::SystematicSet* systset) const {
        ATH_CHECK(m_XAMPPInfo->SetSystematic(systset));
        return true;
    }
    StatusCode SUSYTruthAnalysisHelper::initializeEventVariables() {
        // Lets define some variables which we want to store in the output tree
        // / use in the cutflow
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<float>("JetHt"));
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<int>("N_bjets"));
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<int>("N_SignalLeptons", false));  // A variable on which we are cutting
                                                                                  // does not to be stored in the tree
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<int>("N_Jets", false));           // A variable on which we are cutting does not
                                                                                  // to be stored in the tree

        ATH_CHECK(m_XAMPPInfo->BookParticleStorage("Elec"));
        ATH_CHECK(m_XAMPPInfo->BookParticleStorage("Muon"));
        ATH_CHECK(m_XAMPPInfo->BookParticleStorage("Jet"));
        ATH_CHECK(m_XAMPPInfo->BookParticleStorage("TruthParticles"));

        XAMPP::ParticleStorage* ElectronStore = m_XAMPPInfo->GetParticleStorage("Elec");
        ATH_CHECK(ElectronStore->SaveFloat("charge"));
        ATH_CHECK(ElectronStore->SaveChar("passOR"));
        ATH_CHECK(ElectronStore->SaveChar("signal"));

        XAMPP::ParticleStorage* MuonStore = m_XAMPPInfo->GetParticleStorage("Muon");
        ATH_CHECK(MuonStore->SaveFloat("charge"));
        ATH_CHECK(MuonStore->SaveChar("passOR"));
        ATH_CHECK(MuonStore->SaveChar("signal"));

        XAMPP::ParticleStorage* TruthStore = m_XAMPPInfo->GetParticleStorage("TruthParticles");
        ATH_CHECK(TruthStore->SaveInteger("pdgId"));
        ATH_CHECK(TruthStore->SaveFloat("charge"));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTruthAnalysisHelper::ComputeEventVariables() {
        // First lets define the Pointer to the Storage elements
        static XAMPP::Storage<float>* dec_JetHt = m_XAMPPInfo->GetVariableStorage<float>("JetHt");
        // Static avoids that the Storage element is
        // retrieved each function call
        static XAMPP::Storage<int>* dec_Nbjet = m_XAMPPInfo->GetVariableStorage<int>("N_bjets");
        static XAMPP::Storage<int>* dec_Nlep = m_XAMPPInfo->GetVariableStorage<int>("N_SignalLeptons");
        static XAMPP::Storage<int>* dec_Njets = m_XAMPPInfo->GetVariableStorage<int>("N_Jets");
        // Then lets calculate our event variables... there are lots of
        // functions in the AnalysisUtils in order to do that, feel free to add
        // other functions for this purpose
        int N_Lep = 2;
        int NJets = 2;
        int Nbjets = 2;
        float Ht = 1.;

        // Finally store the variables they are then used by the Cut Class or
        // just written out into the trees
        ATH_CHECK(dec_JetHt->Store(Ht));
        ATH_CHECK(dec_Nbjet->Store(Nbjets));
        ATH_CHECK(dec_Nlep->Store(N_Lep));
        ATH_CHECK(dec_Njets->Store(NJets));

        static XAMPP::ParticleStorage* ElectronStore = m_XAMPPInfo->GetParticleStorage("Elec");
        ATH_CHECK(ElectronStore->Fill(m_truth_selection->GetTruthBaselineElectrons()));
        static XAMPP::ParticleStorage* MuonStore = m_XAMPPInfo->GetParticleStorage("Muon");
        ATH_CHECK(MuonStore->Fill(m_truth_selection->GetTruthBaselineMuons()));

        static XAMPP::ParticleStorage* JetStore = m_XAMPPInfo->GetParticleStorage("Jet");
        ATH_CHECK(JetStore->Fill(m_truth_selection->GetTruthBaselineJets()));

        static XAMPP::ParticleStorage* TruthStore = m_XAMPPInfo->GetParticleStorage("TruthParticles");
        ATH_CHECK(TruthStore->Fill(m_truth_selection->GetTruthParticles()));
        return StatusCode::SUCCESS;
    }
}  // namespace XAMPP
