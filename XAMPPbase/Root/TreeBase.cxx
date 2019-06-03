#include <GaudiKernel/ITHistSvc.h>
#include <PATInterfaces/SystematicSet.h>
#include <XAMPPbase/AnalysisConfig.h>
#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/ISystematics.h>
#include <XAMPPbase/TreeBase.h>
#include <XAMPPbase/TreeHelpers.h>
#include <iostream>

namespace XAMPP {

    TreeBase::TreeBase(const CP::SystematicSet* set) : TreeBase(set, std::shared_ptr<SystematicGroup>()) {}
    TreeBase::TreeBase(const CP::SystematicSet* set, std::shared_ptr<SystematicGroup> grp) :
        m_set(set),
        m_syst_group(grp),
        m_XAMPPInfo(nullptr),
        m_systematics(nullptr),
        m_config(nullptr),
        m_tree(nullptr),
        m_directory(nullptr),
        m_init(false),
        m_isWritten(false),
        m_histSvc("THistSvc", set != nullptr ? RemoveAllExpInStr("TreeBase" + set->name(), "_") : "CommonTree"),
        m_friend_trees(),
        m_Branches(),
        m_mcChannelNumber(-1),
        m_eventId() {
        m_eventId[0] = m_eventId[1] = 0;
    }

    TreeBase::~TreeBase() {}
    std::shared_ptr<SystematicGroup> TreeBase::getSystematicGroup() const { return m_syst_group; }
    std::string TreeBase::tree_name() const {
        if (m_config == nullptr) {
            Warning("TreeBase::tree_name()", "No analysis config defined thus far");
            return "Unkown name";
        }
        if (m_set == nullptr) return "CommonTree_" + m_config->TreeName();
        if (m_syst_group != nullptr) { return "SystGroup_" + m_syst_group->name() + "_" + m_config->TreeName(); }
        if (m_set == m_systematics->GetNominal()) return m_config->TreeName() + "_Nominal";
        return m_config->TreeName() + "_" + m_set->name();
    }
    bool TreeBase::isInitialized() const { return m_init; }
    TTree* TreeBase::Tree() const { return m_tree.get(); }
    const CP::SystematicSet* TreeBase::systematic() const { return m_set; }
    void TreeBase::SetAnalysisConfig(const ToolHandle<XAMPP::IAnalysisConfig>& config) {
        if (!m_init) m_config = config.operator->();
    }

    void TreeBase::SetEventInfoHandler(const XAMPP::EventInfo* Info) {
        if (!m_init) m_XAMPPInfo = Info;
    }
    void TreeBase::SetListOfFriends(const std::vector<std::shared_ptr<TreeBase>>& friends) {
        for (const auto& fr : friends) AddFriend(fr);
    }
    void TreeBase::AddFriend(std::shared_ptr<TreeBase> tree_base) {
        if (tree_base.get() == this || !tree_base.get() || m_init) { return; }
        if (IsInVector(tree_base, m_friend_trees)) return;
        m_friend_trees.push_back(tree_base);
    }
    void TreeBase::SetSystematicsTool(const ToolHandle<XAMPP::ISystematics>& Syst) {
        if (!m_init) m_systematics = Syst.operator->();
    }

    StatusCode TreeBase::InitializeTree() {
        if (m_init) {
            Error("TreeBase::InitializeTree()", "The TreeBase is already intialized for systematic %s", m_set->name().c_str());
            return StatusCode::FAILURE;
        }
        if (!m_XAMPPInfo) {
            Error("TreeBase::InitializeTree()", "No XAMPPInfo object could be found");
            return StatusCode::FAILURE;
        }
        if (!m_systematics) {
            Error("TreeBase::InitializeTree()", "Could not retrieve the systematics tool");
            return StatusCode::FAILURE;
        }
        if (!m_config) {
            Error("TreeBase::InitializeTree()", "Unable to get the associated AnalysisConfig class");
            return StatusCode::FAILURE;
        }
        if (m_set != m_systematics->GetNominal() && m_syst_group) {
            Error("TreeBase::InitializeTree()", "Trees assigned to systematic group must only be initialized with nominal only.");
            return StatusCode::FAILURE;
        }
        if (!m_set) {
            Info("TreeBase::InitializeTree()", "The tree %s saving the common event variables only", tree_name().c_str());
        } else if (m_set != m_systematics->GetNominal())
            Info("TreeBase::InitializeTree()",
                 "The tree writes  %s kinematic systematics %s. Variations of the event weights will not be stored", tree_name().c_str(),
                 m_set->name().c_str());
        else if (m_syst_group) {
            Info("TreeBase::InitializeTree()", "The tree %s writes the nominal branches of the systematic group %s", tree_name().c_str(),
                 m_syst_group->name().c_str());
        }
        m_tree = std::make_unique<TTree>(tree_name().c_str(), "SmallTree for fast analysis");
        /// cf. documentation
        /// here https://root.cern.ch/doc/v614/classTTree.html#ad4c7c7d70caf5657104832bcfbd83a9f
        /// Set the autoflush value to 3MB of data.
        m_tree->SetAutoFlush(-3000000);
        m_tree->SetAutoSave(-3000000);
        if (!m_histSvc->regTree(Form("/XAMPP/%s", tree_name().c_str()), Tree()).isSuccess()) { return StatusCode::FAILURE; }
        m_directory = m_tree->GetDirectory();
        if (!m_directory) {
            Error("TreeBase::InitializeTree()", "Where is my directory to write later?");
            return StatusCode::FAILURE;
        }
        if (!m_systematics->isData()) {
            if (!m_set || (m_friend_trees.empty() && !m_syst_group)) { m_tree->Branch("mcChannelNumber", &m_mcChannelNumber); }
        }
        if (!m_friend_trees.empty() || !m_set || m_syst_group) m_tree->Branch("CommonEventHash", m_eventId, "CommonEventHash[2]/l");
        CreateBranches();
        std::vector<XAMPP::Storage<XAMPPmet>*> MetStorageVector = m_XAMPPInfo->GetStorages<XAMPPmet>(IEventInfo::OutputElement::Tree);
        for (auto& entry : MetStorageVector) {
            if (addVariable(entry)) m_Branches.push_back(std::make_shared<TreeHelperMet>(entry, Tree()));
        }
        std::vector<DataVectorStorage*> Particles = m_XAMPPInfo->GetContainerStorages(IEventInfo::OutputElement::Tree);
        for (const auto& P : Particles) {
            if (!addVariable(P)) continue;
            CopyVector(P->CreateParticleTree(this), m_Branches, false);
        }
        std::sort(m_Branches.begin(), m_Branches.end(),
                  [](const std::shared_ptr<XAMPP::ITreeBranchVariable>& a, const std::shared_ptr<XAMPP::ITreeBranchVariable>& b) {
                      return a->name() < b->name();
                  });
        for (const auto& B : m_Branches) {
            if (!B->Init()) {
                Error("TreeBase::InitializeTree()", "Could not setup branch %s", B->name().c_str());
                return StatusCode::FAILURE;
            }
        }
        if (m_Branches.empty()) {
            Error("TreeBase::initializeTree()", "%s has no branches assigned.", tree_name().c_str());
            return StatusCode::FAILURE;
        }
        m_isWritten = false;
        m_init = true;
        return StatusCode::SUCCESS;
    }
    void TreeBase::CreateBranches() {
        CreateEventBranches<double>();
        CreateEventBranches<float>();
        CreateEventBranches<int>();
        CreateEventBranches<char>();
        CreateEventBranches<bool>();
        CreateEventBranches<unsigned int>();
        CreateEventBranches<size_t>();
        CreateEventBranches<unsigned long long>();
        CreateEventBranches<long long>();
    }
    StatusCode TreeBase::FillTree() {
        // No run number is beyond 400000 thus far
        // But the runNumber usually is 284500 310000 300000 so we can get rid of the last 2 digits
        static const unsigned max_period_bit = max_bit(9999);

        ULong64_t event_id[2] = {m_XAMPPInfo->eventNumber(), 0};
        if (!m_systematics->isData()) {
            // recall that the mcChannelNumber is 6 digits long as well as the runNumber
            m_mcChannelNumber = m_XAMPPInfo->mcChannelNumber();
            event_id[1] = (m_mcChannelNumber << max_period_bit) | int(m_XAMPPInfo->runNumber() / 100);
        } else {
            event_id[1] = m_XAMPPInfo->runNumber();
        }
        /// The FillTree method has already been called from another TreeBase object  so exit silently not filling the tree again
        if (event_id[0] == m_eventId[0] && event_id[1] == m_eventId[1]) { return StatusCode::SUCCESS; }
        m_eventId[0] = event_id[0];
        m_eventId[1] = event_id[1];

        /// Fill the friends first
        for (auto& fr : m_friend_trees) {
            if (!fr->FillTree().isSuccess()) return StatusCode::FAILURE;
        }
        for (auto& B : m_Branches) {
            if (!B->Fill()) {
                Error("TreeBase::FillTree()",
                      "The branch %s was not updated since the last call of ComputeEventVariables(). Tree %s cannot be filled",
                      B->name().c_str(), tree_name().c_str());
                return StatusCode::FAILURE;
            }
        }
        if (m_tree->Fill() < 0) {
            Error("TreeBase::FillTree()", "Failed to fill the tree");
            return StatusCode::FAILURE;
        }
        return StatusCode::SUCCESS;
    }
    StatusCode TreeBase::FinalizeTree() {
        if (m_isWritten) { return StatusCode::SUCCESS; }
        m_isWritten = true;
        if (!m_histSvc->deReg(Tree()).isSuccess()) {
            Error("TreeBase::FinalizeTree()", "Failed to put the tree out of the HistService");
            return StatusCode::FAILURE;
        }
        for (auto& fr : m_friend_trees) {
            if (!fr->FinalizeTree().isSuccess()) return StatusCode::FAILURE;
            if (m_tree->AddFriend(fr->Tree()) == nullptr) {
                Error("TreeBase::FinalizeTree()", "Failed to establish the friendship to %s", fr->tree_name().c_str());
                return StatusCode::FAILURE;
            }
        }
        m_directory->WriteObject(m_tree.get(), m_tree->GetName());
        Info("TreeBase::FinalizeTree()", "Successfully written %s containing %llu entries.", tree_name().c_str(), m_tree->GetEntries());
        if (!m_friend_trees.empty() || !(!m_set || m_syst_group)) {
            m_tree.reset();
            m_Branches.clear();
        }
        m_friend_trees.clear();
        return StatusCode::SUCCESS;
    }
    void TreeBase::SetHistService(ServiceHandle<ITHistSvc>& Handle) {
        if (!m_init) m_histSvc = Handle;
    }
    bool TreeBase::addVariable(XAMPP::IStorage* store) const {
        if (m_set == nullptr) return store->IsCommonVariable();
        if (!m_friend_trees.empty() && store->IsCommonVariable()) return false;

        /// Current tree is a group tree and the group does not belong to that tree
        if (m_syst_group && store->getSystematicGroup() != m_syst_group) return false;
        if (m_set != m_systematics->GetNominal()) return store->SaveVariations();
        // Since the particle variable adds many branches it's not settled a priori which of them
        // belong to the systematic group and which not. The p4, the charge is for sure part of it but flags like
        // passOR
        if (store->IsParticleVariable()) return true;
        if (!m_friend_trees.empty() && store->getSystematicGroup()) return store->getSystematicGroup()->isAffectedBySyst(systematic());
        return true;
    }

}  // namespace XAMPP
