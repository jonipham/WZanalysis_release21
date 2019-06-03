#ifndef XAMPPbase_EventInfo_IXX
#define XAMPPbase_EventInfo_IXX
#include <XAMPPbase/EventInfo.h>

namespace XAMPP {
    template <typename T> bool EventInfo::DoesVariableExist(const std::string &Name) const {
        return StorageKeeper::GetInstance()->EventStorageExists(Name, this);
    }
    template <typename T> StatusCode EventInfo::NewEventVariable(const std::string &Name, bool saveToTree, bool SaveVariations) {
        if (isLocked()) {
            ATH_MSG_ERROR(
                "The tool is already intialized. Please add all variables "
                "BEFORE "
                "initialization");
            return StatusCode::FAILURE;
        }
        if (DoesVariableExist<T>(Name)) {
            ATH_MSG_ERROR("The event storage for the event variable " << Name << " has already been created");
            return StatusCode::FAILURE;
        } else {
            Storage<T> *S = new Storage<T>(Name, this);
            S->SetSaveTrees(saveToTree);
            S->SetSaveVariations(SaveVariations);
        }
        ATH_MSG_INFO("Create new event variable " << Name);
        return StatusCode::SUCCESS;
    }
    template <typename T> StatusCode EventInfo::NewCommonEventVariable(const std::string &Name, bool saveToTree, bool SaveVariations) {
        if (StorageKeeper::GetInstance()->EventStorageExists(Name, nullptr)) {
            ATH_MSG_DEBUG("The event storage for the event variable " << Name << " has already been created");
            return StatusCode::SUCCESS;
        }
        if (StorageKeeper::GetInstance()->isLocked()) {
            ATH_MSG_ERROR("The tool is already intialized. Please add all variables BEFORE initialization");
            return StatusCode::FAILURE;
        }
        Storage<T> *S = new Storage<T>(Name, this, true);
        S->SetSaveTrees(saveToTree);
        S->SetSaveVariations(SaveVariations);
        ATH_MSG_INFO("Create new common event variable " << Name);
        return StatusCode::SUCCESS;
    }
    template <typename T> void EventInfo::RemoveVariableFromOutput(const std::string &Name) {
        Storage<T> *S = GetVariableStorage<T>(Name);
        if (!S) return;
        S->SetSaveTrees(false);
        S->SetSaveHistos(false);
    }
    template <typename T> Storage<T> *EventInfo::GetVariableStorage(const std::string &Name) const {
        Storage<T> *V = StorageKeeper::GetInstance()->RetrieveEventStorage<T>(Name, this);
        if (V == nullptr) ATH_MSG_WARNING("Could not find the variable " << Name);
        return V;
    }

    template <typename T> std::vector<Storage<T> *> EventInfo::GetStorages(unsigned int e) const {
        std::vector<Storage<T> *> in = StorageKeeper::GetInstance()->GetEventStorages<T>(this);
        std::vector<Storage<T> *> out;
        for (auto &entry : in) {
            if (returnStorage(entry, e)) out.push_back(entry);
        }
        return out;
    }
}  // namespace XAMPP
#endif
