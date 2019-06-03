#include <XAMPPbase/SUSYPhotonSelector.h>

#include <PhotonEfficiencyCorrection/AsgPhotonEfficiencyCorrectionTool.h>

namespace XAMPP {
    SUSYPhotonSelector::SUSYPhotonSelector(const std::string& myname) :
        SUSYParticleSelector(myname),
        m_xAODPhotons(nullptr),
        m_Photons(nullptr),
        m_PhotonsAux(nullptr),
        m_PrePhotons(nullptr),
        m_BaselinePhotons(nullptr),
        m_SignalPhotons(nullptr),
        m_SignalQualPhotons(nullptr),
        m_photonDecorations(),
        m_SeparateSF(false),
        m_SF(),
        m_doRecoSF(true),
        m_doIsoSF(true),
        m_requireIso_PreSelection(false),
        m_requireIso_Baseline(false),
        m_requireIso_Signal(false),
        m_writeBaselineSF(false) {
        declareProperty("SeparateSF", m_SeparateSF);  // if this option is switched on the Photon SF are
                                                      // saved spliited into ID, Reco, Isol and Trigger.
        declareProperty("ApplyIsoSF", m_doIsoSF);
        declareProperty("ApplyRecoSF", m_doRecoSF);

        declareProperty("RequireIsoPreSelection", m_requireIso_PreSelection);
        declareProperty("RequireIsoBaseline", m_requireIso_Baseline);
        declareProperty("RequireIsoSignal", m_requireIso_Signal);

        declareProperty("WriteBaselineSF", m_writeBaselineSF);

        SetContainerKey("Photons");
        SetObjectType(XAMPP::SelectionObject::Photon);
    }
    void SUSYPhotonSelector::setupDecorations(std::shared_ptr<PhotonDecorations> input) {
        // as for the particle selector, use the input if provided, or use a default
        // if not set before.
        if (input) { m_photonDecorations = input; }
        if (!m_photonDecorations) { m_photonDecorations = std::make_shared<PhotonDecorations>(); }
        // also set up the particle selector decoration members
        ParticleSelector::setupDecorations(m_photonDecorations);
    }
    SUSYPhotonSelector::~SUSYPhotonSelector() {}
    xAOD::PhotonContainer* SUSYPhotonSelector::GetPhotons() const { return m_Photons; }
    xAOD::PhotonContainer* SUSYPhotonSelector::GetPrePhotons() const { return m_PrePhotons; }
    const xAOD::PhotonContainer* SUSYPhotonSelector::GetPhotonContainer() const { return m_xAODPhotons; }
    xAOD::PhotonContainer* SUSYPhotonSelector::GetSignalPhotons() const { return m_SignalPhotons; }
    xAOD::PhotonContainer* SUSYPhotonSelector::GetSignalNoORPhotons() const { return m_SignalQualPhotons; }
    xAOD::PhotonContainer* SUSYPhotonSelector::GetBaselinePhotons() const { return m_BaselinePhotons; }
    std::shared_ptr<PhotonDecorations> SUSYPhotonSelector::GetPhotonDecorations() const { return m_photonDecorations; }

    StatusCode SUSYPhotonSelector::initializeScaleFactors(const std::string& sf_type, PhotonEffToolHandle& sf_tool, PhotonWeightMap& map,
                                                          unsigned int content) {
        for (auto& syst_set : m_systematics->GetWeightSystematics(ObjectType())) {
            if (!XAMPP::ToolIsAffectedBySystematic(sf_tool, syst_set)) continue;
            PhotonWeight_Ptr Weight(new PhotonWeight(sf_tool));
            ATH_CHECK(initIParticleWeight(*Weight, sf_type, syst_set, content, m_SeparateSF));
            if (map.find(syst_set) != map.end()) {
                ATH_MSG_FATAL("There is already a sf stored in map for systematic " << syst_set->name());
                return StatusCode::FAILURE;
            }
            map.insert(std::pair<const CP::SystematicSet*, PhotonWeight_Ptr>(syst_set, Weight));
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYPhotonSelector::initialize() {
        if (isInitialized()) { return StatusCode::SUCCESS; }
        ATH_CHECK(SUSYParticleSelector::initialize());
        if (!ProcessObject()) { return StatusCode::SUCCESS; }
        setupDecorations();
        ToolHandle<IAsgPhotonEfficiencyCorrectionTool> RecoTool =
            GetCPTool<IAsgPhotonEfficiencyCorrectionTool>("PhotonEfficiencyCorrectionTool");
        ToolHandle<IAsgPhotonEfficiencyCorrectionTool> IsoTool =
            GetCPTool<IAsgPhotonEfficiencyCorrectionTool>("PhotonIsolationCorrectionTool");

        PhotonWeightMap Reco_SF;
        PhotonWeightMap Iso_SF;
        ATH_CHECK(initializeScaleFactors("Reco", RecoTool, Reco_SF,
                                         m_writeBaselineSF ? ScaleFactorMapContains::SignalAndBaseSf : ScaleFactorMapContains::SignalSf));
        ATH_CHECK(initializeScaleFactors("Iso", IsoTool, Iso_SF,
                                         m_writeBaselineSF ? ScaleFactorMapContains::SignalAndBaseSf : ScaleFactorMapContains::SignalSf));

        for (const auto set : m_systematics->GetWeightSystematics(ObjectType())) {
            PhotonWeightHandler_Ptr Weight = std::make_shared<PhotonWeightHandler>(set);
            bool Append = false;
            if (Weight->append(Reco_SF, m_systematics->GetNominal())) Append = true;
            if (Weight->append(Iso_SF, m_systematics->GetNominal())) Append = true;
            if (Append) {
                ATH_CHECK(initIParticleWeight(
                    *Weight, "", set, m_writeBaselineSF ? ScaleFactorMapContains::SignalAndBaseSf : ScaleFactorMapContains::SignalSf));
                m_SF.push_back(Weight);
            }
        }
        return StatusCode::SUCCESS;
    }

    StatusCode SUSYPhotonSelector::LoadContainers() {
        if (!ProcessObject()) return StatusCode::SUCCESS;
        ATH_CHECK(LoadContainer(ContainerKey(), m_xAODPhotons));
        for (auto& ScaleFactors : m_SF) { ATH_CHECK(ScaleFactors->initEvent()); }
        return StatusCode::SUCCESS;
    }
    PhoLink SUSYPhotonSelector::GetLink(const xAOD::Photon& ph) const { return PhoLink(*GetPhotons(), ph.index()); }
    PhoLink SUSYPhotonSelector::GetOrigLink(const xAOD::Photon& ph) const {
        const xAOD::Photon* Oph = dynamic_cast<const xAOD::Photon*>(xAOD::getOriginalObject(ph));
        return PhoLink(*GetPhotonContainer(), Oph != nullptr ? Oph->index() : ph.index());
    }
    xAOD::PhotonContainer* SUSYPhotonSelector::GetCustomPhotons(const std::string& kind) const {
        xAOD::PhotonContainer* customPhotons = nullptr;
        if (LoadViewElementsContainer(kind, customPhotons).isSuccess()) return customPhotons;
        return m_BaselinePhotons;
    }

    StatusCode SUSYPhotonSelector::CallSUSYTools() {
        ATH_MSG_DEBUG("Calling SUSYPhotonSelector::CallSUSYTools()..");
        return m_susytools->GetPhotons(m_Photons, m_PhotonsAux, false);
    }

    StatusCode SUSYPhotonSelector::InitialFill(const CP::SystematicSet& systset) {
        SetSystematics(systset);
        if (ProcessObject())
            ATH_CHECK(FillFromSUSYTools(m_Photons, m_PhotonsAux, m_PrePhotons));
        else {
            ATH_CHECK(ViewElementsContainer("cont", m_Photons));
            ATH_CHECK(ViewElementsContainer("presel", m_PrePhotons));
        }
        return StatusCode::SUCCESS;
    }

    StatusCode SUSYPhotonSelector::FillPhotons(const CP::SystematicSet& systset) {
        SetSystematics(systset);
        ATH_CHECK(ViewElementsContainer("baseline", m_BaselinePhotons));
        ATH_CHECK(ViewElementsContainer("signal", m_SignalPhotons));
        ATH_CHECK(ViewElementsContainer("GQobj", m_SignalQualPhotons));
        for (const auto iphot : *GetPrePhotons()) {
            if (PassBaseline(*iphot)) GetBaselinePhotons()->push_back(iphot);
            if (PassSignalNoOR(*iphot)) GetSignalNoORPhotons()->push_back(iphot);
            if (PassSignal(*iphot)) GetSignalPhotons()->push_back(iphot);
        }

        ATH_MSG_DEBUG("Number of all photons: " << GetPhotons()->size());
        ATH_MSG_DEBUG("Number of preselected photons: " << GetPrePhotons()->size());
        ATH_MSG_DEBUG("Number of selected baseline photons: " << GetBaselinePhotons()->size());
        ATH_MSG_DEBUG("Number of selected signal photons: " << GetSignalPhotons()->size());

        return StatusCode::SUCCESS;
    }
    bool SUSYPhotonSelector::PassPreSelection(const xAOD::IParticle& P) const {
        return ParticleSelector::PassPreSelection(P) && (!m_requireIso_PreSelection || PassIsolation(P));
    }
    bool SUSYPhotonSelector::PassSignal(const xAOD::IParticle& P) const {
        return ParticleSelector::PassSignal(P) && (!m_requireIso_Signal || PassIsolation(P));
    }
    bool SUSYPhotonSelector::PassBaseline(const xAOD::IParticle& P) const {
        return ParticleSelector::PassBaseline(P) && (!m_requireIso_Baseline || PassIsolation(P));
    }
    StatusCode SUSYPhotonSelector::SaveScaleFactor() {
        if (m_SF.empty()) return StatusCode::SUCCESS;

        xAOD::PhotonContainer* SFPhotons = m_writeBaselineSF ? GetBaselinePhotons() : GetSignalPhotons();
        ATH_MSG_DEBUG("Save SF of " << SFPhotons->size() << " photons");
        const CP::SystematicSet* kineSet = m_systematics->GetCurrent();
        for (auto const& ScaleFactors : m_SF) {
            if (kineSet != m_systematics->GetNominal() && ScaleFactors->systematic() != m_systematics->GetNominal()) continue;
            ATH_CHECK(m_systematics->setSystematic(ScaleFactors->systematic()));
            for (auto photon : *SFPhotons) {
                bool is_Signal = !m_writeBaselineSF || PassSignal(*photon);
                ATH_CHECK(ScaleFactors->saveSF(*photon, is_Signal));
            }
            ATH_CHECK(ScaleFactors->applySF());
        }
        return StatusCode::SUCCESS;
    }

    //##########################################################################
    //                            PhotonWeightDecorator
    //##########################################################################
    PhotonWeightDecorator::PhotonWeightDecorator() : IPartilceWeightDecorator() {}
    PhotonWeightDecorator::~PhotonWeightDecorator() {}
    StatusCode PhotonWeightDecorator::saveSF(const xAOD::Photon& Photon, bool isSignal) {
        double SF = 1.;
        if (!isSFcalculated(Photon)) {
            if (!calculateSF(Photon, SF).isSuccess()) return StatusCode::FAILURE;
        } else
            SF = getSF(Photon);
        return saveEventSF(Photon, SF, isSignal);
    }
    //##########################################################################
    //                            PhotonWeight
    //##########################################################################
    PhotonWeight::PhotonWeight(ToolHandle<IAsgPhotonEfficiencyCorrectionTool>& SFTool) : PhotonWeightDecorator(), m_SFTool(SFTool) {}
    PhotonWeight::~PhotonWeight() {}
    StatusCode PhotonWeight::calculateSF(const xAOD::Photon& Photon, double& SF) {
        if (m_SFTool->getEfficiencyScaleFactor(Photon, SF) == CP::CorrectionCode::Error) return StatusCode::FAILURE;
        return StatusCode::SUCCESS;
    }
    //##########################################################################
    //                            PhotonWeightHandler
    //##########################################################################
    PhotonWeightHandler::PhotonWeightHandler(const CP::SystematicSet* syst_set) :
        PhotonWeightDecorator(),
        m_Syst(syst_set),
        m_Weights(),
        m_init(false) {}
    StatusCode PhotonWeightHandler::applySF() {
        if (!m_init) {
            Error("PhotonWeightHandler", "No Mayonaise is not an instrument.");
            return StatusCode::FAILURE;
        }
        for (auto& W : m_Weights) {
            if (!W->applySF().isSuccess()) return StatusCode::FAILURE;
        }
        return PhotonWeightDecorator::applySF();
    }

    const CP::SystematicSet* PhotonWeightHandler::systematic() const { return m_Syst; }
    StatusCode PhotonWeightHandler::calculateSF(const xAOD::Photon& Photon, double& SF) {
        for (auto& W : m_Weights) { SF *= W->getSF(Photon); }
        return StatusCode::SUCCESS;
    }
    StatusCode PhotonWeightHandler::saveSF(const xAOD::Photon& Photon, bool isSignal) {
        for (auto& W : m_Weights) {
            if (!W->saveSF(Photon, isSignal).isSuccess()) return StatusCode::FAILURE;
        }
        return PhotonWeightDecorator::saveSF(Photon, isSignal);
    }
    bool PhotonWeightHandler::append(const PhotonWeightMap& map, const CP::SystematicSet* nominal) {
        PhotonWeightMap::const_iterator Itr = map.find(systematic());
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

}  // namespace XAMPP
