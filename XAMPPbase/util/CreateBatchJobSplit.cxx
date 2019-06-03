#include <TFile.h>
#include <TSystem.h>
#include <XAMPPbase/AnalysisUtils.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "xAODRootAccess/Init.h"
#include "xAODRootAccess/TEvent.h"
bool OpenNewList(std::ofstream &List, std::string &Dir, unsigned int NumberOfJobs);
int main(int argc, char *argv[]) {
    int EventsPerJob = 10000;
    int Events = 0;
    int EventsCurrentJob = 0;
    unsigned int NumberOfJobs = 0;
    std::string InFileList = "";
    std::string OutDir = "";
    std::vector<std::string> Files;
    std::ofstream OutList;
    std::ofstream TotalEvList;
    // Set up the job for xAOD access:
    if (!xAOD::Init("CreateBatchJobSplit").isSuccess()) {
        Error("CreateBatchJobSplit", "Could not setup xAOD");
        return EXIT_FAILURE;
    }
    // Reading the Arguments parsed to the executable
    for (int a = 1; a < argc; ++a) {
        if (strcmp(argv[a], "-I") == 0 && (a + 1) != argc)
            InFileList = argv[a + 1];
        else if (strcmp(argv[a], "-O") == 0 && (a + 1) != argc)
            OutDir = argv[a + 1];
        else if (strcmp(argv[a], "-EpJ") == 0 && (a + 1) != argc)
            EventsPerJob = (atoi(argv[a + 1]) - 1);
    }
    std::ifstream list(InFileList);
    if (InFileList.empty() || !list.good()) {
        Error("CreateBatchJobSplit", "Could not read the FileList");
        return -1;
    } else {
        std::string Line;
        while (XAMPP::GetLine(list, Line)) { XAMPP::FillVectorFromString(Files, Line); }
    }
    Info("CreateBatchJobSplit", "Read in of list done.. Now Checking files");
    if (OutDir.empty()) {
        Error("CreateBatchJobSplit", "Please provide an OutDir to the script");
        return EXIT_FAILURE;
    }
    gSystem->mkdir(OutDir.c_str(), true);
    if (!OpenNewList(OutList, OutDir, NumberOfJobs)) return EXIT_FAILURE;
    xAOD::TEvent m_event(xAOD::TEvent::kClassAccess);

    for (size_t f = 0; f < Files.size(); ++f) {
        Info("CreateBatchJobSplit", "Check file (%lu/%lu): %s", f + 1, Files.size(), Files.at(f).c_str());
        TFile *File = TFile::Open(Files.at(f).c_str(), "READ");
        if (!File || !File->IsOpen()) {
            Error("CreateBatchJobSplit", "The File Could not be opened");
            return EXIT_FAILURE;
        }
        if (!m_event.readFrom(File).isSuccess()) {
            Error("CreateBatchJobSplit", "Could not read in the File");
            continue;
        }
        int EventsInFile = m_event.getEntries();
        if (EventsInFile == 0) { Warning("CreateBatchJobSplit", "The file contains 0 Events."); }
        Events += EventsInFile;
        EventsCurrentJob += EventsInFile;
        OutList << Files.at(f) << std::endl;
        if (EventsCurrentJob >= EventsPerJob && f < Files.size() - 1) {
            EventsCurrentJob = 0;
            ++NumberOfJobs;
            if (!OpenNewList(OutList, OutDir, NumberOfJobs)) return EXIT_FAILURE;
        }
        File->Close();
        delete File;
    }
    OutList.close();
    Info("CreateBatchJobSplit", "Read in of the files is finished. Found %i events and created in total %u jobs", Events, NumberOfJobs + 1);
    return EXIT_SUCCESS;
}
bool OpenNewList(std::ofstream &List, std::string &Dir, unsigned int NumberOfJobs) {
    if (List.is_open()) List.close();
    List.open(Form("%s/SplitList_%d.txt", Dir.c_str(), NumberOfJobs + 1));
    return List.good();
}
