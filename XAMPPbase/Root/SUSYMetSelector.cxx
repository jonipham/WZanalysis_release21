#include <METInterface/IMETMaker.h>
#include <METInterface/IMETSystematicsTool.h>
#include <METUtilities/METSignificance.h>
#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/Defs.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/IElectronSelector.h>
#include <XAMPPbase/IJetSelector.h>
#include <XAMPPbase/IMuonSelector.h>
#include <XAMPPbase/IPhotonSelector.h>
#include <XAMPPbase/ISystematics.h>
#include <XAMPPbase/ITauSelector.h>
#include <XAMPPbase/SUSYMetSelector.h>
#include <xAODJet/JetContainerInfo.h>
#include <functional>
namespace XAMPP {
    //##################################################################
    //                   MetSignificanceHandler
    //##################################################################
    MetSignificanceHandler::MetSignificanceHandler(const std::string& grp_name, SUSYMetSelector* MetSelector,
                                                   const std::string& description, bool isEnabled) :
        m_grp_name(grp_name),
        m_met_selector(MetSelector),
        m_init(false),
        m_enabled(isEnabled),
        m_use_track_term(true),
        m_metSignif(),
        m_event_info_handle(),
        m_propertySet(false),
        m_met_container(nullptr),
        m_Significance(nullptr),
        m_Significance_Rho(nullptr),
        m_Significance_VarL(nullptr),
        m_store_sqrt_vars(false),
        m_OverSqrtSumET(nullptr),
        m_OverSqrtHT(nullptr) {
        m_metSignif.declarePropertyFor(MetSelector, std::string("MetSignificanceTool") + std::string(group().empty() ? "" : "_") + group(),
                                       description);
        MetSelector->declareProperty(std::string("DoMetSign") + std::string(group().empty() ? "" : "_") + group(), m_enabled);
    }
    void MetSignificanceHandler::setMissingEt(xAOD::MissingETContainer*& Met) { m_met_container = &Met; }
    void MetSignificanceHandler::storeSqrtVariables(bool B) {
        if (!isInitialized()) m_store_sqrt_vars = B;
    }
    void MetSignificanceHandler::useSofTrackTerm(bool B) {
        if (!isInitialized()) m_use_track_term = B;
    }

    StatusCode MetSignificanceHandler::initialize() {
        if (isInitialized()) {
            Warning("MetSignificanceHandler()", "%s met significance has already been initialized", group().c_str());
            return StatusCode::SUCCESS;
        }
        if (isDisabled()) {
            Error("MetSignificanceHandler()", "%s met significance is disabled", group().c_str());
            return StatusCode::FAILURE;
        }
        // properties were set from outside. So we need to set common properties
        if (m_propertySet) {
            // Here we need may-be some tweaking if we want to be in
            // XAOD_STANDALONE
            ToolHandle<ST::ISUSYObjDef_xAODTool> susytools_handle(m_met_selector->getProperty("SUSYTools").toString());
            ST::SUSYObjDef_xAOD* ST_Ptr = dynamic_cast<ST::SUSYObjDef_xAOD*>(susytools_handle.operator->());

            if (!m_metSignif.setProperty("IsAFII", ST_Ptr->isAtlfast()).isSuccess()) return StatusCode::FAILURE;
        }
        // try to retrieve the tool
        if (!m_metSignif.retrieve().isSuccess()) return StatusCode::FAILURE;

        // Here we need may-be some tweaking if we want to be in XAOD_STANDALONE
        m_event_info_handle = ToolHandle<XAMPP::IEventInfo>(m_met_selector->getProperty("EventInfoHandler").toString());
        XAMPP::EventInfo* XAMPPInfo = dynamic_cast<XAMPP::EventInfo*>(m_event_info_handle.operator->());

        // check if the given MET is valid
        if (m_met_container == nullptr) {
            Error("MetSignificanceHandler()", "No met has been given to %s", group().c_str());
            return StatusCode::FAILURE;
        }

        if (!XAMPPInfo->NewEventVariable<float>("MetTST_Significance" + std::string(group().empty() ? "" : "_") + group()).isSuccess())
            return StatusCode::FAILURE;
        else if (!XAMPPInfo->NewEventVariable<float>("MetTST_Significance" + std::string(group().empty() ? "" : "_") + group() + "_Rho")
                      .isSuccess())
            return StatusCode::FAILURE;
        else if (!XAMPPInfo->NewEventVariable<float>("MetTST_Significance" + std::string(group().empty() ? "" : "_") + group() + "_VarL")
                      .isSuccess())
            return StatusCode::FAILURE;

        m_Significance = XAMPPInfo->GetVariableStorage<float>("MetTST_Significance" + std::string(group().empty() ? "" : "_") + group());
        m_Significance_Rho =
            XAMPPInfo->GetVariableStorage<float>("MetTST_Significance" + std::string(group().empty() ? "" : "_") + group() + "_Rho");
        m_Significance_VarL =
            XAMPPInfo->GetVariableStorage<float>("MetTST_Significance" + std::string(group().empty() ? "" : "_") + group() + "_VarL");

        if (m_store_sqrt_vars) {
            if (!XAMPPInfo->NewEventVariable<float>("MetTST_OverSqrtSumET" + std::string(group().empty() ? "" : "_") + group()).isSuccess())
                return StatusCode::FAILURE;
            if (!XAMPPInfo->NewEventVariable<float>("MetTST_OverSqrtHT" + std::string(group().empty() ? "" : "_") + group()).isSuccess())
                return StatusCode::FAILURE;
            m_OverSqrtSumET =
                XAMPPInfo->GetVariableStorage<float>("MetTST_OverSqrtSumET" + std::string(group().empty() ? "" : "_") + group());
            m_OverSqrtHT = XAMPPInfo->GetVariableStorage<float>("MetTST_OverSqrtHT" + std::string(group().empty() ? "" : "_") + group());
        }
        m_init = true;
        return StatusCode::SUCCESS;
    }
    StatusCode MetSignificanceHandler::fill() {
        if (!isInitialized()) {
            Error("MetSignificanceHandler()", "%s is not initialized", group().c_str());
            return StatusCode::FAILURE;
        }
        if (!m_metSignif
                 ->varianceMET(*m_met_container, m_event_info_handle->GetEventInfo()->averageInteractionsPerCrossing(),
                               m_met_selector->referenceTerm(xAOD::Type::ObjectType::Jet),
                               m_use_track_term ? m_met_selector->softTrackTerm() : m_met_selector->softCaloTerm(),
                               m_met_selector->FinalMetTerm())
                 .isSuccess())
            return StatusCode::FAILURE;

        else if (!m_Significance->Store(m_metSignif->GetSignificance()).isSuccess())
            return StatusCode::FAILURE;
        else if (!m_Significance_Rho->Store(m_metSignif->GetRho()).isSuccess())
            return StatusCode::FAILURE;
        else if (!m_Significance_VarL->Store(m_metSignif->GetVarL()).isSuccess())
            return StatusCode::FAILURE;

        // The sqrt variables shall not be stored
        if (!m_store_sqrt_vars)
            return StatusCode::SUCCESS;
        else if (!m_OverSqrtSumET->Store(m_metSignif->GetMETOverSqrtSumET()).isSuccess())
            return StatusCode::FAILURE;
        else if (!m_OverSqrtHT->Store(m_metSignif->GetMETOverSqrtHT()).isSuccess())
            return StatusCode::FAILURE;

        return StatusCode::SUCCESS;
    }
    bool MetSignificanceHandler::isInitialized() const { return m_init; }
    bool MetSignificanceHandler::isDisabled() const { return !m_enabled; }
    bool MetSignificanceHandler::isUserConfigured() const { return m_metSignif.isUserConfigured(); }
    std::string MetSignificanceHandler::group() const { return m_grp_name; }
    //#######################################
    //          SUSYMetSelector
    //#######################################
    SUSYMetSelector::SUSYMetSelector(const std::string& myname) :
        AsgTool(myname),
        m_XAMPPInfo(),
        m_susytools("SUSYTools"),
        m_InfoHandle("EventInfoHandler"),
        m_metSignifHandlers(),
        m_metSignif(nullptr),
        m_metSignif_noPUJets(nullptr),
        m_metSignif_noPUJets_noSoftTerm(nullptr),
        m_metSignif_dataJER(nullptr),
        m_metSignif_dataJER_noPUJets(nullptr),
        m_metSignif_phireso_noPUJets(nullptr),
        m_init(false),
        m_doTrackMet(false),
        m_doMetCST(false),
        m_IncludePhotons(false),
        m_IncludeTaus(false),
        m_elec_selection("SUSYElectronSelector"),
        m_muon_selection("SUSYMuonSelector"),
        m_jet_selection("SUSYJetSelector"),
        m_phot_selection("SUSYPhotonSelector"),
        m_tau_selection("SUSYTauSelector"),
        m_systematics("SystematicsTool"),
        m_MetTST(nullptr),
        m_MetCST(nullptr),
        m_MetTrack(nullptr),
        m_sto_MetTST(nullptr),
        m_sto_MetCST(nullptr),
        m_sto_MetTrack(nullptr),
        m_xAODMet(nullptr),
        m_xAODMap(nullptr),
        m_xAODTrackMet(nullptr),
        m_met_key(""),
        m_met_map_key(""),
        m_met_track_key(""),
        m_systName(),
        m_store_significance(false),
        m_metMaker(""),
        m_metSystTool(""),
        m_EleRefTerm(""),
        m_MuoRefTerm(""),
        m_TauRefTerm(""),
        m_JetRefTerm(""),
        m_PhoRefTerm(""),
        m_TrackSoftTerm("PVSoftTrk"),
        m_CaloSoftTerm("SoftClus"),
        m_FinalMetTerm("Final"),
        m_FinalTrackTerm("Track"),
        m_trkJetsyst(false),
        m_trkMETsyst(false) {
        declareProperty("DoTrackMet", m_doTrackMet);
        declareProperty("DoMetCST", m_doMetCST);
        declareProperty("IncludeTaus", m_IncludeTaus);
        declareProperty("IncludePhotons", m_IncludePhotons);
        declareProperty("MetContainer", m_met_key);
        declareProperty("TrackMetContainer", m_met_track_key);
        declareProperty("MetAssociationMap", m_met_map_key);

        declareProperty("WriteMetSignificance", m_store_significance);

        m_susytools.declarePropertyFor(this, "SUSYTools", "The SUSYTools instance");
        m_InfoHandle.declarePropertyFor(this, "EventInfoHandler", "The XAMPP EventInfo handle");

        // Properties to retrieve the MET maker. If not set we'll use the ones
        // from SUSYTools Usually the user shall not bother with this stuff
        declareProperty("METMaker", m_metMaker, "The METMaker instance");
        declareProperty("METSystTool", m_metSystTool, "The METSystematicsTool instance");
        declareProperty("ElectronSoftTerm", m_EleRefTerm);
        declareProperty("MuonSoftTerm", m_MuoRefTerm);
        declareProperty("TauSoftTerm", m_TauRefTerm);
        declareProperty("JetSoftTerm", m_JetRefTerm);
        declareProperty("PhotonSoftTerm", m_PhoRefTerm);

        declareProperty("TrackSoftTerm", m_TrackSoftTerm);
        declareProperty("CaloSoftTerm", m_CaloSoftTerm);
        declareProperty("FinalMET", m_FinalMetTerm);
        declareProperty("FinalTrackMET", m_FinalTrackTerm);

        declareProperty("doTrkJetSyst", m_trkJetsyst);
        declareProperty("doTrkSyst", m_trkMETsyst);

        m_metSignif = newSignificanceTool("");
        m_metSignif->storeSqrtVariables(true);
        m_metSignif_noPUJets_noSoftTerm = newSignificanceTool("noPUJets_noSoftTerm", true);

        m_metSignif_dataJER = newSignificanceTool("dataJER", false);
        m_metSignif_noPUJets = newSignificanceTool("noPUJets", false);
        m_metSignif_dataJER_noPUJets = newSignificanceTool("dataJER_noPUJets", false);
        m_metSignif_phireso_noPUJets = newSignificanceTool("phireso_noPUJets", true);
    }
    MetSignificanceHandler_Ptr SUSYMetSelector::newSignificanceTool(const std::string& grp, bool disabled) {
        if (isInitialized()) {
            ATH_MSG_ERROR("The met selector has already been initialized");
            return MetSignificanceHandler_Ptr();
        }
        MetSignificanceHandler_Ptr tool = std::make_shared<MetSignificanceHandler>(grp, this, "", disabled);
        tool->setMissingEt(m_MetTST);
        m_metSignifHandlers.push_back(tool);
        return tool;
    }

    StatusCode SUSYMetSelector::initialize() {
        if (isInitialized()) { return StatusCode::SUCCESS; }

        ATH_CHECK(m_systematics.retrieve());
        if (!m_systematics->ProcessObject(XAMPP::SelectionObject::MissingET)) {
            ATH_MSG_INFO("Do not process the missing ET...");
            m_init = true;
            return StatusCode::SUCCESS;
        }

        ATH_MSG_INFO("Initialising...");

        // check if we really, really need SUSYTools
        if (m_EleRefTerm.empty() || m_MuoRefTerm.empty() || m_TauRefTerm.empty() || m_PhoRefTerm.empty() || m_JetRefTerm.empty() ||
            m_FinalMetTerm.empty() || !m_metMaker.isSet() || !m_metSystTool.isSet()) {
            ATH_CHECK(m_susytools.retrieve());
        }

        //
        //  Retrieve all selectors
        //
        ATH_CHECK(m_elec_selection.retrieve());
        ATH_CHECK(m_jet_selection.retrieve());
        ATH_CHECK(m_muon_selection.retrieve());
        ATH_CHECK(m_phot_selection.retrieve());
        ATH_CHECK(m_tau_selection.retrieve());

        //
        // Obtain configuration of the terms from ST if not already set
        //
        if (m_EleRefTerm.empty()) m_EleRefTerm = GetProperty<std::string>("METEleTerm", m_susytools);
        if (m_MuoRefTerm.empty()) m_MuoRefTerm = GetProperty<std::string>("METMuonTerm", m_susytools);
        if (m_TauRefTerm.empty()) m_TauRefTerm = GetProperty<std::string>("METTauTerm", m_susytools);
        if (m_PhoRefTerm.empty()) m_PhoRefTerm = GetProperty<std::string>("METGammaTerm", m_susytools);
        if (m_JetRefTerm.empty()) m_JetRefTerm = GetProperty<std::string>("METJetTerm", m_susytools);
        if (m_FinalMetTerm.empty()) m_FinalMetTerm = GetProperty<std::string>("METOutputTerm", m_susytools);

        if (!m_metMaker.isSet()) m_metMaker = GetCPTool<IMETMaker>("METMaker");
        if (!m_metSystTool.isSet()) {
            m_metSystTool = GetCPTool<IMETSystematicsTool>("METSystTool");
            m_trkJetsyst = GetProperty<bool>("METDoTrkJetSyst", m_susytools);
            m_trkMETsyst = GetProperty<bool>("METDoTrkSyst", m_susytools);
        }
        if (m_met_key.empty() || m_met_map_key.empty()) {
            int JetCollectionType = GetProperty<int>("JetCollectionType", m_jet_selection);
            m_met_key = "MET_Core_AntiKt4" + xAOD::JetInput::typeName(xAOD::JetInput::Type(JetCollectionType));
            m_met_map_key = "METAssoc_AntiKt4" + xAOD::JetInput::typeName(xAOD::JetInput::Type(JetCollectionType));
            // Thus Far the TrackMet is built analogusly to the MET.
            if (m_met_track_key.empty()) m_met_track_key = m_met_key;
        }

        ATH_CHECK(m_InfoHandle.retrieve());
        m_XAMPPInfo = dynamic_cast<XAMPP::EventInfo*>(m_InfoHandle.operator->());
        // Create the variables
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<XAMPPmet>("MetTST"));
        m_sto_MetTST = m_XAMPPInfo->GetVariableStorage<XAMPPmet>("MetTST");
        if (m_doMetCST) {
            ATH_CHECK(m_XAMPPInfo->NewEventVariable<XAMPPmet>("MetCST"));
            m_sto_MetCST = m_XAMPPInfo->GetVariableStorage<XAMPPmet>("MetCST");
        }
        if (m_doTrackMet) {
            ATH_CHECK(m_XAMPPInfo->NewEventVariable<XAMPPmet>("MetTrack"));
            m_sto_MetTrack = m_XAMPPInfo->GetVariableStorage<XAMPPmet>("MetTrack");
        }

        if (m_store_significance) {
            EraseFromVector(m_metSignifHandlers, std::function<bool(const MetSignificanceHandler_Ptr&)>(
                                                     [](const MetSignificanceHandler_Ptr& obj) { return obj->isDisabled(); }));
            ATH_CHECK(initializeMetSignificance());
            for (const auto& sign : m_metSignifHandlers) { ATH_CHECK(sign->initialize()); }
        } else
            m_metSignifHandlers.clear();
        m_init = true;

        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMetSelector::initializeMetSignificance() {
        if (!m_metSignif->isDisabled() && !m_metSignif->isUserConfigured()) {
            ATH_CHECK(m_metSignif->setProperty("SoftTermParam", 0));
            ATH_CHECK(m_metSignif->setProperty("TreatPUJets", true));
            ATH_CHECK(m_metSignif->setProperty("IsDataJet", false));
        }
        if (!m_metSignif_noPUJets->isDisabled() && !m_metSignif_noPUJets->isUserConfigured()) {
            ATH_CHECK(m_metSignif_noPUJets->setProperty("SoftTermParam", 0));
            ATH_CHECK(m_metSignif_noPUJets->setProperty("TreatPUJets", false));
            ATH_CHECK(m_metSignif_noPUJets->setProperty("IsDataJet", false));
        }
        if (!m_metSignif_noPUJets_noSoftTerm->isDisabled() && !m_metSignif_noPUJets_noSoftTerm->isUserConfigured()) {
            ATH_CHECK(m_metSignif_noPUJets_noSoftTerm->setProperty("SoftTermParam", 0));
            ATH_CHECK(m_metSignif_noPUJets_noSoftTerm->setProperty("TreatPUJets", false));
            ATH_CHECK(m_metSignif_noPUJets_noSoftTerm->setProperty("SoftTermReso", 0));
            ATH_CHECK(m_metSignif_noPUJets_noSoftTerm->setProperty("IsDataJet", false));
        }
        if (!m_metSignif_dataJER->isDisabled() && !m_metSignif_dataJER->isUserConfigured()) {
            ATH_CHECK(m_metSignif_dataJER->setProperty("SoftTermParam", 0));
            ATH_CHECK(m_metSignif_dataJER->setProperty("TreatPUJets", true));
            ATH_CHECK(m_metSignif_dataJER->setProperty("IsDataJet", true));
        }
        if (!m_metSignif_dataJER_noPUJets->isDisabled() && !m_metSignif_dataJER_noPUJets->isUserConfigured()) {
            ATH_CHECK(m_metSignif_dataJER_noPUJets->setProperty("SoftTermParam", 0));
            ATH_CHECK(m_metSignif_dataJER_noPUJets->setProperty("TreatPUJets", false));
            ATH_CHECK(m_metSignif_dataJER_noPUJets->setProperty("IsDataJet", true));
        }
        if (!m_metSignif_phireso_noPUJets->isDisabled() && !m_metSignif_phireso_noPUJets->isUserConfigured()) {
            ATH_CHECK(m_metSignif_phireso_noPUJets->setProperty("SoftTermParam", 0));
            ATH_CHECK(m_metSignif_phireso_noPUJets->setProperty("TreatPUJets", false));
            ATH_CHECK(m_metSignif_phireso_noPUJets->setProperty("IsDataJet", false));
            ATH_CHECK(m_metSignif_phireso_noPUJets->setProperty("DoPhiReso", true));
        }
        return StatusCode::SUCCESS;
    }
    bool SUSYMetSelector::isInitialized() const { return m_init; }
    StatusCode SUSYMetSelector::LoadContainers() {
        if (!m_systematics->ProcessObject(XAMPP::SelectionObject::MissingET)) {
            ATH_MSG_DEBUG("Do not process the missing ET...");
            return StatusCode::SUCCESS;
        }
        ATH_CHECK(evtStore()->retrieve(m_xAODMet, m_met_key));
        ATH_CHECK(evtStore()->retrieve(m_xAODMap, m_met_map_key));

        if (m_doTrackMet) ATH_CHECK(evtStore()->retrieve(m_xAODTrackMet, m_met_track_key));
        return StatusCode::SUCCESS;
    }
    xAOD::MissingETContainer* SUSYMetSelector::GetCustomMet(const std::string& kind) const {
        if (kind.empty()) return m_MetTST;
        const std::string ContainerName = storeName(kind);
        if (evtStore()->contains<xAOD::MissingETContainer>(ContainerName)) {
            xAOD::MissingETContainer* customMet = nullptr;
            if (evtStore()->retrieve(customMet, ContainerName).isSuccess()) return customMet;
        }
        return m_MetTST;
    }
    std::string SUSYMetSelector::storeName(const std::string& container) const { return name() + container + m_systName; }
    StatusCode SUSYMetSelector::SetSystematic(const CP::SystematicSet& systset) {
        m_systName = systset.name();
        ATH_CHECK(m_XAMPPInfo->SetSystematic(&systset));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMetSelector::CreateContainer(const std::string& Name, xAOD::MissingETContainer*& Cont) {
        Cont = new xAOD::MissingETContainer;
        xAOD::MissingETAuxContainer* Aux = new xAOD::MissingETAuxContainer;
        Cont->setStore(Aux);
        const std::string store_name = storeName(Name);
        ATH_CHECK(evtStore()->record(Cont, store_name));
        ATH_CHECK(evtStore()->record(Aux, store_name + "Aux."));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMetSelector::FillMet(const CP::SystematicSet& systset) {
        if (!m_systematics->ProcessObject(XAMPP::SelectionObject::MissingET)) {
            ATH_MSG_DEBUG("Do not process the missing ET...");
            return StatusCode::SUCCESS;
        }
        ATH_CHECK(SetSystematic(systset));
        // pass the taus and photons via
        xAOD::TauJetContainer* MetTaus = IncludeTaus() ? m_tau_selection->GetPreTaus() : nullptr;
        xAOD::PhotonContainer* MetPhotons = IncludePhotons() ? m_phot_selection->GetPrePhotons() : nullptr;
        ATH_CHECK(buildMET(m_MetTST, "TST", softTrackTerm(), m_elec_selection->GetPreElectrons(), m_muon_selection->GetPreMuons(), MetTaus,
                           MetPhotons, true));
        ATH_CHECK(m_sto_MetTST->Store(XAMPP::GetMET_obj(m_FinalMetTerm, m_MetTST)));
        if (m_doMetCST) {
            ATH_CHECK(buildMET(m_MetCST, "CST", softCaloTerm(), m_elec_selection->GetPreElectrons(), m_muon_selection->GetPreMuons(),
                               MetTaus, MetPhotons, false));
            ATH_CHECK(m_sto_MetCST->Store(XAMPP::GetMET_obj(m_FinalMetTerm, m_MetCST)));
        }
        if (m_doTrackMet) {
            ATH_CHECK(buildTrackMET(m_MetTrack, "TrackMET", m_elec_selection->GetPreElectrons(), m_muon_selection->GetPreMuons()));
            ATH_CHECK(m_sto_MetTrack->Store(XAMPP::GetMET_obj(m_FinalTrackTerm, m_MetTrack)));
        }
        if (m_store_significance) {
            for (auto& sign : m_metSignifHandlers) { ATH_CHECK(sign->fill()); }
        }

        return StatusCode::SUCCESS;
    }

    StatusCode SUSYMetSelector::SaveScaleFactor() { return StatusCode::SUCCESS; }
    StatusCode SUSYMetSelector::addToInvisible(const xAOD::IParticleContainer* container, const std::string& invis_container) {
        return addToInvisible(*container, invis_container);
    }
    StatusCode SUSYMetSelector::addToInvisible(const xAOD::IParticleContainer& container, const std::string& invis_container) {
        ATH_CHECK(SetSystematic(*m_XAMPPInfo->GetSystematic()));
        xAOD::IParticleContainer* invisible_container = getInvisible(invis_container);
        if (invisible_container == nullptr) {
            invisible_container = new xAOD::IParticleContainer(SG::VIEW_ELEMENTS);
            ATH_CHECK(evtStore()->record(invisible_container, storeName(std::string(" InvisiblePart ") + invis_container)));
            ATH_MSG_DEBUG("Created invisible container for the invisible particles in " << invis_container << ".");
        }
        for (auto particle : container) {
            if (IsInContainer(particle, invisible_container)) {
                ATH_MSG_ERROR("The particle has already been added to the invisible met");
                PromptParticle(particle);
                return StatusCode::FAILURE;
            }
            invisible_container->push_back(particle);
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMetSelector::addToInvisible(xAOD::IParticle* particle, const std::string& invis_container) {
        xAOD::IParticleContainer container(SG::VIEW_ELEMENTS);
        container.push_back(particle);
        ATH_CHECK(addToInvisible(container, invis_container));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMetSelector::addToInvisible(xAOD::IParticle& particle, const std::string& invis_container) {
        return addToInvisible(&particle, invis_container);
    }
    xAOD::IParticleContainer* SUSYMetSelector::getInvisible(const std::string& invis_container) const {
        const std::string ContainerName = storeName(std::string(" InvisiblePart ") + invis_container);
        if (evtStore()->contains<xAOD::IParticleContainer>(ContainerName)) {
            xAOD::IParticleContainer* invisible = nullptr;
            evtStore()->retrieve(invisible, ContainerName).isSuccess();
            return invisible;
        }
        return nullptr;
    }
    std::string SUSYMetSelector::referenceTerm(xAOD::Type::ObjectType Type) const {
        if (Type == xAOD::Type::ObjectType::Electron)
            return m_EleRefTerm;
        else if (Type == xAOD::Type::ObjectType::Muon)
            return m_MuoRefTerm;
        else if (Type == xAOD::Type::ObjectType::Tau)
            return m_TauRefTerm;
        else if (Type == xAOD::Type::ObjectType::Jet)
            return m_JetRefTerm;
        else if (Type == xAOD::Type::ObjectType::Photon)
            return m_PhoRefTerm;
        ATH_MSG_ERROR(
            "You did not put the right selection object to this function. "
            "Dafuq? "
            "Think pink!!");
        return std::string("");
    }

    StatusCode SUSYMetSelector::addContainerToMet(xAOD::MissingETContainer* MET, const xAOD::IParticleContainer* particles,
                                                  xAOD::Type::ObjectType type, const xAOD::IParticleContainer* invisible) {
        std::string ref_term = referenceTerm(type);
        if (ref_term.empty()) return StatusCode::FAILURE;
        if (particles == nullptr) return StatusCode::SUCCESS;
        if (invisible == nullptr || invisible->empty()) { return m_metMaker->rebuildMET(ref_term, type, MET, particles, m_xAODMap); }
        xAOD::IParticleContainer invis_free(SG::VIEW_ELEMENTS);
        for (auto met_part : invis_free) {
            if (!IsInContainer(met_part, invisible)) invis_free.push_back(met_part);
        }
        ATH_CHECK(m_metMaker->rebuildMET(ref_term, type, MET, &invis_free, m_xAODMap));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMetSelector::markInvisible(xAOD::MissingETContainer* MET, const xAOD::IParticleContainer* invisible) {
        if (invisible != nullptr && !invisible->empty()) { ATH_CHECK(m_metMaker->markInvisible(invisible, m_xAODMap, MET)); }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYMetSelector::buildMET(xAOD::MissingETContainer* MET, const std::string& softTerm, bool doJvt,
                                         const xAOD::IParticleContainer* invisible, bool build_track) {
        if (m_JetRefTerm.empty()) return StatusCode::FAILURE;
        //
        //  No invisible container is given -> pass everything directly to the
        //  MET interface tool
        if (invisible == nullptr || invisible->empty()) {
            if (!build_track)
                ATH_CHECK(m_metMaker->rebuildJetMET(m_JetRefTerm, softTerm, MET, m_jet_selection->GetJets(), m_xAODMet, m_xAODMap, doJvt));
            else
                ATH_CHECK(
                    m_metMaker->rebuildTrackMET(m_JetRefTerm, softTerm, MET, m_jet_selection->GetJets(), m_xAODTrackMet, m_xAODMap, doJvt));
        } else {
            xAOD::JetContainer invis_free(SG::VIEW_ELEMENTS);
            for (auto jet : *m_jet_selection->GetJets()) {
                if (!IsInContainer(jet, invisible)) invis_free.push_back(jet);
            }
            if (!build_track)
                ATH_CHECK(m_metMaker->rebuildJetMET(m_JetRefTerm, softTerm, MET, &invis_free, m_xAODMet, m_xAODMap, doJvt));
            else
                ATH_CHECK(m_metMaker->rebuildTrackMET(m_JetRefTerm, softTerm, MET, &invis_free, m_xAODTrackMet, m_xAODMap, doJvt));
        }
        xAOD::MissingET* softTerm_MET = GetMET_obj(softTerm, MET);
        if (!m_systematics->isData() && m_systematics->AffectsOnlyMET(m_XAMPPInfo->GetSystematic())) {
            if (!build_track && m_metSystTool->applyCorrection(*softTerm_MET) != CP::CorrectionCode::Ok) {
                ATH_MSG_WARNING("Something went wrong with the systematics");
            } else if (build_track) {
                if (m_trkMETsyst && m_metSystTool->applyCorrection(*softTerm_MET, m_xAODMap) != CP::CorrectionCode::Ok) {
                    ATH_MSG_WARNING(
                        "GetMET: Failed to apply MET track (PVSoftTrk) "
                        "systematics.");
                }
                xAOD::MissingET* jetTerm_MET = GetMET_obj(m_JetRefTerm, MET);
                if (m_trkJetsyst && m_metSystTool->applyCorrection(*jetTerm_MET, m_xAODMap) != CP::CorrectionCode::Ok) {
                    ATH_MSG_WARNING(
                        "GetMET: Failed to apply MET track (RefJet) "
                        "systematics.");
                }
            }
        }
        ATH_CHECK(m_metMaker->buildMETSum(build_track ? m_FinalTrackTerm : m_FinalMetTerm, MET, softTerm_MET->source()));

        return StatusCode::SUCCESS;
    }
    bool SUSYMetSelector::IncludePhotons() const { return m_IncludePhotons; }
    bool SUSYMetSelector::IncludeTaus() const { return m_IncludeTaus; }
    ST::SUSYObjDef_xAOD* SUSYMetSelector::SUSYToolsPtr() {
        ST::SUSYObjDef_xAOD* ST = dynamic_cast<ST::SUSYObjDef_xAOD*>(m_susytools.operator->());
        return ST;
    }
    std::string SUSYMetSelector::softTrackTerm() const { return m_TrackSoftTerm; }

    std::string SUSYMetSelector::softCaloTerm() const { return m_CaloSoftTerm; }
    StatusCode SUSYMetSelector::buildMET(xAOD::MissingETContainer*& MET, const std::string& met_name, const std::string& soft_term,
                                         const xAOD::ElectronContainer* electrons, const xAOD::MuonContainer* muons,
                                         const xAOD::TauJetContainer* taus, const xAOD::PhotonContainer* photons, bool doJvt) {
        m_xAODMap->resetObjSelectionFlags();
        ATH_CHECK(CreateContainer(met_name, MET));
        const xAOD::IParticleContainer* invisible_container = getInvisible(met_name);
        ATH_CHECK(markInvisible(MET, invisible_container));
        ATH_CHECK(addContainerToMet(MET, electrons, xAOD::Type::ObjectType::Electron, invisible_container));
        ATH_CHECK(addContainerToMet(MET, muons, xAOD::Type::ObjectType::Muon, invisible_container));
        ATH_CHECK(addContainerToMet(MET, taus, xAOD::Type::ObjectType::Tau, invisible_container));
        ATH_CHECK(addContainerToMet(MET, photons, xAOD::Type::ObjectType::Photon, invisible_container));
        ATH_CHECK(buildMET(MET, soft_term, doJvt, invisible_container, false));
        return StatusCode::SUCCESS;
    }

    StatusCode SUSYMetSelector::buildTrackMET(xAOD::MissingETContainer*& MET, const std::string& met_name,
                                              const xAOD::ElectronContainer* electrons, const xAOD::MuonContainer* muons,
                                              const xAOD::TauJetContainer* taus, const xAOD::PhotonContainer* photons, bool doJvt) {
        m_xAODMap->resetObjSelectionFlags();
        ATH_CHECK(CreateContainer(met_name, MET));
        const xAOD::IParticleContainer* invisible_container = getInvisible(met_name);
        ATH_CHECK(markInvisible(MET, invisible_container));
        ATH_CHECK(addContainerToMet(MET, electrons, xAOD::Type::ObjectType::Electron, invisible_container));
        ATH_CHECK(addContainerToMet(MET, muons, xAOD::Type::ObjectType::Muon, invisible_container));
        ATH_CHECK(addContainerToMet(MET, taus, xAOD::Type::ObjectType::Tau, invisible_container));
        ATH_CHECK(addContainerToMet(MET, photons, xAOD::Type::ObjectType::Photon, invisible_container));
        ATH_CHECK(buildMET(MET, softTrackTerm(), doJvt, invisible_container, true));
        return StatusCode::SUCCESS;
    }
    std::string SUSYMetSelector::FinalMetTerm() const { return m_FinalMetTerm; }
    std::string SUSYMetSelector::FinalTrackMetTerm() const { return m_FinalTrackTerm; }

}  // namespace XAMPP
