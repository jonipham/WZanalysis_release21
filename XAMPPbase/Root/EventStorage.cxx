#include <PATInterfaces/SystematicSet.h>
#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/Cuts.h>
#include <XAMPPbase/EventStorage.h>
#include <XAMPPbase/TreeBase.h>
namespace XAMPP {
    StorageKeeper* StorageKeeper::m_Inst = nullptr;
    StorageKeeper* StorageKeeper::GetInstance() {
        if (!m_Inst) m_Inst = new StorageKeeper();
        return m_Inst;
    }
    StorageKeeper::StorageKeeper() :
        m_CommonKeeper(std::make_shared<StorageKeeper::InfoKeeper>()),
        m_Keepers(),
        m_Cuts(),
        m_Locked(false) {}
    bool StorageKeeper::isLocked() const { return m_Locked; }
    void StorageKeeper::Lock() { m_Locked = true; }
    StorageKeeper::~StorageKeeper() {
        for (auto& Cut : m_Cuts) {
            if (Cut != nullptr) delete Cut;
        }
        Info("StoreageKeeper()", "Destructor called");
        m_Inst = nullptr;
    }
    StorageKeeper::InfoKeeper* StorageKeeper::FindKeeper(const IEventInfo* Info) const {
        if (Info == nullptr) { return m_CommonKeeper.get(); }
        for (const auto& InfoKeeper : m_Keepers) {
            if (InfoKeeper->GetInfo() == Info) { return InfoKeeper.get(); }
        }
        return nullptr;
    }
    bool StorageKeeper::EventStorageExists(const std::string& Name, const IEventInfo* EvInfo) const {
        StorageKeeper::InfoKeeper* Keeper = FindKeeper(EvInfo);
        if (Keeper && Keeper->EventStorageExists(Name)) { return true; }
        return Keeper != m_CommonKeeper.get() && m_CommonKeeper->EventStorageExists(Name);
    }
    bool StorageKeeper::ParticleDefined(const std::string& Name, const IEventInfo* EvInfo) const {
        StorageKeeper::InfoKeeper* Keeper = FindKeeper(EvInfo);
        if (Keeper && Keeper->ParticleDefined(Name)) { return true; }
        return Keeper != m_CommonKeeper.get() && m_CommonKeeper->ParticleDefined(Name);
    }
    bool StorageKeeper::Register(IStorage* S) {
        if (!S) {
            Error("StorageKeeper::Register()", "No element given");
            return false;
        }
        if (isLocked()) Error("StorageKeeper::Register()", "Already locked");
        if (S->IsCommonVariable()) { return m_CommonKeeper->Register(S) && !isLocked(); }
        StorageKeeper::InfoKeeper* Keeper = FindKeeper(S->XAMPPInfo());
        if (!Keeper) {
            Info("StorageKeeper::Register()", ("Found new Info object " + S->XAMPPInfo()->name()).c_str());
            m_Keepers.push_back(std::make_shared<StorageKeeper::InfoKeeper>(S->XAMPPInfo()));
            Keeper = FindKeeper(S->XAMPPInfo());
        }
        return Keeper->Register(S) && !isLocked();
    }
    std::vector<DataVectorStorage*> StorageKeeper::RetrieveContainerStores(const IEventInfo* Info) const {
        std::vector<DataVectorStorage*> S = m_CommonKeeper->GetContainerStorages();
        StorageKeeper::InfoKeeper* Keeper = FindKeeper(Info);
        if (Keeper) {
            std::vector<DataVectorStorage*> S1 = Keeper->GetContainerStorages();
            std::copy(S1.begin(), S1.end(), std::back_inserter(S));
        }
        return S;
    }
    DataVectorStorage* StorageKeeper::RetrieveContainerStorage(const std::string& Name, const IEventInfo* Info) const {
        DataVectorStorage* S = m_CommonKeeper->GetContainerStorage(Name);
        if (S) { return S; }
        StorageKeeper::InfoKeeper* Keeper = FindKeeper(Info);
        if (!Keeper) { return nullptr; }
        return Keeper->GetContainerStorage(Name);
    }

    StorageKeeper::InfoKeeper::InfoKeeper(const IEventInfo* Info) : m_RefInfo(Info), m_Storages(), m_ParticleStores() {}
    const IEventInfo* StorageKeeper::InfoKeeper::GetInfo() const { return m_RefInfo; }
    bool StorageKeeper::InfoKeeper::EventStorageExists(const std::string& Name) const { return m_Storages.find(Name) != m_Storages.end(); }
    bool StorageKeeper::InfoKeeper::ParticleDefined(const std::string& Name) const {
        return m_ParticleStores.find(Name) != m_ParticleStores.end();
    }
    bool StorageKeeper::InfoKeeper::Register(IStorage* Storage) {
        if (!Storage->IsParticleVariable()) {
            if (EventStorageExists(Storage->name())) {
                Error("InfoKeeper::Register()", "The storage %s already exists.", Storage->name().c_str());
                return false;
            }
            m_Storages.insert(std::pair<std::string, std::shared_ptr<IStorage>>(Storage->name(), std::shared_ptr<IStorage>(Storage)));
            return true;
        }
        if (ParticleDefined(Storage->name())) {
            Error("InfoKeeper::Register()", "The particle %s is already defined", Storage->name().c_str());
            return false;
        }

        m_ParticleStores.insert(std::pair<std::string, std::shared_ptr<IStorage>>(Storage->name(), std::shared_ptr<IStorage>(Storage)));
        return true;
    }
    DataVectorStorage* StorageKeeper::InfoKeeper::GetContainerStorage(const std::string& Name) const {
        if (!ParticleDefined(Name)) { return nullptr; }
        return dynamic_cast<DataVectorStorage*>(m_ParticleStores.at(Name).get());
    }
    std::vector<DataVectorStorage*> StorageKeeper::InfoKeeper::GetContainerStorages() const {
        std::vector<DataVectorStorage*> S;
        for (auto& StorePairs : m_ParticleStores) { S.push_back(dynamic_cast<DataVectorStorage*>(StorePairs.second.get())); }
        return S;
    }
    StorageKeeper::InfoKeeper::~InfoKeeper() {}
    void StorageKeeper::AttachCut(Cut* C) {
        if (!IsInVector(C, m_Cuts)) m_Cuts.push_back(C);
    }
    void StorageKeeper::DettachCut(Cut* C) {
        for (auto& defined : m_Cuts) {
            if (C == defined) defined = nullptr;
        }
    }
    //################################################################################################################################
    //                                              SystematicGroup
    //################################################################################################################################
    SystematicGroup::SystematicGroup(const std::string& name, XAMPP::SelectionObject obj, const ToolHandle<ISystematics>& syst_tool) :
        m_name(name),
        m_obj(obj),
        m_systematics(syst_tool) {}
    std::string SystematicGroup::name() const { return m_name; }
    bool SystematicGroup::isAffectedBySyst(const CP::SystematicSet* syst) const {
        return (syst != m_systematics->GetNominal() || m_systematics->GetKinematicSystematics(m_obj).size() == 1) &&
               IsInVector(syst, m_systematics->GetKinematicSystematics(m_obj));
    }
    //################################################################################################################################
    //                                              IStorage
    //################################################################################################################################
    IStorage::IStorage(const std::string& Name, IEventInfo* Info, bool IsParticleVariable, bool IsCommonVariable) :
        m_Name(Name),
        m_Info(Info),
        m_IsParticleVariable(IsParticleVariable),
        m_Registered(false),
        m_SaveHisto(false),
        m_SaveTree(false),
        m_SaveAlways(true),
        m_IsCommon(IsCommonVariable),
        m_HistoTemplates(),
        m_systGroup(nullptr) {
        m_Registered = StorageKeeper::GetInstance()->Register(this);
    }
    IEventInfo* IStorage::XAMPPInfo() const { return m_Info; }
    std::string IStorage::name() const { return m_Name; }
    bool IStorage::IsCommonVariable() const { return m_IsCommon; }
    void IStorage::SetSaveTrees(bool B) { m_SaveTree = B; }
    void IStorage::SetSaveHistos(bool B) { m_SaveHisto = B; }
    void IStorage::SetSaveVariations(bool B) { m_SaveAlways = B; }
    bool IStorage::SaveTrees() const { return m_SaveTree; }
    bool IStorage::SaveHistos() const { return m_SaveHisto; }
    bool IStorage::SaveVariations() const { return m_SaveAlways; }
    bool IStorage::IsParticleVariable() const { return m_IsParticleVariable; }

    bool IStorage::SaveVariable() const { return SaveTrees() || SaveHistos(); }
    bool IStorage::isRegistered() const { return m_Registered; }
    IStorage::~IStorage() {}
    std::vector<std::string> IStorage::GetHistoVariables() const {
        std::vector<std::string> Variables;
        if (SaveHistos()) {
            Variables.reserve(m_HistoTemplates.size());
            for (const auto& Temp : m_HistoTemplates) { Variables.push_back(Temp.first); }
        }
        return Variables;
    }
    TH1* IStorage::Template(const std::string& Name) const {
        std::map<std::string, std::shared_ptr<TH1>>::const_iterator Itr = m_HistoTemplates.find(Name);
        if (Itr != m_HistoTemplates.end()) { return Itr->second.get(); }
        Error("IStorage::Template()", "No template called %s has been provided thus for Storage %s", Name.c_str(), name().c_str());
        return nullptr;
    }
    StatusCode IStorage::CreateHistogram(const std::string& variable, TH1* Template) {
        return CreateHistogram(variable, std::shared_ptr<TH1>(Template));
    }
    StatusCode IStorage::CreateHistogram(const std::string& variable, std::shared_ptr<TH1> Template) {
        if (!Template) {
            Error("IStorage::CreateHistogram()", "No TH1 object has been given to %s", name().c_str());
            return StatusCode::FAILURE;
        }
        if (m_HistoTemplates.find(variable) != m_HistoTemplates.end()) {
            Error("IStorage::Template()", "The histogram name %s is already defined for %s", variable.c_str(), name().c_str());
            return StatusCode::FAILURE;
        }
        m_HistoTemplates.insert(std::pair<std::string, std::shared_ptr<TH1>>(variable, Template));
        return StatusCode::SUCCESS;
    }
    std::shared_ptr<SystematicGroup> IStorage::getSystematicGroup() const { return m_systGroup; }

    StatusCode IStorage::setSystematicGroup(const std::string& grp_name) {
        if (grp_name.empty()) {
            Error("IStorage::setSystematicGroup()", "Empty group names are not allowed");
            return StatusCode::FAILURE;
        } else if (m_systGroup) {
            Error("IStorage::setSystematicGroup()", "%s has already the systematig group %s assigned.", name().c_str(),
                  m_systGroup->name().c_str());
            return StatusCode::FAILURE;
        }
        m_systGroup = XAMPPInfo()->getSystematicGroup(grp_name);
        if (!m_systGroup) {
            Error("IStorage::setSystematicGroup", "Systematic group %s does not exists.", grp_name.c_str());
            return StatusCode::FAILURE;
        }
        return StatusCode::SUCCESS;
    }

    SystematicContainer::SystematicContainer(const DataVectorStorage* store, const CP::SystematicSet* set) :
        m_refStore(store),
        m_set(set),
        m_particle_container(nullptr),
        m_sg_container(nullptr),
        m_converted_container(nullptr),
        m_lastSet(-1) {}

    void SystematicContainer::setContainer(xAOD::IParticleContainer* container) {
        m_particle_container = container;
        m_sg_container = nullptr;
        m_lastSet = m_refStore->XAMPPInfo()->eventNumber();
    }
    void SystematicContainer::createAuxElementsContainer() {
        if (m_particle_container == nullptr || m_sg_container != nullptr || !isContainerValid()) return;
        m_converted_container = std::make_unique<DataVector<SG::AuxElement>>(SG::VIEW_ELEMENTS);
        m_converted_container->reserve(m_particle_container->size());
        for (const auto obj : *m_particle_container) m_converted_container->push_back(obj);
        m_sg_container = m_converted_container.get();
    }
    void SystematicContainer::setContainer(DataVector<SG::AuxElement>* container) {
        m_sg_container = container;
        m_particle_container = nullptr;
        m_lastSet = m_refStore->XAMPPInfo()->eventNumber();
    }
    xAOD::IParticleContainer* SystematicContainer::particleContainer() const { return isContainerValid() ? m_particle_container : nullptr; }
    DataVector<SG::AuxElement>* SystematicContainer::auxElementContainer() const { return isContainerValid() ? m_sg_container : nullptr; }
    const CP::SystematicSet* SystematicContainer::systematic() const { return m_set; }
    bool SystematicContainer::isContainerValid() const { return m_lastSet == m_refStore->XAMPPInfo()->eventNumber(); }
    //################################################################################################################################
    //                                              DataVectorStorage
    //################################################################################################################################
    DataVectorStorage::DataVectorStorage(const std::string& Name, XAMPP::IEventInfo* info, bool IsCommon) :
        IStorage(Name, info, true, IsCommon),
        m_containers(),
        m_vars_to_all(),
        m_Cleared(false) {}
    StatusCode DataVectorStorage::Fill(DataVector<SG::AuxElement>* Container) {
        if (Container == nullptr) {
            Error("DataVectorStorage::Fill()", "No valid SG::AuxElement container has been given to %s", name().c_str());
            return StatusCode::FAILURE;
        }
        SystematicContainer* to_store = findContainer();
        to_store->setContainer(Container);
        clear();
        return StatusCode::SUCCESS;
    }
    StatusCode DataVectorStorage::Fill(xAOD::IParticleContainer* Particles) {
        if (Particles == nullptr) {
            Error("DataVectorStorage::Fill()", "No valid IParticleContainercontainer has been given to %s", name().c_str());
            return StatusCode::FAILURE;
        }
        SystematicContainer* to_store = findContainer();
        to_store->setContainer(Particles);
        clear();
        // Check for the systematic group
        if (!getSystematicGroup()) return StatusCode::SUCCESS;
        // The container is not independent from the nominal container... make sure that the nominal
        // container is valid and set
        const CP::SystematicSet* current_syst = XAMPPInfo()->GetSystematic();
        if (XAMPPInfo()->GetNominal() == current_syst || getSystematicGroup()->isAffectedBySyst(current_syst)) return StatusCode::SUCCESS;

        SystematicContainer* nominal_container = findContainer(XAMPPInfo()->GetNominal());
        if (!nominal_container || !nominal_container->isContainerValid()) {
            Error("DataVectorStorage::Fill()",
                  "Systematic groups demand that the nominal container is filled in the same event. Which is not the case for %s in "
                  "systematic %s",
                  name().c_str(), current_syst->name().c_str());
            return StatusCode::FAILURE;
        }
        /// At the next step we need to ensure synchronization between systematic
        /// container and nominal containter...
        xAOD::IParticleContainer* nominal_particles = nominal_container->particleContainer();
        /// Two times the same container is given...
        if (nominal_particles == Particles) return StatusCode::SUCCESS;

        /// Before looping let's check their sizes
        if (nominal_particles->size() != Particles->size()) {
            Error("DataVectorStorage::Fill()", "The nominal container has a different size than the current given container  (%lu vs. %lu)",
                  Particles->size(), nominal_container->particleContainer()->size());
            return StatusCode::FAILURE;
        }
        /// If empty then we can safely bail out
        if (Particles->empty()) return StatusCode::SUCCESS;
        xAOD::IParticleContainer::const_iterator nominal_begin = nominal_particles->begin();
        xAOD::IParticleContainer::const_iterator syst_begin = Particles->begin();

        xAOD::IParticleContainer::const_iterator nominal_end = nominal_particles->end();
        xAOD::IParticleContainer::const_iterator syst_end = Particles->end();
        /// loop element by element and compare whether they acctually are the same
        while (nominal_begin != nominal_end && syst_begin != syst_end) {
            if (*nominal_begin != *syst_begin) {
                PromptParticle(*syst_begin, current_syst->name());
                PromptParticle(*nominal_begin, "Nominal");
                Error("DataVectorStorage::Fill()", "The two containers do not match in terms of particle content");
                return StatusCode::FAILURE;
            }
            ++nominal_begin;
            ++syst_begin;
        }
        return StatusCode::SUCCESS;
    }
    void DataVectorStorage::clear() {
        if (!m_Cleared) {
            m_vars_to_all.clear();
            m_Vars.clear();
            m_Cleared = true;
        }
    }
    SystematicContainer* DataVectorStorage::findContainer(const CP::SystematicSet* set) {
        std::map<const CP::SystematicSet*, std::unique_ptr<SystematicContainer>>::const_iterator itr = m_containers.find(set);
        if (itr != m_containers.end()) return itr->second.get();
        m_containers.insert(std::pair<const CP::SystematicSet*, std::unique_ptr<SystematicContainer>>(
            set, std::make_unique<SystematicContainer>(this, set)));
        return findContainer(set);
    }
    SystematicContainer* DataVectorStorage::findContainer() {
        return findContainer(!IsCommonVariable() ? XAMPPInfo()->GetSystematic() : XAMPPInfo()->GetNominal());
    }
    std::vector<std::shared_ptr<ITreeBranchVariable>> DataVectorStorage::CreateParticleTree(TreeBase* base_tree) {
        std::vector<std::shared_ptr<ITreeBranchVariable>> branches;
        if (!base_tree) {
            Error("ParticleStoarge::CreateParticleTree()", "No tree was given to %s", name().c_str());
            return branches;
        }
        // Some mechanism to store all the branches;
        for (const auto& Branch : m_Vars) {
            // Skip variables which should only be saved to the nominal tree
            if (!Branch.second.SaveSyst && base_tree->systematic() != XAMPPInfo()->GetNominal()) continue;
            // Variable is not going to be propagated to all trees
            if (getSystematicGroup() && getSystematicGroup() != base_tree->getSystematicGroup() &&
                !getSystematicGroup()->isAffectedBySyst(base_tree->systematic()) && !IsInVector(Branch.first, m_vars_to_all))
                continue;
            // Skip variables which are going to be propagated to all trees
            if (getSystematicGroup() && getSystematicGroup() == base_tree->getSystematicGroup() && IsInVector(Branch.first, m_vars_to_all))
                continue;
            branches.push_back((this->*Branch.second.DataType)(Branch.first, base_tree));
        }
        return branches;
    }
    DataVector<SG::AuxElement>* DataVectorStorage::Container() {
        return Container(IsCommonVariable() ? XAMPPInfo()->GetNominal() : XAMPPInfo()->GetSystematic());
    }
    DataVector<SG::AuxElement>* DataVectorStorage::Container(const CP::SystematicSet* set) {
        SystematicContainer* syst = findContainer(set);
        if (!syst->auxElementContainer()) { syst->createAuxElementsContainer(); }
        return syst->auxElementContainer();
    }

    StatusCode DataVectorStorage::AddVariable(const std::string& Branch, DataVectorStorage::StoreVariable V, bool SaveVariations) {
        if (!m_containers.empty()) {
            Error("DataVectorStorage::AddVariable()", "Storage %s is already locked", name().c_str());
            return StatusCode::FAILURE;
        }
        if (m_Vars.find(Branch) != m_Vars.end()) {
            Error("DataVectorStorage::AddVariable()", "Variable %s has already been added for %s", Branch.c_str(), name().c_str());
            return StatusCode::FAILURE;
        }
        m_Vars.insert(std::pair<std::string, AdditionalBranches>(Branch, AdditionalBranches(SaveVariations, V)));
        return StatusCode::SUCCESS;
    }
    StatusCode DataVectorStorage::SaveInteger(const std::string& Branch, bool SaveVariations) {
        return AddVariable(Branch, &DataVectorStorage::createBranch<int>, SaveVariations);
    }
    StatusCode DataVectorStorage::SaveFloat(const std::string& Branch, bool SaveVariations) {
        return AddVariable(Branch, &DataVectorStorage::createBranch<float>, SaveVariations);
    }
    StatusCode DataVectorStorage::SaveDouble(const std::string& Branch, bool SaveVariations) {
        return AddVariable(Branch, &DataVectorStorage::createBranch<double>, SaveVariations);
    }
    StatusCode DataVectorStorage::SaveChar(const std::string& Branch, bool SaveVariations) {
        return AddVariable(Branch, &DataVectorStorage::createBranch<char>, SaveVariations);
    }
    StatusCode DataVectorStorage::SaveIntegerVector(const std::string& Branch, bool SaveVariations) {
        return AddVariable(Branch, &DataVectorStorage::createBranch<std::vector<int>>, SaveVariations);
    }
    StatusCode DataVectorStorage::SaveFloatVector(const std::string& Branch, bool SaveVariations) {
        return AddVariable(Branch, &DataVectorStorage::createBranch<std::vector<float>>, SaveVariations);
    }
    StatusCode DataVectorStorage::SaveDoubleVector(const std::string& Branch, bool SaveVariations) {
        return AddVariable(Branch, &DataVectorStorage::createBranch<std::vector<double>>, SaveVariations);
    }
    StatusCode DataVectorStorage::SaveCharVector(const std::string& Branch, bool SaveVariations) {
        return AddVariable(Branch, &DataVectorStorage::createBranch<std::vector<char>>, SaveVariations);
    }
    void DataVectorStorage::pipeVariableToAllTrees(const std::string& variable) {
        if (!variable.empty() && !IsInVector(variable, m_vars_to_all)) m_vars_to_all.push_back(variable);
    }
    void DataVectorStorage::pipeVariableToAllTrees(const std::vector<std::string>& variables) {
        for (const auto& var : variables) pipeVariableToAllTrees(var);
    }
    //################################################################################################################################
    //                                              ParticleStorage
    //################################################################################################################################
    ParticleStorage::ParticleStorage(const std::string& Name, XAMPP::IEventInfo* info, bool IsCommon) :
        DataVectorStorage(Name, info, IsCommon),
        m_UseMass(false) {}
    void ParticleStorage::SaveMassInP4(bool B) { m_UseMass = B; }
    xAOD::IParticleContainer* ParticleStorage::Container(const CP::SystematicSet* set) { return findContainer(set)->particleContainer(); }
    xAOD::IParticleContainer* ParticleStorage::Container() {
        return Container(IsCommonVariable() ? XAMPPInfo()->GetNominal() : XAMPPInfo()->GetSystematic());
    }
    std::vector<std::shared_ptr<ITreeBranchVariable>> ParticleStorage::CreateParticleTree(TreeBase* base_tree) {
        std::vector<std::shared_ptr<ITreeBranchVariable>> branches = DataVectorStorage::CreateParticleTree(base_tree);
        // pipe the four momenta either into the corresponding systematic groups or in the tree affected by the systematic
        if (base_tree->getSystematicGroup() == getSystematicGroup() ||
            (getSystematicGroup() && getSystematicGroup()->isAffectedBySyst(base_tree->systematic()))) {
            branches.push_back(std::make_shared<ITreeParticleFourMomentConstAccessor>(this, base_tree, &xAOD::IParticle::pt));
            branches.push_back(std::make_shared<ITreeParticleFourMomentConstAccessor>(this, base_tree, &xAOD::IParticle::eta));
            branches.push_back(std::make_shared<ITreeParticleFourMomentConstAccessor>(this, base_tree, &xAOD::IParticle::phi));
            if (m_UseMass) {
                branches.push_back(std::make_shared<ITreeParticleFourMomentConstAccessor>(this, base_tree, &xAOD::IParticle::m));
            } else {
                branches.push_back(std::make_shared<ITreeParticleFourMomentConstAccessor>(this, base_tree, &xAOD::IParticle::e));
            }
        }
        return branches;
    }
}  // namespace XAMPP
