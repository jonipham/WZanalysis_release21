#ifndef XAMPPbase_SUSYTriggerTool_IXX
#define XAMPPbase_SUSYTriggerTool_IXX

#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/EventStorage.h>
#include <XAMPPbase/SUSYTriggerTool.h>

#include <SUSYTools/SUSYObjDef_xAOD.h>
#include <xAODBase/IParticleHelpers.h>
#include <xAODCore/ShallowCopy.h>

#include <PATInterfaces/SystematicSet.h>

namespace XAMPP {

    template <typename TriggerContainer>
    SUSYTriggerTool::LinkStatus SUSYTriggerTool::CreateContainerLinks(const std::string& key, TriggerContainer*& container) {
        std::string store_string = name() + key;
        if (evtStore()->contains<TriggerContainer>(store_string)) {
            if (!evtStore()->retrieve(container, store_string).isSuccess()) {
                ATH_MSG_ERROR("Could not load shallow container " << key << " from the Store Gate");
                return LinkStatus::Failed;
            } else
                return LinkStatus::Created;
        }
        const TriggerContainer* const_container = nullptr;
        ATH_MSG_DEBUG("Create new ShallowCopy Container " << key << ".");
        if (!evtStore()->retrieve(const_container, key).isSuccess()) return LinkStatus::Failed;
        std::pair<TriggerContainer*, xAOD::ShallowAuxContainer*> shallowcopy = xAOD::shallowCopyContainer(*const_container);
        container = shallowcopy.first;
        if (!evtStore()->record(container, store_string).isSuccess() ||
            !evtStore()->record(shallowcopy.second, store_string + "Aux.").isSuccess()) {
            ATH_MSG_ERROR("Failed to parse the containers to the Store Gate");
            return LinkStatus::Failed;
        }
        return LinkStatus::Created;
    }
    template <typename Container> StatusCode SUSYTriggerTool::ViewElementsContainer(const std::string& Key, Container*& Cont) {
        if (!m_init) {
            ATH_MSG_ERROR("The tool is not initalized");
            return StatusCode::FAILURE;
        }
        if (Key.empty()) {
            ATH_MSG_ERROR("Empty keys are not allowed");
            return StatusCode::FAILURE;
        }
        ATH_MSG_DEBUG("Create new SG::VIEW_ELEMENTS container for " << Key);
        Cont = new Container(SG::VIEW_ELEMENTS);
        return evtStore()->record(Cont, Key + name() + m_XAMPPInfo->GetSystematic()->name());
    }

    template <typename T> ToolHandle<T> SUSYTriggerTool::GetCPTool(const std::string& name) {
        return ToolHandle<T>(SUSYToolsPtr()->getProperty(name).toString());
    }
}  // namespace XAMPP
#endif
