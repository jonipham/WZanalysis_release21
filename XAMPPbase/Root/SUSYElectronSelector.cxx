#include <ElectronEfficiencyCorrection/AsgElectronEfficiencyCorrectionTool.h>
#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/SUSYElectronSelector.h>
#include <XAMPPbase/SUSYTriggerTool.h>
#include <xAODEgamma/EgammaxAODHelpers.h>

#include <IsolationSelection/IIsolationSelectionTool.h>
#include <SUSYTools/SUSYObjDef_xAOD.h>

#include <XAMPPbase/ToolHandleSystematics.h>

#define CONFIG_BASELINE_IDISO_SFTOOl(TOOLHANDLE, TOOLNAME, ISOWP)                                                                        \
    if (TOOLHANDLE.empty()) {                                                                                                            \
        asg::AnaToolHandle<EleEffTool> AnaTool("AsgElectronEfficiencyCorrectionTool/" + m_Baseline_Id_WP + TOOLNAME);                    \
        ATH_CHECK(AnaTool.setProperty("IdKey", eleId));                                                                                  \
        ATH_CHECK(AnaTool.setProperty("ForceDataType",                                                                                   \
                                      int(m_systematics->isAF2() ? PATCore::ParticleDataType::Fast : PATCore::ParticleDataType::Full))); \
        ATH_CHECK(AnaTool.setProperty("CorrelationModel", m_CorrelationModel));                                                          \
        if (ISOWP) ATH_CHECK(AnaTool.setProperty("IsoKey", m_Baseline_Iso_WP));                                                          \
        ATH_CHECK(AnaTool.initialize());                                                                                                 \
        TOOLHANDLE = AnaTool.getHandle();                                                                                                \
        ATH_CHECK(DeclareAsWeightSyst(AnaTool));                                                                                         \
    }

namespace XAMPP {
    std::string SUSYElectronSelector::EG_WP(const std::string& wp) const {
        // translate our electron wps to EGamma internal jargon
        //@ElectronPhotonSelectorTools/EGSelectorConfigurationMapping.h
        return TString(wp).Copy().ReplaceAll("AndBLayer", "BLayer").ReplaceAll("LLH", "").Data();
    }

    std::string SUSYElectronSelector::Trig_Iso_WP(const std::string& trig_exp, bool use_signal) const {
        if (trig_exp.find(";") == std::string::npos) return use_signal ? m_Signal_Iso_WP : m_Baseline_Iso_WP;
        std::string trig_iso = trig_exp.substr(trig_exp.find(";") + 1, std::string::npos);
        if (trig_iso.find(":") != std::string::npos) return trig_iso.substr(0, trig_iso.find(":"));
        return trig_iso;
    }
    std::string SUSYElectronSelector::Trig_EG_WP(const std::string& trig_exp, bool use_signal) const {
        if (trig_exp.find(":") == std::string::npos) return EG_WP(use_signal ? m_Signal_Id_WP : m_Baseline_Id_WP);
        std::string trig_id = trig_exp.substr(trig_exp.find(":") + 1, std::string::npos);
        if (trig_id.find(":") != std::string::npos) return EG_WP(trig_id.substr(0, trig_id.find(":")));
        return EG_WP(trig_id);
    }

    SUSYElectronSelector::SUSYElectronSelector(const std::string& myname) :
        SUSYParticleSelector(myname),
        m_PreSelectionD0SigCut(-1.),
        m_PreSelectionZ0SinThetaCut(-1.),
        m_RequireIsoPreSelection(false),
        m_BaselineD0SigCut(-1),
        m_BaselineZ0SinThetaCut(-1),
        m_RequireIsoBaseline(false),
        m_SignalD0SigCut(-1),
        m_SignalZ0SinThetaCut(-1),
        m_RequireIsoSignal(true),
        m_StoreTruthClassifier(false),
        m_xAODElectrons(nullptr),
        m_Electrons(nullptr),
        m_ElectronsAux(nullptr),
        m_PreElectrons(nullptr),
        m_BaselineElectrons(nullptr),
        m_SignalNoORElectrons(nullptr),
        m_SignalElectrons(nullptr),
        m_electronDecorations(nullptr),
        m_trigger_tool(""),
        m_SF(),
        m_SignalTrig_SF_Tools(),
        m_BaselineTrig_SF_Tools(),
        m_Reco_SF_Handle(""),
        m_SignalId_SF_Handle(""),
        m_SignalIso_SF_Handle(""),
        m_BaselineId_SF_Handle(""),
        m_BaselineIso_SF_Handle(""),
        m_SeparateSF(false),
        m_doRecoSF(true),
        m_doIdSF(true),
        m_doTriggerSF(true),
        m_doIsoSF(true),
        m_TriggerExp(),
        m_TriggerSFConf(),
        m_StoreMultiTrigSF(false),
        m_writeBaselineSF(false),
        m_Baseline_Id_WP(),
        m_Signal_Id_WP(),
        m_EfficiencyMap(),
        m_CorrelationModel(),
        m_Signal_Iso_WP(),
        m_Baseline_Iso_WP(),
        m_iso_tool(""),
        m_force_iso_calc(false) {
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
        // if this option is switched on the electron SF are saved split into ID, Reco, Isol and Trigger.
        declareProperty("SeparateSF", m_SeparateSF);

        declareProperty("ApplyIsoSF", m_doIsoSF);
        declareProperty("ApplyRecoSF", m_doRecoSF);
        declareProperty("ApplyIdSF", m_doIdSF);
        declareProperty("ApplyTriggerSF", m_doTriggerSF);

        declareProperty("WriteBaselineSF", m_writeBaselineSF);

        declareProperty("BaselineID", m_Baseline_Id_WP);
        declareProperty("SignalID", m_Signal_Id_WP);
        declareProperty("CorrelationModel", m_CorrelationModel);
        declareProperty("SignalIso", m_Signal_Iso_WP);
        declareProperty("BaselineIso", m_Baseline_Iso_WP);

        declareProperty("SFTrigger", m_TriggerExp);
        declareProperty("TriggerSFconfig", m_TriggerSFConf);
        declareProperty("ExcludeTrigSFfromTotalWeight", m_StoreMultiTrigSF);
        declareProperty("EfficiencyMapFilePath", m_EfficiencyMap);

        declareProperty("Reco_SF_Handle", m_Reco_SF_Handle);
        declareProperty("SignalId_SF_Handle", m_SignalId_SF_Handle);
        declareProperty("SignalIso_SF_Handle", m_SignalIso_SF_Handle);
        declareProperty("BaselineId_SF_Handle", m_BaselineId_SF_Handle);
        declareProperty("BaselineIso_SF_Handle", m_BaselineIso_SF_Handle);

        // SUSYTools disabled the calculation of the isolation wp if it's not
        // part of the signal selection
        declareProperty("IsolationTool", m_iso_tool);
        declareProperty("RecalcIsolation", m_force_iso_calc);
        declareProperty("TriggerTool", m_trigger_tool);
        SetContainerKey("Electrons");
        SetObjectType(XAMPP::SelectionObject::Electron);
    }

    SUSYElectronSelector::~SUSYElectronSelector() {}
    StatusCode SUSYElectronSelector::initialize() {
        if (isInitialized()) { return StatusCode::SUCCESS; }
        ATH_CHECK(SUSYParticleSelector::initialize());
        if (!ProcessObject()) { return StatusCode::SUCCESS; }
        if (m_Signal_Id_WP.empty()) m_Signal_Id_WP = (*SUSYToolsPtr()->getProperty<std::string>("EleId"));
        if (m_Baseline_Id_WP.empty()) m_Baseline_Id_WP = (*SUSYToolsPtr()->getProperty<std::string>("EleBaselineId"));
        if (m_CorrelationModel.empty()) m_CorrelationModel = (*SUSYToolsPtr()->getProperty<std::string>("ElEffNPcorrModel"));
        if (m_Baseline_Iso_WP.empty()) m_Baseline_Iso_WP = (*SUSYToolsPtr()->getProperty<std::string>("EleIso"));
        if (m_Signal_Iso_WP.empty()) m_Signal_Iso_WP = (*SUSYToolsPtr()->getProperty<std::string>("EleIso"));
        m_Baseline_Id_WP = EraseWhiteSpaces(m_Baseline_Id_WP);
        m_Signal_Id_WP = EraseWhiteSpaces(m_Signal_Id_WP);
        setupDecorations();

        if (m_force_iso_calc) {
            if (m_iso_tool.empty()) m_iso_tool = GetCPTool<CP::IIsolationSelectionTool>("IsolationSelectionTool");
            ATH_CHECK(m_iso_tool.retrieve());
        }
        if (!isData()) {
            if (!m_Reco_SF_Handle.isSet()) m_Reco_SF_Handle = GetCPTool<EleEffTool>("ElectronEfficiencyCorrectionTool_reco");
            if (!m_SignalId_SF_Handle.isSet()) m_SignalId_SF_Handle = GetCPTool<EleEffTool>("ElectronEfficiencyCorrectionTool_id");
            if (!m_SignalIso_SF_Handle.isSet()) m_SignalIso_SF_Handle = GetCPTool<EleEffTool>("ElectronEfficiencyCorrectionTool_iso");
        }
        if (m_EfficiencyMap.empty()) { m_EfficiencyMap = (*SUSYToolsPtr()->getProperty<std::string>("EleEffMapFilePath")); }
        ATH_CHECK(setupScaleFactors());
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYElectronSelector::setupScaleFactors() {
        // We do not need any SFs for data
        if (isData() || !ProcessObject()) return StatusCode::SUCCESS;

        if (m_TriggerExp.empty() && m_doTriggerSF) {
            ATH_MSG_INFO("No triggers were given... Switch trigger SF off");
            m_doTriggerSF = false;
        }

        if (m_doTriggerSF) {
            ATH_CHECK(m_trigger_tool.retrieve());
            ATH_CHECK(initializeTriggerSFTools());
        }
        m_StoreMultiTrigSF = m_StoreMultiTrigSF || m_TriggerExp.size() > 1;

        bool Baseline_Signal_IsSame = (m_Signal_Id_WP == m_Baseline_Id_WP);

        if (m_writeBaselineSF && Baseline_Signal_IsSame) {
            m_BaselineId_SF_Handle = m_SignalId_SF_Handle;
            m_BaselineIso_SF_Handle = m_SignalIso_SF_Handle;
        } else if (m_writeBaselineSF) {
            std::string eleId = EG_WP(m_Baseline_Id_WP);
            CONFIG_BASELINE_IDISO_SFTOOl(m_BaselineId_SF_Handle, "BaselineID", false);
            CONFIG_BASELINE_IDISO_SFTOOl(m_BaselineIso_SF_Handle, "BaselineIso", true);
        }
        ElectronWeightMap Reco_SFs;

        ElectronWeightMap BaselineID_SFs;
        ElectronWeightMap SignalID_SFs;

        ElectronWeightMap BaselineIso_SFs;
        ElectronWeightMap SignalIso_SFs;

        ElectronWeight_VectorMap Baseline_TriggerSF;
        ElectronWeight_VectorMap Signal_TriggerSF;

        if (!isData() && m_systematics->GetWeightSystematics(ObjectType()).empty()) {
            ATH_MSG_INFO("Declare the weight systematics");
            if (m_doRecoSF) ATH_CHECK(DeclareAsWeightSyst(m_Reco_SF_Handle));
            if (m_doIdSF) ATH_CHECK(DeclareAsWeightSyst(m_SignalId_SF_Handle));
            if (m_doIsoSF) ATH_CHECK(DeclareAsWeightSyst(m_SignalIso_SF_Handle));
        }
        if (m_doRecoSF)
            ATH_CHECK(
                initializeScaleFactors("Reco", m_Reco_SF_Handle, Reco_SFs,
                                       m_writeBaselineSF ? ScaleFactorMapContains::SignalAndBaseSf : ScaleFactorMapContains::SignalSf));
        if (m_doIdSF) {
            ATH_CHECK(initializeScaleFactors(
                "Id", m_SignalId_SF_Handle, SignalID_SFs,
                m_writeBaselineSF && Baseline_Signal_IsSame ? ScaleFactorMapContains::SignalAndBaseSf : ScaleFactorMapContains::SignalSf));
            if (m_writeBaselineSF && !Baseline_Signal_IsSame) {
                ATH_CHECK(initializeScaleFactors("Id", m_BaselineId_SF_Handle, BaselineID_SFs, ScaleFactorMapContains::BaselineSf));
            }
        }
        if (m_doIsoSF) {
            ATH_CHECK(initializeScaleFactors(
                "Iso", m_SignalIso_SF_Handle, SignalIso_SFs,
                m_writeBaselineSF && Baseline_Signal_IsSame ? ScaleFactorMapContains::SignalAndBaseSf : ScaleFactorMapContains::SignalSf));
            if (m_writeBaselineSF && !Baseline_Signal_IsSame) {
                ATH_CHECK(initializeScaleFactors("Iso", m_BaselineIso_SF_Handle, BaselineIso_SFs, ScaleFactorMapContains::BaselineSf));
            }
        }
        if (m_doTriggerSF) {
            ATH_CHECK(initializeTriggerScaleFactors(
                m_SignalTrig_SF_Tools, Signal_TriggerSF,
                m_writeBaselineSF && Baseline_Signal_IsSame ? ScaleFactorMapContains::SignalAndBaseSf : ScaleFactorMapContains::SignalSf));
            if (m_writeBaselineSF && !Baseline_Signal_IsSame) {
                ATH_CHECK(initializeTriggerScaleFactors(m_BaselineTrig_SF_Tools, Baseline_TriggerSF, ScaleFactorMapContains::BaselineSf));
            }
        }
        for (const auto set : m_systematics->GetWeightSystematics(ObjectType())) {
            ElectronWeightHandler_Ptr Signal(new ElectronWeightHandler(set));
            ElectronWeightHandler_Ptr Baseline(new ElectronWeightHandler(set));
            Signal->multipleTriggerSF(m_StoreMultiTrigSF);
            Baseline->multipleTriggerSF(m_StoreMultiTrigSF);

            if (m_writeBaselineSF && !Baseline_Signal_IsSame) {
                bool append = Baseline->append(Reco_SFs, m_systematics->GetNominal());
                if (Baseline->append(BaselineID_SFs, m_systematics->GetNominal())) append = true;
                if (Baseline->append(BaselineIso_SFs, m_systematics->GetNominal())) append = true;
                if (Baseline->setSignalTriggerSF(Baseline_TriggerSF, m_systematics->GetNominal())) append = true;
                if (append) {
                    if (Baseline->nWeights() > 0) ATH_CHECK(initIParticleWeight(*Baseline, "", set, ScaleFactorMapContains::BaselineSf));
                    m_SF.push_back(Baseline);
                }
            }
            bool append = Signal->append(Reco_SFs, m_systematics->GetNominal());
            if (Signal->append(SignalID_SFs, m_systematics->GetNominal())) append = true;
            if (Signal->append(SignalIso_SFs, m_systematics->GetNominal())) append = true;
            if (Signal->setSignalTriggerSF(Signal_TriggerSF, m_systematics->GetNominal())) append = true;
            if (append) {
                if (Signal->nWeights() > 0)
                    ATH_CHECK(initIParticleWeight(*Signal, "", set,
                                                  m_writeBaselineSF && Baseline_Signal_IsSame ? ScaleFactorMapContains::SignalAndBaseSf
                                                                                              : ScaleFactorMapContains::SignalSf));
                m_SF.push_back(Signal);
            }
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYElectronSelector::initializeScaleFactors(const std::string& sf_type, EleEffToolHandle& sf_tool, ElectronWeightMap& map,
                                                            unsigned int content) {
        for (auto& syst_set : m_systematics->GetWeightSystematics(ObjectType())) {
            if (!XAMPP::ToolIsAffectedBySystematic(sf_tool, syst_set)) continue;
            ElectronWeight_Ptr Weight(new ElectronWeight(sf_tool));
            ATH_CHECK(initIParticleWeight(*Weight, sf_type, syst_set, content, m_SeparateSF));
            if (map.find(syst_set) != map.end()) {
                ATH_MSG_FATAL("There is already a sf stored in map for systematic " << syst_set->name());
                return StatusCode::FAILURE;
            }
            map.insert(std::pair<const CP::SystematicSet*, ElectronWeight_Ptr>(syst_set, Weight));
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYElectronSelector::LoadContainers() {
        if (!ProcessObject()) return StatusCode::SUCCESS;
        ATH_CHECK(LoadContainer(ContainerKey(), m_xAODElectrons));
        for (auto& ScaleFactors : m_SF) { ATH_CHECK(ScaleFactors->initEvent()); }
        return StatusCode::SUCCESS;
    }
    EleLink SUSYElectronSelector::GetLink(const xAOD::Electron& el) const { return EleLink(*GetElectrons(), el.index()); }
    EleLink SUSYElectronSelector::GetOrigLink(const xAOD::Electron& el) const {
        const xAOD::Electron* Oel = dynamic_cast<const xAOD::Electron*>(xAOD::getOriginalObject(el));
        return EleLink(*m_xAODElectrons, Oel != nullptr ? Oel->index() : el.index());
    }
    xAOD::ElectronContainer* SUSYElectronSelector::GetCustomElectrons(const std::string& kind) const {
        xAOD::ElectronContainer* customElectrons = nullptr;
        if (LoadViewElementsContainer(kind, customElectrons).isSuccess()) return customElectrons;
        return m_BaselineElectrons;
    }
    StatusCode SUSYElectronSelector::CallSUSYTools() {
        ATH_MSG_DEBUG("Calling SUSYElectronSelector::CallSUSYTools()..");
        return m_susytools->GetElectrons(m_Electrons, m_ElectronsAux, false);
    }
    StatusCode SUSYElectronSelector::InitialFill(const CP::SystematicSet& systset) {
        SetSystematics(systset);
        if (ProcessObject()) {
            ATH_CHECK(FillFromSUSYTools(m_Electrons, m_ElectronsAux, m_PreElectrons));
        } else {
            ATH_CHECK(ViewElementsContainer("container", m_Electrons));
            ATH_CHECK(ViewElementsContainer("presel", m_PreElectrons));
        }
        return StatusCode::SUCCESS;
    }

    StatusCode SUSYElectronSelector::LoadSelection(const CP::SystematicSet& systset) {
        ATH_CHECK(LoadPreSelectedContainer(m_PreElectrons, &systset));
        SetSystematics(systset);
        ATH_CHECK(LoadViewElementsContainer("baseline", m_BaselineElectrons));
        ATH_CHECK(LoadViewElementsContainer("signal", m_SignalElectrons));
        ATH_CHECK(LoadViewElementsContainer("goodQuality", m_SignalNoORElectrons));

        return StatusCode::SUCCESS;
    }
    StatusCode SUSYElectronSelector::FillElectrons(const CP::SystematicSet& systset) {
        SetSystematics(systset);
        ATH_CHECK(ViewElementsContainer("baseline", m_BaselineElectrons));
        ATH_CHECK(ViewElementsContainer("signal", m_SignalElectrons));
        ATH_CHECK(ViewElementsContainer("goodQuality", m_SignalNoORElectrons));

        for (const auto& ielec : *m_PreElectrons) {
            if (m_StoreTruthClassifier) {
                const xAOD::TrackParticle* track = xAOD::EgammaHelpers::getOriginalTrackParticle(ielec);
                m_electronDecorations->truthType.set(*ielec, getParticleTruthType(track));
                m_electronDecorations->truthOrigin.set(*ielec, getParticleTruthOrigin(track));
            }
            if (m_force_iso_calc) { m_electronDecorations->passIsolation.set(*ielec, m_iso_tool->accept(*ielec)); }
            if (PassBaseline(*ielec)) m_BaselineElectrons->push_back(ielec);
            if (PassSignalNoOR(ielec)) m_SignalNoORElectrons->push_back(ielec);
            if (PassSignal(*ielec)) m_SignalElectrons->push_back(ielec);
        }

        ATH_MSG_DEBUG("Number of all electrons: " << m_Electrons->size());
        ATH_MSG_DEBUG("Number of preselected electrons: " << m_PreElectrons->size());
        ATH_MSG_DEBUG("Number of selected baseline electrons: " << m_BaselineElectrons->size());
        ATH_MSG_DEBUG("Number of selected good quality electrons: " << m_SignalNoORElectrons->size());

        ATH_MSG_DEBUG("Number of selected signal electrons: " << m_SignalElectrons->size());

        return StatusCode::SUCCESS;
    }
    bool SUSYElectronSelector::PassPreSelection(const xAOD::IParticle& P) const {
        if (!ParticleSelector::PassPreSelection(P)) return false;
        float aux = 0;
        if (!m_electronDecorations->z0sinTheta.get(P, aux)) {
            ATH_MSG_WARNING("Failed to read electron z0 sin theta decoration");
            return false;
        }
        if (m_PreSelectionZ0SinThetaCut >= 0. && fabs(aux) >= m_PreSelectionZ0SinThetaCut) return false;
        aux = 0;
        if (!m_electronDecorations->d0sig.get(P, aux)) {
            ATH_MSG_WARNING("Failed to read electron d0 significance decoration");
            return false;
        }
        if (m_PreSelectionD0SigCut >= 0. && fabs(aux) >= m_PreSelectionD0SigCut) return false;
        if (m_RequireIsoPreSelection && !PassIsolation(P)) return false;
        return true;
    }
    bool SUSYElectronSelector::PassBaseline(const xAOD::IParticle& P) const {
        if (!ParticleSelector::PassBaseline(P)) return false;
        float aux = 0;
        if (!m_electronDecorations->z0sinTheta.get(P, aux)) {
            ATH_MSG_WARNING("Failed to read electron z0 sin theta decoration");
            return false;
        }
        if (m_BaselineZ0SinThetaCut >= 0. && fabs(aux) >= m_BaselineZ0SinThetaCut) return false;
        aux = 0;
        if (!m_electronDecorations->d0sig.get(P, aux)) {
            ATH_MSG_WARNING("Failed to read electron d0 significance decoration");
            return false;
        }
        if (m_BaselineD0SigCut >= 0. && fabs(aux) >= m_BaselineD0SigCut) return false;
        if (m_RequireIsoBaseline && !PassIsolation(P)) return false;
        return true;
    }
    bool SUSYElectronSelector::GetSignalDecorator(const xAOD::IParticle& P) const {
        if (!ParticleSelector::GetSignalDecorator(P)) return false;
        float aux = 0;
        if (!m_electronDecorations->z0sinTheta.get(P, aux)) {
            ATH_MSG_WARNING("Failed to read electron z0 sin theta decoration");
            return false;
        }
        if (m_SignalZ0SinThetaCut >= 0. && fabs(aux) >= m_SignalZ0SinThetaCut) return false;
        aux = 0;
        if (!m_electronDecorations->d0sig.get(P, aux)) {
            ATH_MSG_WARNING("Failed to read electron d0 significance decoration");
            return false;
        }
        if (m_SignalD0SigCut >= 0. && fabs(aux) >= m_SignalD0SigCut) return false;
        if (m_RequireIsoSignal && !PassIsolation(P)) return false;
        return true;
    }

    StatusCode SUSYElectronSelector::SaveScaleFactor() {
        if (m_SF.empty()) return StatusCode::SUCCESS;
        const CP::SystematicSet* kineSet = m_XAMPPInfo->GetSystematic();
        xAOD::ElectronContainer* SFElectrons = m_writeBaselineSF ? GetBaselineElectrons() : GetSignalElectrons();
        ATH_MSG_DEBUG("Save SF of " << SFElectrons->size() << " " << (m_writeBaselineSF ? "baseline" : "signal") << " electrons");

        for (auto const& ScaleFactors : m_SF) {
            if (kineSet != m_systematics->GetNominal() && ScaleFactors->systematic() != m_systematics->GetNominal()) continue;
            ATH_CHECK(m_systematics->setSystematic(ScaleFactors->systematic()));
            for (auto electron : *SFElectrons) {
                bool is_Signal = !m_writeBaselineSF || PassSignal(*electron);
                ATH_CHECK(ScaleFactors->saveSF(*electron, is_Signal));
            }
            ATH_CHECK(ScaleFactors->applySF());
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYElectronSelector::initializeTriggerSFTools() {
        ATH_CHECK(initializeTriggerSFTools(m_SignalTrig_SF_Tools, true));
        if (m_writeBaselineSF && m_Signal_Id_WP != m_Baseline_Id_WP) ATH_CHECK(initializeTriggerSFTools(m_BaselineTrig_SF_Tools, false));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYElectronSelector::initializeTriggerSFTools(TrigSFTool_Map& map, bool use_signal) {
        for (auto& SF : m_TriggerSFConf) {
            if (map.find(SF) != map.end()) {
                ATH_MSG_DEBUG("Configuration " << SF << " has already been created by SUSYTools");
                continue;
            }
            asg::AnaToolHandle<EleEffTool> NewTool("AsgElectronEfficiencyCorrectionTool/ElectronTriggerSF " + Trig_EG_WP(SF, use_signal) +
                                                   std::to_string(map.size()));
            std::string trig_sf_key = SF.substr(0, std::min(SF.find(":"), SF.find(";")));
            ATH_MSG_INFO("Setup new SF tool using " << trig_sf_key << " as config with " << Trig_EG_WP(SF, use_signal) << " and isolation "
                                                    << Trig_Iso_WP(SF, true) << ".");
            ATH_CHECK(NewTool.setProperty("MapFilePath", m_EfficiencyMap));
            ATH_CHECK(NewTool.setProperty("IdKey", Trig_EG_WP(SF, use_signal)));
            ATH_CHECK(NewTool.setProperty("IsoKey", Trig_Iso_WP(SF, use_signal)));
            ATH_CHECK(NewTool.setProperty("TriggerKey", trig_sf_key));
            ATH_CHECK(NewTool.setProperty("CorrelationModel", m_CorrelationModel));

            ATH_CHECK(NewTool.initialize());
            ATH_CHECK(NewTool.retrieve());
            map.insert(TrigSFTool(SF, NewTool.getHandle()));
            ATH_CHECK(DeclareAsWeightSyst(NewTool));
        }
        return StatusCode::SUCCESS;
    }
    std::string SUSYElectronSelector::FindBestSFTool(const TrigSFTool_Map& SFTools, const std::string& TriggerSF) {
        static const std::vector<std::string> del{"HLT_2015_2016", "HLT_2015_2017", "HLT_2015", "HLT_2016_2017",
                                                  "HLT_2016_2018", "HLT_2016",      "HLT_2017", "HLT_2018"};
        std::string Config;

        std::vector<std::string> ExtractedTriggers = SUSYToolsPtr()->GetTriggerOR(TriggerSF);
        unsigned int GlobalMatches = 0;
        for (auto& SFconfig : SFTools) {
            std::vector<std::string> Tokens15, Tokens16, Tokens17, Tokens18;
            GetTriggerTokens(SFconfig.first, Tokens15, Tokens16, Tokens17, Tokens18);
            std::vector<std::string> AllTokens;
            AllTokens.insert(AllTokens.end(), Tokens15.begin(), Tokens15.end());
            AllTokens.insert(AllTokens.end(), Tokens16.begin(), Tokens16.end());
            AllTokens.insert(AllTokens.end(), Tokens17.begin(), Tokens17.end());
            AllTokens.insert(AllTokens.end(), Tokens18.begin(), Tokens18.end());
            for (auto& T : AllTokens) {
                for (const auto& D : del) { T = ReplaceExpInString(T, D, "HLT"); }
            }
            // Match the trigger scale factor to the config token
            unsigned int Found = 0;
            for (auto& T : ExtractedTriggers) {
                if (IsInVector(T, AllTokens)) ++Found;
            }
            if (Found > GlobalMatches) {
                if (!Config.empty())
                    ATH_MSG_DEBUG("Found already " << Config << " matching to trigger SF expression " << TriggerSF
                                                   << ". Will replace it by " << SFconfig.first);
                Config = SFconfig.first;
                GlobalMatches = Found;
            }
        }
        if (!GlobalMatches) {
            // Check for dilepton triggers
            if (TriggerSF.find("2e") == 0 || TriggerSF.find("_2e") != std::string::npos) {
                std::string DiTriggerSF = ReplaceExpInString(TriggerSF, "_2e", "_e");
                if (DiTriggerSF.find("2e") == 0) DiTriggerSF = DiTriggerSF.substr(1, std::string::npos);
                return FindBestSFTool(SFTools, DiTriggerSF);
            }
        }
        return Config;
    }
    StatusCode SUSYElectronSelector::initializeTriggerScaleFactors(TrigSFTool_Map& SFTools, ElectronWeight_VectorMap& SFs,
                                                                   unsigned int content) {
        for (auto& syst_set : m_systematics->GetWeightSystematics(ObjectType())) {
            for (auto& Trigger : m_TriggerExp) {
                const std::string BestTool = FindBestSFTool(SFTools, Trigger);
                if (BestTool.empty()) {
                    ATH_MSG_FATAL("Could not find an appropiate tool for trigger " << Trigger);
                    return StatusCode::FAILURE;
                }
                EleEffToolHandle& ToolToSelect = SFTools.at(BestTool);
                // Assume that all trigger sfs are affected by the same
                // systematics
                if (!XAMPP::ToolIsAffectedBySystematic(ToolToSelect, syst_set)) break;
                if (SFs.find(syst_set) == SFs.end()) {
                    SFs.insert(std::pair<const CP::SystematicSet*, ElectronWeight_Vector>(syst_set, ElectronWeight_Vector()));
                }
                ElectronWeight_Ptr Weight(new ElectronTriggerSFHandler(Trigger, m_trigger_tool, ToolToSelect));
                const std::string sf_type = "Trig" + (m_StoreMultiTrigSF ? "_" + Trigger : "");
                ATH_CHECK(initIParticleWeight(*Weight, sf_type, syst_set, content, m_StoreMultiTrigSF || m_SeparateSF));
                ATH_CHECK(Weight->initialize());
                SFs.at(syst_set).push_back(Weight);
            }
        }
        return StatusCode::SUCCESS;
    }

    void SUSYElectronSelector::GetTriggerTokens(std::string trigExpr, std::vector<std::string>& v_trigs15_cache,
                                                std::vector<std::string>& v_trigs16_cache, std::vector<std::string>& v_trigs17_cache,
                                                std::vector<std::string>& v_trigs18_cache) const {
        // e.g.
        // SINGLE_E_2015_e24_lhmedium_L1EM20VH_OR_e60_lhmedium_OR_e120_lhloose_2016_2018_e26_lhtight_nod0_ivarloose_OR_e60_lhmedium_nod0_OR_e140_lhloose_nod0

        static std::string del15 = "_2015_";
        static std::string del16 = "_2016_";
        static std::string del17 = "_2017_";
        static std::string del18 = "_2018_";

        size_t pos = 0;
        std::string token15, token16, token17, token18;

        // get trigger tokens for 2015, 2016, 2017, and 2018
        if ((pos = trigExpr.find(del15)) != std::string::npos) {
            trigExpr.erase(0, pos + del15.length());

            pos = 0;
            while ((pos = trigExpr.find(del16)) != std::string::npos) {
                token15 = trigExpr.substr(0, pos);
                token16 = trigExpr.erase(0, pos + del16.length() + del17.length() - 1);
                // 2016-2018 use exact the same trigger string
                token17 = token16;
                token18 = token16;
            }
        }

        // redefine in case of custom user input
        if (token15.empty()) token15 = trigExpr;

        if (token16.empty()) token16 = trigExpr;

        if (token17.empty()) token17 = trigExpr;

        if (token18.empty()) token18 = trigExpr;

        // get trigger chains for matching in 2015 and 2016
        v_trigs15_cache = SUSYToolsPtr()->GetTriggerOR(token15);
        v_trigs16_cache = SUSYToolsPtr()->GetTriggerOR(token16);
        v_trigs17_cache = SUSYToolsPtr()->GetTriggerOR(token17);
        v_trigs18_cache = SUSYToolsPtr()->GetTriggerOR(token18);
    }

    void SUSYElectronSelector::setupDecorations(std::shared_ptr<ElectronDecorations> input) {
        m_StoreTruthClassifier = m_StoreTruthClassifier && !isData();
        // if user-supplied arg exists, place this as our decoration object
        if (input) { m_electronDecorations = input; }
        // if nothing in place yet, fill it here
        if (!m_electronDecorations) { m_electronDecorations = std::make_shared<ElectronDecorations>(); }
        // sync up the particle level decorations and apply any properties to them
        ParticleSelector::setupDecorations(m_electronDecorations);

        // here we can configure elec-specific decorations
        // so far, nothing electron specific is configurable...
    }

    //##########################################################################
    //                            ElectronWeightDecorator
    //##########################################################################
    ElectronWeightDecorator::ElectronWeightDecorator() : IPartilceWeightDecorator() {}
    ElectronWeightDecorator::~ElectronWeightDecorator() {}
    StatusCode ElectronWeightDecorator::saveSF(const xAOD::Electron& Electron, bool isSignal) {
        double SF = 1.;
        if (!isSFcalculated(Electron)) {
            if (!calculateSF(Electron, SF).isSuccess()) return StatusCode::FAILURE;
        } else
            SF = getSF(Electron);
        return saveEventSF(Electron, SF, isSignal);
    }
    StatusCode ElectronWeightDecorator::initialize() { return StatusCode::SUCCESS; }
    //##########################################################################
    //                            ElectronWeight
    //##########################################################################
    ElectronWeight::ElectronWeight(EleEffToolHandle& SFTool) : ElectronWeightDecorator(), m_SFTool(SFTool) {}
    ElectronWeight::~ElectronWeight() {}
    StatusCode ElectronWeight::calculateSF(const xAOD::Electron& Electron, double& SF) {
        if (m_SFTool->getEfficiencyScaleFactor(Electron, SF) == CP::CorrectionCode::Error) return StatusCode::FAILURE;
        return StatusCode::SUCCESS;
    }
    //##########################################################################
    //                            ElectronWeightHandler
    //##########################################################################
    ElectronWeightHandler::ElectronWeightHandler(const CP::SystematicSet* syst_set) :
        ElectronWeightDecorator(),
        m_Syst(syst_set),
        m_Weights(),
        m_init(false),
        m_signal_trig_SF(),
        m_multiple_trig_sf(false) {}
    StatusCode ElectronWeightHandler::applySF() {
        if (!m_init) {
            Error("ElectronWeightHandler()", "Where should I get my SF from?");
            return StatusCode::FAILURE;
        }
        for (auto& W : m_Weights) {
            if (!W->applySF().isSuccess()) return StatusCode::FAILURE;
        }
        for (auto& W : m_signal_trig_SF) {
            if (!W->applySF().isSuccess()) return StatusCode::FAILURE;
        }
        return ElectronWeightDecorator::applySF();
    }
    void ElectronWeightHandler::multipleTriggerSF(bool B) { m_multiple_trig_sf = B; }
    const CP::SystematicSet* ElectronWeightHandler::systematic() const { return m_Syst; }
    size_t ElectronWeightHandler::nWeights() const { return m_Weights.size(); }
    StatusCode ElectronWeightHandler::calculateSF(const xAOD::Electron& Electron, double& SF) {
        for (auto& W : m_Weights) { SF *= W->getSF(Electron); }
        return StatusCode::SUCCESS;
    }
    StatusCode ElectronWeightHandler::saveSF(const xAOD::Electron& Electron, bool isSignal) {
        for (auto& W : m_Weights) {
            if (!W->saveSF(Electron, isSignal).isSuccess()) return StatusCode::FAILURE;
        }
        for (auto& W : m_signal_trig_SF) {
            if (!W->saveSF(Electron, isSignal).isSuccess()) return StatusCode::FAILURE;
        }
        // Weighters are emptied for multiple trigger sf
        // on systematics
        if (m_Weights.empty()) return StatusCode::SUCCESS;
        return ElectronWeightDecorator::saveSF(Electron, isSignal);
    }
    bool ElectronWeightHandler::append(const ElectronWeightMap& map, const CP::SystematicSet* nominal) {
        ElectronWeightMap::const_iterator Itr = map.find(systematic());
        if (Itr != map.end()) {
            if (!IsInVector(Itr->second, m_Weights)) m_Weights.push_back(Itr->second);
            m_init = true;
        } else {
            Itr = map.find(nominal);
            if (Itr != map.end() && !IsInVector(Itr->second, m_Weights)) m_Weights.push_back(Itr->second);
        }
        return m_init;
    }
    bool ElectronWeightHandler::setSignalTriggerSF(const ElectronWeight_VectorMap& map, const CP::SystematicSet* nominal) {
        ElectronWeight_VectorMap::const_iterator Itr = map.find(systematic());
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
    //                                  ElectronTriggerSFHandler
    //#################################################################################################
    ElectronTriggerSFHandler::ElectronTriggerSFHandler(const std::string& Trigger, const ToolHandle<ITriggerTool>& triggerTool,
                                                       EleEffToolHandle& TriggerSF) :
        m_TrigStr(Trigger),
        m_TriggerSFTool(TriggerSF),
        m_trig_tool(triggerTool),
        m_Triggers() {}
    bool ElectronTriggerSFHandler::RetrieveMatchers(const std::string& Trigger) {
        std::shared_ptr<TriggerInterface> iface = m_trig_tool->GetActiveTrigger(Trigger);
        if (!iface) return false;
        m_Triggers.push_back(iface);
        return true;
    }
    StatusCode ElectronTriggerSFHandler::initialize() {
        std::vector<std::string> SingleT = m_trig_tool->GetTriggerOR(m_TrigStr);
        for (auto& T : SingleT) {
            if (!RetrieveMatchers(T)) {
                // Renaming of dilepton level 1 items
                // Currently no scale-factors are provided for lhmedium -> lhloose as fall-back
                static const std::vector<int> L1_items{10, 15};
                for (const auto& L1 : L1_items) {
                    if ((T.find(Form("L1EM%d", L1)) != std::string::npos &&
                         !(RetrieveMatchers(ReplaceExpInString(T, Form("L12EM%d", L1), Form("L1EM%d", L1))) ||

                           RetrieveMatchers(ReplaceExpInString(ReplaceExpInString(T, "lhmedium", "lhloose"), Form("L1EM%d", L1),
                                                               Form("L12EM%d", L1))))) ||
                        (T.find("lhmedium") != std::string::npos && !RetrieveMatchers(ReplaceExpInString(T, "lhmedium", "lhloose")))) {
                        Warning("ElectronTriggerSFHandler()", "Could not find the trigger %s, but you want to apply a SF on it", T.c_str());
                    }
                }
            }
        }
        if (m_Triggers.empty()) {
            Error("ElectronTriggerSFHandler()", "None of the triggers you gave is part of the trigger selection %s", m_TrigStr.c_str());
            Error("ElectronTriggerSFHandler()", "Naively one would expect at least one of the following triggers");
            for (auto T : SingleT) { Error("ElectronTriggerSFHandler()", " *** %s", T.c_str()); }
            return StatusCode::FAILURE;
        }
        return StatusCode::SUCCESS;
    }
    StatusCode ElectronTriggerSFHandler::calculateSF(const xAOD::Electron& El, double& SF) {
        // Check if the trigger has fired at all and if it is already matched
        if (IsTriggerMachted(El)) {
            if (m_TriggerSFTool->getEfficiencyScaleFactor(El, SF) == CP::CorrectionCode::Error) { return StatusCode::FAILURE; }
        }
        return StatusCode::SUCCESS;
    }
    ElectronTriggerSFHandler::~ElectronTriggerSFHandler() {}
    bool ElectronTriggerSFHandler::IsTriggerMachted(const xAOD::Electron& El) const {
        for (auto& T : m_Triggers) {
            if (!T->PassTrigger()) continue;
            if (T->isMatched(El)) return true;
        }
        return false;
    }
}  // namespace XAMPP
