#ifndef XAMPPbase_TreeHelpers_H
#define XAMPPbase_TreeHelpers_H

#include <AsgTools/IAsgTool.h>
#include <TMath.h>
#include <TTree.h>
#include <XAMPPbase/Defs.h>
#include <xAODBase/IParticleContainer.h>
#include <iostream>
#include <memory>
namespace CP {
    class SystematicSet;
}
namespace XAMPP {
    template <class T> class Storage;

    class ParticleStorage;
    class DataVectorStorage;
    class TreeBase;

    /// General interface class to communicate with the single branches
    /// throughout the data types considered
    class ITreeBranchVariable {
    public:
        virtual bool Init() = 0;
        virtual std::string name() const = 0;
        virtual bool Fill() = 0;
        virtual ~ITreeBranchVariable() = default;
    };

    class TreeHelper {
    public:
        virtual ~TreeHelper() = default;

    protected:
        TreeHelper(TTree* tree);
        template <typename T> bool AddBranch(const std::string& Name, T& Element);

    private:
        std::string branch_name(const std::string& b_name) const;
        TTree* m_tree;
    };

    // Interface class to store particle variables in Vector Branches

    class ITreeContainerAccessor : public ITreeBranchVariable, public TreeHelper {
    public:
        virtual void Clear(size_t Reserve = 0) = 0;
        virtual std::string name() const;

    protected:
        ITreeContainerAccessor(DataVectorStorage* store, TreeBase* tree, const std::string& Name);
        ITreeContainerAccessor(const ITreeContainerAccessor&) = delete;

        const CP::SystematicSet* systematic() const;
        DataVectorStorage* getStore() const;

        template <typename T> bool AddBranch(T& Element);
        using TreeHelper::AddBranch;

    private:
        TreeBase* m_tree_base;
        DataVectorStorage* m_store;
        std::string m_name;
    };

    class ITreeDataVectorAccessor : public ITreeContainerAccessor {
    public:
        virtual bool Fill();

    protected:
        ITreeDataVectorAccessor(DataVectorStorage* store, TreeBase* tree, const std::string& Name);
        virtual bool PushBack(const SG::AuxElement* element, bool Resize, size_t Reserve) = 0;
    };
    class ITreeParticleAccessor : public ITreeContainerAccessor {
    public:
        virtual bool Fill();

    protected:
        ITreeParticleAccessor(DataVectorStorage* store, TreeBase* tree, const std::string& Name);
        virtual bool PushBack(const xAOD::IParticle* P, bool Resize, size_t Reserve) = 0;
    };

    class TreeHelperMet : public TreeHelper, virtual public ITreeBranchVariable {
    public:
        TreeHelperMet(XAMPP::Storage<XAMPPmet>* met, TTree* tree);
        virtual bool Init();
        virtual std::string name() const;
        virtual bool Fill();

    protected:
        XAMPP::Storage<XAMPPmet>* m_store;
        float m_met;
        float m_phi;
        float m_sumet;
    };

    template <class T> class TreeBranch : public TreeHelper, virtual public ITreeBranchVariable {
    public:
        TreeBranch(TTree* tree, const std::string& name);
        virtual bool Init();
        virtual std::string name() const;
        virtual bool Fill();
        virtual ~TreeBranch() {}
        void setValue(const T& Value);

    private:
        std::string m_name;
        T m_Var;
        bool m_isUpdated;
    };

    template <class T> class EventBranchVariable : public TreeHelper, virtual public ITreeBranchVariable {
    public:
        EventBranchVariable(XAMPP::Storage<T>* Store, TTree* tree);
        virtual bool Init();
        virtual std::string name() const;
        virtual bool Fill();
        virtual ~EventBranchVariable() {}

    private:
        XAMPP::Storage<T>* m_Store;
    };

    // Class to store a SG::AuxElement variable from a container
    template <class T> class TreeDataVectorAccessor : public ITreeDataVectorAccessor {
    public:
        TreeDataVectorAccessor(DataVectorStorage* store, TreeBase* tree, const std::string& Name);

        virtual bool PushBack(const SG::AuxElement* element, bool Resize, size_t Reserve);
        virtual void Clear(size_t Reserve = 0);
        virtual bool Init();

    private:
        std::vector<T> m_Vector;
        SG::AuxElement::Accessor<T> m_Acc;
    };

    // Classes to store the momentum Components of the particle
    class ITreeParticleFourMomentConstAccessor : public ITreeParticleAccessor {
    public:
        virtual bool PushBack(const xAOD::IParticle* P, bool Resize, size_t Reserve);
        virtual void Clear(size_t Reserve = 0);
        virtual bool Init();
        ITreeParticleFourMomentConstAccessor(ParticleStorage* store, TreeBase* tree, Momentum Mom);

    protected:
        Momentum m_Momentum;
        std::vector<float> m_Vector;
    };

}  // namespace XAMPP
#include <XAMPPbase/TreeHelpers.ixx>
#endif
