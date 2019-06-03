#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/BaseAnalysisModule.h>
#include <XAMPPbase/ToolHandleSystematics.h>
namespace XAMPP {
    BaseAnalysisModule::~BaseAnalysisModule() {}
    BaseAnalysisModule::BaseAnalysisModule(const std::string& myname) :
        AsgTool(myname),
        m_InfoHandle("EventInfoHandler"),
        m_ParticleConstructor("ParticleConstructor"),
        m_XAMPPInfo(nullptr),
        m_defaults(),
        m_groupName(""),
        m_suffixNotation(false) {
        m_InfoHandle.declarePropertyFor(this, "EventInfoHandler", "The XAMPP EventInfo handle");
        m_ParticleConstructor.declarePropertyFor(this, "ParticleConstructor", "The XAMPP particle constructor");
        declareProperty("GroupName", m_groupName);
        declareProperty("GroupNameAsSuffix", m_suffixNotation);
    }
    StatusCode BaseAnalysisModule::initialize() {
        ATH_CHECK(m_InfoHandle.retrieve());
        ATH_CHECK(m_ParticleConstructor.retrieve());
        m_XAMPPInfo = dynamic_cast<XAMPP::EventInfo*>(m_InfoHandle.operator->());
        return StatusCode::SUCCESS;
    }
    StatusCode BaseAnalysisModule::retrieve(XAMPP::ParticleStorage*& store, const std::string& name) {
        store = m_XAMPPInfo->GetParticleStorage(name);
        if (store == nullptr) {
            ATH_MSG_ERROR("Failed to retrieve particle container " << name << " from storage.");
            return StatusCode::FAILURE;
        } else
            ATH_MSG_DEBUG("Retrieved successfully " << name << " from storage.");
        return StatusCode::SUCCESS;
    }
    StatusCode BaseAnalysisModule::fillDefaultValues() {
        for (const auto& D : m_defaults) ATH_CHECK(D->Fill());
        return StatusCode::SUCCESS;
    }
    StatusCode BaseAnalysisModule::bookParticleStore(const std::string& name, ParticleStorage*& store, bool use_mass, bool saveVariations,
                                                     bool writeTrees) {
        ATH_CHECK(m_XAMPPInfo->BookParticleStorage(full_name(name), use_mass, saveVariations, writeTrees));
        return retrieve(store, full_name(name));
    }
    std::string BaseAnalysisModule::full_name(const std::string& name) const {
        if (m_suffixNotation)
            return name + (m_groupName.empty() ? "" : "_") + m_groupName;
        else
            return m_groupName + (m_groupName.empty() ? "" : "_") + name;
    }
}  // namespace XAMPP
