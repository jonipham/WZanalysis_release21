#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/ISystematics.h>
#include <XAMPPbase/TreeBase.h>
#include <XAMPPbase/TreeHelpers.h>

#include <iostream>
#include <vector>

// EDM include(s):
#include <AsgAnalysisInterfaces/IGoodRunsListSelectionTool.h>
#include <AsgAnalysisInterfaces/IPileupReweightingTool.h>
#include <PATInterfaces/SystematicSet.h>
#include <PileupReweighting/PileupReweightingTool.h>
#include <PileupReweighting/TPileupReweighting.h>
#include <xAODCore/ShallowCopy.h>
namespace XAMPP {
    EventInfo::EventInfo(const std::string& myname) :
        AsgTool(myname),
        m_ConstEvtInfo(),
        m_EvtInfo(),
        m_primaryVtx(nullptr),
        m_ActSys(nullptr),
        m_systematics("SystematicsTool"),
        m_prwTool("CP::PileupReweightingTool/PrwTool"),
        m_GrlTool("GoodRunsListSelectionTool"),
        m_ApplyPRW(true),
        m_ApplyGRL(true),
        m_Init(false),
        m_Locked(false),
        m_Filter(true),
        m_RecoFlags(true),
        m_MultiPRWPeriods(false),
        m_DecPup(),
        m_nNVtx(nullptr),
        m_PassGRL(nullptr),
        m_PassLArTile(nullptr),
        m_HasVtx(nullptr),
        m_mu_density(nullptr),
        m_shiftDSISD(false),
        m_dsidOffSet(0),
        m_OutlierStrat(IEventInfo::doNothing),
        m_outlierWeightThreshold(100),
        m_systGroups() {
        declareProperty("SystematicsTool", m_systematics);
        declareProperty("ApplyGRLTool", m_ApplyGRL);
        declareProperty("ApplyPRW", m_ApplyPRW);
        declareProperty("FilterOutput", m_Filter);
        declareProperty("OutlierWeightStrategy", m_OutlierStrat);
        declareProperty("OutlierWeightThreshold", m_outlierWeightThreshold);
        declareProperty("SaveRecoFlags", m_RecoFlags);
        m_GrlTool.declarePropertyFor(this, "GRLTool", "The GRLTool");
        m_prwTool.declarePropertyFor(this, "PileupReweightingTool", "The pile up reweighting tool");

        declareProperty("SwitchOnDSIDshift", m_shiftDSISD);
        declareProperty("DSIDshift", m_dsidOffSet);
    }
    void EventInfo::Lock() { m_Locked = true; }
    int EventInfo::getOutlierWeightStrategy() const { return m_OutlierStrat; }

    StatusCode EventInfo::initialize() {
        if (m_Init) {
            ATH_MSG_WARNING("The event info is already initialized");
            return StatusCode::SUCCESS;
        }
        ATH_MSG_INFO("initialize...");

        ATH_CHECK(NewCommonEventVariable<char>("PassGRL", !m_Filter));
        ATH_CHECK(NewCommonEventVariable<char>("HasVtx", !m_Filter));
        ATH_CHECK(NewCommonEventVariable<char>("passLArTile", !m_Filter));
        ATH_CHECK(NewCommonEventVariable<int>("Vtx_n", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned long long>("eventNumber"));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("runNumber"));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("pixelFlags", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("sctFlags", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("trtFlags", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("larFlags", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("tileFlags", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("muonFlags", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("forwardDetFlags", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("coreFlags", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("backgroundFlags", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("lumiFlags", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<float>("averageInteractionsPerCrossing", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<float>("actualInteractionsPerCrossing", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("bcid", m_RecoFlags));
        ATH_CHECK(NewCommonEventVariable<unsigned int>("lumiBlock", m_RecoFlags));

        ATH_CHECK(m_systematics.retrieve());

        if (m_systematics->isData() && m_ApplyGRL) ATH_CHECK(m_GrlTool.retrieve());
        if (!m_systematics->isData()) {
            switch (m_OutlierStrat) {
                case doNothing: ATH_MSG_INFO("No special treatment for outlier gen weights configured"); break;
                case ignoreEvent:
                    ATH_MSG_INFO("Events with outlier gen weights ( abs > " << m_outlierWeightThreshold << ") will be ignored");
                    break;
                case resetWeight:
                    ATH_MSG_INFO("Events with outlier gen weights ( abs > " << m_outlierWeightThreshold << ") will be set to +/-1 weight");
                    break;
                default: ATH_MSG_FATAL("Invalid outlier strategy has been set"); return StatusCode::FAILURE;
            }
        }
        if (ApplyPileUp()) {
            ATH_CHECK(m_prwTool.retrieve());
            ATH_CHECK(NewCommonEventVariable<float>("mu_density"));
            m_mu_density = GetVariableStorage<float>("mu_density");
            for (auto set : m_systematics->GetWeightSystematics(XAMPP::SelectionObject::EventWeight)) {
                if (!ToolIsAffectedBySystematic(m_prwTool.getHandle(), set)) continue;
                std::string syst_prefix = (set->name().empty() ? "" : "_" + set->name());
                ATH_CHECK(NewCommonEventVariable<double>("muWeight" + syst_prefix, !m_systematics->isData()));
                ATH_CHECK(NewCommonEventVariable<float>("corr_avgIntPerX" + syst_prefix));
                // By default the PRW tool does no longer vary the Random runNumber
                // Disable the saving of these branches for now.
                ATH_CHECK(NewCommonEventVariable<unsigned int>("RandomRunNumber" + syst_prefix, !m_systematics->isData()));
                ATH_CHECK(NewCommonEventVariable<unsigned int>("RandomLumiBlockNumber" + syst_prefix, !m_systematics->isData()));
                PileUpDecorators Decorator;
                Decorator.RandomRunNumber = GetVariableStorage<unsigned int>("RandomRunNumber" + syst_prefix);
                Decorator.RandomLumiBlock = GetVariableStorage<unsigned int>("RandomLumiBlockNumber" + syst_prefix);
                Decorator.muWeight = GetVariableStorage<double>("muWeight" + syst_prefix);
                Decorator.AverageCross = GetVariableStorage<float>("corr_avgIntPerX" + syst_prefix);
                m_DecPup.insert(std::pair<const CP::SystematicSet*, PileUpDecorators>(set, Decorator));
            }
            m_MultiPRWPeriods = m_prwTool->expert()->GetPeriodNumbers().size() > 2;
        }
        m_PassGRL = GetVariableStorage<char>("PassGRL");
        m_PassLArTile = GetVariableStorage<char>("passLArTile");
        m_HasVtx = GetVariableStorage<char>("HasVtx");
        m_nNVtx = GetVariableStorage<int>("Vtx_n");
        m_Init = true;
        return StatusCode::SUCCESS;
    }
    StatusCode EventInfo::LoadInfo() {
        m_ActSys = nullptr;
        m_EvtInfo = nullptr;
        m_primaryVtx = nullptr;
        ATH_CHECK(evtStore()->retrieve(m_ConstEvtInfo, "EventInfo"));
        // Cache the vertex and determine the cleaning
        ATH_CHECK(FindPrimaryVertex());
        ATH_CHECK(m_PassGRL->ConstStore((!m_ApplyGRL || isMC() || m_GrlTool->passRunLB(*m_ConstEvtInfo))));
        bool PassLAR = (isMC() || !((m_ConstEvtInfo->errorState(xAOD::EventInfo::LAr) == xAOD::EventInfo::Error) ||
                                    (m_ConstEvtInfo->errorState(xAOD::EventInfo::Tile) == xAOD::EventInfo::Error) ||
                                    (m_ConstEvtInfo->errorState(xAOD::EventInfo::SCT) == xAOD::EventInfo::Error) ||
                                    (m_ConstEvtInfo->eventFlags(xAOD::EventInfo::Core) & 0x40000)));  // core flag checks??
        if (!isMC() && dataYear() < 2017) {
            static CharAccessor acc_BatMan("DFCommonJets_isBadBatman");
            if (!acc_BatMan.isAvailable(*m_ConstEvtInfo))
                ATH_MSG_DEBUG("Batman is seeking for Robin?");
            else
                PassLAR &= !acc_BatMan(*m_ConstEvtInfo);
        }
        ATH_CHECK(m_PassLArTile->ConstStore(PassLAR));
        ATH_CHECK(RunPRWTool());
        return StatusCode::SUCCESS;
    }
    StatusCode EventInfo::SetSystematic(const CP::SystematicSet* set) {
        if (m_ActSys == set) return StatusCode::SUCCESS;
        m_ActSys = set;
        // Check if the current systematic affects the kinematics-> otherwise
        // return Nominal Info
        for (const auto s : m_systematics->GetKinematicSystematics())
            if (s == set) return GetInfoFromStore(set);
        ATH_MSG_WARNING("The systematic " << set->name() << " is not part of the kinematics. Load the nominal container");
        return GetInfoFromStore(m_systematics->GetNominal());
    }
    StatusCode EventInfo::GetInfoFromStore(const CP::SystematicSet* set) {
        if (set == nullptr) {
            ATH_MSG_ERROR("The systematic set pointer is nullptr");
            return StatusCode::FAILURE;
        }
        // The Info Object is already given in the Event Store
        std::string storeName = name() + "EvI" + set->name();
        if (evtStore()->contains<xAOD::EventInfo>(storeName)) {
            ATH_CHECK(evtStore()->retrieve(m_EvtInfo, storeName));
            return StatusCode::SUCCESS;
        }
        // Create a new object and copy the information from the original to the  new container
        std::pair<xAOD::EventInfo*, xAOD::ShallowAuxInfo*> copy = shallowCopyObject(*m_ConstEvtInfo);
        ATH_CHECK(evtStore()->record(copy.first, storeName));
        ATH_CHECK(evtStore()->record(copy.second, storeName + "Aux."));
        m_EvtInfo = copy.first;
        return StatusCode::SUCCESS;
    }
    StatusCode EventInfo::FindPrimaryVertex() {
        ATH_CHECK(m_HasVtx->ConstStore(!m_RecoFlags));
        ATH_CHECK(m_nNVtx->ConstStore(0));
        if (!m_RecoFlags) {
            ATH_MSG_DEBUG("No reconstruction flags flags required");
            return StatusCode::SUCCESS;
        }
        const xAOD::VertexContainer* primVertices = nullptr;
        ATH_CHECK(evtStore()->retrieve(primVertices, "PrimaryVertices"));
        if (primVertices->empty()) {
            ATH_MSG_DEBUG("Found no PV candidate for IP computation!");
            return StatusCode::SUCCESS;
        }
        unsigned int Vtx_n = 0;
        for (const auto& vx : *primVertices) {
            if (vx->vertexType() == xAOD::VxType::PriVtx) {
                if (m_primaryVtx) ATH_MSG_WARNING("Found already a primary vertex with z=" << m_primaryVtx->z());
                m_primaryVtx = vx;
            }
            if (vx->vertexType() == xAOD::VxType::PriVtx || vx->vertexType() == xAOD::VxType::PileUp) ++Vtx_n;
        }
        ATH_MSG_DEBUG("Primary vertex found with z=" << m_primaryVtx->z());
        ATH_CHECK(m_HasVtx->ConstStore(m_primaryVtx != nullptr));
        ATH_CHECK(m_nNVtx->ConstStore(Vtx_n));
        return StatusCode::SUCCESS;
    }
    StatusCode EventInfo::RunPRWTool() {
        static UIntAccessor acc_Random("RandomRunNumber");
        static UIntDecorator dec_Random("RandomRunNumber");

        static UIntAccessor acc_Lumi("RandomLumiBlockNumber");
        static FloatAccessor acc_PuW("PileupWeight");
        static FloatAccessor acc_CaX("corrected_averageInteractionsPerCrossing");

        if (!ApplyPileUp()) {
            // Overwrite the random runnumber with the current runNumber
            if (isMC()) dec_Random(*GetOrigInfo()) = runNumber();
            return StatusCode::SUCCESS;
        }
        for (auto set : m_systematics->GetWeightSystematics(XAMPP::SelectionObject::EventWeight)) {
            if (!ToolIsAffectedBySystematic(m_prwTool.getHandle(), set)) continue;
            std::map<const CP::SystematicSet*, PileUpDecorators>::iterator Itr = m_DecPup.find(set);
            if (Itr == m_DecPup.end()) {
                ATH_MSG_ERROR("Could not find prw decoration for systematic " << set->name());
                return StatusCode::FAILURE;
            }
            PileUpDecorators& Decorator = Itr->second;
            ATH_CHECK(m_systematics->setSystematic(set));
            ATH_CHECK(m_prwTool->apply(*m_ConstEvtInfo));
            if (acc_CaX.isAvailable(*m_ConstEvtInfo))
                ATH_CHECK(Decorator.AverageCross->ConstStore(acc_CaX(*m_ConstEvtInfo)));
            else
                ATH_CHECK(Decorator.AverageCross->ConstStore(m_ConstEvtInfo->averageInteractionsPerCrossing()));
            if (isMC()) {
                ATH_CHECK(
                    Decorator.muWeight->ConstStore(isnan(acc_PuW(*m_ConstEvtInfo)) ? 1. : acc_PuW(*m_ConstEvtInfo) / GetPeriodWeight()));
                ATH_CHECK(Decorator.RandomRunNumber->ConstStore(acc_Random(*m_ConstEvtInfo)));
                ATH_CHECK(Decorator.RandomLumiBlock->ConstStore(acc_Lumi(*m_ConstEvtInfo)));
            }
        }

        float pu_density = m_primaryVtx
                               ? m_prwTool->getCorrectedActualInteractionsPerCrossing(*m_ConstEvtInfo) *
                                     TMath::Gaus(m_primaryVtx->z(), m_ConstEvtInfo->beamPosZ(), m_ConstEvtInfo->beamPosSigmaZ(), true)
                               : FLT_MAX;
        ATH_CHECK(m_mu_density->ConstStore(pu_density));
        return StatusCode::SUCCESS;
    }
    StatusCode EventInfo::CopyInfoFromNominal(const CP::SystematicSet* To) {
        if (To == m_systematics->GetNominal()) {
            ATH_MSG_ERROR("Cannot copy the nominal EventInfo to itself");
            return StatusCode::SUCCESS;
        }
        ATH_CHECK(GetInfoFromStore(m_systematics->GetNominal()));
        xAOD::EventInfo* NominalInfo = m_EvtInfo;
        ATH_CHECK(GetInfoFromStore(To));
        *m_EvtInfo = *NominalInfo;
        return StatusCode::SUCCESS;
    }
    const CP::SystematicSet* EventInfo::GetNominal() const { return m_systematics->GetNominal(); }
    StatusCode EventInfo::BookParticleStorage(const std::string& Name, bool StoreMass, bool SaveVariations, bool saveTrees) {
        if (StorageKeeper::GetInstance()->ParticleDefined(Name, this)) {
            ATH_MSG_ERROR("Error there has already been booked a particle Storage " << Name);
            return StatusCode::FAILURE;
        }
        if (isLocked()) {
            ATH_MSG_ERROR("The storage " << Name << " cannot be booked. Since the info is initialized");
            return StatusCode::FAILURE;
        }
        ATH_MSG_INFO("Book new branches to store particles " << Name);
        ParticleStorage* S = new ParticleStorage(Name, this);
        S->SetSaveVariations(SaveVariations);
        S->SaveMassInP4(StoreMass);
        S->SetSaveTrees(saveTrees);
        return StatusCode::SUCCESS;
    }

    StatusCode EventInfo::BookContainerStorage(const std::string& Name, bool SaveVariations, bool saveToTree) {
        if (StorageKeeper::GetInstance()->ParticleDefined(Name, this)) {
            ATH_MSG_ERROR("Error there has already been booked a particle Storage " << Name);
            return StatusCode::FAILURE;
        }
        if (isLocked()) {
            ATH_MSG_ERROR("The storage " << Name << " cannot be booked. Since the info is initialized");
            return StatusCode::FAILURE;
        }
        ATH_MSG_INFO("Book new branches to store particles " << Name);
        DataVectorStorage* S = new DataVectorStorage(Name, this);
        S->SetSaveVariations(SaveVariations);
        S->SetSaveTrees(saveToTree);
        return StatusCode::SUCCESS;
    }
    StatusCode EventInfo::BookCommonContainerStorage(const std::string& Name, bool saveToTree) {
        if (StorageKeeper::GetInstance()->ParticleDefined(Name, this)) {
            ATH_MSG_ERROR("Error there has already been booked a particle Storage " << Name);
            return StatusCode::FAILURE;
        }
        if (isLocked()) {
            ATH_MSG_ERROR("The storage " << Name << " cannot be booked. Since the info is initialized");
            return StatusCode::FAILURE;
        }
        ATH_MSG_INFO("Book new common branches to store particles " << Name);
        DataVectorStorage* S = new DataVectorStorage(Name, this, true);
        S->SetSaveTrees(saveToTree);
        return StatusCode::SUCCESS;
    }

    StatusCode EventInfo::BookCommonParticleStorage(const std::string& Name, bool StoreMass, bool saveToTree) {
        if (StorageKeeper::GetInstance()->ParticleDefined(Name, this)) {
            ATH_MSG_ERROR("Error there has already been booked a particle Storage " << Name);
            return StatusCode::FAILURE;
        }
        if (isLocked()) {
            ATH_MSG_ERROR("The storage " << Name << " cannot be booked. Since the info is initialized");
            return StatusCode::FAILURE;
        }
        ATH_MSG_INFO("Book new common branches to store particles " << Name);
        ParticleStorage* S = new ParticleStorage(Name, this, true);
        S->SaveMassInP4(StoreMass);
        S->SetSaveTrees(saveToTree);
        return StatusCode::SUCCESS;
    }

    DataVectorStorage* EventInfo::GetContainerStorage(const std::string& Name) const {
        if (!StorageKeeper::GetInstance()->ParticleDefined(Name, this)) {
            ATH_MSG_WARNING("The Container " << Name << " does not exist");
            return nullptr;
        }
        return StorageKeeper::GetInstance()->RetrieveContainerStorage(Name, this);
    }
    ParticleStorage* EventInfo::GetParticleStorage(const std::string& Name) const {
        ParticleStorage* Store = dynamic_cast<ParticleStorage*>(GetContainerStorage(Name));
        if (Store == nullptr) ATH_MSG_WARNING("The particle storage " << Name << " has not been found");
        return Store;
    }
    bool EventInfo::returnStorage(IStorage* store, unsigned int bit_mask) const {
        if (!store) {
            ATH_MSG_ERROR("No storage was given");
            return false;
        }
        if ((bit_mask & OutputElement::Tree) && (bit_mask & OutputElement::Histo)) {
            ATH_MSG_ERROR("For what do you need " << store->name() << "? Trees or histograms?");
            return false;
        }
        if ((bit_mask & OutputElement::Tree) && !store->SaveTrees()) return false;
        if ((bit_mask & OutputElement::Histo) && !store->SaveHistos()) return false;
        return true;
    }
    std::vector<DataVectorStorage*> EventInfo::GetContainerStorages(unsigned int e) const {
        std::vector<DataVectorStorage*> out;
        std::vector<DataVectorStorage*> in = StorageKeeper::GetInstance()->RetrieveContainerStores(this);
        for (auto& entry : in) {
            if (returnStorage(entry, e)) out.push_back(entry);
        }
        return out;
    }
    bool EventInfo::isMC() const {
        if (!GetOrigInfo()) {
            ATH_MSG_WARNING("Currently no event is loaded");
            return false;
        }
        return GetOrigInfo()->eventType(xAOD::EventInfo::IS_SIMULATION);
    }
    const xAOD::EventInfo* EventInfo::GetOrigInfo() const { return m_ConstEvtInfo; }
    const CP::SystematicSet* EventInfo::GetSystematic() const { return m_ActSys; }
    EventInfo::~EventInfo() {
        delete StorageKeeper::GetInstance();
        ATH_MSG_DEBUG("Destructor called");
    }
    unsigned long long EventInfo::eventNumber() const { return GetOrigInfo()->eventNumber(); }
    int EventInfo::mcChannelNumber() const {
        if (!isMC()) {
            ATH_MSG_WARNING("Invalid access of the mcChannelNumber. Return -1");
            return -1;
        }
        return (m_ConstEvtInfo->mcChannelNumber() ? m_ConstEvtInfo->mcChannelNumber() : runNumber()) + (m_shiftDSISD ? dsidOffSet() : 0);
    }
    double EventInfo::GetGenWeight(unsigned int idx) const {
        double w = GetRawGenWeight(idx);
        if (isOutlierGenWeight(w)) {
            if (m_OutlierStrat == ignoreEvent) {
                return 0;
            } else if (m_OutlierStrat == resetWeight) {
                return w / fabs(w);
            }
        }
        return w;
    }
    double EventInfo::GetRawGenWeight(unsigned int idx) const {
        if (!m_systematics->isData() && m_ConstEvtInfo && m_ConstEvtInfo->mcEventWeights().size() > idx) {
            return *(m_ConstEvtInfo->mcEventWeights().begin() + idx);
        }
        ATH_MSG_WARNING("The current event has no mcWeight saved. Return 1.");
        return 1.;
    }

    bool EventInfo::isOutlierGenWeight(unsigned int idx) const { return isOutlierGenWeight(GetRawGenWeight(idx)); }

    bool EventInfo::isOutlierGenWeight(double w) const { return (m_OutlierStrat != doNothing && fabs(w) > m_outlierWeightThreshold); }

    unsigned int EventInfo::runNumber() const { return m_ConstEvtInfo->runNumber(); }
    unsigned int EventInfo::randomRunNumber() const {
        static UIntAccessor acc_Random("RandomRunNumber");
        if (!GetOrigInfo()) {
            ATH_MSG_WARNING("No event info object?!?!");
            return -1;
        } else if (!isMC() || !acc_Random.isAvailable(*GetOrigInfo()))
            return runNumber();
        else
            return acc_Random(*GetOrigInfo());
    }
    xAOD::EventInfo* EventInfo::GetEventInfo() const { return m_EvtInfo; }
    bool EventInfo::PassCleaning() const {
        if (!m_Filter) {
            ATH_MSG_DEBUG("Filter disabled");
            return true;
        }
        if (!m_PassGRL->GetValue()) {
            ATH_MSG_DEBUG("Failed GRL application");
            return false;
        }
        if (!m_PassLArTile->GetValue()) {
            ATH_MSG_DEBUG("Tile is not working");
            return false;
        }
        if (!m_HasVtx->GetValue()) {
            ATH_MSG_DEBUG("Has no vertex");
            return false;
        }
        return true;
    }
    const xAOD::Vertex* EventInfo::GetPrimaryVertex() const {
        if (!m_primaryVtx) { ATH_MSG_WARNING("No vertex could be found"); }
        return m_primaryVtx;
    }
    bool EventInfo::isLocked() const { return m_Locked; }
    void EventInfo::LockKeeper() { StorageKeeper::GetInstance()->Lock(); }
    unsigned int EventInfo::dataYear() const {
        // Run number range. Copied from SUSYTools for the moment.
        // https://gitlab.cern.ch/atlas/athena/blob/21.2/PhysicsAnalysis/SUSYPhys/SUSYTools/Root/SUSYObjDef_xAOD.cxx#L2311
        static const unsigned int runs_data_15 = 290000;
        static const unsigned int runs_data_16 = 320000;
        static const unsigned int runs_data_17 = 350000;
        if (isMC() && (!ApplyPileUp() || randomRunNumber() == 0)) {
            ATH_MSG_DEBUG("No pile-up reweighting has been applied");
            return -1;
        } else if (randomRunNumber() <= runs_data_15)
            return 2015;
        else if (randomRunNumber() <= runs_data_16)
            return 2016;
        else if (randomRunNumber() <= runs_data_17)
            return 2017;
        return 2018;
    }
    double EventInfo::GetPileUpLuminosity() {
        if (ApplyPileUp()) return m_prwTool->expert()->GetIntegratedLumi(runNumber(), 0, -1);
        return 1;
    }
    double EventInfo::GetPeriodWeight() {
        if (m_MultiPRWPeriods && ApplyPileUp()) return m_prwTool->expert()->GetPeriodWeight(runNumber(), mcChannelNumber());
        return 1;
    }
    bool EventInfo::ApplyPileUp() const { return m_ApplyPRW; }
    bool EventInfo::applyDSIDShift() const { return m_shiftDSISD; }

    int EventInfo::dsidOffSet() const {
        if (m_shiftDSISD && m_dsidOffSet != 0) {
            ATH_MSG_WARNING("The dsid will be shifted by "
                            << m_dsidOffSet
                            << ". This option is really not recommended, since it might clash with the cross-section & meta-data and use "
                               "it only if you're knowing what you're doing!!!");
        }
        return m_dsidOffSet;
    }

    StatusCode EventInfo::createSystematicGroup(const std::string& name, XAMPP::SelectionObject obj_type) {
        if (name.empty()) {
            ATH_MSG_FATAL("Empty names are not allowed for sytstematic groups");
            return StatusCode::FAILURE;
        } else if (obj_type == SelectionObject::Other) {
            ATH_MSG_FATAL("Please specify an object type");
            return StatusCode::FAILURE;
        } else if (obj_type == SelectionObject::EventWeight || obj_type == SelectionObject::BTag) {
            ATH_MSG_FATAL("Systematic groups do not make sense for event weights");
            return StatusCode::FAILURE;
        }
        if (isLocked()) {
            ATH_MSG_FATAL("No storages can be created anymore because the tool is locked");
            return StatusCode::FAILURE;
        }
        std::vector<std::shared_ptr<SystematicGroup>>::const_iterator itr = std::find_if(
            m_systGroups.begin(), m_systGroups.end(), [name](const std::shared_ptr<SystematicGroup>& a) { return name == a->name(); });
        if (itr != m_systGroups.end()) {
            ATH_MSG_FATAL("Systematic group " << name << " has already been defined before");
            return StatusCode::FAILURE;
        }
        m_systGroups.push_back(std::make_shared<SystematicGroup>(name, obj_type, m_systematics));
        ATH_MSG_INFO("Created successfully systematic group " << name << ".");
        return StatusCode::SUCCESS;
    }
    std::shared_ptr<SystematicGroup> EventInfo::getSystematicGroup(const std::string& name) const {
        std::vector<std::shared_ptr<SystematicGroup>>::const_iterator itr = std::find_if(
            m_systGroups.begin(), m_systGroups.end(), [name](const std::shared_ptr<SystematicGroup>& a) { return name == a->name(); });
        if (itr == m_systGroups.end()) {
            ATH_MSG_ERROR("Systematic group " << name << " has not been defined thus far.");
            return std::shared_ptr<SystematicGroup>();
        }
        return (*itr);
    }
    const std::vector<std::shared_ptr<SystematicGroup>>& EventInfo::getSystematicGroups() const { return m_systGroups; }
}  // namespace XAMPP
