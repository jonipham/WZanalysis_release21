#include <XAMPPbase/MetaDataTree.h>
#include <iostream>

#include <XAMPPbase/IAnalysisHelper.h>

#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/EventInfo.h>
#include <xAODCutFlow/CutBookkeeper.h>
#include <xAODCutFlow/CutBookkeeperContainer.h>

#include <xAODMetaData/FileMetaData.h>
#include <xAODTruth/TruthMetaDataContainer.h>

//#include <PMGTools/PMGTruthWeightTool.h>
#include <xAODLuminosity/LumiBlockRange.h>
#include <xAODLuminosity/LumiBlockRangeContainer.h>

#ifndef XAOD_STANDALONE
#include <AthAnalysisBaseComps/AthAnalysisHelper.h>
#include <EventInfo/EventStreamInfo.h>
#include <GaudiKernel/ITHistSvc.h>
#endif
namespace XAMPP {
    // List of prcoess IDs
    // https://twiki.cern.ch/twiki/bin/view/AtlasProtected/SUSYSignalUncertainties#Subprocess_IDs
    std::vector<unsigned int> SUSYprocessIDs() {
        static std::vector<unsigned int> proc_ids;
        if (proc_ids.empty()) {
            for (int i = 1; i <= 220; ++i) {
                // Skip all ranges not occupied by any SUSYprocess
                if ((i > 4 && i < 51) || (i > 52 && i < 61) || (i > 62 && i < 71) || (i > 78 && i < 81) || (i > 89 && i < 111)) continue;
                if ((i > 118 && i < 122) || (i > 128 && i < 133) || (i > 138 && i < 144) || (i > 158 && i < 167)) continue;
                if ((i > 168 && i < 201) || i == 215) continue;
                proc_ids.push_back(i);
            }
        }
        return proc_ids;
    }
    //#################################################################
    //                          MetaDataTree
    //#################################################################
    MetaDataTree::MetaDataTree(const std::string& myname) :
        AsgMetadataTool(myname),
        m_TreeName("MetaDataTree"),
        m_UseFileMetaData(true),
        m_fillLHEWeights(false),
        m_shiftMetaDSID(false),
        m_tree(nullptr),
        m_histSvc("THistSvc", myname),
        m_isData(false),
        m_init(false),
        m_analysis_helper("AnalysisHelper"),
        m_XAMPPInfo("EventInfoHandler"),
        m_MetaDB(),
        m_ActDB(m_MetaDB.end()),
        m_DummyEntry(nullptr) {
        declareProperty("isData", m_isData);
        declareProperty("TreeName", m_TreeName);
        declareProperty("AnalysisHelper", m_analysis_helper);
        declareProperty("useFileMetaData", m_UseFileMetaData);
        declareProperty("fillLHEWeights", m_fillLHEWeights);
        declareProperty("SwitchOnDSIDshift", m_shiftMetaDSID);
        m_XAMPPInfo.declarePropertyFor(this, "EventInfoHandler", "The XAMPPInfo event Handler");
    }

    StatusCode MetaDataTree::initialize() {
        ATH_MSG_INFO("Initialising...");
        ATH_CHECK(m_analysis_helper.retrieve());
        ATH_CHECK(m_XAMPPInfo.retrieve());
        if (m_init) {
            ATH_MSG_WARNING("The metadata tree is already intialized");
            return StatusCode::SUCCESS;
        }
        m_tree = new TTree(m_TreeName.c_str(), "MetaData Tree for Small Analysis Ntuples");
        m_tree->Branch("isData", &m_isData);
        ATH_CHECK(m_histSvc->regTree("/XAMPP/" + m_TreeName, m_tree));
        m_init = true;
        if (m_isData) m_fillLHEWeights = false;
        return StatusCode::SUCCESS;
    }
    MetaDataTree::~MetaDataTree() {}
    void MetaDataTree::LoadMCMetaData(unsigned int mcChannel, unsigned int periodNumber) {
        MetaID Ident(mcChannel, periodNumber);
        std::vector<std::string> LHEnames;
        if (m_fillLHEWeights) { LHEnames = getLHEWeightNames(); }
        if (m_ActDB == m_MetaDB.end() || m_ActDB->first != Ident) {
            m_ActDB = m_MetaDB.find(Ident);
            if (m_ActDB == m_MetaDB.end()) {
                ATH_MSG_INFO("Found new mcChannel " << mcChannel << ".  Create new MCMetaData entry.");
                m_MetaDB.insert(std::pair<MetaID, std::shared_ptr<MetaDataElement>>(
                    Ident, std::make_shared<MetaDataMC>(mcChannel, periodNumber, m_analysis_helper, m_XAMPPInfo.getHandle())));
                m_ActDB = m_MetaDB.find(Ident);
                if (m_fillLHEWeights) { m_ActDB->second->setLHEWeightNames(LHEnames); }
            }
        }
    }
    void MetaDataTree::LoadRunMetaData(unsigned int run) {
        MetaID Ident(run, run);
        if (m_ActDB == m_MetaDB.end() || m_ActDB->first != Ident) {
            m_ActDB = m_MetaDB.find(Ident);
            if (m_ActDB == m_MetaDB.end()) {
                ATH_MSG_INFO("Found new runNumber " << run << ".  Create new runMetaData entry.");
                m_MetaDB.insert(std::pair<MetaID, std::shared_ptr<MetaDataElement>>(
                    Ident, std::make_shared<runMetaData>(run, m_XAMPPInfo.getHandle())));
                m_ActDB = m_MetaDB.find(Ident);
            }
        }
    }
    StatusCode MetaDataTree::beginEvent() {
        ATH_CHECK(m_analysis_helper->LoadContainers());
        if (m_XAMPPInfo->isMC() == m_isData) {
            ATH_MSG_FATAL("The analysis is configured to run over data while the input file is MC...");
            return StatusCode::FAILURE;
        }
        // The current event is MC
        if (m_XAMPPInfo->isMC()) {
            LoadMCMetaData(m_XAMPPInfo->mcChannelNumber(), m_XAMPPInfo->runNumber());
        }
        // The event is data
        else
            LoadRunMetaData(m_XAMPPInfo->runNumber());

        if (m_DummyEntry) {
            ATH_CHECK(m_ActDB->second->CopyStore(m_DummyEntry.get()));
            m_DummyEntry = std::shared_ptr<MetaDataElement>();
        }
        ATH_CHECK(m_ActDB->second->newEvent());
        return StatusCode::SUCCESS;
    }
    StatusCode MetaDataTree::finalize() {
        if (!m_tree) { return StatusCode::SUCCESS; }
        for (const auto& Meta : m_MetaDB) ATH_CHECK(Meta.second->finalize(m_tree));
        m_tree = nullptr;
        return StatusCode::SUCCESS;
    }
    StatusCode MetaDataTree::CheckLumiBlockContainer(const std::string& Container, bool& HasCont) {
        if (!inputMetaStore()->contains<xAOD::LumiBlockRangeContainer>(Container)) {
            ATH_MSG_DEBUG("Lumi block range container " << Container << " not present");
            return StatusCode::SUCCESS;
        }
        const xAOD::LumiBlockRangeContainer* LumiBlocks = nullptr;
        ATH_CHECK(inputMetaStore()->retrieve(LumiBlocks, Container));
        for (const auto& lumi : *LumiBlocks) {
            if (lumi->startRunNumber() != lumi->stopRunNumber())
                ATH_MSG_WARNING("Found different start (" << lumi->startRunNumber() << " and end (" << lumi->stopRunNumber()
                                                          << ") runNumbers for lumiblock range " << lumi->startRunNumber() << "-"
                                                          << lumi->stopLumiBlockNumber());
            LoadRunMetaData(lumi->startRunNumber());
            ATH_CHECK(m_ActDB->second->newFile(lumi));
        }
        HasCont = true;
        return StatusCode::SUCCESS;
    }
    StatusCode MetaDataTree::fillLHEMetaData(unsigned int Idx) {
        if (Idx == 0) {
            ATH_MSG_ERROR(
                "This method is meant to store the variational weights in the "
                "metadata. Not nominal");
            return StatusCode::FAILURE;
        }
        if (m_isData) {
            ATH_MSG_ERROR("Data does not have any weight variations");
            return StatusCode::FAILURE;
        }
        if (!m_fillLHEWeights) {
            ATH_MSG_DEBUG("No lhe meta-data should be stored");
            return StatusCode::SUCCESS;
        }
        const xAOD::EventInfo* info = m_XAMPPInfo->GetOrigInfo();

        if (Idx >= info->mcEventWeights().size()) {
            ATH_MSG_ERROR("Index " << Idx << " is out of range");
            return StatusCode::FAILURE;
        }
        LoadMCMetaData(m_XAMPPInfo->mcChannelNumber(), m_XAMPPInfo->runNumber());
        double W = m_XAMPPInfo->GetGenWeight(Idx);
        unsigned int finalState = m_analysis_helper->finalState();
        if (finalState > 0 && !m_ActDB->second->fillVariation((Idx + 1000) * 1.e4 + finalState, W).isSuccess()) {
            return StatusCode::FAILURE;
        }
        return m_ActDB->second->fillVariation(Idx + 1000, W);
    }
    std::vector<std::string> MetaDataTree::getLHEWeightNames() const {
        std::vector<std::string> ret;
        if (m_isData) return ret;
        if (inputMetaStore()->contains<xAOD::TruthMetaDataContainer>("TruthMetaData")) {
            const xAOD::TruthMetaDataContainer* truthmetadata = nullptr;
            if (inputMetaStore()->retrieve(truthmetadata, "TruthMetaData").isFailure()) {
                ATH_MSG_WARNING("Failed to pick up truth metadata, can not write LHE by name");
                return ret;
            };
            for (auto meta : *truthmetadata) {
                if (!meta->weightNames().empty()) ret.insert(ret.end(), meta->weightNames().begin(), meta->weightNames().end());
            }
        }
        return ret;
    }

    StatusCode MetaDataTree::beginInputFile() {
        if (!m_UseFileMetaData) {
            ATH_MSG_INFO("File meta data is disabled");
            return StatusCode::SUCCESS;
        }
        bool HasLumiCont = false;
        if (m_isData) {
            ATH_CHECK(CheckLumiBlockContainer("LumiBlocks", HasLumiCont));
            ATH_CHECK(CheckLumiBlockContainer("IncompleteLumiBlocks", HasLumiCont));
            for (auto& Meta : m_MetaDB) Meta.second->CutBookKeeperAvailable(HasLumiCont);
        }
        if (inputMetaStore()->contains<xAOD::CutBookkeeperContainer>("CutBookkeepers")) {
            const xAOD::CutBookkeeperContainer* bks = nullptr;
            ATH_CHECK(inputMetaStore()->retrieve(bks, "CutBookkeepers"));
            if (!m_isData) {
                std::shared_ptr<MetaDataElement> MC;
                const EventStreamInfo* esi = nullptr;
                ATH_CHECK(inputMetaStore()->retrieve(esi));
                if (esi->getEventTypes().size() > 1) ATH_MSG_WARNING("There seem to be more event types than one");
                if (m_shiftMetaDSID && !m_XAMPPInfo->applyDSIDShift() && m_XAMPPInfo->dsidOffSet() != 0) {
                    ATH_MSG_WARNING(
                        "The meta-data information will be shifted by "
                        << m_XAMPPInfo->dsidOffSet()
                        << ". Please make sure that the information in the meta-data is consistent with the information given in the tree");
                }
                LoadMCMetaData(esi->getEventTypes().begin()->mc_channel_number() +
                                   (m_shiftMetaDSID || m_XAMPPInfo->applyDSIDShift() ? m_XAMPPInfo->dsidOffSet() : 0),
                               (*esi->getRunNumbers().begin()));
                MC = m_ActDB->second;

                if (m_fillLHEWeights) {
                    std::map<std::string, int> VariationNames;
                    if (inputMetaStore()->contains<xAOD::TruthMetaDataContainer>("TruthMetaData")) {
                        const xAOD::TruthMetaDataContainer* truthmetadata = nullptr;
                        ATH_CHECK(inputMetaStore()->retrieve(truthmetadata, "TruthMetaData"));
                        for (auto meta : *truthmetadata) {
                            if (!meta->weightNames().empty()) MC->setLHEWeightNames(meta->weightNames());
                        }
                    }
                }
                ATH_CHECK(MC->newFile(bks));
            }
            // Data instance and no lumi container is present
            else if (!HasLumiCont) {
                if (m_DummyEntry) ATH_MSG_WARNING("There is still an instance of the dummy meta");
                m_DummyEntry = std::shared_ptr<MetaDataElement>(new runMetaData(-1, m_XAMPPInfo.getHandle()));
                ATH_CHECK(m_DummyEntry->newFile(bks));
            }
        } else {
            ATH_MSG_DEBUG("No CutBookKeeper has been found.");
            for (auto& Meta : m_MetaDB) Meta.second->CutBookKeeperAvailable(false);
        }
        return StatusCode::SUCCESS;
    }
    void MetaDataTree::subtractEventFromMetaData(unsigned int index) {
        if (m_isData) return;
        ATH_MSG_WARNING("Subtract event " << m_XAMPPInfo->eventNumber() << " in sample " << m_XAMPPInfo->mcChannelNumber()
                                          << " from the Sum of weights at index " << index);
        LoadMCMetaData(m_XAMPPInfo->mcChannelNumber(), m_XAMPPInfo->runNumber());
        unsigned int finalState = m_analysis_helper->finalState();
        unsigned int LHE_Idx = (index == 0 ? finalState : (finalState > 0 ? (index + 1000) * 1.e4 + finalState : index + 1000));
        m_ActDB->second->SubtractEvent(LHE_Idx, m_XAMPPInfo->GetRawGenWeight(index));
    }
    void MetaDataTree::subtractEventFromMetaData() {
        if (m_isData) return;
        ATH_MSG_WARNING("Subtract event " << m_XAMPPInfo->eventNumber() << " in sample " << m_XAMPPInfo->mcChannelNumber()
                                          << " from the Sum of weights.");
        const xAOD::EventInfo* info = m_XAMPPInfo->GetOrigInfo();

        size_t nWeights = m_fillLHEWeights ? info->mcEventWeights().size() : 1;
        for (size_t Idx = 0; Idx < nWeights; ++Idx) { subtractEventFromMetaData(Idx); }
    }

    //########################################################################################################
    //                                 MetaDataElement
    //########################################################################################################
    MetaDataElement::MetaDataElement(ToolHandle<XAMPP::IEventInfo> XAMPPInfo) : m_XAMPPInfo(XAMPPInfo), m_LHE_WeightNames() {}
    void MetaDataElement::setLHEWeightNames(const std::vector<std::string>& weights) { m_LHE_WeightNames = weights; }
    size_t MetaDataElement::numOfWeights() const { return m_LHE_WeightNames.size(); }
    const xAOD::CutBookkeeper* MetaDataElement::FindCutBookKeeper(const xAOD::CutBookkeeperContainer* container,
                                                                  MetaDataElement::BookKeeperType Type, unsigned int procID) {
        static bool lhe3MessageShown = false;
        const xAOD::CutBookkeeper* all = nullptr;
        int maxCycle = -1;  // need to find the max cycle where input stream is
                            // StreamAOD and the name is AllExecutedEvents

        std::string cbk_name = "AllExecutedEvents";
        std::string alt_cbk_name;
        if (Type == MetaDataElement::BookKeeperType::SUSY) {
            cbk_name = Form("SUSYWeight_ID_%d", procID);
        } else if (Type == MetaDataElement::BookKeeperType::LHE3) {
            if (procID >= m_LHE_WeightNames.size()) {
                Error("MetaDataElement::FindCutBookKeeper()", "Invalid variation ID: %d", procID);
                return nullptr;
            }
            cbk_name = "LHE3Weight_" + RemoveAllExpInStr(m_LHE_WeightNames.at(procID), ".");
            alt_cbk_name = RemoveAllExpInStr(cbk_name, " ");
        }

        for (auto cbk : *container) {
            bool Stream = (cbk->inputStream() == "StreamAOD" || cbk->inputStream().find("DAOD_TRUTH") != std::string::npos);
            if (Type == MetaDataElement::BookKeeperType::Original) {
                Stream = Stream && cbk->name() == cbk_name;
            } else if (Type == MetaDataElement::BookKeeperType::SUSY && procID > 0) {
                Stream = Stream && cbk->name() == cbk_name;
                if (Stream) return cbk;
            } else if (Type == MetaDataElement::BookKeeperType::LHE3) {
                Stream = Stream && (cbk->name() == cbk_name || cbk->name() == alt_cbk_name || cbk->name() == m_LHE_WeightNames.at(procID));
                if (Stream) {
                    if (!lhe3MessageShown)
                        Info("MetaDataElement::FindCutBookKeeper()", "Found new lhe3 variation %s which will be associated to ID %u",
                             cbk->name().c_str(), 1000 + procID);
                    return cbk;
                }
            }
            if (Stream && cbk->cycle() > maxCycle) {
                maxCycle = cbk->cycle();
                all = cbk;
            }
        }
        if (Type == MetaDataElement::BookKeeperType::LHE3) lhe3MessageShown = true;
        return all;
    }
    //########################################################################################################
    //                                 MetaDataMC
    //########################################################################################################

    MetaDataMC::MetaDataMC(unsigned int mcChannel, unsigned int periodNumber, ToolHandle<XAMPP::IAnalysisHelper>& helper,
                           ToolHandle<XAMPP::IEventInfo> info) :
        MetaDataElement(info),
        m_MC(mcChannel),
        m_periodNumber(periodNumber),
        m_helper(helper),
        m_Data(),
        m_ActMeta(m_Data.end()),
        m_Inclusive(0),
        m_init(false) {
        m_init = m_helper.retrieve().isSuccess() && m_XAMPPInfo.retrieve().isSuccess();
        if (m_init) LoadMetaData(0);
    }
    StatusCode MetaDataMC::newFile(const xAOD::LumiBlockRange*) {
        Warning("MetaDataMC::newFile()", "Lumi blocks not supported.");
        return StatusCode::SUCCESS;
    }
    void MetaDataMC::LoadMetaData(unsigned int ID) {
        // See whether the process ID is already Loaded;
        if (m_ActMeta == m_Data.end() || m_ActMeta->first != ID) {
            m_ActMeta = m_Data.find(ID);
            // No Meta data has been found
            if (m_ActMeta == m_Data.end()) {
                std::string procName = "";
                if (ID > 1000 && (ID - 1000) < m_LHE_WeightNames.size()) {
                    Info("MetaDataMC::LoadMetaData()", Form("New LHE variation %u found for mcChannelNumber %u "
                                                            "corresponding to %s",
                                                            ID, m_MC, m_LHE_WeightNames[ID - 1000].c_str()));

                    procName = m_LHE_WeightNames.at(ID - 1000);
                } else {
                    Info("MetaDataMC::LoadMetaData()", Form("New process Id %u found for mcChannelNumber %u", ID, m_MC));
                    Info("MetaDataMC::LoadMetaData()", Form(" have %lu names ", m_LHE_WeightNames.size()));
                    if (ID == 0) {
                        procName = "Nominal_Inclusive";
                    } else {
                        procName = std::to_string(ID);
                    }
                }

                m_Data.insert(
                    std::pair<unsigned int, std::shared_ptr<MetaDataMC::MetaData>>(ID, std::make_shared<MetaDataMC::MetaData>((ID))));
                m_ActMeta = m_Data.find(ID);
                m_ActMeta->second->procName = procName;
                // If the SUSY process id = 0 -> inclusive state
                if (ID == 0) m_Inclusive = m_ActMeta->second;
            }
        }
    }

    StatusCode MetaDataMC::newFile(const xAOD::CutBookkeeperContainer* container) {
        if (!m_init) {
            Error("MetaDataMC::newFile()", "The component has not been intialized");
            return StatusCode::FAILURE;
        }
        if (!container) {
            Error("MetaDataMC::newFile()", "No CutBookkeeperContainer was given");
            return StatusCode::FAILURE;
        }
        CutBookKeeperAvailable(false);
        const xAOD::CutBookkeeper* all = FindCutBookKeeper(container);
        if (!all) {
            Error("MetaDataMC::newFile()", "Could not read out the CutBookKeeper");
            return StatusCode::FAILURE;
        }
        std::function<double(const xAOD::CutBookkeeper*)> sumW = [this](const xAOD::CutBookkeeper* cbk) {
            return m_XAMPPInfo->getOutlierWeightStrategy() != IEventInfo::resetWeight
                       ? cbk->sumOfEventWeights()
                       : cbk->sumOfEventWeights();  // MG: The truncated version is broken as of 19-03-18
                                                    //   : cbk->sumOfTruncatedEventWeights();
        };
        std::function<double(const xAOD::CutBookkeeper*)> sumW2 = [this](const xAOD::CutBookkeeper* cbk) {
            return m_XAMPPInfo->getOutlierWeightStrategy() != IEventInfo::resetWeight
                       ? cbk->sumOfEventWeightsSquared()
                       : cbk->sumOfEventWeightsSquared();  // MG: The truncated version is broken as of 19-03-18
                                                           //   : cbk->sumOfTruncatedEventWeightsSquared();
        };

        AddFileInformation(m_Inclusive, all->nAcceptedEvents(), sumW(all), sumW2(all));
        // Load the SUSY Cut BookKeepers
        for (auto& ID : SUSYprocessIDs()) {
            const xAOD::CutBookkeeper* proc = FindCutBookKeeper(container, MetaDataElement::BookKeeperType::SUSY, ID);
            if (proc == nullptr || !proc->sumOfEventWeights()) continue;
            LoadMetaData(ID);
            AddFileInformation(m_ActMeta->second, proc->nAcceptedEvents(), sumW(proc), sumW2(proc));
        }
        // Load the LHE3 CutBookKeepers
        if (numOfWeights() > 0) {
            for (size_t lhe = numOfWeights() - 1; lhe > 0; --lhe) {
                const xAOD::CutBookkeeper* lhe_keeper = FindCutBookKeeper(container, MetaDataElement::BookKeeperType::LHE3, lhe);
                if (lhe_keeper == nullptr || !lhe_keeper->sumOfEventWeights()) continue;
                LoadMetaData(1000 + lhe);
                AddFileInformation(m_ActMeta->second, lhe_keeper->nAcceptedEvents(), sumW(lhe_keeper), sumW2(lhe_keeper));
            }
        }

        return StatusCode::SUCCESS;
    }
    StatusCode MetaDataMC::newEvent() {
        if (!m_init) {
            Error("MetaDataMC::newFile()", "The component has not been inialized");
            return StatusCode::FAILURE;
        }
        unsigned int ProcID = m_helper->finalState();
        double Weight = m_XAMPPInfo->GetGenWeight();

        if (ProcID > 0) {
            LoadMetaData(ProcID);
            AddEventInformation(m_ActMeta->second, Weight);
        }
        AddEventInformation(m_Inclusive, Weight);
        return StatusCode::SUCCESS;
    }
    void MetaDataMC::SubtractEvent(unsigned int ProcID, double W) {
        if (ProcID > 0) {
            LoadMetaData(ProcID);
            SubtractEvent(m_ActMeta->second, W);
        }
        if (ProcID <= 1000) SubtractEvent(m_Inclusive, W);
    }
    StatusCode MetaDataMC::fillVariation(unsigned int Id, double W) {
        if (Id <= 1000) {
            Error("MetaDataMC::fillVariation", Form("Process id %u is protected.", Id));
            return StatusCode::FAILURE;
        }
        LoadMetaData(Id);
        AddEventInformation(m_ActMeta->second, W);
        return StatusCode::SUCCESS;
    }

    void MetaDataMC::AddEventInformation(std::shared_ptr<MetaDataMC::MetaData> Meta, double GenWeight) {
        ++Meta->NumProcessedEvents;
        if (!Meta->MetaInit) {
            // Retrieve the processId
            unsigned int SUSYId = Meta->ProcID != 0 ? m_helper->finalState() : 0;
            Meta->xSec = m_helper->GetMCXsec(m_MC, SUSYId);
            double rel_var_xs_down = -1;
            double rel_var_xs_up = -1;
            m_helper->GetMCXsecErrors(Meta->has_xSec_err, rel_var_xs_down, rel_var_xs_up, m_MC, SUSYId);
            // wait with setting the meta since we promise to ignore the two double refs if there is no well defined XS err
            if (Meta->has_xSec_err) {
                Meta->xSec_err_down = rel_var_xs_down;
                Meta->xSec_err_up = rel_var_xs_up;
            }
            Meta->FilterEff = m_helper->GetMCFilterEff(m_MC, SUSYId);
            Meta->kFaktor = m_helper->GetMCkFactor(m_MC, SUSYId);
            Meta->luminosity = m_XAMPPInfo->GetPileUpLuminosity();

            Meta->MetaInit = true;
            double xSecTimes = Meta->xSec * Meta->kFaktor * Meta->FilterEff;
            Info("MetaDataMC::AddEventInformation()",
                 Form("Set xSection to %f for process %u with mcChannelNumber %u", xSecTimes, Meta->ProcID, m_MC));
        }
        if (Meta->KeeperAvailable) return;
        Meta->SumW = Meta->SumW + GenWeight;
        Meta->SumW2 = Meta->SumW2 + GenWeight * GenWeight;
        ++Meta->NumTotalEvents;
    }
    void MetaDataMC::SubtractEvent(std::shared_ptr<MetaDataMC::MetaData> Meta, double GenWeight) {
        Meta->SumW = Meta->SumW - GenWeight;
        Meta->SumW2 = Meta->SumW2 - GenWeight * GenWeight;
    }
    void MetaDataMC::CutBookKeeperAvailable(bool B) {
        for (auto& MC : m_Data) MC.second->KeeperAvailable = B;
    }
    StatusCode MetaDataMC::finalize(TTree* MetaDataTree) {
        if (!m_init) {
            Error("MetaDataMC::finalize()", "The component has not been inialized");
            return StatusCode::FAILURE;
        }
        if (!SetBranchAddress(MetaDataTree, "mcChannelNumber", m_MC)) {
            Error("MetaDataMC::finalize()", Form("Cannot finalize DSID %u", m_MC));
            return StatusCode::FAILURE;
        }
        if (!SetBranchAddress(MetaDataTree, "runNumber", m_periodNumber)) {
            Error("MetaDataMC::finalize()", Form("Cannot finalize DSID %u", m_MC));
            return StatusCode::FAILURE;
        }
        for (auto& MC : m_Data) {
            if (!SaveMetaDataInTree(MC.second, MetaDataTree)) {
                Error("MetaDataMC::finalize()", Form("Cannot finalize DSID %u", m_MC));
                return StatusCode::FAILURE;
            }
        }
        m_init = false;
        return StatusCode::SUCCESS;
    }
    bool MetaDataMC::SaveMetaDataInTree(std::shared_ptr<MetaDataMC::MetaData> Meta, TTree* tree) {
        if (!Meta) {
            Error("MetaDataMC::SaveMetaDataInTree()", "No element has been given");
            return false;
        }
        if (!SetBranchAddress(tree, "ProcessID", Meta->ProcID)) return false;
        if (!SetBranchAddress(tree, "ProcessName", &(Meta->procName))) return false;
        if (!SetBranchAddress(tree, "xSection", Meta->xSec)) return false;
        if (!SetBranchAddress(tree, "xSection_RelErrorDown", Meta->xSec_err_down)) return false;
        if (!SetBranchAddress(tree, "xSection_RelErrorUp", Meta->xSec_err_up)) return false;
        if (!SetBranchAddress(tree, "xSection_has_Uncertainties", Meta->has_xSec_err)) return false;
        if (!SetBranchAddress(tree, "kFactor", Meta->kFaktor)) return false;
        if (!SetBranchAddress(tree, "FilterEfficiency", Meta->FilterEff)) return false;
        if (!SetBranchAddress(tree, "TotalEvents", Meta->NumTotalEvents)) return false;
        if (!SetBranchAddress(tree, "TotalSumW", Meta->SumW)) return false;
        if (!SetBranchAddress(tree, "TotalSumW2", Meta->SumW2)) return false;
        if (!SetBranchAddress(tree, "ProcessedEvents", Meta->NumProcessedEvents)) return false;
        // Luminosity of the data to which the dataset is going to be assigned
        if (m_XAMPPInfo->ApplyPileUp()) {
            if (!SetBranchAddress(tree, "prwLuminosity", Meta->luminosity)) return false;
        }
        // Check that the weights in the meta-data tree are finite
        std::function<bool(const double&)> is_finite = [](const double& V) { return !std::isnan(V) && !std::isinf(V); };
        if (!is_finite(Meta->NumTotalEvents)) {
            Error("MetaDataMC()", "Total events is not a number");
            return false;
        }
        if (!is_finite(Meta->SumW)) {
            Error("MetaDataMC()", "The SumW is not a number.");
            return false;
        }
        if (!is_finite(Meta->SumW2)) {
            Error("MetaDataMC()", "The SumW2 is not a number.");
            return false;
        }

        tree->Fill();
        Info("MetaDataMC::SaveMetaDataInTree()",
             Form("Write metadata in file with DSID: %u, ProcessID : %u, process name: %s SumW: %f, SumW2: %f, TotalEvents: %llu, "
                  "ProcessedEvents: %llu",
                  m_MC, Meta->ProcID, Meta->procName.c_str(), Meta->SumW, Meta->SumW2, Meta->NumTotalEvents, Meta->NumProcessedEvents));
        return true;
    }
    MetaDataMC::~MetaDataMC() {}
    void MetaDataMC::AddFileInformation(std::shared_ptr<MetaDataMC::MetaData> Meta, Long64_t TotEv, double SumW, double SumW2) {
        Meta->NumTotalEvents = Meta->NumTotalEvents + TotEv;
        Meta->SumW = Meta->SumW + SumW;
        Meta->SumW2 = Meta->SumW2 + SumW2;
        Meta->KeeperAvailable = true;
        if (Meta->ProcID > 1000 && Meta->ProcID < 1000 + m_LHE_WeightNames.size()) {
            Meta->procName = m_LHE_WeightNames.at(Meta->ProcID - 1000);
        } else if (Meta->ProcID == 0) {
            Meta->procName = "Nominal_Inclusive";
        } else {
            Meta->procName = std::to_string(Meta->ProcID);
        }
    }
    StatusCode MetaDataMC::CopyStore(const MetaDataElement* Store) {
        const MetaDataMC* Other = dynamic_cast<const MetaDataMC*>(Store);
        if (!Store) {
            Error("MetaDataMC::CopyStore()", "Could not copy the input store");
            return StatusCode::FAILURE;
        }
        if (Other->m_MC != m_MC && Other->m_MC != (unsigned int)-1) {
            Error("MetaDataMC::CopyStore()", "Wrong MC channel");
            return StatusCode::FAILURE;
        }
        for (auto& toCopy : Other->m_Data) {
            LoadMetaData(toCopy.second->ProcID);
            AddFileInformation(m_ActMeta->second, toCopy.second->NumTotalEvents, toCopy.second->SumW, toCopy.second->SumW2);
            m_ActMeta->second->procName = toCopy.second->procName;
        }
        return StatusCode::SUCCESS;
    }

    //########################################################################################################
    //                                 runMetaData
    //########################################################################################################
    runMetaData::runMetaData(unsigned int runNumber, ToolHandle<XAMPP::IEventInfo> info) :
        MetaDataElement(info),
        m_runNumber(runNumber),
        m_NumTotalEvents(0),
        m_NumProcessedEvents(0),
        m_ProcessedBlocks(),
        m_TotalBlocks(),
        m_KeeperAvailable(false) {}
    StatusCode runMetaData::newFile(const xAOD::CutBookkeeperContainer* container) {
        const xAOD::CutBookkeeper* all = FindCutBookKeeper(container);
        if (!all) {
            Error("MetaDataMC::newFile()", "Could not read out the CutBookKeeper");
            return StatusCode::FAILURE;
        }
        CutBookKeeperAvailable(true);
        m_NumTotalEvents = m_NumTotalEvents + all->nAcceptedEvents();
        return StatusCode::SUCCESS;
    }
    StatusCode runMetaData::newEvent() {
        ++m_NumProcessedEvents;
        m_ProcessedBlocks.insert(m_XAMPPInfo->GetOrigInfo()->lumiBlock());
        if (!m_KeeperAvailable) {
            ++m_NumTotalEvents;
            m_TotalBlocks.insert(m_XAMPPInfo->GetOrigInfo()->lumiBlock());
        }
        return StatusCode::SUCCESS;
    }
    void runMetaData::CutBookKeeperAvailable(bool B) { m_KeeperAvailable = B; }
    StatusCode runMetaData::finalize(TTree* MetaDataTree) {
        if (!SetBranchAddress(MetaDataTree, "runNumber", m_runNumber)) return StatusCode::FAILURE;
        if (!SetBranchAddress(MetaDataTree, "TotalEvents", m_NumTotalEvents)) return StatusCode::FAILURE;
        if (!SetBranchAddress(MetaDataTree, "ProcessedEvents", m_NumProcessedEvents)) return StatusCode::FAILURE;
        std::set<unsigned int>* ProcLumiPtr = &m_ProcessedBlocks;
        if (!SetBranchAddress(MetaDataTree, "ProcessedLumiBlocks", ProcLumiPtr)) return StatusCode::FAILURE;
        std::set<unsigned int>* TotalLumiPtr = &m_TotalBlocks;
        if (!SetBranchAddress(MetaDataTree, "TotalLumiBlocks", TotalLumiPtr)) return StatusCode::FAILURE;

        Info("runMetaData::finalize()", Form("Fill metadata tree with runNumber: %u, TotalEvents: %llu, "
                                             "ProcessedEvents: %llu",
                                             m_runNumber, m_NumTotalEvents, m_NumProcessedEvents));
        MetaDataTree->Fill();
        return StatusCode::SUCCESS;
    }
    StatusCode runMetaData::CopyStore(const MetaDataElement* Store) {
        const runMetaData* toCopy = dynamic_cast<const runMetaData*>(Store);
        if (!toCopy) {
            Error("runMetaData::CopyStore()", "Failed to copy information");
            return StatusCode::FAILURE;
        }
        if (toCopy->m_runNumber != m_runNumber && toCopy->m_runNumber != (unsigned int)-1) {
            Error("runMetaData::CopyStore()", "Wrong run to copy");
            return StatusCode::FAILURE;
        }
        m_NumProcessedEvents += toCopy->m_NumProcessedEvents;
        m_NumTotalEvents += toCopy->m_NumTotalEvents;
        for (auto& BCID : toCopy->m_ProcessedBlocks) m_ProcessedBlocks.insert(BCID);
        for (auto& BCID : toCopy->m_TotalBlocks) m_TotalBlocks.insert(BCID);
        return StatusCode::SUCCESS;
    }
    runMetaData::~runMetaData() {
        m_TotalBlocks.clear();
        m_ProcessedBlocks.clear();
    }
    StatusCode runMetaData::newFile(const xAOD::LumiBlockRange* LumiBlock) {
        if (!LumiBlock) {
            Error("runMetaData::newFile()", "No LumiBlock element given");
            return StatusCode::FAILURE;
        }
        m_NumTotalEvents = m_NumTotalEvents + LumiBlock->eventsSeen();
        for (unsigned int L = LumiBlock->startLumiBlockNumber(); L <= LumiBlock->stopLumiBlockNumber(); ++L) m_TotalBlocks.insert(L);

        return StatusCode::SUCCESS;
    }
    StatusCode runMetaData::fillVariation(unsigned int, double) { return StatusCode::FAILURE; }

}  // namespace XAMPP
