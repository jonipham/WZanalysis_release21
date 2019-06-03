#ifndef XAMPPbase_MetaDataTree_H
#define XAMPPbase_MetaDataTree_H

#include <AsgTools/AnaToolHandle.h>
#include <AsgTools/AsgMetadataTool.h>
#include <AsgTools/IAsgTool.h>
#include <AsgTools/ToolHandle.h>
#include <TFile.h>
#include <TTree.h>
#include <XAMPPbase/IEventInfo.h>
#include <xAODCutFlow/CutBookkeeper.h>
#include <xAODCutFlow/CutBookkeeperContainer.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODLuminosity/LumiBlockRange.h>
#include <xAODLuminosity/LumiBlockRangeContainer.h>
#include <map>
#include <memory>
#include <set>

class ITHistSvc;

namespace XAMPP {
    class IAnalysisHelper;
    // helper function to  get the Process IDs
    std::vector<unsigned int> SUSYprocessIDs();

    class IMetaDataTree : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(XAMPP::IMetaDataTree)
    public:
        virtual StatusCode initialize() = 0;
        virtual StatusCode beginInputFile() = 0;
        virtual StatusCode beginEvent() = 0;
        virtual StatusCode finalize() = 0;
        virtual StatusCode fillLHEMetaData(unsigned int Idx) = 0;
        virtual void subtractEventFromMetaData() = 0;
        virtual void subtractEventFromMetaData(unsigned int index) = 0;

        virtual std::vector<std::string> getLHEWeightNames() const = 0;
        virtual ~IMetaDataTree() {}
    };

    class MetaDataElement {
    public:
        MetaDataElement(ToolHandle<XAMPP::IEventInfo> m_XAMPPInfo);

        enum BookKeeperType { Original = 1, SUSY = 2, LHE3 = 3 };
        virtual ~MetaDataElement() {}
        virtual StatusCode newFile(const xAOD::CutBookkeeperContainer* container) = 0;
        virtual StatusCode newFile(const xAOD::LumiBlockRange* LumiBlock) = 0;
        virtual void CutBookKeeperAvailable(bool B) = 0;

        virtual StatusCode newEvent() = 0;
        virtual StatusCode finalize(TTree* MetaDataTree) = 0;
        virtual StatusCode CopyStore(const MetaDataElement* Store) = 0;
        virtual StatusCode fillVariation(unsigned int, double) = 0;

        void setLHEWeightNames(const std::vector<std::string>& weights);
        size_t numOfWeights() const;
        virtual void SubtractEvent(unsigned int, double) {}

    protected:
        bool SetBranchAddress(TTree* tree, const std::string& Name, std::string* Element) {
            if (!tree) {
                // Error("MetaDataTreeElement::SetBranchAddress()",
                // "noTreeElement given");
                return false;
            }
            if (tree->FindBranch(Name.c_str())) {
                // address of a ptr for strings
                if (tree->SetBranchAddress(Name.c_str(), &Element) != 0) { return false; }
                return true;
            }
            if (tree->Branch(Name.c_str(), Element) == nullptr) { return false; }
            return true;
        }

        template <typename T> bool SetBranchAddress(TTree* tree, const std::string& Name, T& Element) {
            if (!tree) {
                // Error("MetaDataTreeElement::SetBranchAddress()",
                // "noTreeElement given");
                return false;
            }
            if (tree->FindBranch(Name.c_str())) {
                if (tree->SetBranchAddress(Name.c_str(), &Element) != 0) { return false; }
                return true;
            }
            if (tree->Branch(Name.c_str(), &Element) == nullptr) { return false; }
            return true;
        }
        const xAOD::CutBookkeeper* FindCutBookKeeper(const xAOD::CutBookkeeperContainer* container,
                                                     MetaDataElement::BookKeeperType Type = MetaDataElement::BookKeeperType::Original,
                                                     unsigned int procID = 0);

        ToolHandle<XAMPP::IEventInfo> m_XAMPPInfo;
        std::vector<std::string> m_LHE_WeightNames;
    };

    class MetaDataTree : public asg::AsgMetadataTool, virtual public IMetaDataTree {
    public:
        MetaDataTree(const std::string& myname);
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(MetaDataTree, XAMPP::IMetaDataTree)

        virtual ~MetaDataTree();
        virtual StatusCode beginInputFile();
        virtual StatusCode beginEvent();
        virtual StatusCode finalize();
        virtual StatusCode initialize();
        virtual StatusCode fillLHEMetaData(unsigned int Idx);
        virtual void subtractEventFromMetaData();
        virtual void subtractEventFromMetaData(unsigned int ix);

        virtual std::vector<std::string> getLHEWeightNames() const;

    private:
        void LoadMCMetaData(unsigned int mcChannel, unsigned int periodNumber);
        void LoadRunMetaData(unsigned int run);
        StatusCode CheckLumiBlockContainer(const std::string& Container, bool& HasCont);
        std::string m_TreeName;
        bool m_UseFileMetaData;
        bool m_fillLHEWeights;
        /// Flag determining whether the mcChannelNumber from the
        /// file meta-data is shifted by the m_XAMPPInfo->dsidOffSet() call
        bool m_shiftMetaDSID;
        TTree* m_tree;
        ServiceHandle<ITHistSvc> m_histSvc;

        bool m_isData;
        bool m_init;
        ToolHandle<XAMPP::IAnalysisHelper> m_analysis_helper;
        asg::AnaToolHandle<XAMPP::IEventInfo> m_XAMPPInfo;
        //  In order to disentangle mc16a from mc16c the code must ensure that
        //  there is a seperate entry in the metadata tree for each mc_channel ,
        //  peridod_number pair. This entanglement becomes neccesary if users
        //  want to compare data15 + data16 only to mc16a, but passed all three
        //  lumi-calc files to the code while processing.
        typedef std::pair<unsigned int, unsigned int> MetaID;
        std::map<MetaID, std::shared_ptr<MetaDataElement>> m_MetaDB;
        std::map<MetaID, std::shared_ptr<MetaDataElement>>::iterator m_ActDB;

        std::shared_ptr<XAMPP::MetaDataElement> m_DummyEntry;
    };

    class MetaDataMC : virtual public MetaDataElement {
    public:
        MetaDataMC(unsigned int mcChannel, unsigned int periodNumber, ToolHandle<XAMPP::IAnalysisHelper>& helper,
                   ToolHandle<XAMPP::IEventInfo> info);
        virtual StatusCode newFile(const xAOD::CutBookkeeperContainer* container);
        virtual StatusCode newFile(const xAOD::LumiBlockRange*);
        virtual StatusCode newEvent();
        virtual void CutBookKeeperAvailable(bool B);
        virtual StatusCode finalize(TTree* MetaDataTree);
        virtual StatusCode CopyStore(const MetaDataElement* Store);
        virtual StatusCode fillVariation(unsigned int Id, double W);
        virtual ~MetaDataMC();
        virtual void SubtractEvent(unsigned int Id, double W);

    private:
        struct MetaData {
            MetaData(unsigned int ID) {
                ProcID = ID;
                procName = "";
                xSec = 0.;
                xSec_err_down = 0;
                xSec_err_up = 0;
                has_xSec_err = false;
                kFaktor = 0.;
                FilterEff = 0.;
                NumTotalEvents = 0.;
                NumProcessedEvents = 0.;
                SumW = 0.;
                SumW2 = 0.;
                MetaInit = false;
                KeeperAvailable = false;
                luminosity = 1;
            }
            unsigned int ProcID;
            std::string procName;
            double xSec;
            double xSec_err_down;
            double xSec_err_up;
            bool has_xSec_err;
            double kFaktor;
            double FilterEff;
            Long64_t NumTotalEvents;
            Long64_t NumProcessedEvents;
            double SumW;
            double SumW2;
            bool MetaInit;
            bool KeeperAvailable;
            // prw information to be propagated into the meta-data
            double luminosity;
        };
        void AddEventInformation(std::shared_ptr<MetaDataMC::MetaData> Meta, double GenWeight);
        void AddFileInformation(std::shared_ptr<MetaDataMC::MetaData> Meta, Long64_t TotEv, double SumW, double SumW2);
        void SubtractEvent(std::shared_ptr<MetaDataMC::MetaData> Meta, double GenWeight);

        void LoadMetaData(unsigned int ID);
        bool SaveMetaDataInTree(std::shared_ptr<MetaDataMC::MetaData> Meta, TTree* tree);

        unsigned int m_MC;
        unsigned int m_periodNumber;
        ToolHandle<XAMPP::IAnalysisHelper> m_helper;
        std::map<unsigned int, std::shared_ptr<MetaDataMC::MetaData>> m_Data;
        std::map<unsigned int, std::shared_ptr<MetaDataMC::MetaData>>::iterator m_ActMeta;
        std::shared_ptr<MetaDataMC::MetaData> m_Inclusive;
        bool m_init;
    };
    class runMetaData : virtual public MetaDataElement {
    public:
        runMetaData(unsigned int runNumber, ToolHandle<XAMPP::IEventInfo> info);
        virtual StatusCode newFile(const xAOD::CutBookkeeperContainer* Container);
        virtual StatusCode newFile(const xAOD::LumiBlockRange* LumiBlock);
        virtual StatusCode newEvent();
        virtual void CutBookKeeperAvailable(bool B);
        virtual StatusCode finalize(TTree* MetaDataTree);
        virtual StatusCode CopyStore(const MetaDataElement* Store);
        virtual StatusCode fillVariation(unsigned int, double);

        virtual ~runMetaData();

    private:
        unsigned int m_runNumber;

        Long64_t m_NumTotalEvents;
        Long64_t m_NumProcessedEvents;
        std::set<unsigned int> m_ProcessedBlocks;
        std::set<unsigned int> m_TotalBlocks;
        bool m_KeeperAvailable;
    };
}  // namespace XAMPP
#endif
