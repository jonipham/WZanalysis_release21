#ifndef XAMPPBASE_TREEHELPERS_IXX
#define XAMPPBASE_TREEHELPERS_IXX
#include <XAMPPbase/EventStorage.h>
#include <XAMPPbase/TreeHelpers.h>
#include <memory>
namespace XAMPP {

    //#############################################################################
    //                              TreeHelper
    //#############################################################################
    template <typename T> bool TreeHelper::AddBranch(const std::string& Name, T& Element) {
        std::string bName = branch_name(Name);
        if (m_tree->FindBranch(bName.c_str())) {
            Error("TreeHelper::AddBranch()", "The branch %s already exists in TTree %s", Name.c_str(), m_tree->GetName());
            return false;
        }
        if (m_tree->Branch(bName.c_str(), &Element) == nullptr) {
            Error("TreeHelper::AddBranch()", "Could not create the branch %s in TTree %s", Name.c_str(), m_tree->GetName());
            return false;
        }
        return true;
    }
    //#############################################################################
    //                              ITreeContainerAccessor
    //#############################################################################
    template <typename T> bool ITreeContainerAccessor::AddBranch(T& Element) { return AddBranch(name(), Element); }

    //#############################################################################
    //                              TreeBranch
    //#############################################################################
    template <class T>
    TreeBranch<T>::TreeBranch(TTree* tree, const std::string& name) : TreeHelper(tree), m_name(name), m_Var(), m_isUpdated(false) {}

    template <class T> bool TreeBranch<T>::Init() { return AddBranch(name(), m_Var); }
    template <class T> std::string TreeBranch<T>::name() const { return m_name; }
    template <class T> bool TreeBranch<T>::Fill() {
        if (!m_isUpdated) {
            Error("TreeBranch::Fill()", "Failed to update %s", name().c_str());
            return false;
        }
        m_isUpdated = false;
        return true;
    }
    template <class T> void TreeBranch<T>::setValue(const T& Value) {
        m_Var = Value;
        m_isUpdated = true;
    }
    //#############################################################################
    //                              EventBranchVariable
    //#############################################################################
    template <class T>
    EventBranchVariable<T>::EventBranchVariable(XAMPP::Storage<T>* Store, TTree* tree) : TreeHelper(tree), m_Store(Store) {}
    template <class T> bool EventBranchVariable<T>::Init() { return AddBranch(m_Store->name(), m_Store->GetValue()); }
    template <class T> std::string EventBranchVariable<T>::name() const { return m_Store->name(); }
    template <class T> bool EventBranchVariable<T>::Fill() {
        if (!m_Store->isAvailable()) {
            Error("EventBranchVariable::Fill()", "Nothing has been stored in %s", name().c_str());
            return false;
        }
        m_Store->GetValue();
        return true;
    }
    //#############################################################################
    //                              TreeDataVectorAccessor
    //#############################################################################
    template <class T>
    TreeDataVectorAccessor<T>::TreeDataVectorAccessor(DataVectorStorage* store, TreeBase* tree, const std::string& Name) :
        ITreeDataVectorAccessor(store, tree, Name),
        m_Vector(),
        m_Acc(Name) {}

    template <class T> bool TreeDataVectorAccessor<T>::PushBack(const SG::AuxElement* element, bool Resize, size_t Reserve) {
        if (Resize) Clear(Reserve);
        if (element == nullptr) {
            Error("TreeDataVectorAccessor::PushBack()", "No AuxElement was given to write branch %s", name().c_str());
            return false;
        }
        if (!m_Acc.isAvailable(*element)) {
            Error("TreeDataVectorAccessor::PushBack()", "The AuxElement does not contain %s", name().c_str());
            return false;
        }
        m_Vector.push_back(m_Acc(*element));
        return true;
    }
    template <class T> bool TreeDataVectorAccessor<T>::Init() { return AddBranch(m_Vector); }
    template <class T> void TreeDataVectorAccessor<T>::Clear(size_t Reserve) {
        m_Vector.clear();
        if (Reserve > m_Vector.capacity()) m_Vector.reserve(Reserve);
    }
}  // namespace XAMPP
#endif
