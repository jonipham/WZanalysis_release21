#ifndef XAMPPbase_TreeBase_H
#define XAMPPbase_TreeBase_H
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/TreeHelpers.h>

#include <AsgTools/ToolHandle.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "TFile.h"
#include "TTree.h"

namespace CP {
    class SystematicSet;
}

class ITHistSvc;

namespace XAMPP {
    class IAnalysisConfig;
    class ISystematics;
    class ITreeBranchVariable;

    class TreeBase {
    public:
        TreeBase(const CP::SystematicSet* set);
        TreeBase(const CP::SystematicSet* set, std::shared_ptr<SystematicGroup> grp);
        virtual ~TreeBase();
        StatusCode InitializeTree();
        StatusCode FillTree();
        StatusCode FinalizeTree();
        void SetHistService(ServiceHandle<ITHistSvc>& Handle);

        void SetEventInfoHandler(const XAMPP::EventInfo* Info);
        void SetSystematicsTool(const ToolHandle<XAMPP::ISystematics>& Syst);
        void SetAnalysisConfig(const ToolHandle<XAMPP::IAnalysisConfig>& config);

        void SetListOfFriends(const std::vector<std::shared_ptr<TreeBase>>& friends);
        void AddFriend(std::shared_ptr<TreeBase> tree_base);
        TTree* Tree() const;
        const CP::SystematicSet* systematic() const;
        std::shared_ptr<SystematicGroup> getSystematicGroup() const;
        bool isInitialized() const;

        std::string tree_name() const;

    protected:
        virtual void CreateBranches();
        template <typename T> void CreateEventBranches() {
            std::vector<XAMPP::Storage<T>*> Scalars = m_XAMPPInfo->GetStorages<T>(IEventInfo::OutputElement::Tree);
            for (auto& S : Scalars) {
                if (addVariable(S)) m_Branches.push_back(std::make_shared<EventBranchVariable<T>>(S, Tree()));
            }
            std::vector<XAMPP::Storage<std::vector<T>>*> Vector = m_XAMPPInfo->GetStorages<std::vector<T>>(IEventInfo::OutputElement::Tree);
            for (auto& V : Vector) {
                if (addVariable(V)) m_Branches.push_back(std::make_shared<EventBranchVariable<std::vector<T>>>(V, Tree()));
            }
        }

    private:
        const CP::SystematicSet* m_set;
        std::shared_ptr<SystematicGroup> m_syst_group;
        const XAMPP::EventInfo* m_XAMPPInfo;
        const XAMPP::ISystematics* m_systematics;
        const XAMPP::IAnalysisConfig* m_config;
        std::unique_ptr<TTree> m_tree;
        TDirectory* m_directory;
        bool m_init;
        bool m_isWritten;

        ServiceHandle<ITHistSvc> m_histSvc;
        /// Friend trees rely on
        std::vector<std::shared_ptr<TreeBase>> m_friend_trees;
        std::vector<std::shared_ptr<XAMPP::ITreeBranchVariable>> m_Branches;
        bool addVariable(IStorage* store) const;
        int m_mcChannelNumber;
        ULong64_t m_eventId[2];
    };
}  // namespace XAMPP
#endif
