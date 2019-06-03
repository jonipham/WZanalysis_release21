#ifndef XAMPPbase_PileupUtils_H
#define XAMPPbase_PileupUtils_H

#include <XAMPPbase/AnalysisUtils.h>
namespace CP {
    class IPileupReweightingTool;
}
//#############################################################################
//  The PileupHelper class is to be used in the context of the                #
//  CreateMergedNTUP_PILEUP.py macro. It setups an instance of the            #
//  pileupreweighting tool and provides the most important features of the    #
//  prw tool to perform the consistency check as external methods             #
//#############################################################################
namespace XAMPP {

    struct prwElement {
        std::string histName;
        Int_t channel = 0;
        UInt_t runNumber = 0;
        std::vector<UInt_t> pStarts;
        std::vector<UInt_t> pEnds;
        std::shared_ptr<TH1> histo;
        prwElement(UInt_t dsid, UInt_t run, std::string name) {
            channel = dsid;
            runNumber = run;
            histName = name;
        }
        void AddStart(const std::vector<UInt_t>& run) {
            XAMPP::CopyVector(run, pStarts, false);
            std::sort(pStarts.begin(), pStarts.end());
        }
        void AddEnd(const std::vector<UInt_t>& run) {
            XAMPP::CopyVector(run, pEnds, false);
            std::sort(pEnds.begin(), pEnds.end());
        }
    };

    class PileupHelper {
    public:
        PileupHelper(const std::string& name);
        void setLumiCalcFiles(const std::vector<std::string>& files);
        void setConfigFiles(const std::vector<std::string>& files);
        bool initialize();
        const CP::IPileupReweightingTool* getTool() const;
        bool isDSIDvalid(unsigned int dsid);
        double GetSumW(unsigned int dsid);
        Long64_t GetNumberOfEvents(unsigned int dsid);

        // This function opens all prw config files and retrieves all stored
        // histograms. If the prw config file is split into subsets then the
        // histograms per channel are add up as long as the second argument is
        // set to false. Otherwise it's assumed that the prw files are foreach
        // at final stage, but there are some DSIDs missing in one or the other.
        static std::map<std::string, XAMPP::prwElement> load_prw_configFiles(const std::vector<std::string>& config_files,
                                                                             bool only_OneHistPerChannel = false);

        // These methods are helper methods to retrieve the number of events per
        // prw period. Unfortunateley the prw Tool does not provide any
        // functionallity to split the events in each prw campaign. So we need
        // to come up with our own solution. These functions were actually
        // designed to be executed within the XAMPPplotting/CheckMetaData
        // script. To disentangle the full and fast sim samples, the methods are
        // implemented twice
        Long64_t nEventsPerPRWperiod_full(int dsid, unsigned int runNumber) const;
        Long64_t nEventsPerPRWperiod_faststim(int dsid, unsigned int runNumber) const;

        void loadPRWperiod_fastsim(const std::vector<std::string>& files);
        void loadPRWperiod_fullsim(const std::vector<std::string>& files);

        std::vector<unsigned int> getPRWperiods_fullsim() const;
        std::vector<unsigned int> getPRWperiods_fastsim() const;

    private:
        Long64_t nEvents(const std::vector<XAMPP::prwElement>& pileupInfo, int dsid, unsigned int runNumber) const;
        std::vector<unsigned int> getPRWperiods(const std::vector<XAMPP::prwElement>& pileupInfo) const;
        void loadPrwConfig(const std::vector<std::string>& files, std::vector<XAMPP::prwElement>& pileupInfo);

        asg::AnaToolHandle<CP::IPileupReweightingTool> m_tool;

        std::vector<XAMPP::prwElement> m_mc_pileupInfo_full;
        std::vector<XAMPP::prwElement> m_mc_pileupInfo_fast;
    };
}  // namespace XAMPP
#endif
