#include <PATInterfaces/SystematicSet.h>
#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/ReconstructedParticles.h>

namespace XAMPP {
    ReconstructedParticles::ReconstructedParticles(std::string myname) :
        ParticleSelector(myname),
        m_DecWell(nullptr),
        m_AccWell(nullptr),
        m_Container(nullptr),
        m_AutoFillContainer(nullptr),
        m_ViewCont(nullptr),
        m_AuxContainer(nullptr) {
        SetContainerKey("RecoCandidatePart");
        SetObjectType(XAMPP::SelectionObject::RecoParticle);
    }

    ReconstructedParticles::~ReconstructedParticles() {}
    void ReconstructedParticles::WellDefined(xAOD::IParticle* P, bool W) { (*m_DecWell)(*P) = W; }
    StatusCode ReconstructedParticles::initialize() {
        if (isInitialized()) { return StatusCode::SUCCESS; }
        ATH_CHECK(ParticleSelector::initialize());
        // The current object type returns all kinematic systematics sets
        m_DecWell = SelectionDecorator(new CharDecorator("WellDefined"));
        m_AccWell = SelectionAccessor(new CharAccessor("WellDefined"));
        return StatusCode::SUCCESS;
    }
    StatusCode ReconstructedParticles::PrepareContainer(const CP::SystematicSet* Set) {
        SetSystematics(Set);
        if (evtStore()->contains<xAOD::ParticleContainer>(StoreName())) {
            ATH_MSG_DEBUG("Found container already in store.. Will return that one");
            ATH_CHECK(LoadContainer(StoreName(), m_Container));
            ATH_CHECK(LoadContainer(StoreName() + "Aux.", m_AuxContainer));
            ATH_CHECK(LoadContainer(StoreName() + "View.", m_ViewCont));
            return StatusCode::SUCCESS;
        }
        m_Container = new xAOD::ParticleContainer();
        m_AuxContainer = new xAOD::ParticleAuxContainer();
        m_Container->setStore(m_AuxContainer);
        m_ViewCont = new xAOD::ParticleContainer(SG::VIEW_ELEMENTS);
        m_AutoFillContainer = nullptr;
        ATH_CHECK(evtStore()->record(m_Container, StoreName()));
        ATH_CHECK(evtStore()->record(m_ViewCont, StoreName() + "View."));
        ATH_CHECK(evtStore()->record(m_AuxContainer, StoreName() + "Aux."));
        return StatusCode::SUCCESS;
    }
    xAOD::Particle* ReconstructedParticles::CreateEmptyParticle() {
        xAOD::Particle* P = new xAOD::Particle;
        m_Container->push_back(P);
        WellDefined(P, true);
        if (m_AutoFillContainer) m_AutoFillContainer->push_back(P);
        return P;
    }
    xAOD::ParticleContainer* ReconstructedParticles::GetContainer() {
        m_ViewCont->clear();
        for (auto P : *m_Container)
            if ((*m_AccWell)(*P)) m_ViewCont->push_back(P);
        return m_ViewCont;
    }
    StatusCode ReconstructedParticles::CreateSubContainer(const std::string& Name) {
        ATH_CHECK(ViewElementsContainer(Name, m_AutoFillContainer));
        return StatusCode::SUCCESS;
    }
    void ReconstructedParticles::DetachSubContainer() { m_AutoFillContainer = nullptr; }
    xAOD::ParticleContainer* ReconstructedParticles::GetSubContainer(const std::string& Name) {
        xAOD::ParticleContainer* Cont = nullptr;
        if (!LoadViewElementsContainer(Name, Cont).isSuccess()) return nullptr;
        return Cont;
    }
    StatusCode ReconstructedParticles::LoadSubContainer(const std::string& Name) {
        ATH_CHECK(LoadViewElementsContainer(Name, m_AutoFillContainer));
        return StatusCode::SUCCESS;
    }
    bool ReconstructedParticles::HasSubContainer(const std::string& name) const {
        return evtStore()->contains<xAOD::ParticleContainer>(name + StoreName());
    }
}  // namespace XAMPP
