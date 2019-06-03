#ifndef XAMPPbase_BaseAnalysisModule_H
#define XAMPPbase_BaseAnalysisModule_H
#include <AsgTools/AnaToolHandle.h>
#include <AsgTools/AsgTool.h>
#include <AsgTools/ToolHandle.h>
#include <XAMPPbase/Defs.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/EventStorage.h>
#include <XAMPPbase/IAnalysisModule.h>
#include <XAMPPbase/ReconstructedParticles.h>

//  Basic Tool class equipped with some common functions
//      -- EventInfo && ParticleConstructor are available by default
//      -- EventVariables can be assigned with default values. Such that one does not need to bother to set them if they're invalid
//          --> in these cases please call fillDefaultValues() in your fill method.
//      -- All made branches can have a preposition which can be set via property (GroupName)
//      -- The class itself is still virtual since Book and Fill are not overwritten!
namespace XAMPP {
    class IStorageDefaultValues {
    public:
        virtual ~IStorageDefaultValues() {}
        virtual StatusCode Fill() = 0;
    };
    class BaseAnalysisModule : public asg::AsgTool, virtual public IAnalysisModule {
    public:
        BaseAnalysisModule(const std::string& myname);
        virtual ~BaseAnalysisModule();
        virtual StatusCode initialize();

    private:
        asg::AnaToolHandle<XAMPP::IEventInfo> m_InfoHandle;

    protected:
        StatusCode fillDefaultValues();
        StatusCode bookParticleStore(const std::string& name, ParticleStorage*& store, bool use_mass = true, bool saveVariations = true,
                                     bool writeTrees = true);
        // Final name of the branch in the output tree
        std::string full_name(const std::string& name) const;
        template <typename T>
        StatusCode newVariable(const std::string& name, XAMPP::Storage<T>*& store, bool saveToTree = true, bool SaveVariations = true);
        template <typename T, typename T1>
        StatusCode newVariable(const std::string& name, const T1&, XAMPP::Storage<T>*& store, bool saveToTree = true,
                               bool SaveVariations = true);
        StatusCode retrieve(XAMPP::ParticleStorage*& store, const std::string& name);

        asg::AnaToolHandle<XAMPP::IReconstructedParticles> m_ParticleConstructor;
        XAMPP::EventInfo* m_XAMPPInfo;

    private:
        std::vector<std::unique_ptr<IStorageDefaultValues>> m_defaults;
        std::string m_groupName;
        // make groupName be interpreted as a suffix rather than prefix
        bool m_suffixNotation;
    };
    template <class T> class StorageDefaultValues : public IStorageDefaultValues {
    public:
        StorageDefaultValues(XAMPP::Storage<T>* store, const T def_value);
        virtual StatusCode Fill();

    private:
        XAMPP::Storage<T>* m_store;
        const T m_def_value;
    };
    template <class T>
    StorageDefaultValues<T>::StorageDefaultValues(XAMPP::Storage<T>* store, const T def_value) : m_store(store), m_def_value(def_value) {}
    template <class T> StatusCode StorageDefaultValues<T>::Fill() {
        if (m_store == nullptr) return StatusCode::FAILURE;
        if (!m_store->isAvailable()) return m_store->Store(m_def_value);
        return StatusCode::SUCCESS;
    }
    template <typename T>
    StatusCode BaseAnalysisModule::newVariable(const std::string& name, XAMPP::Storage<T>*& store, bool saveToTree, bool SaveVariations) {
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<T>(full_name(name), saveToTree, SaveVariations));
        store = m_XAMPPInfo->GetVariableStorage<T>(full_name(name));
        return StatusCode::SUCCESS;
    }
    template <typename T, typename T1>
    StatusCode BaseAnalysisModule::newVariable(const std::string& name, const T1& def, XAMPP::Storage<T>*& store, bool saveToTree,
                                               bool SaveVariations) {
        ATH_CHECK(newVariable(name, store, saveToTree, SaveVariations));
        m_defaults.push_back(std::unique_ptr<IStorageDefaultValues>(new StorageDefaultValues<T>(store, def)));
        return StatusCode::SUCCESS;
    }
}  // namespace XAMPP
#endif
