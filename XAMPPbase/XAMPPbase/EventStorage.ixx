#ifndef XAMPPBASE_EventStorage_IXX
#define XAMPPBASE_EventStorage_IXX
#include <XAMPPbase/EventStorage.h>
#include <XAMPPbase/IEventInfo.h>
#include <XAMPPbase/TreeHelpers.h>
#include <memory>
#include <string>
namespace XAMPP {

    template <class T> Storage<T>* StorageKeeper::RetrieveEventStorage(const std::string& Name, const IEventInfo* EvInfo) const {
        Storage<T>* S = m_CommonKeeper->RetrieveEventStorage<T>(Name);
        if (S) { return S; }
        StorageKeeper::InfoKeeper* Keeper = FindKeeper(EvInfo);
        if (Keeper) { return Keeper->RetrieveEventStorage<T>(Name); }
        return nullptr;
    }
    template <class T> std::vector<Storage<T>*> StorageKeeper::GetEventStorages(const IEventInfo* EvInfo) const {
        std::vector<Storage<T>*> S = m_CommonKeeper->GetEventStorages<T>();
        StorageKeeper::InfoKeeper* Keeper = FindKeeper(EvInfo);
        if (Keeper) {
            std::vector<Storage<T>*> S1 = Keeper->GetEventStorages<T>();
            for (auto& Element : S1) { S.push_back(Element); }
        }
        return S;
    }
    template <class T> Storage<T>* StorageKeeper::InfoKeeper::RetrieveEventStorage(const std::string& Name) const {
        if (!EventStorageExists(Name)) { return nullptr; }
        return dynamic_cast<Storage<T>*>(m_Storages.at(Name).get());
    }

    template <class T> std::vector<Storage<T>*> StorageKeeper::InfoKeeper::GetEventStorages() const {
        std::vector<Storage<T>*> Stores;
        for (auto& Element : m_Storages) {
            Storage<T>* Store = dynamic_cast<Storage<T>*>(Element.second.get());
            if (Store) Stores.push_back(Store);
        }
        return Stores;
    }

    //#############################################################################
    //                              Storage
    //#############################################################################
    template <class T>
    Storage<T>::Storage(const std::string& Name, IEventInfo* Info, bool IsCommon) :
        IStorage(Name, Info, false, IsCommon),
        m_Variable(),
        m_Decorator(Name),
        m_Accessor(Name) {}
    template <class T> StatusCode Storage<T>::Store(T Value) {
        if (!isRegistered()) {
            Error(name().c_str(), "The storage element is not registered");
            return StatusCode::FAILURE;
        }
        if (!XAMPPInfo()) {
            Error(name().c_str(), "There is no XAMPPInfo object");
            return StatusCode::FAILURE;
        }
        if (!XAMPPInfo()->GetEventInfo()) {
            Error(name().c_str(), "The XAMPPInfo has not been initialized in this event");
            return StatusCode::FAILURE;
        }
        m_Decorator(*XAMPPInfo()->GetEventInfo()) = Value;
        return StatusCode::SUCCESS;
    }
    template <class T> T& Storage<T>::GetValue() {
        if (isAvailable()) {
            if (XAMPPInfo()->GetEventInfo())
                m_Variable = m_Accessor(*XAMPPInfo()->GetEventInfo());
            else if (XAMPPInfo()->GetOrigInfo())
                m_Variable = m_Accessor(*XAMPPInfo()->GetOrigInfo());
        }
        return m_Variable;
    }
    template <class T> StatusCode Storage<T>::ConstStore(T Value) {
        if (!isRegistered() || !XAMPPInfo()->GetOrigInfo()) { return StatusCode::FAILURE; }
        /// In the old days of XAMPP hard copies of the EventInfo were
        /// made to ensure that the information per systematic is actually
        /// seperate from each other. ConstStore was meant to be used
        /// before any systematic loop is called, but now one can use ConstStore
        /// at a later stage and the information is propagated. However, I think
        /// it's worth to warn the user in these cases once!
        static bool printedStoreMisUse = false;
        if (XAMPPInfo()->GetEventInfo() && !printedStoreMisUse) {
            printedStoreMisUse = true;
            Warning("ConstStore()",
                    "You're calling this method at a stage where the systematic %s has already been cached. Please be aware that this "
                    "information should be popagated to all systematics unless you're calling Store again.",
                    XAMPPInfo()->GetSystematic()->name().c_str());
        }
        m_Decorator(*XAMPPInfo()->GetOrigInfo()) = Value;
        return StatusCode::SUCCESS;
    }
    template <class T> Storage<T>::~Storage() {}
    template <class T> bool Storage<T>::isAvailable() const {
        if (!isRegistered() || !XAMPPInfo()) { return false; }
        if (XAMPPInfo()->GetEventInfo()) return m_Accessor.isAvailable(*XAMPPInfo()->GetEventInfo());
        if (XAMPPInfo()->GetOrigInfo()) return m_Accessor.isAvailable(*XAMPPInfo()->GetOrigInfo());
        return false;
    }

    //#############################################################################
    //                              DataVectorStorage
    //#############################################################################
    template <typename T> StatusCode DataVectorStorage::SaveVariable(const std::string& Branch, bool SaveVariations) {
        return AddVariable(Branch, &DataVectorStorage::createBranch<T>, SaveVariations);
    }
    template <typename T> std::shared_ptr<ITreeBranchVariable> DataVectorStorage::createBranch(const std::string& Name, TreeBase* base) {
        return std::make_shared<TreeDataVectorAccessor<T>>(this, base, Name);
    }

    template <typename T> StatusCode DataVectorStorage::SaveVariable(const std::vector<std::string>& BranchVector, bool SaveVariations) {
        for (const auto& Branch : BranchVector) {
            if (!SaveVariable<T>(Branch, SaveVariations).isSuccess()) return StatusCode::FAILURE;
        }
        return StatusCode::SUCCESS;
    }

}  // namespace XAMPP
#endif
