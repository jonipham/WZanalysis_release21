#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/EventStorage.h>
#include <XAMPPbase/TreeBase.h>
#include <XAMPPbase/TreeHelpers.h>
namespace XAMPP {
    TreeHelper::TreeHelper(TTree* tree) : m_tree(tree) {}
    std::string TreeHelper::branch_name(const std::string& b_name) const { return EraseWhiteSpaces(b_name); }
    //################################################################################################################################
    //                                              ITreeContainerAccessor
    //################################################################################################################################
    ITreeContainerAccessor::ITreeContainerAccessor(DataVectorStorage* store, TreeBase* tree, const std::string& Name) :
        TreeHelper(tree->Tree()),
        m_tree_base(tree),
        m_store(store),
        m_name(Name) {}

    const CP::SystematicSet* ITreeContainerAccessor::systematic() const { return m_tree_base->systematic(); }
    DataVectorStorage* ITreeContainerAccessor::getStore() const { return m_store; }
    std::string ITreeContainerAccessor::name() const { return m_store->name() + "_" + m_name; }
    //################################################################################################################################
    //                                              ITreeDataVectorAccessor
    //################################################################################################################################
    ITreeDataVectorAccessor::ITreeDataVectorAccessor(DataVectorStorage* store, TreeBase* tree, const std::string& Name) :
        ITreeContainerAccessor(store, tree, Name) {}

    bool ITreeDataVectorAccessor::Fill() {
        const DataVector<SG::AuxElement>* data_container = getStore()->Container();
        if (!data_container) {
            Error("ITreeContainerAccessor::Fill()", "Error no container was given to %s", name().c_str());
            return false;
        }
        if (data_container->empty()) {
            Clear();
            return true;
        }
        bool Clear = true;
        size_t ContSize = data_container->size();
        for (auto obj : *data_container) {
            if (!PushBack(obj, Clear, ContSize)) return false;
            Clear = false;
        }
        return true;
    }
    //################################################################################################################################
    //                                              ITreeParticleAccessor
    //################################################################################################################################
    ITreeParticleAccessor::ITreeParticleAccessor(DataVectorStorage* store, TreeBase* tree, const std::string& Name) :
        ITreeContainerAccessor(store, tree, Name) {}
    bool ITreeParticleAccessor::Fill() {
        SystematicContainer* syst_container = getStore()->findContainer();
        if (!syst_container) {
            Error("ITreeParticleAccessor::Fill()", "Invalid systematic %s for branch %s",
                  getStore()->XAMPPInfo()->GetSystematic()->name().c_str(), name().c_str());
            return false;
        }
        const xAOD::IParticleContainer* data_container = syst_container->particleContainer();
        if (!data_container) {
            Error("ITreeParticleAccessor::Fill()", "Error no container was given to %s", name().c_str());
            return false;
        }
        if (data_container->empty()) {
            Clear();
            return true;
        }
        bool Clear = true;
        size_t ContSize = data_container->size();
        for (auto obj : *data_container) {
            if (!PushBack(obj, Clear, ContSize)) return false;
            Clear = false;
        }
        return true;
    }
    //################################################################################################################################
    //                                              ITreeParticleFourMomentConstAccessor
    //################################################################################################################################
    bool ITreeParticleFourMomentConstAccessor::PushBack(const xAOD::IParticle* P, bool Resize, size_t Reserve) {
        if (Resize) Clear(Reserve);
        if (!P) return false;
        m_Vector.push_back((P->*m_Momentum)());
        return true;
    }
    void ITreeParticleFourMomentConstAccessor::Clear(size_t Reserve) {
        m_Vector.clear();
        if (Reserve > m_Vector.capacity()) m_Vector.reserve(Reserve);
    }
    bool ITreeParticleFourMomentConstAccessor::Init() { return AddBranch(m_Vector); }
    ITreeParticleFourMomentConstAccessor::ITreeParticleFourMomentConstAccessor(ParticleStorage* store, TreeBase* tree, Momentum Mom) :
        ITreeParticleAccessor(store, tree, FourMomentum(Mom)),
        m_Momentum(Mom),
        m_Vector() {}
    //################################################################################################################################
    //                                              TreeHelperMet
    //################################################################################################################################
    TreeHelperMet::TreeHelperMet(XAMPP::Storage<XAMPPmet>* met, TTree* tree) :
        TreeHelper(tree),
        m_store(met),
        m_met(),
        m_phi(),
        m_sumet() {}
    bool TreeHelperMet::Init() {
        if (!m_store) {
            Error("TreeHelperMet::initialize()", "The MET does not exist");
            return false;
        }
        if (!AddBranch(m_store->name() + "_met", m_met)) return false;
        if (!AddBranch(m_store->name() + "_phi", m_phi)) return false;
        if (!AddBranch(m_store->name() + "_sumet", m_sumet)) return false;
        return true;
    }
    bool TreeHelperMet::Fill() {
        if (m_store->GetValue()) {
            m_met = m_store->GetValue()->met();
            m_phi = m_store->GetValue()->phi();
            m_sumet = m_store->GetValue()->sumet();
        } else {
            m_met = -FLT_MAX;
            m_phi = -FLT_MAX;
            m_sumet = -FLT_MAX;
        }
        return true;
    }
    std::string TreeHelperMet::name() const { return m_store->name(); }
}  // namespace XAMPP
