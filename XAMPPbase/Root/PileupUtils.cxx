#include <AsgAnalysisInterfaces/IPileupReweightingTool.h>
#include <PileupReweighting/PileupReweightingTool.h>
#include <PileupReweighting/TPileupReweighting.h>
#include <XAMPPbase/PileupUtils.h>
namespace XAMPP {
    //#############################################################
    //                  load prw map
    //#############################################################
    std::map<std::string, XAMPP::prwElement> PileupHelper::load_prw_configFiles(const std::vector<std::string>& config_files,
                                                                                bool only_OneHistPerChannel) {
        std::map<std::string, XAMPP::prwElement> prw_map;
        for (auto file : config_files) {
            std::unique_ptr<TFile> ROOT_File(TFile::Open(file.c_str(), "READ"));
            if (!ROOT_File || !ROOT_File->IsOpen()) {
                prw_map.clear();
                return prw_map;
            }
            TTree* tree = nullptr;
            ROOT_File->GetObject("PileupReweighting/MCPileupReweighting", tree);
            TDirectory* dir = nullptr;
            ROOT_File->GetObject("PileupReweighting", dir);
            Char_t histName[150];
            Int_t channel = 0;
            UInt_t runNumber = 0;
            std::vector<UInt_t>* pStarts = nullptr;
            std::vector<UInt_t>* pEnds = nullptr;

            tree->SetBranchAddress("Channel", &channel);
            tree->SetBranchAddress("RunNumber", &runNumber);
            tree->SetBranchAddress("PeriodStarts", &pStarts);
            tree->SetBranchAddress("PeriodEnds", &pEnds);
            tree->SetBranchAddress("HistName", &histName);

            for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry) {
                tree->GetEntry(entry);
                std::string name = Form("%s", histName);
                std::map<std::string, XAMPP::prwElement>::iterator Itr = prw_map.find(name);
                if (Itr == prw_map.end()) {
                    prw_map.insert(std::pair<std::string, XAMPP::prwElement>(name, XAMPP::prwElement(channel, runNumber, name)));
                    Itr = prw_map.find(name);
                    // Info("NewChannel", "Found new channel %d with runNumber
                    // %d",channel,runNumber);
                }
                Itr->second.AddStart(*pStarts);
                Itr->second.AddEnd(*pEnds);
            }
            for (auto& prw : prw_map) {
                TH1* histo = nullptr;
                dir->GetObject(prw.first.c_str(), histo);
                if (!histo) continue;
                if (!prw.second.histo) {
                    histo->SetDirectory(0);
                    prw.second.histo = std::shared_ptr<TH1>(histo);
                } else if (!only_OneHistPerChannel)
                    prw.second.histo->Add(histo);
            }
        }
        return prw_map;
    }
    PileupHelper::PileupHelper(const std::string& name) :
        m_tool("CP::PileupReweightingTool/" + name),
        m_mc_pileupInfo_full(),
        m_mc_pileupInfo_fast() {}
    void PileupHelper::setLumiCalcFiles(const std::vector<std::string>& files) {
        m_tool.setProperty("LumiCalcFiles", GetPathResolvedFileList(files)).isSuccess();
    }
    void PileupHelper::setConfigFiles(const std::vector<std::string>& files) {
        m_tool.setProperty("ConfigFiles", GetPathResolvedFileList(files)).isSuccess();
    }
    void PileupHelper::loadPrwConfig(const std::vector<std::string>& files, std::vector<XAMPP::prwElement>& pileupInfo) {
        std::vector<std::string> config_files = GetPathResolvedFileList(files);
        std::map<std::string, XAMPP::prwElement> elements = load_prw_configFiles(config_files, false);
        pileupInfo.clear();
        for (auto& ele : elements) { pileupInfo.push_back(ele.second); }
        // Sort the list by channel prioritised followed by runNumber
        std::sort(pileupInfo.begin(), pileupInfo.end(), [](const prwElement& a, const prwElement& b) {
            if (a.channel != b.channel) return a.channel < b.channel;
            return a.runNumber < b.runNumber;
        });
    }
    bool PileupHelper::initialize() {
        if (!m_tool.retrieve().isSuccess()) return false;
        m_tool->expert()->Initialize();
        return true;
    }
    const CP::IPileupReweightingTool* PileupHelper::getTool() const { return m_tool.getHandle().operator->(); }
    bool PileupHelper::isDSIDvalid(unsigned int dsid) { return GetNumberOfEvents(dsid) > 0.; }
    double PileupHelper::GetSumW(unsigned int dsid) {
        double SumW = 0;
        try {
            SumW = m_tool->GetSumOfEventWeights(dsid);
        } catch (...) { SumW = 0.; }
        return SumW;
    }
    Long64_t PileupHelper::GetNumberOfEvents(unsigned int dsid) {
        Long64_t N = 0;
        try {
            N = m_tool->GetNumberOfEvents(dsid);
        } catch (...) { N = 0; }
        return N;
    }
    Long64_t PileupHelper::nEventsPerPRWperiod_full(int dsid, unsigned int runNumber) const {
        return nEvents(m_mc_pileupInfo_full, dsid, runNumber);
    }
    Long64_t PileupHelper::nEventsPerPRWperiod_faststim(int dsid, unsigned int runNumber) const {
        return nEvents(m_mc_pileupInfo_fast, dsid, runNumber);
    }

    Long64_t PileupHelper::nEvents(const std::vector<XAMPP::prwElement>& pileupInfo, int dsid, unsigned int runNumber) const {
        for (auto& ele : pileupInfo) {
            if (ele.channel == dsid && ele.runNumber == runNumber) return ele.histo->GetEntries();
        }
        return 0;
    }
    void PileupHelper::loadPRWperiod_fastsim(const std::vector<std::string>& files) { loadPrwConfig(files, m_mc_pileupInfo_fast); }
    void PileupHelper::loadPRWperiod_fullsim(const std::vector<std::string>& files) { loadPrwConfig(files, m_mc_pileupInfo_full); }
    std::vector<unsigned int> PileupHelper::getPRWperiods(const std::vector<XAMPP::prwElement>& pileupInfo) const {
        std::vector<unsigned int> periods;
        for (auto& ele : pileupInfo) {
            if (!IsInVector(ele.runNumber, periods)) periods.push_back(ele.runNumber);
        }
        return periods;
    }
    std::vector<unsigned int> PileupHelper::getPRWperiods_fullsim() const { return getPRWperiods(m_mc_pileupInfo_full); }
    std::vector<unsigned int> PileupHelper::getPRWperiods_fastsim() const { return getPRWperiods(m_mc_pileupInfo_fast); }

}  // namespace XAMPP
