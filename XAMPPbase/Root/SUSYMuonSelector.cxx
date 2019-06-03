#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/Cuts.h>
#include <XAMPPbase/SUSYMuonSelector.h>

#include <IsolationSelection/IIsolationSelectionTool.h>
#include <MuonAnalysisInterfaces/IMuonEfficiencyScaleFactors.h>
#include <MuonAnalysisInterfaces/IMuonTriggerScaleFactors.h>
#include <PATInterfaces/SystematicSet.h>
#include <SUSYTools/SUSYObjDef_xAOD.h>

namespace XAMPP {
    const float TTVA_SF_MIN_PT = 1.e4;

    //######################################################
    //						SUSYMuonSelector
    //######################################################
    SUSYMuonSelector::SUSYMuonSelector(const std::string& myname) :
        SUSYParticleSelector(myname),
        m_PreSelectionD0SigCut(-1.),
        m_PreSelectionZ0SinThetaCut(-1.),
        m_RequireIsoPreSelection(false),
        m_BaselineD0SigCut(-1.),
        m_BaselineZ0SinThetaCut(-1.),
        m_RequireIsoBaseline(false),
        m_SignalD0SigCut(-1),
        m_SignalZ0SinThetaCut(-1),
        m_RequireIsoSignal(true),
        m_StoreTruthClassifier(false),
        m_xAODMuons(nullptr),
        m_Muons(nullptr),
        m_MuonsAux(nullptr),
        m_PreMuons(nullptr),
        m_SignalMuons(nullptr),
        m_BaselineMuons(nullptr),
        m_SignalQualMuons(nullptr),
        m_muonDecorations(nullptr),
        m_SeparateSF(false),
        m_SF(),
        m_doRecoSF(true),
        m_doIsoSF(true),
        m_doTTVASF(true),
        m_doTriggerSF(true),
        m_TriggerExp2015(),
        m_TriggerExp2016(),
        m_TriggerExp2017(),
        m_TriggerExp2018(),
        m_StoreMultipleTrigSF(false),
        m_writeBaselineSF(false),
        m_SFtool_Reco(""),
        m_SFtool_Iso(""),
        m_SFtool_TTVA(""),
        m_SFtool_BaseReco(""),

        m_SFtool_Trig(""),
        m_SFtool_BasTrig(""),
        m_SFtoolName_Trig("MuonTriggerScaleFactorsTool"),
        m_NumBadMuons(nullptr),
        m_NumCosmics(nullptr),
        m_Baseline_Muon_Id(-1),
        m_Signal_Muon_Id(-1),
        m_quality_Cosmic(),
        m_quality_BadMuon(),
        m_iso_tool(""),
        m_force_iso_calc(false) {
        SetContainerKey("Muons");
        SetObjectType(XAMPP::SelectionObject::Muon);

        declareProperty("PreSelectionIP_d0Cut", m_PreSelectionD0SigCut);
        declareProperty("PreSelectionIP_Z0Cut", m_PreSelectionZ0SinThetaCut);
        declareProperty("RequireIsoPreSelection", m_RequireIsoPreSelection);

        declareProperty("BaslineIP_d0Cut", m_BaselineD0SigCut);
        declareProperty("BaslineIP_Z0Cut", m_BaselineZ0SinThetaCut);
        declareProperty("RequireIsoBaseline", m_RequireIsoBaseline);

        declareProperty("SignalIP_d0Cut", m_SignalD0SigCut);
        declareProperty("SignalIP_Z0Cut", m_SignalZ0SinThetaCut);
        declareProperty("RequireIsoSignal", m_RequireIsoSignal);

        declareProperty("StoreTruthClassifier", m_StoreTruthClassifier);

        declareProperty("SeparateSF", m_SeparateSF);
        declareProperty("ApplyIsoSF", m_doIsoSF);
        declareProperty("ApplyTTVASF", m_doTTVASF);
        declareProperty("ApplyRecoSF", m_doRecoSF);
        declareProperty("ApplyTriggerSF", m_doTriggerSF);
        declareProperty("WriteBaselineSF", m_writeBaselineSF);

        declareProperty("SFTrigger2015", m_TriggerExp2015);
        declareProperty("SFTrigger2016", m_TriggerExp2016);
        declareProperty("SFTrigger2017", m_TriggerExp2017);
        declareProperty("SFTrigger2018", m_TriggerExp2018);

        declareProperty("Reco_SFTool", m_SFtool_Reco);
        declareProperty("Iso_SFTool", m_SFtool_Iso);
        declareProperty("TTVA_SFTool", m_SFtool_TTVA);

        declareProperty("BaselineReco_SFTool", m_SFtool_BaseReco);
        //        declareProperty("BaselineIsol_SFTool", m_SFtool_BaseIso);

        declareProperty("Trig_SFTool", m_SFtool_Trig);
        declareProperty("BaselineTrig_SFTool", m_SFtool_BasTrig);
        declareProperty("ExcludeTrigSFfromTotalWeight", m_StoreMultipleTrigSF);

        declareProperty("Trig_SFToolName", m_SFtoolName_Trig);

        declareProperty("BaselineRecoWP", m_Baseline_Muon_Id);
        declareProperty("SignalRecoWP", m_Signal_Muon_Id);
        // Optionally if the user changes the definition of the
        // Muon baseline selection accessor i.e. passOR -> passFancyOR
        // but wants the BadMuon and Cosmic selection to be applied w.r.t
        // the old definitions.
        declareProperty("CosmicSelectionDecorator", m_quality_Cosmic);
        declareProperty("BadMuonSelectionDecorator", m_quality_BadMuon);

        // SUSYTools disabled the calculation of the isolation wp if it's not
        // part of the signal selection
        declareProperty("IsolationTool", m_iso_tool);
        declareProperty("RecalcIsolation", m_force_iso_calc);
    }

    SUSYMuonSelector::~SUSYMuonSelector() {}
    StatusCode SUSYMuonSelector::LoadSelection(const CP::SystematicSet& systset) {
        ATH_CHECK(LoadPreSelectedContainer(m_PreMuons, &systset));
        SetSystematics(systset);
        ATH_CHECK(LoadViewElementsContainer("baseline", m_BaselineMuons));
        ATH_CHECK(LoadViewElementsContainer("signal", m_SignalMuons));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMuonSelector::initTriggerSFDecorators(const std::vector<std::string>& Triggers,
                                                         std::vector<SUSYMuonTriggerSFHandler_Ptr>& Decorators,
                                                         const CP::SystematicSet* set, unsigned int year, bool isBaseline) {
        // XAMPP::ToolIsAffectedBySystematic(m_SFtool_Trig, set);
        for (const auto& T : Triggers) {
            std::string sf_name = "Trig" + (m_StoreMultipleTrigSF ? T : "");
            SUSYMuonTriggerSFHandler_Ptr Helper =
                SUSYMuonTriggerSFHandler_Ptr(new XAMPP::SUSYMuonTriggerSFHandler(m_XAMPPInfo, m_susytools.getHandle(), T, year));
            Decorators.push_back(Helper);
            XAMPP::Storage<double>* Base = nullptr;
            XAMPP::Storage<double>* Signal = nullptr;
            if (isBaseline)
                ATH_CHECK(initEventBaselineSf(Base, sf_name, set, m_SeparateSF || m_StoreMultipleTrigSF));
            else
                ATH_CHECK(initEventSignalSf(Signal, sf_name, set, m_SeparateSF || m_StoreMultipleTrigSF));
            Helper->setEventSfStores(Base, Signal);
            ATH_CHECK(Helper->initialize());
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMuonSelector::initializeTrigSFMap(ToolHandle<CP::IMuonTriggerScaleFactors>& sf_tool, SUSYMuonTriggerSFHandler_Map& map,
                                                     bool isBaseline) {
        for (const auto syst_set : m_systematics->GetWeightSystematics(ObjectType())) {
            if (!XAMPP::ToolIsAffectedBySystematic(sf_tool, syst_set)) continue;
            map.insert(std::pair<const CP::SystematicSet*, SUSYMuonTriggerSFHandler_Vector>(syst_set, SUSYMuonTriggerSFHandler_Vector()));
            SUSYMuonTriggerSFHandler_Map::iterator Itr = map.find(syst_set);
            ATH_CHECK(initTriggerSFDecorators(m_TriggerExp2015, Itr->second, syst_set, 2015, isBaseline));
            ATH_CHECK(initTriggerSFDecorators(m_TriggerExp2016, Itr->second, syst_set, 2016, isBaseline));
            ATH_CHECK(initTriggerSFDecorators(m_TriggerExp2017, Itr->second, syst_set, 2017, isBaseline));
            ATH_CHECK(initTriggerSFDecorators(m_TriggerExp2018, Itr->second, syst_set, 2018, isBaseline));
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMuonSelector::SetupSelection() {
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<int>("BadMuon", false));
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<int>("CosmicMuon", false));
        m_NumBadMuons = m_XAMPPInfo->GetVariableStorage<int>("BadMuon");
        m_NumCosmics = m_XAMPPInfo->GetVariableStorage<int>("CosmicMuon");
        m_StoreTruthClassifier = m_StoreTruthClassifier && !isData();
        if (m_force_iso_calc) ATH_CHECK(m_iso_tool.retrieve());
        if (!m_quality_Cosmic.empty()) {
            ATH_MSG_INFO("The user changed the selection stage of the muons to be applied on the cosmic veto to " << m_quality_Cosmic);
        }
        if (!m_quality_BadMuon.empty()) {
            ATH_MSG_INFO("The user changed the selection stage of the muons to be applied on the bad muon veto to " << m_quality_BadMuon);
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMuonSelector::initialize() {
        if (isInitialized()) { return StatusCode::SUCCESS; }
        ATH_CHECK(SUSYParticleSelector::initialize());
        setupDecorations();
        if (m_force_iso_calc && m_iso_tool.empty()) { m_iso_tool = GetCPTool<CP::IIsolationSelectionTool>("IsolationSelectionTool"); }
        ATH_CHECK(SetupSelection());
        // Always create these decorators since they're neccessary for the standard event cutflow
        if (isData() || !ProcessObject()) return StatusCode::SUCCESS;

        if ((m_TriggerExp2015.empty() || m_TriggerExp2016.empty()) && m_doTriggerSF) {
            ATH_MSG_INFO("No trigger string given... Switch trigger SF off");
            m_TriggerExp2015.clear();
            m_TriggerExp2016.clear();
            m_TriggerExp2017.clear();
            m_doTriggerSF = false;
        }
        if (m_doRecoSF && !m_SFtool_Reco.isSet()) {
            m_SFtool_Reco = GetCPTool<CP::IMuonEfficiencyScaleFactors>("MuonEfficiencyScaleFactorsTool");
        }
        if (m_doIsoSF && !m_SFtool_Iso.isSet()) {
            m_SFtool_Iso = GetCPTool<CP::IMuonEfficiencyScaleFactors>("MuonIsolationScaleFactorsTool");
        }
        if (m_doTTVASF && !m_SFtool_TTVA.isSet()) {
            m_SFtool_TTVA = GetCPTool<CP::IMuonEfficiencyScaleFactors>("MuonTTVAEfficiencyScaleFactorsTool");
        }
        if (m_doTriggerSF && !m_SFtool_Trig.isSet()) {
            m_SFtool_Trig = GetCPTool<CP::IMuonTriggerScaleFactors>(m_SFtoolName_Trig);
            // If we switch the WP we need another trigger SF tool
            m_SFtool_BasTrig = m_SFtool_Trig;
            ATH_CHECK(m_SFtool_Trig.retrieve());
        }
        ATH_CHECK(SetupScaleFactors());
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMuonSelector::SetupScaleFactors() {
        if (isData() || !ProcessObject()) return StatusCode::SUCCESS;
        std::vector<std::string> all_triggers;
        CopyVector(m_TriggerExp2015, all_triggers, false);
        CopyVector(m_TriggerExp2016, all_triggers, false);
        CopyVector(m_TriggerExp2017, all_triggers, false);
        CopyVector(m_TriggerExp2018, all_triggers, false);

        m_StoreMultipleTrigSF = m_StoreMultipleTrigSF || all_triggers.size() > 1;

        MuonWeightMap Signal_RecoMap;
        MuonWeightMap Baseline_RecoMap;

        MuonWeightMap IsoMap;
        MuonWeightMap TTVA_Map;

        SUSYMuonTriggerSFHandler_Map Signal_TriggerSF;
        SUSYMuonTriggerSFHandler_Map Baseline_TriggerSF;

        if (!isData() && m_systematics->GetWeightSystematics(ObjectType()).empty()) {
            ATH_MSG_INFO("Declare the weight systematics");
            if (m_doRecoSF) ATH_CHECK(DeclareAsWeightSyst(m_SFtool_Reco));
            if (m_doIsoSF) ATH_CHECK(DeclareAsWeightSyst(m_SFtool_Iso));
            if (m_doTTVASF) ATH_CHECK(DeclareAsWeightSyst(m_SFtool_TTVA));
        }
        if (m_doRecoSF) {
            ATH_CHECK(m_SFtool_Reco.retrieve());
            if (!m_writeBaselineSF || m_Baseline_Muon_Id != m_Signal_Muon_Id) {
                if (m_writeBaselineSF)
                    ATH_CHECK(initializeSfMap("Reco", m_SFtool_BaseReco, Baseline_RecoMap, ScaleFactorMapContains::BaselineSf));
                ATH_CHECK(initializeSfMap("Reco", m_SFtool_Reco, Signal_RecoMap, ScaleFactorMapContains::SignalSf));
            } else if (m_writeBaselineSF) {
                ATH_CHECK(initializeSfMap("Reco", m_SFtool_Reco, Signal_RecoMap, ScaleFactorMapContains::SignalAndBaseSf));
                Baseline_RecoMap = Signal_RecoMap;
            }
        }
        if (m_doIsoSF) {
            ATH_CHECK(m_SFtool_Iso.retrieve());
            ATH_CHECK(initializeSfMap("Isol", m_SFtool_Iso, IsoMap,
                                      m_writeBaselineSF ? ScaleFactorMapContains::SignalAndBaseSf : ScaleFactorMapContains::SignalSf));
        }
        if (m_doTTVASF) {
            ATH_CHECK(m_SFtool_TTVA.retrieve());
            ATH_CHECK(initializeSfMap("TTVA", m_SFtool_TTVA, TTVA_Map,
                                      m_writeBaselineSF ? ScaleFactorMapContains::SignalAndBaseSf : ScaleFactorMapContains::SignalSf));
        }
        if (m_doTriggerSF) {
            ATH_CHECK(initializeTrigSFMap(m_SFtool_Trig, Signal_TriggerSF, false));
            if (m_writeBaselineSF) ATH_CHECK(initializeTrigSFMap(m_SFtool_BasTrig, Baseline_TriggerSF, true));
        }
        for (const auto set : m_systematics->GetWeightSystematics(ObjectType())) {
            MuonWeightHandler_Ptr Signal_SF = std::make_shared<MuonWeightHandler>(set);
            MuonWeightHandler_Ptr Baseline_SF = std::make_shared<MuonWeightHandler>(set);
            Signal_SF->multipleTriggerSF(m_StoreMultipleTrigSF);
            Baseline_SF->multipleTriggerSF(m_StoreMultipleTrigSF);

            bool append_Signal = false;
            bool append_Baseline = false;

            if (m_writeBaselineSF && m_Baseline_Muon_Id != m_Signal_Muon_Id) {
                append_Baseline = Baseline_SF->append(Baseline_RecoMap, m_systematics->GetNominal());
                append_Baseline = Baseline_SF->append(IsoMap, m_systematics->GetNominal()) || append_Baseline;
                append_Baseline = Baseline_SF->append(TTVA_Map, m_systematics->GetNominal()) || append_Baseline;
                append_Baseline = Baseline_SF->setBaseTriggerSF(Baseline_TriggerSF, m_systematics->GetNominal()) || append_Baseline;
            } else if (m_writeBaselineSF)
                append_Signal = Signal_SF->setBaseTriggerSF(Baseline_TriggerSF, m_systematics->GetNominal());

            append_Signal = Signal_SF->append(Signal_RecoMap, m_systematics->GetNominal()) || append_Signal;
            append_Signal = Signal_SF->append(IsoMap, m_systematics->GetNominal()) || append_Signal;
            append_Signal = Signal_SF->append(TTVA_Map, m_systematics->GetNominal()) || append_Signal;
            append_Signal = Signal_SF->setSignalTriggerSF(Signal_TriggerSF, m_systematics->GetNominal()) || append_Signal;

            if (append_Baseline) {
                if (Baseline_SF->nWeights() > 0) ATH_CHECK(initIParticleWeight(*Baseline_SF, "", set, ScaleFactorMapContains::BaselineSf));
                m_SF.push_back(Baseline_SF);
            }
            if (append_Signal) {
                unsigned int sf_content = ScaleFactorMapContains::SignalSf;
                if (m_writeBaselineSF && m_Baseline_Muon_Id == m_Signal_Muon_Id) sf_content = ScaleFactorMapContains::SignalAndBaseSf;
                if (Signal_SF->nWeights() > 0) ATH_CHECK(initIParticleWeight(*Signal_SF, "", set, sf_content));
                m_SF.push_back(Signal_SF);
            }
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMuonSelector::initializeSfMap(const std::string& sf_type, ToolHandle<CP::IMuonEfficiencyScaleFactors>& sf_tool,
                                                 MuonWeightMap& map, unsigned int content) {
        for (auto& syst_set : m_systematics->GetWeightSystematics(ObjectType())) {
            if (!XAMPP::ToolIsAffectedBySystematic(sf_tool, syst_set)) continue;
            MuonWeight_Ptr Weight(new MuonWeight(sf_tool, m_XAMPPInfo));
            if (sf_type == "TTVA") { Weight->setValidityRangeAbsEta(-1, 2.5); }
            ATH_CHECK(initIParticleWeight(*Weight, sf_type, syst_set, content, m_SeparateSF));
            if (map.find(syst_set) != map.end()) {
                ATH_MSG_FATAL("There is already a sf stored in map for systematic " << syst_set->name());
                return StatusCode::FAILURE;
            }
            map.insert(std::pair<const CP::SystematicSet*, MuonWeight_Ptr>(syst_set, Weight));
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMuonSelector::LoadContainers() {
        if (!ProcessObject()) return StatusCode::SUCCESS;
        ATH_CHECK(LoadContainer(ContainerKey(), m_xAODMuons));
        for (auto& ScaleFactors : m_SF) { ATH_CHECK(ScaleFactors->initEvent()); }
        return StatusCode::SUCCESS;
    }

    xAOD::MuonContainer* SUSYMuonSelector::GetCustomMuons(const std::string& kind) const {
        xAOD::MuonContainer* customMuons = nullptr;
        if (LoadViewElementsContainer(kind, customMuons).isSuccess()) return customMuons;
        return m_BaselineMuons;
    }

    MuoLink SUSYMuonSelector::GetLink(const xAOD::Muon& mu) const { return MuoLink(*GetMuons(), mu.index()); }
    MuoLink SUSYMuonSelector::GetOrigLink(const xAOD::Muon& mu) const {
        const xAOD::Muon* Omu = dynamic_cast<const xAOD::Muon*>(xAOD::getOriginalObject(mu));
        return MuoLink(*m_xAODMuons, Omu != nullptr ? Omu->index() : mu.index());
    }
    StatusCode SUSYMuonSelector::CallSUSYTools() {
        ATH_MSG_DEBUG("Calling SUSYMuonSelector::CallSUSYTools()..");
        return m_susytools->GetMuons(m_Muons, m_MuonsAux, false);
    }
    StatusCode SUSYMuonSelector::InitialFill(const CP::SystematicSet& systset) {
        SetSystematics(systset);
        if (ProcessObject()) {
            ATH_CHECK(FillFromSUSYTools(m_Muons, m_MuonsAux, m_PreMuons));
        } else {
            ATH_CHECK(ViewElementsContainer("container", m_Muons));
            ATH_CHECK(ViewElementsContainer("presel", m_PreMuons));
        }
        return StatusCode::SUCCESS;
    }

    StatusCode SUSYMuonSelector::FillMuons(const CP::SystematicSet& systset) {
        SetSystematics(systset);
        ATH_CHECK(ViewElementsContainer("baseline", m_BaselineMuons));
        ATH_CHECK(ViewElementsContainer("signal", m_SignalMuons));
        ATH_CHECK(ViewElementsContainer("GQquality", m_SignalQualMuons));

        int BadMuons = 0;
        int CosmicMuons = 0;

        for (auto imuon : *m_PreMuons) {
            if (m_StoreTruthClassifier) ATH_CHECK(StoreTruthClassifer(*imuon));
            if (m_force_iso_calc) { m_muonDecorations->passIsolation.set(*imuon, m_iso_tool->accept(*imuon)); }

            if (IsBadMuon(*imuon)) ++BadMuons;
            if (IsCosmicMuon(*imuon)) ++CosmicMuons;
            if (PassBaseline(*imuon)) GetBaselineMuons()->push_back(imuon);
            if (PassSignalNoOR(imuon) && !m_muonDecorations->isBadMuon(imuon) && !m_muonDecorations->isCosmicMuon(imuon)) {
                GetSignalNoORMuons()->push_back(imuon);
            }
            if (PassSignal(*imuon)) GetSignalMuons()->push_back(imuon);
        }

        ATH_CHECK(m_NumBadMuons->Store(BadMuons));
        ATH_CHECK(m_NumCosmics->Store(CosmicMuons));

        ATH_MSG_DEBUG("Number of all muons: " << GetMuons()->size());
        ATH_MSG_DEBUG("Number of preselected muons: " << GetPreMuons()->size());
        ATH_MSG_DEBUG("Number of selected baseline muons: " << GetBaselineMuons()->size());
        ATH_MSG_DEBUG("Number of selected signal muons: " << GetSignalMuons()->size());

        return StatusCode::SUCCESS;
    }

    bool SUSYMuonSelector::PassPreSelection(const xAOD::IParticle& P) const {
        if (!ParticleSelector::PassPreSelection(P)) return false;
        float aux = 0;
        if (!m_muonDecorations->z0sinTheta.get(P, aux)) {
            ATH_MSG_ERROR("Failed to read z0 * sin(theta) decoration");
            return false;
        }
        if (m_PreSelectionZ0SinThetaCut >= 0. && fabs(aux) >= m_PreSelectionZ0SinThetaCut) return false;
        aux = 0;
        if (!m_muonDecorations->d0sig.get(P, aux)) {
            ATH_MSG_ERROR("Failed to read d0 significance decoration");
            return false;
        }
        if (m_PreSelectionD0SigCut >= 0. && fabs(aux) >= m_PreSelectionD0SigCut) return false;
        if (m_RequireIsoPreSelection && !PassIsolation(P)) return false;
        return true;
    }

    bool SUSYMuonSelector::PassBaseline(const xAOD::IParticle& P) const {
        if (!ParticleSelector::PassBaseline(P)) return false;
        float aux = 0;
        if (!m_muonDecorations->z0sinTheta.get(P, aux)) {
            ATH_MSG_ERROR("Failed to read z0 * sin(theta) decoration");
            return false;
        }
        if (m_BaselineZ0SinThetaCut >= 0. && fabs(aux) >= m_BaselineZ0SinThetaCut) return false;
        aux = 0;
        if (!m_muonDecorations->d0sig.get(P, aux)) {
            ATH_MSG_ERROR("Failed to read d0 significance decoration");
            return false;
        }
        if (m_BaselineD0SigCut >= 0. && fabs(aux) >= m_BaselineD0SigCut) return false;
        if (m_RequireIsoBaseline && !PassIsolation(P)) return false;
        return true;
    }
    bool SUSYMuonSelector::GetSignalDecorator(const xAOD::IParticle& P) const {
        if (!ParticleSelector::GetSignalDecorator(P)) return false;
        float aux = 0;
        if (!m_muonDecorations->z0sinTheta.get(P, aux)) {
            ATH_MSG_ERROR("Failed to read z0 * sin(theta) decoration");
            return false;
        }
        if (m_SignalZ0SinThetaCut >= 0. && fabs(aux) >= m_SignalZ0SinThetaCut) return false;

        aux = 0;
        if (!m_muonDecorations->d0sig.get(P, aux)) {
            ATH_MSG_ERROR("Failed to read d0 significance decoration");
            return false;
        }
        if (m_SignalD0SigCut >= 0. && fabs(aux) >= m_SignalD0SigCut) return false;
        if (m_RequireIsoSignal && !PassIsolation(P)) return false;
        return true;
    }
    StatusCode SUSYMuonSelector::SaveScaleFactor() {
        if (m_SF.empty()) return StatusCode::SUCCESS;
        const CP::SystematicSet* kineSet = m_systematics->GetCurrent();
        xAOD::MuonContainer* SFMuons = m_writeBaselineSF ? GetBaselineMuons() : GetSignalMuons();
        ATH_MSG_DEBUG("Save SF of " << SFMuons->size() << " " << (m_writeBaselineSF ? "baseline" : "signal") << " muons");
        for (auto const& ScaleFactors : m_SF) {
            if (kineSet != m_systematics->GetNominal() && ScaleFactors->systematic() != m_systematics->GetNominal()) continue;
            ATH_CHECK(m_systematics->setSystematic(ScaleFactors->systematic()));
            if (m_doTriggerSF) {
                if (m_writeBaselineSF) ATH_CHECK(ScaleFactors->saveBaselineTriggerSF(GetBaselineMuons()));
                ATH_CHECK(ScaleFactors->saveSignalTriggerSF(GetBaselineMuons()));
            }
            for (auto muon : *SFMuons) {
                bool is_signal = !m_writeBaselineSF || PassSignal(*muon);
                ATH_CHECK(ScaleFactors->saveSF(*muon, is_signal));
            }

            ATH_CHECK(ScaleFactors->applySF());
        }
        ATH_CHECK(m_systematics->resetSystematics());
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMuonSelector::StoreTruthClassifer(xAOD::Muon& mu) const {
        if (!m_XAMPPInfo->isMC()) return StatusCode::SUCCESS;

        const xAOD::TrackParticle* mu_trk = mu.primaryTrackParticle();
        if (!mu_trk) {
            ATH_MSG_DEBUG("No InDet track found for the muon");
            mu_trk = mu.primaryTrackParticle();
            if (!mu_trk) {
                ATH_MSG_ERROR("There is no muon track");
                return StatusCode::FAILURE;
            }
        }
        static SG::AuxElement::Decorator<ElementLink<xAOD::TruthParticleContainer>> dec_truthLink("truthParticleLink");
        ElementLink<xAOD::TruthParticleContainer> truthLink;
        if (!m_muonDecorations->truthParticleLink.get(*mu_trk, truthLink)) {
            ATH_MSG_WARNING("No truth link for muon track");
        } else {
            m_muonDecorations->truthParticleLink.set(mu, truthLink);
        }
        int auxTruth = -1;
        if (!m_muonDecorations->truthType.get(*mu_trk, auxTruth)) {
            ATH_MSG_WARNING("No truth type for muon track");
        } else {
            m_muonDecorations->truthType.set(mu, auxTruth);
        }
        auxTruth = -1;
        if (!m_muonDecorations->truthOrigin.get(*mu_trk, auxTruth)) {
            ATH_MSG_WARNING("No truth origin for muon track");
        } else {
            m_muonDecorations->truthOrigin.set(mu, auxTruth);
        }
        return StatusCode::SUCCESS;
    }
    bool SUSYMuonSelector::IsBadMuon(const xAOD::Muon& mu) const {
        return m_muonDecorations->enterBadMuonSelection(mu) && m_muonDecorations->isBadMuon(mu);
    }
    bool SUSYMuonSelector::IsCosmicMuon(const xAOD::Muon& mu) const {
        return m_muonDecorations->enterCosmicSelection(mu) && m_muonDecorations->isCosmicMuon(mu);
    }

    void SUSYMuonSelector::setupDecorations(std::shared_ptr<MuonDecorations> input) {
        // as for the particle selector, use the input if provided, or use a default
        // if not set before.
        if (input) { m_muonDecorations = input; }
        if (!m_muonDecorations) { m_muonDecorations = std::make_shared<MuonDecorations>(); }
        // also set up the particle selector decoration members
        ParticleSelector::setupDecorations(m_muonDecorations);
        // bad muon quality: configured string or baseline def
        if (!m_quality_BadMuon.empty()) {
            m_muonDecorations->enterBadMuonSelection.setDecorationString(m_quality_BadMuon);
        } else {
            m_muonDecorations->enterBadMuonSelection.setDecorationString(m_muonDecorations->passBaseline.getDecorationString());
        }
        if (!m_quality_Cosmic.empty()) {
            m_muonDecorations->enterCosmicSelection.setDecorationString(m_quality_Cosmic);
        } else {
            m_muonDecorations->enterCosmicSelection.setDecorationString(m_muonDecorations->passBaseline.getDecorationString());
        }
        // so far, the cosmic / bad muon decoration strings themselves
        // are not configurable properties
    }
    //##########################################################################
    //							MuonWeightDecorator
    //##########################################################################
    MuonWeightDecorator::MuonWeightDecorator() : IPartilceWeightDecorator() {}
    MuonWeightDecorator::~MuonWeightDecorator() {}
    StatusCode MuonWeightDecorator::saveSF(const xAOD::Muon& muon, bool isSignal) {
        float SF = 1.;
        if (!isSFcalculated(muon)) {
            if (!calculateSF(muon, SF).isSuccess()) return StatusCode::FAILURE;
        } else
            SF = getSF(muon);
        return saveEventSF(muon, SF, isSignal);
    }
    //##########################################################################
    //							MuonWeight
    //##########################################################################
    MuonWeight::MuonWeight(ToolHandle<CP::IMuonEfficiencyScaleFactors>& SFTool, XAMPP::EventInfo* info) :
        MuonWeightDecorator(),
        m_SFTool(SFTool),
        m_XAMPPInfo(info),
        m_validity_eta_min(-1),
        m_validity_eta_max(-1),
        m_validity_pt_min(-1),
        m_validity_pt_max(-1) {}
    MuonWeight::~MuonWeight() {}
    StatusCode MuonWeight::calculateSF(const xAOD::Muon& muon, float& SF) {
        if ((m_validity_eta_min > 0 && fabs(muon.eta()) < m_validity_eta_min) ||
            (m_validity_eta_max > 0 && fabs(muon.eta()) > m_validity_eta_min) ||
            (m_validity_pt_min > 0 && muon.pt() * MeVToGeV < m_validity_pt_min) ||
            (m_validity_pt_max > 0 && muon.pt() * MeVToGeV > m_validity_pt_max))
            return StatusCode::SUCCESS;
        if (m_SFTool->getEfficiencyScaleFactor(muon, SF, m_XAMPPInfo->GetOrigInfo()) == CP::CorrectionCode::Error)
            return StatusCode::FAILURE;
        return StatusCode::SUCCESS;
    }
    void MuonWeight::setValidityRangeAbsEta(double min, double max) {
        m_validity_eta_min = min;
        m_validity_eta_max = max;
    }
    void MuonWeight::setValidityRangePt(double min, double max) {
        m_validity_pt_min = min;
        m_validity_pt_max = max;
    }
    //##########################################################################
    //							MuonWeightHandler
    //##########################################################################
    MuonWeightHandler::MuonWeightHandler(const CP::SystematicSet* syst_set) :
        MuonWeightDecorator(),
        m_Syst(syst_set),
        m_Weights(),
        m_init(false),
        m_baseline_trig_SF(),
        m_signal_trig_SF(),
        m_multiple_trig_sf(false) {}
    StatusCode MuonWeightHandler::applySF() {
        if (!m_init) {
            Error("MuonWeightHandler()", "I'm not the crusty crab.");
            return StatusCode::FAILURE;
        }
        for (auto& W : m_Weights) {
            if (!W->applySF().isSuccess()) return StatusCode::FAILURE;
        }
        return MuonWeightDecorator::applySF();
    }
    void MuonWeightHandler::multipleTriggerSF(bool B) { m_multiple_trig_sf = B; }
    const CP::SystematicSet* MuonWeightHandler::systematic() const { return m_Syst; }
    size_t MuonWeightHandler::nWeights() const { return m_Weights.size(); }
    StatusCode MuonWeightHandler::calculateSF(const xAOD::Muon& muon, float& SF) {
        for (auto& W : m_Weights) { SF *= W->getSF(muon); }
        return StatusCode::SUCCESS;
    }
    StatusCode MuonWeightHandler::saveSF(const xAOD::Muon& muon, bool isSignal) {
        for (auto& W : m_Weights) {
            if (!W->saveSF(muon, isSignal).isSuccess()) return StatusCode::FAILURE;
        }
        // Weighters are emptied for multiple trigger sf
        // on systematics
        if (m_Weights.empty()) return StatusCode::SUCCESS;
        return MuonWeightDecorator::saveSF(muon, isSignal);
    }
    bool MuonWeightHandler::append(MuonWeightMap& map, const CP::SystematicSet* nominal) {
        MuonWeightMap::const_iterator Itr = map.find(systematic());
        if (Itr != map.end()) {
            if (!IsInVector(Itr->second, m_Weights)) m_Weights.push_back(Itr->second);
            m_init = true;
        } else {
            Itr = map.find(nominal);
            if (Itr != map.end() && !IsInVector(Itr->second, m_Weights)) m_Weights.push_back(Itr->second);
        }
        return m_init;
    }
    bool MuonWeightHandler::setBaseTriggerSF(SUSYMuonTriggerSFHandler_Map& map, const CP::SystematicSet* nominal) {
        SUSYMuonTriggerSFHandler_Map::iterator Itr = map.find(systematic());
        if (Itr != map.end()) {
            CopyVector(Itr->second, m_baseline_trig_SF, true);
            // Trigger_sf systematic
            if (systematic() != nominal && m_multiple_trig_sf) m_Weights.clear();
            m_init = true;
        } else {
            Itr = map.find(nominal);
            if (Itr != map.end()) CopyVector(Itr->second, m_baseline_trig_SF, true);
        }
        return m_init;
    }
    bool MuonWeightHandler::setSignalTriggerSF(SUSYMuonTriggerSFHandler_Map& map, const CP::SystematicSet* nominal) {
        SUSYMuonTriggerSFHandler_Map::iterator Itr = map.find(systematic());
        if (Itr != map.end()) {
            CopyVector(Itr->second, m_signal_trig_SF, true);
            // Trigger_sf systematic
            if (systematic() != nominal && m_multiple_trig_sf) m_Weights.clear();
            m_init = true;
        } else {
            Itr = map.find(nominal);
            if (Itr != map.end()) CopyVector(Itr->second, m_signal_trig_SF, true);
        }
        // Connect the baseline tools with signal
        for (auto& signal : m_signal_trig_SF) {
            signal->FilterHandlersFromOtherYears(m_signal_trig_SF);
            for (auto& baseline : m_baseline_trig_SF) {
                if (signal->SetBaselineTool(baseline)) break;
            }
        }
        for (auto& baseline : m_baseline_trig_SF) { baseline->FilterHandlersFromOtherYears(m_baseline_trig_SF); }
        return m_init;
    }
    StatusCode MuonWeightHandler::saveBaselineTriggerSF(const xAOD::MuonContainer* muons) {
        for (auto SF : m_baseline_trig_SF)
            if (!SF->SaveSF(muons).isSuccess()) return StatusCode::FAILURE;
        if (!m_multiple_trig_sf && !m_baseline_trig_SF.empty()) {
            return saveBaselineSF((*m_baseline_trig_SF.begin())->getBaselineEventSF());
        }
        return StatusCode::SUCCESS;
    }
    StatusCode MuonWeightHandler::saveSignalTriggerSF(const xAOD::MuonContainer* muons) {
        for (auto SF : m_signal_trig_SF)
            if (!SF->SaveSF(muons).isSuccess()) return StatusCode::FAILURE;
        if (!m_multiple_trig_sf && !m_signal_trig_SF.empty()) { return saveSignalSF((*m_signal_trig_SF.begin())->getSignalEventSF()); }
        return StatusCode::SUCCESS;
    }

    //#################################################################################################
    //                                  SUSYMuonTriggerSFHandler
    //#################################################################################################
    SUSYMuonTriggerSFHandler::SUSYMuonTriggerSFHandler(const XAMPP::EventInfo* Info, ToolHandle<ST::ISUSYObjDef_xAODTool> ST,
                                                       const std::string& Trigger, int year) :
        m_BaselineHandler(),
        m_otherYearHandlers(),
        m_Info(Info),
        m_SUSYTools(ST),
        m_SF_string(Trigger),
        m_Trigger_string(Trigger),
        m_year(year),
        m_n_muons(-1),
        m_DependOnTrigger(false),
        m_Cut(nullptr) {}
    void SUSYMuonTriggerSFHandler::ApplyIfTriggerFired(bool B) { m_DependOnTrigger = B; }
    bool SUSYMuonTriggerSFHandler::isAvailable() const { return hasBaselineSF() || hasSignalSF(); }
    StatusCode SUSYMuonTriggerSFHandler::SaveSF(const xAOD::MuonContainer* Muons) {
        m_n_muons = Muons->size();
        if (nMuons() == 0 || (m_DependOnTrigger && !m_Cut->ApplyCut())) { return applySF(); }
        // The assigned year does not match the year of this handler
        if (m_Info->dataYear() != year() && !m_otherYearHandlers.empty()) {
            for (const auto& other : m_otherYearHandlers) {
                if (m_Info->dataYear() == other->year()) return other->SaveSF(Muons);
            }
            // We've no choice than just returning a flat 1
            return applySF();
        } else if (m_Info->dataYear() != year())
            return applySF();
        if (isAvailable()) return StatusCode::SUCCESS;

        // The number of baseline muons is the same as the number of signal
        // muons -> Let's not call the tool again
        if (m_BaselineHandler != nullptr && m_BaselineHandler->hasBaselineSF() && m_BaselineHandler->nMuons() == nMuons()) {
            return saveEventSF(m_BaselineHandler->getBaselineEventSF(), true);
        }
        return saveEventSF(m_SUSYTools->GetTotalMuonTriggerSF(*Muons, name()), true);
    }
    StatusCode SUSYMuonTriggerSFHandler::initialize() {
        ST::SUSYObjDef_xAOD* ST = dynamic_cast<ST::SUSYObjDef_xAOD*>(m_SUSYTools.operator->());
        if (m_Cut) delete m_Cut;
        std::vector<std::string> Triggers = ST->GetTriggerOR(m_Trigger_string);
        if (!m_DependOnTrigger) { return StatusCode::SUCCESS; }
        for (auto& T : Triggers) {
            if (T.find("HLT_HLT") != std::string::npos) { T = T.substr(4, T.size()); }
            Cut* Trigger = new Cut("PassTrigger", Cut::CutType::CutChar, true);
            if (!Trigger->initialize(m_Info->GetVariableStorage<char>("Trig" + T), 1, Cut::Relation::Equal)) {
                Warning("SUSYMuonTriggerSFHandler::SetDecorators()", "Trigger %s not defined in the TriggerTool", T.c_str());
                delete Trigger;
            } else if (!m_Cut) {
                m_Cut = Trigger;
            } else {
                m_Cut = m_Cut->combine(Trigger, Cut::Combine::OR);
            }
        }
        if (!m_Cut) {
            Error("SUSYMuonTriggerSFHandler::SetDecorators()", "No trigger %s defined in the TriggerTool", m_SF_string.c_str());
            return StatusCode::FAILURE;
        }
        return StatusCode::SUCCESS;
    }
    SUSYMuonTriggerSFHandler::~SUSYMuonTriggerSFHandler() {
        if (m_Cut) delete m_Cut;
    }

    void SUSYMuonTriggerSFHandler::DefineTrigger(const std::string& Trigger) {
        if (!Trigger.empty()) m_Trigger_string = Trigger;
    }
    bool SUSYMuonTriggerSFHandler::SetBaselineTool(SUSYMuonTriggerSFHandler_Ptr Ref) {
        if (Ref.get() == this || !Ref || Ref->name() != name() || year() != Ref->year()) { return false; }
        m_BaselineHandler = Ref;
        return true;
    }
    const std::string& SUSYMuonTriggerSFHandler::name() const { return m_SF_string; }
    unsigned int SUSYMuonTriggerSFHandler::year() const { return m_year; }
    unsigned int SUSYMuonTriggerSFHandler::nMuons() const { return m_n_muons; }
    void SUSYMuonTriggerSFHandler::FilterHandlersFromOtherYears(const SUSYMuonTriggerSFHandler_Vector& handlers) {
        m_otherYearHandlers.clear();
        for (const auto& H : handlers) {
            if (H.get() == this || H->year() == year() || H->name() != name()) continue;
            m_otherYearHandlers.push_back(H.get());
        }
    }

}  // namespace XAMPP
