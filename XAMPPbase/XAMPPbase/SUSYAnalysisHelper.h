#ifndef XAMPPbase_SUSYAnalysisHelper_H
#define XAMPPbase_SUSYAnalysisHelper_H

#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/IAnalysisHelper.h>
#include <XAMPPbase/IAnalysisModule.h>

#include <XAMPPbase/HistoBase.h>
#include <XAMPPbase/TreeHelpers.h>

#include <XAMPPbase/IDiTauSelector.h>
#include <XAMPPbase/IElectronSelector.h>
#include <XAMPPbase/IJetSelector.h>
#include <XAMPPbase/IMuonSelector.h>
#include <XAMPPbase/IPhotonSelector.h>
#include <XAMPPbase/ITauSelector.h>
#include <XAMPPbase/ITruthSelector.h>

#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/EventStorage.h>
#include <XAMPPbase/ITriggerTool.h>

#include <AsgTools/AnaToolHandle.h>
#include <AsgTools/AsgTool.h>
#include <AsgTools/ToolHandle.h>
#include <AsgTools/ToolHandleArray.h>
#include <SUSYTools/SUSYObjDef_xAOD.h>
#include <TFile.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class ITHistSvc;

namespace SUSY {
    class CrossSectionDB;
}
namespace CP {
    class SystematicSet;
    class IJetTileCorrectionTool;
}  // namespace CP
class IGoodRunsListSelectionTool;

namespace XAMPP {
    class TreeBase;
    class IMetaDataTree;
    class IAnalysisConfig;
    class IEventInfo;
    class EventInfo;
    class ReconstructedParticles;
    class IMetSelector;
    class ITriggerTool;
    class ISystematics;

    class SUSYAnalysisHelper : public asg::AsgTool, virtual public IAnalysisHelper {
    public:
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(SUSYAnalysisHelper, XAMPP::IAnalysisHelper)

        SUSYAnalysisHelper(const std::string& myname);
        virtual ~SUSYAnalysisHelper();
        virtual StatusCode initialize();
        virtual StatusCode CheckCutFlow(const CP::SystematicSet* systset);
        virtual StatusCode finalize();
        virtual StatusCode LoadContainers();
        virtual StatusCode FillInitialObjects(const CP::SystematicSet* systset);
        virtual StatusCode FillObjects(const CP::SystematicSet* systset);
        virtual StatusCode RemoveOverlap();

        virtual bool AcceptEvent();
        virtual bool EventCleaning() const;
        virtual bool CleanObjects(const CP::SystematicSet* systset);

        virtual bool CheckTrigger();

        virtual StatusCode FillEvent(const CP::SystematicSet* set);
        // call GetMCXsec before GetMCFilterEff/GetMCkFactor/GetMCXsectTimesEff
        // since it computes the SUSY::finalState
        virtual unsigned int finalState();
        virtual double GetMCXsec(unsigned int mc_channel_number, unsigned int finalState = 0);
        // see IAnalysisHelper for documentation
        virtual void GetMCXsecErrors(bool& error_exists, double& rel_err_down, double& rel_err_up, unsigned int mc_channel_number,
                                     unsigned int finalState = 0);
        virtual double GetMCFilterEff(unsigned int mc_channel_number, unsigned int finalState = 0);
        virtual double GetMCkFactor(unsigned int mc_channel_number, unsigned int finalState = 0);
        virtual double GetMCXsectTimesEff(unsigned int mc_channel_number, unsigned int finalState = 0);

        // get the piped-through decoration interfaces
        std::shared_ptr<ElectronDecorations> GetElectronDecorations() const { return m_electron_selection->GetElectronDecorations(); }
        std::shared_ptr<MuonDecorations> GetMuonDecorations() const { return m_muon_selection->GetMuonDecorations(); }
        std::shared_ptr<JetDecorations> GetJetDecorations() const { return m_jet_selection->GetJetDecorations(); }
        std::shared_ptr<TruthDecorations> GetTruthDecorations() const { return m_truth_selection->GetTruthDecorations(); }

    protected:
        virtual bool isData() const;
        virtual bool doTruth() const;
        virtual bool isInitialized() const;
        void DisableRecoFlags();
        virtual bool storeRecoFlags() const { return m_StoreRecoFlags; }

        virtual StatusCode initializeEventVariables();
        virtual StatusCode ComputeEventVariables();

        // These functions should only be overwritten if there are additional
        // tools needed to  be initialized
        virtual StatusCode initializeObjectTools();
        virtual StatusCode initializeAnalysisTools();
        virtual StatusCode initializeSUSYTools();
        virtual StatusCode initializeGRLTool();

        virtual StatusCode FillEventWeights();
        virtual StatusCode processModules();
        virtual bool applyEventDumpCuts();
        StatusCode SaveCrossSection();

    private:
        ServiceHandle<ITHistSvc> m_histSvc;

    protected:
        // ToolHandles of tools which have to be created externally.
        ToolHandle<IElectronSelector> m_electron_selection;
        ToolHandle<IJetSelector> m_jet_selection;
        ToolHandle<IMetSelector> m_met_selection;
        ToolHandle<IMuonSelector> m_muon_selection;
        ToolHandle<IPhotonSelector> m_photon_selection;
        ToolHandle<ITauSelector> m_tau_selection;
        ToolHandle<IDiTauSelector> m_ditau_selection;
        ToolHandle<ITruthSelector> m_truth_selection;
        ToolHandle<ISystematics> m_systematics;
        ToolHandle<ITriggerTool> m_triggers;

        // SUSYTools as AnaToolHandle because we want to have the possibility in
        // place that SUSYTools is created by the Helper itself
        asg::AnaToolHandle<ST::ISUSYObjDef_xAODTool> m_susytools;

        // Retrieve the SUSYTools pointer from the Tool Handle
        ST::SUSYObjDef_xAOD* SUSYToolsPtr();
        template <typename T> ToolHandle<T> GetCPTool(const std::string& name);
        template <typename Container> StatusCode ViewElementsContainer(const std::string& Key, Container*& Cont);

        virtual bool PassObjectCleaning(const CP::SystematicSet* sys) const;

        StatusCode DumpNtuple(const CP::SystematicSet* sys);
        StatusCode DumpHistos(const CP::SystematicSet* sys);

        StatusCode initializeOuputFormat();
        // Returns a flag stating whether the common trees  including the systematic groups should be activated or not
        bool buildCommonTree() const;

    private:
        std::shared_ptr<HistoBase> CreateHistoClass(const CP::SystematicSet* set);

        StatusCode initHistoClass(std::shared_ptr<HistoBase> HistoClass);
        void CleaningForOutput(const std::string& DecorName, XAMPP::Storage<int>*& Store, bool DoCleaning);
        std::shared_ptr<TreeBase> CreateTreeClass(const CP::SystematicSet* set);
        StatusCode initTreeClass(std::shared_ptr<TreeBase> TreeClass);

        bool m_init;
        bool m_added_output;
        bool m_RunCutFlow;
        bool m_UseFileMetadata;
        bool m_storeLHEbyName;

        bool m_doHistos;
        bool m_doTrees;
        bool m_doTruth;
        bool m_doPRW;
        bool m_isAF2;
        bool m_useGRLTool;

        bool m_StoreRecoFlags;
        bool m_CleanEvent;
        bool m_CleanBadMuon;
        bool m_CleanCosmicMuon;
        bool m_CleanBadJet;

        bool m_FillLHEWeights;
        bool m_shiftMetaDSID;

        std::map<unsigned int, XAMPP::Storage<double>*> m_LHEWeights;
        XAMPP::Storage<int>* m_dec_NumBadMuon;
        XAMPP::Storage<int>* m_decNumBadJet;
        XAMPP::Storage<int>* m_decNumCosmicMuon;

        // SUSYTools properties and settings
        std::string m_STConfigFile;
        std::string m_XsecDBDir;
        std::vector<std::string> m_GoodRunsListVec;
        std::vector<std::string> m_PRWConfigFiles;
        std::vector<std::string> m_PRWLumiCalcFiles;

        bool m_useXsecPMGTool;
        std::string m_XsecPMGToolFile;

        std::unique_ptr<SUSY::CrossSectionDB> m_XsecDB;

        std::map<const CP::SystematicSet*, std::shared_ptr<HistoBase>> m_histoVec;
        std::map<const CP::SystematicSet*, std::shared_ptr<TreeBase>> m_treeVec;

        bool m_buildCommonTree;

        ToolHandle<XAMPP::IAnalysisConfig> m_config;
        ToolHandleArray<XAMPP::IAnalysisModule> m_analysis_modules;
        bool m_hasModules;
        // Flag which ensures that the scale-factors are calculated for the
        // MET variations as well in cases when people have met as skimming
        // requirement in their cutflows and the nominal selection is not
        // passed like the Balrock in Kazadum
        int m_OutlierStrat;
        float m_outlierWeightThreshold;

        // Use anaTool handle for tools which might be created by the Helper
        // itself
        asg::AnaToolHandle<XAMPP::IEventInfo> m_InfoHandle;
        asg::AnaToolHandle<XAMPP::IMetaDataTree> m_MDTree;
        asg::AnaToolHandle<IGoodRunsListSelectionTool> m_grl;

    protected:
        asg::AnaToolHandle<XAMPP::IReconstructedParticles> m_ParticleConstructor;
        XAMPP::EventInfo* m_XAMPPInfo;
    };

    template <typename Container> StatusCode SUSYAnalysisHelper::ViewElementsContainer(const std::string& Key, Container*& Cont) {
        if (Key.empty()) {
            ATH_MSG_ERROR("Empty keys are not allowed");
            return StatusCode::FAILURE;
        }
        ATH_MSG_DEBUG("Create new SG::VIEW_ELEMENTS container for " << Key);
        Cont = new Container(SG::VIEW_ELEMENTS);
        return evtStore()->record(Cont, Key + name() + m_XAMPPInfo->GetSystematic()->name());
    }

    template <typename T> ToolHandle<T> SUSYAnalysisHelper::GetCPTool(const std::string& name) {
        return ToolHandle<T>(SUSYToolsPtr()->getProperty(name).toString());
    }

}  // namespace XAMPP
#endif
