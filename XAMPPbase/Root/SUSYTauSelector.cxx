#include <AsgAnalysisInterfaces/IPileupReweightingTool.h>
#include <TauAnalysisTools/ITauEfficiencyCorrectionsTool.h>
#include <TauAnalysisTools/TauSelectionTool.h>
#include <TauAnalysisTools/TauTruthMatchingTool.h>
#include <XAMPPbase/SUSYTauSelector.h>
#include <XAMPPbase/SUSYTriggerTool.h>
#include <XAMPPbase/ToolHandleSystematics.h>
#include <xAODTau/TauxAODHelpers.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace XAMPP {
    std::string to_string(TauAnalysisTools::EfficiencyCorrectionType T) {
        if (T == TauAnalysisTools::EfficiencyCorrectionType::SFRecoHadTau) return "Reco";
        if (T == TauAnalysisTools::EfficiencyCorrectionType::SFJetIDHadTau) return "Id";
        if (T == TauAnalysisTools::EfficiencyCorrectionType::SFEleOLRHadTau) return "EleORHad";
        if (T == TauAnalysisTools::EfficiencyCorrectionType::SFEleOLRElectron) return "EleOREle";
        if (T == TauAnalysisTools::EfficiencyCorrectionType::SFEleIDHadTau) return "SFEleIDHadTau";
        return "MoreCoffeePlease";
    }
    //##########################################################################
    //                            SUSYTauSelector
    //##########################################################################
    SUSYTauSelector::SUSYTauSelector(const std::string& myname) :
        SUSYParticleSelector(myname),
        m_xAODTaus(nullptr),
        m_Taus(nullptr),
        m_TausAux(nullptr),
        m_PreTaus(nullptr),
        m_BaselineTaus(nullptr),
        m_SignalTaus(nullptr),
        m_SignalQualTaus(nullptr),
        m_tauDecorations(),
        m_TruthMatching("TauTruthMatchingTool"),
        m_SF(),
        m_BaseTauSelectionTool(""),
        m_SignalTauSelectionTool(""),
        m_trigger_tool(),
        m_BaselineSF_Tools(),
        m_SignalSF_Tools(),
        m_Baseline_TrigSF_Tools(),
        m_Signal_TrigSF_Tools(),
        m_SeparateSF(false),
        m_doIdSF(true),
        m_doTrigSF(true),
        m_TriggerExp(),
        m_StoreMultiTriggerSf(false),
        m_RequireTrigMatchForSF(false),
        m_writeBaselineSF(false),
        m_writeBaselineTrigSF(false),
        m_StoreTruthClassifier(false),
        m_ignoreBaseTauIDTool(false) {
        declareProperty("SeparateSF", m_SeparateSF);
        declareProperty("ApplyIdSF", m_doIdSF);
        declareProperty("ApplyTriggerSF", m_doTrigSF);
        declareProperty("WriteBaselineSF", m_writeBaselineSF);
        declareProperty("WriteBaselineTrigSF", m_writeBaselineTrigSF);
        declareProperty("SFTrigger", m_TriggerExp);
        declareProperty("ExcludeTrigSFfromTotalWeight", m_StoreMultiTriggerSf);
        declareProperty("TrigMatchingForTrigSF", m_RequireTrigMatchForSF);

        declareProperty("StoreTruthClassifier", m_StoreTruthClassifier);

        declareProperty("BaselineTauSelectionTool", m_BaseTauSelectionTool);
        declareProperty("SignalTauSelectionTool", m_SignalTauSelectionTool);
        declareProperty("TriggerTool", m_trigger_tool);

        declareProperty("ignoreBaseSelectionTool", m_ignoreBaseTauIDTool);

        m_TruthMatching.declarePropertyFor(this, "TruthMatchingTool", "Tau truth matching tool");

        SetContainerKey("TauJets");
        SetObjectType(XAMPP::SelectionObject::Tau);
    }

    SUSYTauSelector::~SUSYTauSelector() {}
    TauLink SUSYTauSelector::GetLink(const xAOD::TauJet& tau) const { return TauLink(*m_Taus, tau.index()); }
    TauLink SUSYTauSelector::GetOrigLink(const xAOD::TauJet& tau) const {
        const xAOD::TauJet* Otau = dynamic_cast<const xAOD::TauJet*>(xAOD::getOriginalObject(tau));
        return TauLink(*m_xAODTaus, Otau != nullptr ? Otau->index() : tau.index());
    }
    const xAOD::TauJetContainer* SUSYTauSelector::GetTauContainer() const { return m_xAODTaus; }
    xAOD::TauJetContainer* SUSYTauSelector::GetTaus() const { return m_Taus; }
    xAOD::TauJetContainer* SUSYTauSelector::GetPreTaus() const { return m_PreTaus; }
    xAOD::TauJetContainer* SUSYTauSelector::GetSignalTaus() const { return m_SignalTaus; }
    xAOD::TauJetContainer* SUSYTauSelector::GetSignalNoORTaus() const { return m_SignalQualTaus; }
    xAOD::TauJetContainer* SUSYTauSelector::GetBaselineTaus() const { return m_BaselineTaus; }

    std::shared_ptr<TauDecorations> SUSYTauSelector::GetTauDecorations() const { return m_tauDecorations; }

    StatusCode SUSYTauSelector::initialize() {
        if (isInitialized()) { return StatusCode::SUCCESS; }
        ATH_CHECK(SUSYParticleSelector::initialize());
        if (!ProcessObject()) { return StatusCode::SUCCESS; }
        setupDecorations();
        if (!m_BaseTauSelectionTool.isSet()) {
            m_BaseTauSelectionTool = GetCPTool<TauAnalysisTools::ITauSelectionTool>("TauSelectionToolBaseline");
        }
        if (!m_SignalTauSelectionTool.isSet()) {
            m_SignalTauSelectionTool = GetCPTool<TauAnalysisTools::ITauSelectionTool>("TauSelectionTool");
        }
        ATH_CHECK(m_SignalTauSelectionTool.retrieve());
        ATH_CHECK(m_BaseTauSelectionTool.retrieve());
        m_StoreTruthClassifier = m_StoreTruthClassifier && !isData();
        if (isData() || !ProcessObject()) return StatusCode::SUCCESS;

        // Setup the Tau truth-matching tool
        if (!m_TruthMatching.isUserConfigured()) {
            m_TruthMatching.setTypeAndName("TauAnalysisTools::TauTruthMatchingTool/TauTruthMatching");
        }
        ATH_CHECK(m_TruthMatching.retrieve());
        m_doTrigSF = m_doTrigSF && !m_TriggerExp.empty();
        if (!m_doTrigSF) m_TriggerExp.clear();
        m_StoreMultiTriggerSf = m_StoreMultiTriggerSf || m_TriggerExp.size() > 1;

        // For each SF type a seperate map is stored in this vector

        std::vector<TauWeightMap> Signal_SFs;
        std::vector<TauWeightMap> Baseline_SFs;

        // Analogous for the trigger SFs, hence the user can define multiple
        // trigger SFs The map contains a vector auf TauWeights
        TauWeight_VectorMap Baseline_TriggerSF;
        TauWeight_VectorMap Signal_TriggerSF;

        //  If the user is not interested in separating the SFs into single
        //  types retrieve the tool instance from SUSYTools for the signal ID sf
        if (!m_SeparateSF) {
            TauWeightMap RecoSF;
            TauEffiToolHandle IdTool = GetCPTool<TauAnalysisTools::ITauEfficiencyCorrectionsTool>("TauEfficiencyCorrectionsTool");
            ATH_CHECK(initializeScaleFactors("Id", IdTool, RecoSF, ScaleFactorMapContains::SignalSf));
            Signal_SFs.push_back(RecoSF);
        } else {
            // The user wants to split into each component we need to create the
            // Tools first for baseline and signal, seperately
            // Attach them to the appopiate map vector
            if (m_writeBaselineSF) {
                ATH_CHECK(initializeTauEfficiencySFTools(m_BaselineSF_Tools, m_BaseTauSelectionTool));
                ATH_CHECK(initializeScaleFactors(m_BaselineSF_Tools, Baseline_SFs, ScaleFactorMapContains::BaselineSf));
            }
            ATH_CHECK(initializeTauEfficiencySFTools(m_SignalSF_Tools, m_SignalTauSelectionTool));
            ATH_CHECK(initializeScaleFactors(m_SignalSF_Tools, Signal_SFs, ScaleFactorMapContains::SignalSf));
        }
        // initialize the trigger SFs
        if (m_doTrigSF) {
            ATH_CHECK(m_trigger_tool.retrieve());
            if (m_writeBaselineTrigSF && m_writeBaselineSF) {
                ATH_CHECK(initializeTriggerEfficiencySFTools(m_Baseline_TrigSF_Tools, m_BaseTauSelectionTool));
                ATH_CHECK(initializeTriggerScaleFactors(m_Baseline_TrigSF_Tools, Baseline_TriggerSF, ScaleFactorMapContains::BaselineSf));
            }
            ATH_CHECK(initializeTriggerEfficiencySFTools(m_Signal_TrigSF_Tools, m_SignalTauSelectionTool));
            ATH_CHECK(initializeTriggerScaleFactors(m_Signal_TrigSF_Tools, Signal_TriggerSF, ScaleFactorMapContains::SignalSf));
        }
        // Assemble the total tau sf from the created component SFs
        for (auto set : m_systematics->GetWeightSystematics(ObjectType())) {
            TauWeightHandler_Ptr Baseline(new TauWeightHandler(set));
            TauWeightHandler_Ptr Signal(new TauWeightHandler(set));
            Baseline->multipleTriggerSF(m_StoreMultiTriggerSf);
            Signal->multipleTriggerSF(m_StoreMultiTriggerSf);
            if (m_writeBaselineSF) {
                bool append = false;
                for (auto& baseSf : Baseline_SFs) {
                    if (Baseline->append(baseSf, m_systematics->GetNominal())) append = true;
                }
                if (Baseline->setSignalTriggerSF(Baseline_TriggerSF, m_systematics->GetNominal())) append = true;
                if (append) {
                    if (Baseline->nWeights() > 0) ATH_CHECK(initIParticleWeight(*Baseline, "", set, ScaleFactorMapContains::BaselineSf));
                    m_SF.push_back(Baseline);
                }
            }
            // Signal SF
            bool append = false;
            for (auto& sigSf : Signal_SFs) {
                if (Signal->append(sigSf, m_systematics->GetNominal())) append = true;
            }
            if (Signal->setSignalTriggerSF(Signal_TriggerSF, m_systematics->GetNominal())) append = true;
            if (append) {
                if (Signal->nWeights() > 0) ATH_CHECK(initIParticleWeight(*Signal, "", set, ScaleFactorMapContains::SignalSf));
                m_SF.push_back(Signal);
            }
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTauSelector::initializeTauEfficiencySFTools(TauEffiToolMap& SFTools,
                                                               ToolHandle<TauAnalysisTools::ITauSelectionTool>& selectionTool) {
        const std::vector<TauAnalysisTools::EfficiencyCorrectionType> EffiTypes{
            // Reco-sf
            TauAnalysisTools::EfficiencyCorrectionType::SFRecoHadTau,
            // Id
            TauAnalysisTools::EfficiencyCorrectionType::SFJetIDHadTau,
            // EleORHad
            TauAnalysisTools::EfficiencyCorrectionType::SFEleOLRHadTau,
            // SFEleOLRElectron
            TauAnalysisTools::EfficiencyCorrectionType::SFEleOLRElectron};

        std::string TauWP = (&selectionTool == &m_SignalTauSelectionTool ? "signal" : "baseline");
        for (auto& effi : EffiTypes) {
            asg::AnaToolHandle<TauEffiTool> AnaTool("TauAnalysisTools::TauEfficiencyCorrectionsTool/" + TauWP + "TauEfficiencySFs_" +
                                                    to_string(effi));
            ATH_CHECK(AnaTool.setProperty("TauSelectionTool", selectionTool));
            ATH_CHECK(AnaTool.setProperty("EfficiencyCorrectionTypes", std::vector<int>{effi}));
            ATH_CHECK(AnaTool.retrieve());
            // Register the tool in the XAMPP systematic store
            ATH_CHECK(DeclareAsWeightSyst(AnaTool));
            if (SFTools.find(effi) != SFTools.end()) {
                ATH_MSG_ERROR("The tau sf tool " << to_string(effi) << " has already been created. Please check the list");
                return StatusCode::FAILURE;
            }
            SFTools[effi] = AnaTool.getHandle();
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTauSelector::initializeScaleFactors(TauEffiToolMap& SFTools, std::vector<TauWeightMap>& sf_types, unsigned int content) {
        for (auto Itr : SFTools) {
            sf_types.push_back(TauWeightMap());
            ATH_CHECK(initializeScaleFactors(to_string(Itr.first), Itr.second, sf_types.back(), content));
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTauSelector::initializeTriggerEfficiencySFTools(TauTriggEffiToolMap& SFTools,
                                                                   ToolHandle<TauAnalysisTools::ITauSelectionTool>& selectionTool) {
        std::string TauWP = (&selectionTool == &m_SignalTauSelectionTool ? "signal" : "baseline");
        unsigned int TauID = GetProperty<int>("JetIDWP", selectionTool);
        for (auto& Trigger : m_TriggerExp) {
            asg::AnaToolHandle<TauEffiTool> AnaTool("TauAnalysisTools::TauEfficiencyCorrectionsTool/" + TauWP + "TauTriggerEffi" + Trigger);
            ATH_CHECK(AnaTool.setProperty("TauSelectionTool", selectionTool));
            ATH_CHECK(AnaTool.setProperty("EfficiencyCorrectionTypes", std::vector<int>({TauAnalysisTools::SFTriggerHadTau})));
            ATH_CHECK(AnaTool.setProperty("TriggerName", Trigger));
            ATH_CHECK(AnaTool.setProperty("IDLevel", TauID));
            ATH_CHECK(AnaTool.setProperty("PileupReweightingTool", GetCPTool<CP::IPileupReweightingTool>("PileupReweightingTool")));
            ATH_CHECK(AnaTool.retrieve());
            ToolHandleSystematics<TauEffiTool>* SystHandle = new ToolHandleSystematics<TauEffiTool>(AnaTool);
            ATH_CHECK(SystHandle->initialize());
            if (SFTools.find(Trigger) != SFTools.end()) {
                ATH_MSG_ERROR("The trigger efficiency tool for " << Trigger << " has already been created. Please check the list");
                return StatusCode::FAILURE;
            }
            SFTools[Trigger] = AnaTool.getHandle();
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTauSelector::initializeTriggerScaleFactors(TauTriggEffiToolMap& SFTools, TauWeight_VectorMap& SF_Map,
                                                              unsigned int content) {
        for (auto& syst_set : m_systematics->GetWeightSystematics(ObjectType())) {
            TauWeight_Vector Trigger_SFs;
            for (auto& Trig_ToolPair : SFTools) {
                if (!XAMPP::ToolIsAffectedBySystematic(Trig_ToolPair.second, syst_set)) break;
                std::string sf_type = std::string("Trig") + (m_StoreMultiTriggerSf ? Trig_ToolPair.first : "");
                std::shared_ptr<TauTriggerSFHandler> TrigWeight =
                    std::make_shared<TauTriggerSFHandler>(Trig_ToolPair.first, m_trigger_tool, Trig_ToolPair.second);
                ATH_CHECK(initIParticleWeight(*TrigWeight, sf_type, syst_set, content, m_SeparateSF || m_StoreMultiTriggerSf));
                TrigWeight->requireMatching(m_RequireTrigMatchForSF);
                ATH_CHECK(TrigWeight->initialize());
                Trigger_SFs.push_back(TrigWeight);
            }
            if (!Trigger_SFs.empty()) {
                if (SF_Map.find(syst_set) != SF_Map.end()) {
                    ATH_MSG_ERROR("The Trigger SFs are already defined for systematic " << syst_set->name());
                    return StatusCode::FAILURE;
                }
                SF_Map[syst_set] = Trigger_SFs;
            }
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTauSelector::initializeScaleFactors(const std::string& sf_type, TauEffiToolHandle& sf_tool, TauWeightMap& map,
                                                       unsigned int content) {
        for (auto& syst_set : m_systematics->GetWeightSystematics(ObjectType())) {
            if (!XAMPP::ToolIsAffectedBySystematic(sf_tool, syst_set)) continue;
            TauWeight_Ptr Weight(new TauWeight(sf_tool));
            ATH_CHECK(initIParticleWeight(*Weight, sf_type, syst_set, content, m_SeparateSF));
            if (map.find(syst_set) != map.end()) {
                ATH_MSG_FATAL("There is already a sf stored in map for systematic " << syst_set->name());
                return StatusCode::FAILURE;
            }
            map.insert(std::pair<const CP::SystematicSet*, TauWeight_Ptr>(syst_set, Weight));
        }
        return StatusCode::SUCCESS;
    }

    StatusCode SUSYTauSelector::LoadContainers() {
        if (!ProcessObject()) { return StatusCode::SUCCESS; }
        for (auto& ScaleFactors : m_SF) { ATH_CHECK(ScaleFactors->initEvent()); }

        ATH_CHECK(LoadContainer(ContainerKey(), m_xAODTaus));

        return StatusCode::SUCCESS;
    }

    xAOD::TauJetContainer* SUSYTauSelector::GetCustomTaus(const std::string& kind) const {
        xAOD::TauJetContainer* customTaus = nullptr;
        if (LoadViewElementsContainer(kind, customTaus).isSuccess()) return customTaus;
        return GetBaselineTaus();
    }

    StatusCode SUSYTauSelector::CallSUSYTools() {
        ATH_MSG_DEBUG("Calling SUSYTauSelector::CallSUSYTools()..");
        ATH_CHECK(m_susytools->GetTaus(m_Taus, m_TausAux, false));
        for (auto tau : *m_Taus) {
            m_tauDecorations->passBaselineID.set(tau, m_BaseTauSelectionTool->accept(*tau));
            m_tauDecorations->passSignalID.set(tau, m_SignalTauSelectionTool->accept(*tau));

            char passBase(false), passSig(false);
            m_tauDecorations->passBaselineID.get(tau, passBase);
            m_tauDecorations->passSignalID.get(tau, passSig);

            SetPreSelectionDecorator(*tau, m_ignoreBaseTauIDTool || passBase);
            SetSignalDecorator(*tau, passSig);
        }
        return StatusCode::SUCCESS;
    }

    StatusCode SUSYTauSelector::InitialFill(const CP::SystematicSet& systset) {
        SetSystematics(systset);
        if (ProcessObject())
            ATH_CHECK(FillFromSUSYTools(m_Taus, m_TausAux, m_PreTaus));
        else {
            ATH_CHECK(ViewElementsContainer("cont", m_Taus));
            ATH_CHECK(ViewElementsContainer("presel", m_PreTaus));
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTauSelector::FillTaus(const CP::SystematicSet& systset) {
        SetSystematics(systset);
        ATH_CHECK(ViewElementsContainer("baseline", m_BaselineTaus));
        ATH_CHECK(ViewElementsContainer("signal", m_SignalTaus));
        ATH_CHECK(ViewElementsContainer("GQobj", m_SignalQualTaus));

        for (auto itau : *GetPreTaus()) {
            m_tauDecorations->numTracks.set(*itau, itau->nTracks());
            if (m_StoreTruthClassifier) ATH_CHECK(StoreTruthClassifer(*itau));
            if (PassBaseline(*itau)) m_BaselineTaus->push_back(itau);
            if (PassSignalNoOR(*itau)) m_SignalQualTaus->push_back(itau);
            if (PassSignal(*itau)) m_SignalTaus->push_back(itau);
        }

        ATH_MSG_DEBUG("Number of all taus: " << m_Taus->size());
        ATH_MSG_DEBUG("Number of preselected taus: " << GetPreTaus()->size());
        ATH_MSG_DEBUG("Number of selected baseline taus: " << m_BaselineTaus->size());
        ATH_MSG_DEBUG("Number of selected signal taus: " << m_SignalTaus->size());
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTauSelector::SaveScaleFactor() {
        if (m_SF.empty()) return StatusCode::SUCCESS;
        const CP::SystematicSet* kineSet = m_XAMPPInfo->GetSystematic();
        xAOD::TauJetContainer* TausForSF = m_writeBaselineSF ? GetBaselineTaus() : GetSignalTaus();
        ATH_MSG_DEBUG("Save SF of " << TausForSF->size() << " taus");
        for (auto ScaleFactor : m_SF) {
            if (kineSet != m_systematics->GetNominal() && ScaleFactor->systematic() != m_systematics->GetNominal()) continue;
            ATH_CHECK(m_systematics->setSystematic(ScaleFactor->systematic()));
            for (auto tau : *TausForSF) {
                bool is_signal = !m_writeBaselineSF || PassSignal(*tau);
                ATH_CHECK(ScaleFactor->saveSF(*tau, is_signal));
            }
            ATH_CHECK(ScaleFactor->applySF());
        }
        return StatusCode::SUCCESS;
    }
    void SUSYTauSelector::setupDecorations(std::shared_ptr<TauDecorations> input) {
        // as for the particle selector, use the input if provided, or use a default
        // if not set before.
        if (input) { m_tauDecorations = input; }
        if (!m_tauDecorations) { m_tauDecorations = std::make_shared<TauDecorations>(); }
        ParticleSelector::setupDecorations(m_tauDecorations);
    }

    StatusCode SUSYTauSelector::StoreTruthClassifer(xAOD::TauJet& tau) {
        if (isData()) {
            ATH_MSG_DEBUG("Skip truth classification as we're running on data");
            return StatusCode::SUCCESS;
        }
        m_TruthMatching->applyTruthMatch(tau);
        const xAOD::Jet* Jet = xAOD::TauHelpers::getLink<xAOD::Jet>(&tau, "truthJetLink");
        int PartonID = -1;
        int ConeID = -1;
        if (Jet) {
            m_tauDecorations->partonTruthLabelID.get(*Jet, PartonID);
            m_tauDecorations->coneTruthLabelID.get(*Jet, ConeID);
        }
        m_tauDecorations->partonTruthLabelID.set(tau, PartonID);
        m_tauDecorations->coneTruthLabelID.set(tau, ConeID);

        // Then do the TruthClassification
        const xAOD::TruthParticle* TruthTau = m_TruthMatching->getTruth(tau);

        m_tauDecorations->tauTruthType.set(tau, m_TruthMatching->getTruthParticleType(tau));
        bool ClassificationFromTruth = false;
        if (TruthTau) {
            ATH_MSG_DEBUG("Found the truth tau");
            m_tauDecorations->truthType.set(tau, getParticleTruthType(TruthTau));
            m_tauDecorations->truthOrigin.set(tau, getParticleTruthOrigin(TruthTau));
            ClassificationFromTruth = true;
        }
        auto tau_trk = tau.track(0);

        if (!tau_trk) {
            ATH_MSG_ERROR("No track found for the tau");
            return StatusCode::FAILURE;
        }
        // If there is no TruthClassification available obtain the origin from
        // the associated truthParticle

        if (ClassificationFromTruth) return StatusCode::SUCCESS;
        m_tauDecorations->truthType.set(tau, getParticleTruthType(tau_trk));
        m_tauDecorations->truthOrigin.set(tau, getParticleTruthOrigin(tau_trk));
        ATH_MSG_DEBUG("Truth classification done");

        return StatusCode::SUCCESS;
    }

    //##########################################################################
    //                            TauWeightDecorator
    //##########################################################################
    TauWeightDecorator::TauWeightDecorator() : IPartilceWeightDecorator() {}
    TauWeightDecorator::~TauWeightDecorator() {}
    StatusCode TauWeightDecorator::saveSF(const xAOD::TauJet& Tau, bool isSignal) {
        double SF = 1.;
        if (!isSFcalculated(Tau)) {
            if (!calculateSF(Tau, SF).isSuccess()) return StatusCode::FAILURE;
        } else
            SF = getSF(Tau);
        return saveEventSF(Tau, SF, isSignal);
    }
    StatusCode TauWeightDecorator::initialize() { return StatusCode::SUCCESS; }
    //##########################################################################
    //                            TauWeight
    //##########################################################################
    TauWeight::TauWeight(TauEffiToolHandle& SFTool) : TauWeightDecorator(), m_SFTool(SFTool) {}
    TauWeight::~TauWeight() {}
    StatusCode TauWeight::calculateSF(const xAOD::TauJet& Tau, double& SF) {
        if (m_SFTool->getEfficiencyScaleFactor(Tau, SF) == CP::CorrectionCode::Error) return StatusCode::FAILURE;
        return StatusCode::SUCCESS;
    }
    //##########################################################################
    //                            TauWeightHandler
    //##########################################################################
    TauWeightHandler::TauWeightHandler(const CP::SystematicSet* syst_set) :
        TauWeightDecorator(),
        m_Syst(syst_set),
        m_Weights(),
        m_init(false),
        m_signal_trig_SF(),
        m_multiple_trig_sf(false) {}
    StatusCode TauWeightHandler::applySF() {
        if (!m_init) {
            Error("TauWeightHandler()", "What about destiny? I've none");
            return StatusCode::FAILURE;
        }
        for (auto& W : m_Weights) {
            if (!W->applySF().isSuccess()) return StatusCode::FAILURE;
        }
        for (auto& W : m_signal_trig_SF) {
            if (!W->applySF().isSuccess()) return StatusCode::FAILURE;
        }
        return TauWeightDecorator::applySF();
    }
    void TauWeightHandler::multipleTriggerSF(bool B) { m_multiple_trig_sf = B; }
    const CP::SystematicSet* TauWeightHandler::systematic() const { return m_Syst; }
    size_t TauWeightHandler::nWeights() const { return m_Weights.size(); }
    StatusCode TauWeightHandler::calculateSF(const xAOD::TauJet& Tau, double& SF) {
        for (auto& W : m_Weights) { SF *= W->getSF(Tau); }
        return StatusCode::SUCCESS;
    }
    StatusCode TauWeightHandler::saveSF(const xAOD::TauJet& Tau, bool isSignal) {
        for (auto& W : m_Weights) {
            if (!W->saveSF(Tau, isSignal).isSuccess()) return StatusCode::FAILURE;
        }
        for (auto& W : m_signal_trig_SF) {
            if (!W->saveSF(Tau, isSignal).isSuccess()) return StatusCode::FAILURE;
        }
        // Weighters are emptied for multiple trigger sf
        // on systematics
        if (m_Weights.empty()) return StatusCode::SUCCESS;
        return TauWeightDecorator::saveSF(Tau, isSignal);
    }
    bool TauWeightHandler::append(const TauWeightMap& map, const CP::SystematicSet* nominal) {
        TauWeightMap::const_iterator Itr = map.find(systematic());
        if (Itr != map.end()) {
            if (!IsInVector(Itr->second, m_Weights)) m_Weights.push_back(Itr->second);
            m_init = true;
        } else {
            Itr = map.find(nominal);
            if (Itr != map.end() && !IsInVector(Itr->second, m_Weights)) m_Weights.push_back(Itr->second);
        }
        return m_init;
        ;
    }
    bool TauWeightHandler::setSignalTriggerSF(const TauWeight_VectorMap& map, const CP::SystematicSet* nominal) {
        TauWeight_VectorMap::const_iterator Itr = map.find(systematic());
        if (Itr != map.end()) {
            // Trigger_sf systematic
            if (systematic() != nominal && m_multiple_trig_sf) m_Weights.clear();
            if (m_multiple_trig_sf)
                CopyVector(Itr->second, m_signal_trig_SF, false);
            else
                CopyVector(Itr->second, m_Weights, false);
            m_init = true;
        } else {
            Itr = map.find(nominal);
            if (Itr != map.end()) {
                if (m_multiple_trig_sf)
                    CopyVector(Itr->second, m_signal_trig_SF, false);
                else
                    CopyVector(Itr->second, m_Weights, false);
            }
        }
        return m_init;
    }
    //#################################################################################################
    //                                  TauTriggerSFHandler
    //#################################################################################################
    TauTriggerSFHandler::TauTriggerSFHandler(const std::string& Trigger, const ToolHandle<ITriggerTool>& trigger_tool,
                                             TauEffiToolHandle& TriggerSF) :
        m_TrigStr(Trigger),
        m_TriggerSFTool(TriggerSF),
        m_trigger_tool(trigger_tool),
        m_Triggers(),
        m_requireMatching(false) {}
    bool TauTriggerSFHandler::RetrieveMatchers(const std::string& Trigger) {
        std::shared_ptr<TriggerInterface> iface = m_trigger_tool->GetActiveTrigger(Trigger);
        if (!iface) return false;
        m_Triggers.push_back(iface);

        return true;
    }
    StatusCode TauTriggerSFHandler::initialize() {
        // Remove the HLT_ in front of the trigger name because SUSYTools is
        // adding it anyway
        std::string trig_temp_str = m_TrigStr.substr(std::string("HLT_").size());
        std::vector<std::string> SingleT = m_trigger_tool->GetTriggerOR(trig_temp_str);
        for (auto& T : SingleT) {
            if (!RetrieveMatchers(T))
                Warning("TauTriggerSFHandler()", "Could not find the trigger %s, but you want to apply a SF on it", T.c_str());
        }
        if (m_Triggers.empty()) {
            Error("TauTriggerSFHandler()", "None of the triggers you gave is part of the trigger selection");
            return StatusCode::FAILURE;
        }
        return StatusCode::SUCCESS;
    }
    StatusCode TauTriggerSFHandler::calculateSF(const xAOD::TauJet& Tau, double& SF) {
        // Check if the trigger has fired at all and if it is already matched
        if (!m_requireMatching || IsTriggerMachted(Tau)) {
            if (m_TriggerSFTool->getEfficiencyScaleFactor(Tau, SF) == CP::CorrectionCode::Error) { return StatusCode::FAILURE; }
        }
        return StatusCode::SUCCESS;
    }
    TauTriggerSFHandler::~TauTriggerSFHandler() {}
    bool TauTriggerSFHandler::IsTriggerMachted(const xAOD::TauJet& Tau) const {
        for (auto& T : m_Triggers) {
            if (!T->PassTrigger()) continue;
            if (T->isMatched(Tau)) return true;
        }
        return false;
    }
    void TauTriggerSFHandler::requireMatching(bool B) { m_requireMatching = B; }
}  // namespace XAMPP
