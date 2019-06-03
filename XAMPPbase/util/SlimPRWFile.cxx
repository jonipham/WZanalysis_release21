
#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/Defs.h>
#include <XAMPPbase/PileupUtils.h>
#include <XAMPPbase/TreeHelpers.h>

#include <TFile.h>
#include <map>
#include <string>
#include <vector>

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

int main(int argc, char* argv[]) {
    std::vector<std::string> inFiles;
    std::string outFile = "slimed_prw.root";
    // If the script shall merge two already slimmed prw files. To avoid
    // duplications we need to ensure that only one DS from one file or the other is
    // loaded
    bool only_OneHistPerChannel = false;

    // Reading the Arguments parsed to the executable
    for (int a = 1; a < argc; ++a) {
        std::string argument = argv[a];
        if (argument == "--inFile" || argument == "-i") {
            if (a + 1 == argc) return EXIT_FAILURE;
            std::string value = argv[a + 1];
            if (!XAMPP::IsInVector(value, inFiles)) inFiles.push_back(value);
            ++a;
        } else if (argument == "--outFile") {
            if (a + 1 == argc) return EXIT_FAILURE;
            outFile = argv[a + 1];
            ++a;
        } else if (argument == "--InList") {
            if (a + 1 == argc) return EXIT_FAILURE;
            std::ifstream ifst(argv[a + 1]);
            if (!ifst.good()) return EXIT_FAILURE;
            std::string line;
            while (XAMPP::GetLine(ifst, line)) {
                if (!XAMPP::IsInVector(line, inFiles)) inFiles.push_back(line);
            }
        } else if (argument == "--InIsSlimmed") {
            only_OneHistPerChannel = true;
        }
    }
    std::map<std::string, XAMPP::prwElement> prw_map = XAMPP::PileupHelper::load_prw_configFiles(inFiles, only_OneHistPerChannel);

    if (prw_map.empty()) return EXIT_FAILURE;

    std::shared_ptr<TFile> out_ROOTFile(TFile::Open(outFile.c_str(), "RECREATE"));
    if (!out_ROOTFile || !out_ROOTFile->IsOpen()) return EXIT_FAILURE;
    out_ROOTFile->mkdir("PileupReweighting/");
    out_ROOTFile->cd("PileupReweighting/");
    TDirectory* dir = nullptr;
    out_ROOTFile->GetObject("PileupReweighting", dir);

    TTree* outTreeMC = new TTree("MCPileupReweighting", "MCPileupReweighting");
    Char_t histName[150];
    XAMPP::TreeBranch<UInt_t> run_branch(outTreeMC, "RunNumber");
    XAMPP::TreeBranch<Int_t> channel_branch(outTreeMC, "Channel");
    XAMPP::TreeBranch<std::vector<UInt_t>> begin_branch(outTreeMC, "PeriodStarts");
    XAMPP::TreeBranch<std::vector<UInt_t>> end_branch(outTreeMC, "PeriodEnds");
    if (!run_branch.Init() || !channel_branch.Init() || !begin_branch.Init() || !end_branch.Init()) return EXIT_FAILURE;

    outTreeMC->Branch("HistName", histName, "HistName[50]/C");
    for (auto& prw : prw_map) {
        run_branch.setValue(prw.second.runNumber);
        channel_branch.setValue(prw.second.channel);
        begin_branch.setValue(prw.second.pStarts);
        end_branch.setValue(prw.second.pEnds);
        strncpy(histName, prw.second.histName.c_str(), prw.second.histName.size());
        outTreeMC->Fill();
        dir->WriteObject(prw.second.histo.get(), prw.second.histName.c_str());
    }
    out_ROOTFile->cd();
    out_ROOTFile->Write("", TObject::kOverwrite);
    out_ROOTFile->Close();
    return EXIT_SUCCESS;
}
