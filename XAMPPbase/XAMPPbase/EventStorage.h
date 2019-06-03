#ifndef XAMPPbase_EventStorage_H
#define XAMPPbase_EventStorage_H

#include <AsgTools/IAsgTool.h>
#include <AsgTools/ToolHandle.h>
#include <TH1.h>
#include <XAMPPbase/IEventInfo.h>
#include <xAODEventInfo/EventInfo.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace CP {
    class SystematicSet;
}
namespace XAMPP {
    class IStorage;
    class DataVectorStorage;

    class IEventInfo;
    class ISystematics;
    class SystematicGroup;
    class TreeBase;
    class ITreeBranchVariable;
    template <class T> class Storage;
    class Cut;

    class StorageKeeper {
    public:
        static StorageKeeper* GetInstance();
        bool Register(IStorage* S);

        bool EventStorageExists(const std::string& Name, const IEventInfo* EvInfo = nullptr) const;
        bool ParticleDefined(const std::string& Name, const IEventInfo* EvInfo = nullptr) const;

        template <class T> Storage<T>* RetrieveEventStorage(const std::string& Name, const IEventInfo* EvInfo = nullptr) const;

        template <class T> std::vector<Storage<T>*> GetEventStorages(const IEventInfo* EvInfo = nullptr) const;

        std::vector<DataVectorStorage*> RetrieveContainerStores(const IEventInfo* Info = nullptr) const;
        DataVectorStorage* RetrieveContainerStorage(const std::string& Name, const IEventInfo* Info = nullptr) const;

        ~StorageKeeper();
        // Let's attach the cuts to the keeper just for deletion
        void AttachCut(Cut* C);
        void DettachCut(Cut* C);

        // This prevents the keeper from accepting any event variable
        bool isLocked() const;
        void Lock();

    private:
        static StorageKeeper* m_Inst;

        StorageKeeper();
        StorageKeeper(const StorageKeeper&) = delete;
        void operator=(const StorageKeeper&) = delete;

        class InfoKeeper {
        public:
            InfoKeeper(const IEventInfo* = nullptr);
            const IEventInfo* GetInfo() const;
            template <class T> Storage<T>* RetrieveEventStorage(const std::string& Name) const;
            template <class T> std::vector<Storage<T>*> GetEventStorages() const;
            bool EventStorageExists(const std::string& Name) const;

            DataVectorStorage* GetContainerStorage(const std::string& Name) const;
            std::vector<DataVectorStorage*> GetContainerStorages() const;
            bool ParticleDefined(const std::string& Name) const;

            bool Register(IStorage* Storage);
            ~InfoKeeper();

        private:
            const IEventInfo* m_RefInfo;
            std::map<std::string, std::shared_ptr<IStorage>> m_Storages;
            std::map<std::string, std::shared_ptr<IStorage>> m_ParticleStores;
        };
        InfoKeeper* FindKeeper(const IEventInfo* Info = nullptr) const;

        std::shared_ptr<InfoKeeper> m_CommonKeeper;
        std::vector<std::shared_ptr<InfoKeeper>> m_Keepers;
        std::vector<Cut*> m_Cuts;
        bool m_Locked;
    };
    /// Systematic groups are the managing classes to organize
    /// branches into groups by affecting systematics. The systematic
    /// group has an arbitrary name, the Selection object is belongs to
    /// and finally the SystematicsTool connected
    class SystematicGroup {
    public:
        SystematicGroup(const std::string& name, XAMPP::SelectionObject obj, const ToolHandle<ISystematics>& syst_tool);
        std::string name() const;
        bool isAffectedBySyst(const CP::SystematicSet* syst) const;

    private:
        std::string m_name;
        XAMPP::SelectionObject m_obj;
        ToolHandle<ISystematics> m_systematics;
    };
    class IStorage {
    public:
        void SetSaveTrees(bool B);
        void SetSaveHistos(bool B);
        void SetSaveVariations(bool B);

        bool SaveTrees() const;
        bool SaveVariations() const;
        bool IsParticleVariable() const;
        bool IsCommonVariable() const;

        bool SaveHistos() const;
        bool SaveVariable() const;
        virtual ~IStorage();
        std::string name() const;
        IEventInfo* XAMPPInfo() const;

        // Functionallity to save Histos in XAMPP
        std::vector<std::string> GetHistoVariables() const;
        TH1* Template(const std::string& Name) const;
        StatusCode CreateHistogram(const std::string& variable, TH1* Template);
        StatusCode CreateHistogram(const std::string& variable, std::shared_ptr<TH1> Template);

        std::shared_ptr<SystematicGroup> getSystematicGroup() const;
        StatusCode setSystematicGroup(const std::string& name);

    protected:
        IStorage(const std::string& Name, IEventInfo* Info, bool IsParticleVariable, bool IsCommonVariable = false);
        bool isRegistered() const;

    private:
        std::string m_Name;
        IEventInfo* m_Info;
        bool m_IsParticleVariable;
        bool m_Registered;
        bool m_SaveHisto;
        bool m_SaveTree;
        bool m_SaveAlways;
        bool m_IsCommon;
        typedef std::shared_ptr<TH1> TH1Ptr;
        std::map<std::string, std::shared_ptr<TH1>> m_HistoTemplates;
        std::shared_ptr<SystematicGroup> m_systGroup;
    };
    template <class T> class Storage : public IStorage {
    public:
        Storage(const std::string& Name, IEventInfo* Info, bool IsCommon = false);

        StatusCode Store(T Value);
        StatusCode ConstStore(T Value);

        T& GetValue();
        bool isAvailable() const;
        virtual ~Storage();

    private:
        /// Variable to connect the tree to
        T m_Variable;
        /// Access elements to the EventInfo objects storing the actual
        /// information of the variable
        SG::AuxElement::Decorator<T> m_Decorator;
        SG::AuxElement::Accessor<T> m_Accessor;
    };

    class SystematicContainer {
    public:
        SystematicContainer(const DataVectorStorage* store, const CP::SystematicSet* set);

        void setContainer(xAOD::IParticleContainer* container);
        void setContainer(DataVector<SG::AuxElement>* container);

        xAOD::IParticleContainer* particleContainer() const;
        DataVector<SG::AuxElement>* auxElementContainer() const;
        const CP::SystematicSet* systematic() const;
        bool isContainerValid() const;
        void createAuxElementsContainer();

    private:
        const DataVectorStorage* m_refStore;
        const CP::SystematicSet* m_set;
        xAOD::IParticleContainer* m_particle_container;
        DataVector<SG::AuxElement>* m_sg_container;
        std::unique_ptr<DataVector<SG::AuxElement>> m_converted_container;

        unsigned long long m_lastSet;
    };

    class DataVectorStorage : public IStorage {
    public:
        DataVectorStorage(const std::string& Name, XAMPP::IEventInfo* info, bool isCommon = false);
        virtual ~DataVectorStorage() = default;

        StatusCode Fill(DataVector<SG::AuxElement>* Container);
        StatusCode Fill(xAOD::IParticleContainer* Particles);

        DataVector<SG::AuxElement>* Container();
        DataVector<SG::AuxElement>* Container(const CP::SystematicSet* set);

        SystematicContainer* findContainer(const CP::SystematicSet* set);
        SystematicContainer* findContainer();

        virtual std::vector<std::shared_ptr<ITreeBranchVariable>> CreateParticleTree(TreeBase* base_tree);

        // Here you can add extra bits of information
        StatusCode SaveInteger(const std::string& Branch, bool SaveVariations = true);
        StatusCode SaveFloat(const std::string& Branch, bool SaveVariations = true);
        StatusCode SaveDouble(const std::string& Branch, bool SaveVariations = true);
        StatusCode SaveChar(const std::string& Branch, bool SaveVariations = true);
        StatusCode SaveIntegerVector(const std::string& Branch, bool SaveVariations = true);
        StatusCode SaveFloatVector(const std::string& Branch, bool SaveVariations = true);
        StatusCode SaveDoubleVector(const std::string& Branch, bool SaveVariations = true);
        StatusCode SaveCharVector(const std::string& Branch, bool SaveVariations = true);

        template <typename T> StatusCode SaveVariable(const std::string& Branch, bool SaveVariations = true);
        template <typename T> StatusCode SaveVariable(const std::vector<std::string>& BranchVector, bool SaveVariations = true);
        using IStorage::SaveVariable;
        void pipeVariableToAllTrees(const std::string& variable);
        void pipeVariableToAllTrees(const std::vector<std::string>& variables);

    protected:
        typedef std::shared_ptr<ITreeBranchVariable> (DataVectorStorage::*StoreVariable)(const std::string& name, TreeBase* base);
        StatusCode AddVariable(const std::string& Branch, DataVectorStorage::StoreVariable, bool SaveVariations);

    private:
        template <typename T> std::shared_ptr<ITreeBranchVariable> createBranch(const std::string& name, TreeBase* base);
        void clear();

        std::string m_Name;
        std::map<const CP::SystematicSet*, std::unique_ptr<SystematicContainer>> m_containers;
        // Store how many branches were created by the storage to erase the container if the limit is reached

        struct AdditionalBranches {
            AdditionalBranches(bool S, DataVectorStorage::StoreVariable D) {
                SaveSyst = S;
                DataType = D;
            }
            DataVectorStorage::StoreVariable DataType;
            bool SaveSyst;
        };
        std::vector<std::string> m_vars_to_all;
        std::map<std::string, AdditionalBranches> m_Vars;

        bool m_Cleared;
    };

    class ParticleStorage : public DataVectorStorage {
    public:
        ParticleStorage(const std::string& Name, XAMPP::IEventInfo* info, bool isCommon = false);

        virtual std::vector<std::shared_ptr<ITreeBranchVariable>> CreateParticleTree(TreeBase* base_tree);

        xAOD::IParticleContainer* Container(const CP::SystematicSet* set);
        xAOD::IParticleContainer* Container();

        // Saves the mass in the four momentum instead of the energy
        void SaveMassInP4(bool B);
        virtual ~ParticleStorage() = default;

    private:
        bool m_UseMass;
    };
}  // namespace XAMPP
#include <XAMPPbase/EventStorage.ixx>
#endif
