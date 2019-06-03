#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/ISystematics.h>
#include <XAMPPbase/SUSYParticleSelector.h>

namespace XAMPP {

    //#############################################################
    //                    SUSYParticleSelector
    //#############################################################
    SUSYParticleSelector::SUSYParticleSelector(std::string myname) : ParticleSelector(myname), m_susytools("SUSYTools"), m_STPtr(nullptr) {
        m_susytools.declarePropertyFor(this, "SUSYTools", "The SUSYTools instance");
    }

    SUSYParticleSelector::~SUSYParticleSelector() {}
    StatusCode SUSYParticleSelector::initialize() {
        ATH_CHECK(ParticleSelector::initialize());
        if (!ProcessObject()) { return StatusCode::SUCCESS; }
        ATH_CHECK(m_susytools.retrieve());
        m_STPtr = dynamic_cast<ST::SUSYObjDef_xAOD*>(m_susytools.operator->());
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYParticleSelector::CallSUSYTools() {
        ATH_MSG_WARNING("Calling SUSYParticleSelector::CallSUSYTools()...dummy function...");
        return StatusCode::FAILURE;
    }
    ST::SUSYObjDef_xAOD* SUSYParticleSelector::SUSYToolsPtr() const { return m_STPtr; }

}  // namespace XAMPP
