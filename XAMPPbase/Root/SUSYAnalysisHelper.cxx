#include <XAMPPbase/AnalysisConfig.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/HistoBase.h>
#include <XAMPPbase/IDiTauSelector.h>
#include <XAMPPbase/IElectronSelector.h>
#include <XAMPPbase/IJetSelector.h>
#include <XAMPPbase/IMetSelector.h>
#include <XAMPPbase/IMuonSelector.h>
#include <XAMPPbase/IPhotonSelector.h>
#include <XAMPPbase/ITauSelector.h>
#include <XAMPPbase/ITriggerTool.h>
#include <XAMPPbase/ITruthSelector.h>
#include <XAMPPbase/MetaDataTree.h>
#include <XAMPPbase/ReconstructedParticles.h>
#include <XAMPPbase/SUSYAnalysisHelper.h>
#include <XAMPPbase/SUSYSystematics.h>
#include <XAMPPbase/TreeBase.h>

// Tool includes
#include <AsgAnalysisInterfaces/IGoodRunsListSelectionTool.h>
#include <AsgTools/StatusCode.h>
#include <GaudiKernel/ITHistSvc.h>
#include <PATInterfaces/CorrectionCode.h>
#include <PATInterfaces/SystematicSet.h>
#include <PathResolver/PathResolver.h>
#include <SUSYTools/SUSYCrossSection.h>
#include <SUSYTools/SUSYObjDef_xAOD.h>

#include <fstream>
#include <iostream>

namespace XAMPP {
    SUSYAnalysisHelper::SUSYAnalysisHelper(const std::string& myname) :
        AsgTool(myname),
        m_histSvc("THistSvc", myname),
        m_electron_selection("SUSYElectronSelector"),
        m_jet_selection("SUSYJetSelector"),
        m_met_selection("SUSYMetSelector"),
        m_muon_selection("SUSYMuonSelector"),
        m_photon_selection("SUSYPhotonSelector"),
        m_tau_selection("SUSYTauSelector"),
        m_ditau_selection("SUSYDiTauSelector"),
        m_truth_selection("SUSYTruthSelector"),
        m_systematics("SystematicsTool"),
        m_triggers("TriggerTool"),
        m_susytools("SUSYTools"),
        m_init(false),
        m_added_output(false),
        m_RunCutFlow(true),
        m_UseFileMetadata(true),
        m_storeLHEbyName(false),
        m_doHistos(true),
        m_doTrees(true),
        m_doTruth(true),
        m_doPRW(true),
        m_useGRLTool(true),
        m_StoreRecoFlags(true),
        m_CleanEvent(true),
        m_CleanBadMuon(true),
        m_CleanCosmicMuon(true),
        m_CleanBadJet(true),
        m_FillLHEWeights(false),
        m_shiftMetaDSID(false),
        m_LHEWeights(),
        m_dec_NumBadMuon(nullptr),
        m_decNumBadJet(nullptr),
        m_decNumCosmicMuon(nullptr),
        m_GoodRunsListVec(),
        m_PRWConfigFiles(),
        m_PRWLumiCalcFiles(),
        m_useXsecPMGTool(false),
        m_XsecDB(),
        m_histoVec(),
        m_treeVec(),
        m_buildCommonTree(false),
        m_config("AnalysisConfig"),
        m_analysis_modules(),
        m_hasModules(false),
        m_OutlierStrat(IEventInfo::doNothing),
        m_outlierWeightThreshold(100),
        m_InfoHandle("EventInfoHandler"),
        m_MDTree("MetaDataTree"),
        m_grl("GoodRunsListSelectionTool"),
        m_ParticleConstructor("ParticleConstructor"),
        m_XAMPPInfo(nullptr) {
        // Tool properties
        declareProperty("ElectronSelector", m_electron_selection);
        declareProperty("JetSelector", m_jet_selection);
        declareProperty("MetSelector", m_met_selection);
        declareProperty("MuonSelector", m_muon_selection);
        declareProperty("PhotonSelector", m_photon_selection);
        declareProperty("TauSelector", m_tau_selection);
        declareProperty("DiTauSelector", m_ditau_selection);
        declareProperty("TruthSelector", m_truth_selection);
        declareProperty("SystematicsTool", m_systematics);
        declareProperty("TriggerTool", m_triggers);
        declareProperty("AnalysisConfig", m_config);
        declareProperty("AnalysisModules", m_analysis_modules);

        declareProperty("StoreLHEByName", m_storeLHEbyName);
        declareProperty("OutlierWeightStrategy", m_OutlierStrat);
        declareProperty("OutlierWeightThreshold", m_outlierWeightThreshold);

        m_susytools.declarePropertyFor(this, "SUSYTools", "The SUSYTools instance");
        m_InfoHandle.declarePropertyFor(this, "EventInfoHandler", "The XAMPP EventInfo handle");
        m_MDTree.declarePropertyFor(this, "MetaDataTree", "The XAMPP metadata tree");
        m_grl.declarePropertyFor(this, "GRLTool", "The GRLTool");
        m_ParticleConstructor.declarePropertyFor(this, "ParticleConstructor", "The XAMPP particle constructor");

        // I/O properties
        declareProperty("RunCutFlow", m_RunCutFlow);
        declareProperty("UseFileMetaData", m_UseFileMetadata);
        // Split the common event variables apart into the common tree
        declareProperty("createCommonTree", m_buildCommonTree);

        declareProperty("doHistos", m_doHistos);
        declareProperty("doTrees", m_doTrees);

        declareProperty("doTruth", m_doTruth);
        declareProperty("doPRW", m_doPRW);
        declareProperty("useGRLTooL", m_useGRLTool);
        // properties related to the object cleaning
        declareProperty("EventCleaning", m_CleanEvent);
        declareProperty("BadMuonCleaning", m_CleanBadMuon);
        declareProperty("CosmicMuonCleaning", m_CleanCosmicMuon);
        declareProperty("BadJetCleaning", m_CleanBadJet);
        // fill the weights of the LHE variations
        // only for recent derivations avilable
        declareProperty("fillLHEWeights", m_FillLHEWeights);
        // Shift the DSID of the meta data
        declareProperty("MetaDataDDSIDshift", m_shiftMetaDSID);

        // SUSYTools properties and settings
        declareProperty("STConfigFile", m_STConfigFile = "SUSYTools/SUSYTools_Default.conf");
        declareProperty("STCrossSectionDB", m_XsecDBDir = "SUSYTools/mc15_13TeV/");
        declareProperty("GoodRunsLists", m_GoodRunsListVec);
        declareProperty("PRWConfigFiles", m_PRWConfigFiles);
        declareProperty("PRWLumiCalcFiles", m_PRWLumiCalcFiles);
        declareProperty("useXsecPMGTool", m_useXsecPMGTool);
        declareProperty("XsecPMGToolFile", m_XsecPMGToolFile = "dev/PMGTools/PMGxsecDB_mc16.txt");
    }

    SUSYAnalysisHelper::~SUSYAnalysisHelper() { ATH_MSG_DEBUG("Destructor called"); }
    StatusCode SUSYAnalysisHelper::initializeSUSYTools() {
        if (!m_susytools.isUserConfigured()) {
            ATH_MSG_INFO("Setup new instance of SUSYTools");
            ST::ISUSYObjDef_xAODTool::DataSource datasource =
                isData() ? ST::ISUSYObjDef_xAODTool::DataSource::Data
                         : (m_systematics->isAF2() ? ST::ISUSYObjDef_xAODTool::DataSource::AtlfastII
                                                   : ST::ISUSYObjDef_xAODTool::DataSource::FullSim);
            m_susytools.setTypeAndName("ST::SUSYObjDef_xAOD/SUSYTools");
            ATH_CHECK(m_susytools.setProperty("ConfigFile", PathResolverFindCalibFile(m_STConfigFile)));
            if (m_PRWLumiCalcFiles.empty()) {
                ATH_MSG_DEBUG("No prw files were given. Will not initialize the prw-tool");
                m_doPRW = false;
            } else if ((!isData() || !m_PRWConfigFiles.empty()) && !m_PRWLumiCalcFiles.empty()) {
                ATH_CHECK(m_susytools.setProperty("PRWConfigFiles", GetPathResolvedFileList(m_PRWConfigFiles)));
                ATH_CHECK(m_susytools.setProperty("PRWLumiCalcFiles", GetPathResolvedFileList(m_PRWLumiCalcFiles)));
            } else {
                ATH_MSG_WARNING("Either the config files or lumi calc files are not given");
                m_doPRW = false;
            }
            ATH_CHECK(m_susytools.setProperty("DataSource", (int)datasource));
            ATH_CHECK(m_susytools.initialize());
        } else {
            ATH_CHECK(m_susytools.retrieve());
        }
        SUSYToolsSystematicToolHandle* SUSYToolsHandle = new SUSYToolsSystematicToolHandle(m_susytools);

        ATH_CHECK(SUSYToolsHandle->initialize());
        m_PRWConfigFiles.clear();
        m_PRWLumiCalcFiles.clear();
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::initializeGRLTool() {
        ATH_MSG_DEBUG("Initialize the GRL tool");
        m_doTruth = m_doTruth && !isData();
        m_useGRLTool = m_useGRLTool && isData();
        if (m_useGRLTool) {
            if (m_GoodRunsListVec.empty()) {
                ATH_MSG_FATAL("No GRL has been passed to the tool thus far");
                return StatusCode::FAILURE;
            }
            if (!m_grl.isUserConfigured()) {
                ATH_MSG_DEBUG("Setup the GRL tool");
                m_grl.setTypeAndName("GoodRunsListSelectionTool/GoodRunsListSelectionTool");
                ATH_CHECK(m_grl.setProperty("PassThrough", false));
                ATH_CHECK(m_grl.setProperty("GoodRunsListVec", GetPathResolvedFileList(m_GoodRunsListVec)));
            }
            ATH_CHECK(m_grl.retrieve());
        }
        m_GoodRunsListVec.clear();
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::initializeAnalysisTools() {
        ATH_MSG_INFO("Starting Analysis Setup");
        ATH_CHECK(initializeSUSYTools());
        ATH_CHECK(initializeGRLTool());
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::initializeOuputFormat() {
        // Add the output in the first event when the file is loaded
        if (m_added_output) { return StatusCode::SUCCESS; }
        if (!isData() && m_FillLHEWeights) {
            const xAOD::EventInfo* Info;
            ATH_CHECK(evtStore()->retrieve(Info, "EventInfo"));

            std::vector<std::string> LHEnames = m_MDTree->getLHEWeightNames();
            if (LHEnames.size() != Info->mcEventWeights().size()) {
                ATH_MSG_WARNING("LHE Weights size of " << Info->mcEventWeights().size() << " does NOT agree with the " << LHEnames.size()
                                                       << " names we found");
            }

            for (size_t E = Info->mcEventWeights().size() - 1; E > 0; --E) {
                std::string WeightName = "GenWeight_LHE_" + std::to_string(E + 1000);
                if (m_storeLHEbyName) WeightName = "GenWeight_LHE_" + LHEnames.at(E);

                // Create new LHE variational weights to be stored in the
                // Nominal tree
                ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<double>(WeightName, true, false));
                m_LHEWeights.insert(
                    std::pair<unsigned int, XAMPP::Storage<double>*>(E, m_XAMPPInfo->GetVariableStorage<double>(WeightName)));
            }
        }
        std::vector<std::shared_ptr<TreeBase>> group_trees;
        if (m_systematics->GetKinematicSystematics().size() > 1 && m_buildCommonTree) {
            ATH_MSG_INFO("Going to create a common tree where all meta variables are stored in");
            group_trees.push_back(std::make_shared<TreeBase>(nullptr));
            for (auto& group : m_XAMPPInfo->getSystematicGroups()) {
                group_trees.push_back(std::make_shared<TreeBase>(m_systematics->GetNominal(), group));
            }
            for (const auto& tree : group_trees) { ATH_CHECK(initTreeClass(tree)); }
        }
        for (auto current_syst : m_systematics->GetKinematicSystematics()) {
            std::shared_ptr<HistoBase> histo = CreateHistoClass(current_syst);
            ATH_CHECK(initHistoClass(histo));
            m_histoVec.insert(std::pair<const CP::SystematicSet*, std::shared_ptr<HistoBase>>(current_syst, histo));
            std::shared_ptr<TreeBase> tree = CreateTreeClass(current_syst);
            tree->SetListOfFriends(group_trees);
            ATH_CHECK(initTreeClass(tree));
            m_treeVec.insert(std::pair<const CP::SystematicSet*, std::shared_ptr<TreeBase>>(current_syst, tree));
        }
        // From this point on also the commonVariables are locked
        m_XAMPPInfo->LockKeeper();
        m_added_output = true;
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::initialize() {
        if (m_init) { return StatusCode::SUCCESS; }
        m_init = true;
        CP::CorrectionCode::enableFailure();
        ATH_MSG_INFO("Initializing...");
        ATH_CHECK(m_systematics.retrieve());
        ATH_MSG_DEBUG("initialize the analysis tools");
        ATH_CHECK(initializeAnalysisTools());
        if (!m_InfoHandle.isUserConfigured()) {
            ATH_MSG_DEBUG("Create the event info");
            m_InfoHandle.setTypeAndName("XAMPP::EventInfo/EventInfoHandler");
            ATH_CHECK(m_InfoHandle.setProperty("ApplyPRW", m_doPRW));
            ATH_CHECK(m_InfoHandle.setProperty("ApplyGRLTool", m_useGRLTool));
            ATH_CHECK(m_InfoHandle.setProperty("FilterOutput", m_CleanEvent));
            ATH_CHECK(m_InfoHandle.setProperty("SaveRecoFlags", m_StoreRecoFlags));
            ATH_CHECK(m_InfoHandle.setProperty("OutlierWeightStrategy", m_OutlierStrat));
            ATH_CHECK(m_InfoHandle.setProperty("OutlierWeightThreshold", m_outlierWeightThreshold));
        }

        ATH_CHECK(m_InfoHandle.retrieve());
        m_XAMPPInfo = dynamic_cast<XAMPP::EventInfo*>(m_InfoHandle.operator->());

        if (!isData() && m_useXsecPMGTool) {
            ATH_MSG_DEBUG("Setup the cross section database with the PMG cross section tool");
            // SUSY::CrossSectionDB::CrossSectionDB(const std::string& txtfilename, bool usePathResolver, bool isExtended, bool usePMGTool)
            m_XsecDB = std::make_unique<SUSY::CrossSectionDB>(m_XsecPMGToolFile, true, false, true);
        } else if (!isData() && !m_XsecDBDir.empty()) {
            ATH_MSG_DEBUG("Setup the cross section database with directory " << m_XsecDBDir);
            // SUSY::CrossSectionDB::CrossSectionDB(const std::string& txtfilename, bool usePathResolver, bool isExtended, bool usePMGTool)
            m_XsecDB = std::make_unique<SUSY::CrossSectionDB>("", false, false, false);
            // read files in directory by matching file ending to .txt (note that e.g. .txt.bak or .txt.1 will be skipped)
            std::vector<std::string> xsecFiles = ListDirectory(PathResolverFindCalibDirectory(m_XsecDBDir), "", "(.*\\.txt)");
            for (const auto& xsecFile : xsecFiles) {
                ATH_MSG_DEBUG("Load cross-sections from " << xsecFile);
                m_XsecDB->loadFile(xsecFile);
            }
            m_XsecDBDir.clear();
        } else if (isData()) {
            m_doTruth = false;
        }
        if (!m_MDTree.isUserConfigured()) {
            ATH_MSG_DEBUG("Create new metadata tree");
            m_MDTree.setTypeAndName("XAMPP::MetaDataTree/MetaDataTree");
            ATH_CHECK(m_MDTree.setProperty("isData", isData()));
            ATH_CHECK(m_MDTree.setProperty("useFileMetaData", m_UseFileMetadata));
            ATH_CHECK(m_MDTree.setProperty("fillLHEWeights", m_FillLHEWeights));
            ATH_CHECK(m_MDTree.setProperty("SwitchOnDSIDshift", m_shiftMetaDSID));
        } else {
            ATH_MSG_DEBUG("Use configured meta data tree");
        }

        ATH_CHECK(m_MDTree.retrieve());
        if (!m_ParticleConstructor.isUserConfigured()) {
            ATH_MSG_DEBUG("Create class for handling of the reconstructed particles like Z, W, top candidates");
            m_ParticleConstructor.setTypeAndName("XAMPP::ReconstructedParticles/ParticleConstructor");
            ATH_CHECK(m_ParticleConstructor->initialize());
        }
        ATH_CHECK(initializeObjectTools());
        ATH_MSG_DEBUG("Fix the systematics tool");
        ATH_CHECK(m_systematics->FixSystematics());
        ATH_CHECK(m_analysis_modules.retrieve());
        m_hasModules = !m_analysis_modules.empty();

        // Initialize the event weights in MC
        if (!isData()) {
            ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<int>("SUSYFinalState"));
            ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<bool>(+"HasPathologicalWeight",
                                                                m_XAMPPInfo->getOutlierWeightStrategy() == IEventInfo::resetWeight));
            ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<double>("GenWeight"));
        }
        ATH_CHECK(initializeEventVariables());
        if (m_hasModules) {
            for (auto& module : m_analysis_modules) ATH_CHECK(module->bookVariables());
        }
        // Overload this function in order to add event variables to your output
        // files
        // -----------------

        ATH_CHECK(m_XAMPPInfo->initialize());
        m_XAMPPInfo->Lock();
        // -----------------
        ATH_CHECK(m_config.retrieve());

        ATH_CHECK(m_config->initialize());

        ATH_MSG_INFO("Current info: isData: " << isData() << ", isAF2: " << m_systematics->isAF2() << ", doTruth: " << m_doTruth
                                              << ", doPRW: " << m_doPRW);
        return StatusCode::SUCCESS;
    }

    StatusCode SUSYAnalysisHelper::initializeObjectTools() {
        if (!isData()) ATH_CHECK(m_truth_selection.retrieve());
        ATH_CHECK(m_triggers.retrieve());

        ATH_CHECK(m_electron_selection.retrieve());
        ATH_CHECK(m_muon_selection.retrieve());
        ATH_CHECK(m_jet_selection.retrieve());

        ATH_CHECK(m_photon_selection.retrieve());
        ATH_CHECK(m_tau_selection.retrieve());
        if (m_systematics->ProcessObject(XAMPP::SelectionObject::DiTau)) { ATH_CHECK(m_ditau_selection.retrieve()); }

        ATH_CHECK(m_met_selection.retrieve());

        // The XAMPP particle selectors are protected against the
        // arbitray initialization of athena

        ATH_CHECK(m_triggers->initialize());

        ATH_CHECK(m_electron_selection->initialize());
        ATH_CHECK(m_muon_selection->initialize());
        ATH_CHECK(m_jet_selection->initialize());
        ATH_CHECK(m_photon_selection->initialize());
        ATH_CHECK(m_tau_selection->initialize());
        if (m_systematics->ProcessObject(XAMPP::SelectionObject::DiTau)) { ATH_CHECK(m_ditau_selection->initialize()); }
        if (!isData()) ATH_CHECK(m_truth_selection->initialize());
        CleaningForOutput("BadMuon", m_dec_NumBadMuon, m_CleanBadMuon);
        CleaningForOutput("CosmicMuon", m_decNumCosmicMuon, m_CleanCosmicMuon);
        CleaningForOutput("BadJet", m_decNumBadJet, m_CleanBadJet);

        ATH_CHECK(m_met_selection->initialize());

        return StatusCode::SUCCESS;
    }
    void SUSYAnalysisHelper::CleaningForOutput(const std::string& DecorName, XAMPP::Storage<int>*& Store, bool DoCleaning) {
        if (m_XAMPPInfo->DoesVariableExist<int>(DecorName)) {
            Store = m_XAMPPInfo->GetVariableStorage<int>(DecorName);
            Store->SetSaveTrees(!DoCleaning);
            Store->SetSaveVariations(!DoCleaning);
        }
    }
    StatusCode SUSYAnalysisHelper::initTreeClass(std::shared_ptr<TreeBase> TreeClass) {
        TreeClass->SetEventInfoHandler(m_XAMPPInfo);
        TreeClass->SetSystematicsTool(m_systematics);
        TreeClass->SetAnalysisConfig(m_config);
        return TreeClass->InitializeTree();
    }
    StatusCode SUSYAnalysisHelper::initHistoClass(std::shared_ptr<HistoBase> HistoClass) {
        HistoClass->SetEventInfoHandler(m_InfoHandle.getHandle());
        HistoClass->SetSystematicsTool(m_systematics);
        HistoClass->SetAnalysisConfig(m_config);
        HistoClass->SetDoHistos(m_doHistos);
        HistoClass->WriteCutFlow(m_RunCutFlow);

        return HistoClass->InitializeHistos();
    }

    StatusCode SUSYAnalysisHelper::LoadContainers() {
        ATH_CHECK(initializeOuputFormat());
        ATH_MSG_DEBUG("Load all containers from the Store gate");
        ATH_CHECK(m_XAMPPInfo->LoadInfo());
        ATH_MSG_DEBUG("Electrons...");
        ATH_CHECK(m_electron_selection->LoadContainers());
        ATH_MSG_DEBUG("Muons...");
        ATH_CHECK(m_muon_selection->LoadContainers());
        ATH_MSG_DEBUG("Jets...");
        ATH_CHECK(m_jet_selection->LoadContainers());
        ATH_MSG_DEBUG("Taus...");
        ATH_CHECK(m_tau_selection->LoadContainers());
        if (m_systematics->ProcessObject(XAMPP::SelectionObject::DiTau)) {
            ATH_MSG_DEBUG("DiTaus...");
            ATH_CHECK(m_ditau_selection->LoadContainers());
        }
        ATH_MSG_DEBUG("Photons...");
        ATH_CHECK(m_photon_selection->LoadContainers());
        ATH_MSG_DEBUG("Missing Et...");
        ATH_CHECK(m_met_selection->LoadContainers());
        if (isData()) {
            ATH_MSG_DEBUG("The input is data");
            return StatusCode::SUCCESS;
        }
        ATH_MSG_DEBUG("Truth...");
        ATH_CHECK(m_truth_selection->LoadContainers());
        ATH_MSG_DEBUG("Save the xSection...");
        ATH_CHECK(SaveCrossSection());
        ATH_MSG_DEBUG("LoadContainers terminated...");
        return StatusCode::SUCCESS;
    }
    bool SUSYAnalysisHelper::isData() const { return m_systematics->isData(); }
    unsigned int SUSYAnalysisHelper::finalState() {
        if (isData()) {
            ATH_MSG_WARNING("The current event is no simulated event");
            return -1;
        }
        static XAMPP::Storage<int>* dec_FinalState = m_XAMPPInfo->GetVariableStorage<int>("SUSYFinalState");
        if (dec_FinalState->isAvailable()) return dec_FinalState->GetValue();
        return m_truth_selection->GetInitialState();
    }
    StatusCode SUSYAnalysisHelper::SaveCrossSection() {
        static XAMPP::Storage<int>* dec_FinalState = m_XAMPPInfo->GetVariableStorage<int>("SUSYFinalState");
        static XAMPP::Storage<double>* dec_GenW = m_XAMPPInfo->GetVariableStorage<double>("GenWeight");
        static XAMPP::Storage<bool>* dec_hasPatho = m_XAMPPInfo->GetVariableStorage<bool>("HasPathologicalWeight");
        bool hasPathological = false;
        ATH_CHECK(dec_FinalState->ConstStore(finalState()));
        ATH_CHECK(dec_GenW->ConstStore(m_XAMPPInfo->GetGenWeight()));
        hasPathological |= m_XAMPPInfo->isOutlierGenWeight();
        if (m_FillLHEWeights) {
            for (const auto& LHE : m_LHEWeights) {
                if (LHE.second->isAvailable()) continue;
                ATH_CHECK(m_MDTree->fillLHEMetaData(LHE.first));
                ATH_CHECK(LHE.second->ConstStore(m_XAMPPInfo->GetGenWeight(LHE.first)));
                hasPathological |= m_XAMPPInfo->isOutlierGenWeight(LHE.first);
            }
        }
        ATH_CHECK(dec_hasPatho->ConstStore(hasPathological));
        return StatusCode::SUCCESS;
    }

    bool SUSYAnalysisHelper::AcceptEvent() {
        if (m_XAMPPInfo->getOutlierWeightStrategy() == IEventInfo::ignoreEvent) {
            bool has_good = false;
            if (m_XAMPPInfo->isOutlierGenWeight()) {
                ATH_MSG_WARNING("The GenWeight " << m_XAMPPInfo->GetRawGenWeight() << " in event " << m_XAMPPInfo->eventNumber()
                                                 << " in DSID: " << m_XAMPPInfo->mcChannelNumber() << " exceeds the GenWeight limit ");
                m_MDTree->subtractEventFromMetaData(0);
            } else
                has_good = true;
            if (m_FillLHEWeights) {
                for (const auto& LHE : m_LHEWeights) {
                    if (m_XAMPPInfo->isOutlierGenWeight(LHE.first)) {
                        ATH_MSG_WARNING("The " << LHE.first << "-th LHE weight in event " << m_XAMPPInfo->eventNumber()
                                               << " in DSID: " << m_XAMPPInfo->mcChannelNumber() << " exceeds the LHE limit ");
                        m_MDTree->subtractEventFromMetaData(LHE.first);
                    } else
                        has_good = true;
                }
            }
            return has_good;
        }
        return true;
    }
    StatusCode SUSYAnalysisHelper::FillInitialObjects(const CP::SystematicSet* systset) {
        ATH_MSG_DEBUG("FillInitialObjects...");
        ATH_CHECK(m_ParticleConstructor->PrepareContainer(systset));
        if (m_systematics->AffectsOnlyMET(systset)) return StatusCode::SUCCESS;
        ATH_CHECK(m_electron_selection->InitialFill(*systset));
        ATH_CHECK(m_muon_selection->InitialFill(*systset));
        ATH_CHECK(m_jet_selection->InitialFill(*systset));
        ATH_CHECK(m_photon_selection->InitialFill(*systset));
        ATH_CHECK(m_tau_selection->InitialFill(*systset));
        if (m_systematics->ProcessObject(XAMPP::SelectionObject::DiTau)) { ATH_CHECK(m_ditau_selection->InitialFill(*systset)); }
        if (doTruth()) ATH_CHECK(m_truth_selection->InitialFill(*systset));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::RemoveOverlap() {
        ATH_MSG_DEBUG("OverlapRemoval...");
        if (m_systematics->AffectsOnlyMET(m_systematics->GetCurrent())) return StatusCode::SUCCESS;
        // This boolean is usally false in case of the TruthAnalysisHelpers
        if (m_StoreRecoFlags)
            ATH_CHECK(m_susytools->OverlapRemoval(m_electron_selection->GetPreElectrons(), m_muon_selection->GetPreMuons(),
                                                  m_jet_selection->GetPreJets(), m_photon_selection->GetPrePhotons(),
                                                  m_tau_selection->GetPreTaus()));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::FillObjects(const CP::SystematicSet* systset) {
        ATH_MSG_DEBUG("FillObjects...");
        ATH_CHECK(m_XAMPPInfo->SetSystematic(systset));
        ATH_MSG_DEBUG("Create new instance of the reconstructed particle container");
        if (!m_systematics->AffectsOnlyMET(systset)) {
            ATH_MSG_DEBUG("Fill electrons...");
            ATH_CHECK(m_electron_selection->FillElectrons(*systset));
            ATH_MSG_DEBUG("Fill muons...");
            ATH_CHECK(m_muon_selection->FillMuons(*systset));
            ATH_MSG_DEBUG("Fill jets...");
            ATH_CHECK(m_jet_selection->FillJets(*systset));
            ATH_MSG_DEBUG("Fill taus...");
            ATH_CHECK(m_tau_selection->FillTaus(*systset));
            if (m_systematics->ProcessObject(XAMPP::SelectionObject::DiTau)) {
                ATH_MSG_DEBUG("Fill DiTaus...");
                ATH_CHECK(m_ditau_selection->FillDiTaus(*systset));
            }
            ATH_MSG_DEBUG("Fill photons...");
            ATH_CHECK(m_photon_selection->FillPhotons(*systset));
            if (doTruth()) ATH_CHECK(m_truth_selection->FillTruth(*systset));
            m_triggers->CheckTriggerMatching();
        } else
            ATH_CHECK(m_XAMPPInfo->CopyInfoFromNominal(systset));
        ATH_CHECK(m_met_selection->FillMet(*systset));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::finalize() {
        ATH_CHECK(m_MDTree->finalize());
        for (auto& Tree : m_treeVec) ATH_CHECK(Tree.second->FinalizeTree());
        if (m_doTrees) ATH_MSG_INFO("All trees were written successfully.");
        for (auto& Histo : m_histoVec) Histo.second->FinalizeHistos();
        if (m_doHistos) ATH_MSG_INFO("All histograms were written successfully.");
        m_treeVec.clear();
        return StatusCode::SUCCESS;
    }
    bool SUSYAnalysisHelper::CleanObjects(const CP::SystematicSet* systset) {
        return PassObjectCleaning(systset) || (systset == m_systematics->GetNominal() && !m_XAMPPInfo->getSystematicGroups().empty());
    }
    bool SUSYAnalysisHelper::PassObjectCleaning(const CP::SystematicSet* systset) const {
        ATH_CHECK(m_XAMPPInfo->SetSystematic(systset));
        ATH_MSG_DEBUG("Found " << m_dec_NumBadMuon->GetValue() << " bad muons, " << m_decNumCosmicMuon->GetValue() << " cosmics and "
                               << m_decNumBadJet->GetValue() << " bad jets");
        return !((m_CleanBadMuon && m_dec_NumBadMuon->GetValue() > 0) || (m_CleanCosmicMuon && m_decNumCosmicMuon->GetValue() > 0) ||
                 (m_CleanBadJet && m_decNumBadJet->GetValue() > 0));
    }
    StatusCode SUSYAnalysisHelper::FillEventWeights() {
        if (m_systematics->AffectsOnlyMET(m_systematics->GetCurrent())) return StatusCode::SUCCESS;
        ATH_CHECK(m_electron_selection->SaveScaleFactor());
        ATH_CHECK(m_muon_selection->SaveScaleFactor());
        ATH_CHECK(m_photon_selection->SaveScaleFactor());
        ATH_CHECK(m_tau_selection->SaveScaleFactor());
        ATH_CHECK(m_jet_selection->SaveScaleFactor());
        ATH_CHECK(m_met_selection->SaveScaleFactor());
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::FillEvent(const CP::SystematicSet* set) {
        ATH_CHECK(m_XAMPPInfo->SetSystematic(set));
        ATH_MSG_DEBUG("Compute the variables needed for later analysis");
        ATH_CHECK(ComputeEventVariables());
        ATH_MSG_DEBUG("Fill the analysis modules");
        ATH_CHECK(processModules());
        ATH_MSG_DEBUG("Check Dumping cuts");
        // Reset the nominal dumped flag
        bool pass_dump = applyEventDumpCuts();
        // Check again the dumping cuts
        if (set == m_systematics->GetNominal()) {
            // The event cleaning fails the missing ET systematics as well
            // we can safely bail out this event
            if (!PassObjectCleaning(set)) {
                ATH_MSG_DEBUG("Apparently systematic groups are introduced and nominal did not survive the object event cleaning");
                return StatusCode::SUCCESS;
            }
        } else if (!pass_dump)
            return StatusCode::SUCCESS;
        ATH_MSG_DEBUG("Fill all the SFs");
        if (!isData()) ATH_CHECK(FillEventWeights());
        if (!pass_dump) return StatusCode::SUCCESS;
        ATH_MSG_DEBUG("Dump output");
        ATH_CHECK(DumpNtuple(set));
        ATH_CHECK(DumpHistos(set));
        return StatusCode::SUCCESS;
    }  // namespace XAMPP
    bool SUSYAnalysisHelper::applyEventDumpCuts() { return m_config->ApplyCuts(XAMPP::CutKind::EventDump); }
    StatusCode SUSYAnalysisHelper::processModules() {
        if (m_hasModules) {
            for (auto& module : m_analysis_modules) {
                ATH_MSG_DEBUG("Fill module " << module->name() << ".");
                ATH_CHECK(module->fill());
            }
        }
        return StatusCode::SUCCESS;
    }
    bool SUSYAnalysisHelper::CheckTrigger() { return m_triggers->CheckTrigger(); }
    bool SUSYAnalysisHelper::EventCleaning() const { return m_XAMPPInfo->PassCleaning(); }
    double SUSYAnalysisHelper::GetMCXsec(unsigned int mc_channel_number, unsigned int finalState) {
        if (m_XsecDB == nullptr) {
            ATH_MSG_WARNING("I do not know about the cross-section");
            return -1;
        }
        return m_XsecDB->rawxsect(mc_channel_number, finalState);
    }

    void SUSYAnalysisHelper::GetMCXsecErrors(bool& error_exists, double& rel_err_down, double& rel_err_up, unsigned int mc_channel_number,
                                             unsigned int finalState) {
        error_exists = false;
        if (m_XsecDB == nullptr) { ATH_MSG_WARNING("I do not know about the cross-section"); }
        // the SUSY thing seems to have symmetric errors??!
        double relErr = m_XsecDB->rel_uncertainty(mc_channel_number, finalState);
        if (relErr > 0) {
            error_exists = true;
            rel_err_down = relErr;
            rel_err_up = relErr;
        }
    }
    double SUSYAnalysisHelper::GetMCFilterEff(unsigned int mc_channel_number, unsigned int finalState) {
        if (m_XsecDB == nullptr) {
            ATH_MSG_WARNING("I do not know about the filter efficiency");
            return -1;
        }
        return m_XsecDB->efficiency(mc_channel_number, finalState);
    }
    double SUSYAnalysisHelper::GetMCkFactor(unsigned int mc_channel_number, unsigned int finalState) {
        if (m_XsecDB == nullptr) {
            ATH_MSG_WARNING("I do not know about the k-factor");
            return -1;
        }
        return m_XsecDB->kfactor(mc_channel_number, finalState);
    }
    double SUSYAnalysisHelper::GetMCXsectTimesEff(unsigned int mc_channel_number, unsigned int finalState) {
        if (m_XsecDB == nullptr) {
            ATH_MSG_WARNING("I do not know about the cross-section");
            return -1;
        }
        return m_XsecDB->xsectTimesEff(mc_channel_number, finalState);
    }
    StatusCode SUSYAnalysisHelper::DumpNtuple(const CP::SystematicSet* sys) {
        if (!m_doTrees) return StatusCode::SUCCESS;
        if (m_treeVec.empty()) {
            ATH_MSG_FATAL("No trees have been made thus far");
            return StatusCode::FAILURE;
        }
        ATH_CHECK(m_treeVec[sys]->FillTree());
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::DumpHistos(const CP::SystematicSet* sys) {
        if (!m_doHistos) return StatusCode::SUCCESS;
        if (m_histoVec.empty()) return StatusCode::FAILURE;
        ATH_CHECK(m_histoVec[sys]->FillHistos());
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::CheckCutFlow(const CP::SystematicSet* systset) {
        if (!m_RunCutFlow) {
            ATH_MSG_WARNING("Cutflows diabled");
            return StatusCode::SUCCESS;
        }
        ATH_CHECK(m_XAMPPInfo->SetSystematic(systset));
        m_config->ApplyCuts(XAMPP::CutKind::MonitorCutFlow);
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::initializeEventVariables() {
        // Lets define some variables which we want to store in the output tree
        // / use in the cutflow
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<float>("JetHt"));
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<int>("N_bjets"));
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<int>("N_elecs"));

        ATH_CHECK(m_XAMPPInfo->NewEventVariable<int>("N_SignalLeptons", false));  // A variable on which we are cutting
                                                                                  // does not to be stored in the tree
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<int>("N_Jets", false));           // A variable on which we are cutting does not
                                                                                  // to be stored in the tree
        // You can also create trees using the Particle Storage Variable
        // The syntax is at follows
        if (doTruth()) {
            ATH_CHECK(m_XAMPPInfo->BookCommonParticleStorage("TruthParticles", true));
            // The second and third arguments are optional. The second argument
            // switches whether the mass (true) or the energy(false) of the
            // particles will be saved in the output
            XAMPP::ParticleStorage* TruthStore = m_XAMPPInfo->GetParticleStorage("TruthParticles");
            // As for the XAMPP::Storage the third argument determines whether
            // the tree is written for systematics After you have created the
            // ParticleStorage you can retrieve it in order to save further
            // variables of the particle Lets save the charge only for the
            // nominal Case
            ATH_CHECK(TruthStore->SaveFloat("charge"));
            ATH_CHECK(TruthStore->SaveInteger("pdgId"));
        }

        ATH_CHECK(m_XAMPPInfo->BookParticleStorage("Elec"));

        XAMPP::ParticleStorage* ElectronStore = m_XAMPPInfo->GetParticleStorage("Elec");
        // You can also call SaveInteger SaveChar and SaveDouble to store these
        // data types for a particle
        ATH_CHECK(ElectronStore->SaveVariable<float>("charge"));
        ATH_CHECK(ElectronStore->SaveVariable<char>("passOR"));
        // The latest feature of XAMPPbase is the implementation of so-called systematic groups. Systematic groups split
        // a defined subset of branches apart into dedicated tree in which the nominal values of the variables of interest
        // are stored to. Analysis trees not affected by the systematic variation -- including nominal -- are linked
        // against this new tree using the TTree friend mechanism... Tree explictly affected by the variation,
        // still contain the full set of branches !!ATTENTION!! To ensure that the data is always consistent
        // XAMPP always checks that the container are the *same* in terms of particle content and ordering.
        // Therefore, it's recommended to use the Pre containers from each particle selector always referring back to
        // the nominal container which is slimmed by the basic kinematic cuts and quality requirements
        ATH_CHECK(m_XAMPPInfo->createSystematicGroup("ElectronGroup", SelectionObject::Electron));
        ATH_CHECK(ElectronStore->setSystematicGroup("ElectronGroup"));
        // Allthough the variation is not affecting the object directly it has an indirect impact through the overlap
        // removal. Selection flags like passOR cannot be piped to the nominal tree of the systematic group but have to be
        // piped into each tree instead. The following method takes care of the piping. No checks are performed in the background
        // whether the variable is added or not. In case it is the mechanism works otherwise silence remains....
        ElectronStore->pipeVariableToAllTrees("passOR");
        ATH_CHECK(m_XAMPPInfo->GetVariableStorage<int>("N_elecs")->setSystematicGroup("ElectronGroup"));

        ATH_CHECK(m_XAMPPInfo->createSystematicGroup("MuonGroup", SelectionObject::Muon));

        ATH_CHECK(m_XAMPPInfo->BookParticleStorage("Muon"));
        XAMPP::ParticleStorage* MuonStore = m_XAMPPInfo->GetParticleStorage("Muon");
        ATH_CHECK(MuonStore->SaveFloat("charge", false));
        ATH_CHECK(MuonStore->setSystematicGroup("MuonGroup"));

        ATH_CHECK(m_XAMPPInfo->BookParticleStorage("Jet"));
        XAMPP::ParticleStorage* JetStore = m_XAMPPInfo->GetParticleStorage("Jet");
        ATH_CHECK(JetStore->SaveVariable<float>("Jvt"));

        ATH_CHECK(m_XAMPPInfo->BookParticleStorage("RecoCandidates"));
        XAMPP::ParticleStorage* RecoStore = m_XAMPPInfo->GetParticleStorage("RecoCandidates");
        ATH_CHECK(RecoStore->SaveFloat("charge"));
        ATH_CHECK(RecoStore->SaveInteger("pdgId"));

        return StatusCode::SUCCESS;
    }
    StatusCode SUSYAnalysisHelper::ComputeEventVariables() {
        // Now we want to save the SignalElectrons in the tree
        // First lets define the Pointer to the Storage elements
        static XAMPP::Storage<float>* dec_JetHt = m_XAMPPInfo->GetVariableStorage<float>("JetHt");
        // Static avoids that the Storage element is
        // retrieved each function call
        static XAMPP::Storage<int>* dec_Nbjet = m_XAMPPInfo->GetVariableStorage<int>("N_bjets");
        static XAMPP::Storage<int>* dec_Nlep = m_XAMPPInfo->GetVariableStorage<int>("N_SignalLeptons");
        static XAMPP::Storage<int>* dec_Njets = m_XAMPPInfo->GetVariableStorage<int>("N_Jets");

        static XAMPP::Storage<int>* dec_Nelecs = m_XAMPPInfo->GetVariableStorage<int>("N_elecs");
        ATH_CHECK(dec_Nelecs->Store(m_electron_selection->GetPreElectrons()->size()));
        // Then lets calculate our event variables... there are lots of
        // functions in the AnalysisUtils in order to do that, feel free to add
        // other functions for this purpose
        int N_Lep = m_electron_selection->GetSignalElectrons()->size() + m_muon_selection->GetSignalMuons()->size();
        int NJets = m_jet_selection->GetSignalJets()->size();
        int Nbjets = m_jet_selection->GetBJets()->size();
        float Ht = XAMPP::CalculateHt(m_jet_selection->GetSignalJets(), 20.e3);
        bool RecoPass = true;
        // This should reconstruct all Z bosons in the event
        while (RecoPass) {
            xAOD::Particle* Z = m_ParticleConstructor->CreateEmptyParticle();
            RecoPass = XAMPP::RecoZfromLeps(m_electron_selection->GetSignalElectrons(), Z);
            m_ParticleConstructor->WellDefined(Z, RecoPass);
        }
        // Finally store the variables they are then used by the Cut Class or
        // just written out into the trees
        ATH_CHECK(dec_JetHt->Store(Ht));
        ATH_CHECK(dec_Nbjet->Store(Nbjets));
        ATH_CHECK(dec_Nlep->Store(N_Lep));
        ATH_CHECK(dec_Njets->Store(NJets));

        static XAMPP::ParticleStorage* ElectronStore = m_XAMPPInfo->GetParticleStorage("Elec");
        ATH_CHECK(ElectronStore->Fill(m_electron_selection->GetSignalNoORElectrons()));
        static XAMPP::ParticleStorage* MuonStore = m_XAMPPInfo->GetParticleStorage("Muon");
        ATH_CHECK(MuonStore->Fill(m_muon_selection->GetSignalNoORMuons()));

        static XAMPP::ParticleStorage* JetStore = m_XAMPPInfo->GetParticleStorage("Jet");
        ATH_CHECK(JetStore->Fill(m_jet_selection->GetSignalJets()));

        if (doTruth() && m_XAMPPInfo->GetSystematic() == m_systematics->GetNominal()) {
            XAMPP::ParticleStorage* TruthStore = m_XAMPPInfo->GetParticleStorage("TruthParticles");
            ATH_CHECK(TruthStore->Fill(m_truth_selection->GetTruthParticles()));
        }
        static XAMPP::ParticleStorage* RecoStore = m_XAMPPInfo->GetParticleStorage("RecoCandidates");
        ATH_CHECK(RecoStore->Fill(m_ParticleConstructor->GetContainer()));

        return StatusCode::SUCCESS;
    }
    std::shared_ptr<TreeBase> SUSYAnalysisHelper::CreateTreeClass(const CP::SystematicSet* set) { return std::make_shared<TreeBase>(set); }
    std::shared_ptr<HistoBase> SUSYAnalysisHelper::CreateHistoClass(const CP::SystematicSet* set) {
        return std::make_shared<HistoBase>(set);
    }
    void SUSYAnalysisHelper::DisableRecoFlags() { m_StoreRecoFlags = false; }
    ST::SUSYObjDef_xAOD* SUSYAnalysisHelper::SUSYToolsPtr() {
        ST::SUSYObjDef_xAOD* ST = dynamic_cast<ST::SUSYObjDef_xAOD*>(m_susytools.operator->());
        return ST;
    }
    bool SUSYAnalysisHelper::doTruth() const { return m_doTruth; }
    bool SUSYAnalysisHelper::isInitialized() const { return m_init; }
    bool SUSYAnalysisHelper::buildCommonTree() const { return m_buildCommonTree; }
}  // namespace XAMPP
