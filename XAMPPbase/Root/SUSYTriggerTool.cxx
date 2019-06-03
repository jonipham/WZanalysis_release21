#include <SUSYTools/ISUSYObjDef_xAODTool.h>
#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/SUSYTriggerTool.h>
#include <XAMPPbase/TreeHelpers.h>
#include <xAODTrigMissingET/TrigMissingETContainer.h>
#include <xAODTrigger/EnergySumRoI.h>
#include "TriggerMatchingTool/IMatchingTool.h"
// Required to use some functions (see header explanation)
#include "TrigDecisionTool/TrigDecisionTool.h"

namespace XAMPP {
    //############################################################################################
    //                                      TriggerInterface
    //############################################################################################
    bool TriggerInterface::m_SavePrescaling = false;
    bool TriggerInterface::m_saveFullTrigInfo = true;
    bool TriggerInterface::m_SaveObjMatching = false;
    const std::vector<std::string> m_IdExp{"noL1", "tc_ecm", "tc_em", "loose", "tracktwo", "etcut", "nod0", "ivar", "medium", "loose"};
    TriggerInterface::TriggerInterface(const std::string& Name, SUSYTriggerTool* TriggerTool) :
        m_name(Name),
        m_TriggerTool(TriggerTool),
        m_XAMPPInfo(nullptr),
        m_trigDecTool(""),
        m_trigMatchTool(""),
        m_TriggerStore(nullptr),
        m_MatchingStore(nullptr),
        m_PreScalingStore(nullptr),
        m_MatchingDecorator("TrigMatch" + Name),
        m_MatchingAccessor("TrigMatch" + Name),
        m_dec_isMatchingDone("TrigMatchIsDone" + Name),
        m_acc_isMatchingDone("TrigMatchIsDone" + Name),
        m_MatchEle(false),
        m_MatchMuo(false),
        m_MatchTau(false),
        m_MatchPho(false),
        m_Thresholds(),
        m_periods(),
        m_has_periods(false) {}
    int TriggerInterface::num_toMatch() const { return m_Thresholds.size(); }
    int TriggerInterface::num_toMatch(const XAMPP::SelectionObject obj) const {
        int i = 0;
        for (const auto& Thrs : m_Thresholds) {
            if (Thrs.Object == obj) ++i;
        }
        return i;
    }
    void TriggerInterface::SaveTriggerPrescaling(bool B) { m_SavePrescaling = B; }
    void TriggerInterface::SaveFullTriggerInfo(bool B) { m_saveFullTrigInfo = B; }
    void TriggerInterface::SaveObjectMatching(bool B) { m_SaveObjMatching = B; }
    bool TriggerInterface::isMatchingDone(const xAOD::IParticle& P) const {
        return m_acc_isMatchingDone.isAvailable(P) && m_acc_isMatchingDone(P);
    }
    bool TriggerInterface::isMatchingDone(const xAOD::IParticle* P) const { return isMatchingDone(*P); }
    bool TriggerInterface::isMatched_dR(const xAOD::IParticle* P) const { return isMatched_dR(*P); }
    bool TriggerInterface::isMatched_dR(const xAOD::IParticle& P) const { return isMatchingDone(P) && m_MatchingAccessor(P); }
    bool TriggerInterface::isMatched(const xAOD::IParticle* P) const { return isMatched(*P); }
    bool TriggerInterface::isMatched(const xAOD::IParticle& P) const {
        if (!isMatchingDone(P)) {
            Warning("TriggerInteface()", "No matching has been called for trigger %s", name().c_str());
            PromptParticle(P);
            return false;
        }
        if (!m_MatchingAccessor(P)) return false;
        for (const auto& Thrs : m_Thresholds) {
            if (Thrs.Object != P.type()) { continue; }
            if (Thrs.PtThreshold < P.pt()) { return true; }
        }
        return false;
    }
    std::string TriggerInterface::name() const { return m_name; }
    std::string TriggerInterface::MatchStoreName() const {
        if (!m_MatchingStore) {
            Warning("TriggerInterface()", "Trigger has no matching requirement");
            return "Trigger";
        }
        return m_MatchingStore->name();
    }
    std::string TriggerInterface::StoreName() const { return m_TriggerStore->name(); }

    StatusCode TriggerInterface::initialize(XAMPP::EventInfo* Info) {
        // Retrieve the needed tools from the TriggerTool instance via property Manager
        // Some strange noise is out here
        m_trigDecTool = ToolHandle<Trig::TrigDecisionTool>(m_TriggerTool->getProperty("TrigDecisionTool").toString());
        m_trigMatchTool = ToolHandle<Trig::IMatchingTool>(m_TriggerTool->getProperty("TrigMatchTool").toString());

        CHECK(m_trigDecTool.retrieve());
        CHECK(m_trigMatchTool.retrieve());

        m_XAMPPInfo = Info;

        // Check whether you can add The decision variable to the output
        if (!m_XAMPPInfo->NewCommonEventVariable<char>("Trig" + name()).isSuccess()) { return StatusCode::FAILURE; }
        m_TriggerStore = m_XAMPPInfo->GetVariableStorage<char>("Trig" + name());

        // Trigger prescaling for data
        if (m_SavePrescaling && !m_XAMPPInfo->NewCommonEventVariable<float>("PreScale" + name()).isSuccess()) {
            return StatusCode::FAILURE;
        }
        if (m_SavePrescaling) { m_PreScalingStore = m_XAMPPInfo->GetVariableStorage<float>("PreScale" + name()); }
        GetMatchingThresholds();
        m_MatchEle = AssignMatching(xAOD::Type::ObjectType::Electron);
        m_MatchMuo = AssignMatching(xAOD::Type::ObjectType::Muon);
        m_MatchPho = AssignMatching(xAOD::Type::ObjectType::Photon);
        m_MatchTau = AssignMatching(xAOD::Type::ObjectType::Tau);
        // No matching requirement need to be done -> MET trigger
        if (!NeedsTriggerMatching()) { return StatusCode::SUCCESS; }
        // Add the global trigger matching decision to the output
        if (!m_XAMPPInfo->NewEventVariable<char>("TrigMatch" + name()).isSuccess()) { return StatusCode::FAILURE; }
        m_MatchingStore = m_XAMPPInfo->GetVariableStorage<char>("TrigMatch" + name());
        return StatusCode::SUCCESS;
    }

    void TriggerInterface::NewEvent() {
        bool PassTrigger = isInPeriod() && m_trigDecTool->isPassed(name());
        if (!m_TriggerStore->ConstStore(PassTrigger).isSuccess()) { return; }
        if (!m_SavePrescaling) return;
        if (!m_PreScalingStore->ConstStore(m_trigDecTool->getPrescale(name())).isSuccess()) { return; }
    }
    bool TriggerInterface::isInPeriod() const {
        if (!m_has_periods) return true;
        unsigned int run = m_XAMPPInfo->randomRunNumber();
        std::vector<run_range>::const_iterator itr =
            std::find_if(m_periods.begin(), m_periods.end(), [run](const run_range& r) { return r.first <= run && r.second >= run; });
        return itr != m_periods.end();
    }
    bool TriggerInterface::PassTrigger() const { return m_TriggerStore->GetValue(); }
    bool TriggerInterface::AssignMatching(xAOD::Type::ObjectType T) const {
        for (auto& thrs : m_Thresholds) {
            if (thrs.Object == T) return true;
        }
        return false;
    }
    void TriggerInterface::GetMatchingThresholds() {
        std::string TriggerString = name();
        xAOD::Type::ObjectType Type = xAOD::Type::ObjectType::Other;
        // Removing expressions which are not separated by _ from the object
        // definition
        for (auto& Exp : m_IdExp) {
            size_t Pos = TriggerString.find(Exp);
            if (Pos == std::string::npos) continue;
            TriggerString = TriggerString.substr(0, TriggerString.substr(Pos, Pos + 1) != "_" ? Pos : Pos - 1) +
                            TriggerString.substr(Pos + Exp.size(), std::string::npos);
        }
        int Multiple = 0;
        int Thrs = ExtractPtThreshold(TriggerString, Multiple, Type);
        while (Thrs != -1) {
            OfflineMatching M;
            M.PtThreshold = Thrs;
            M.Object = Type;
            m_Thresholds.push_back(M);
            if (Multiple == 0)
                Thrs = ExtractPtThreshold(TriggerString, Multiple, Type);
            else
                --Multiple;
        }
        static unsigned int extraThreshold_Egamma = 1000;
        static unsigned int extraThreshold_DiMuon = 1000;
        static unsigned int extraThreshold_Tau = 5000;

        for (auto& M : m_Thresholds) {
            if (M.Object == xAOD::Type::ObjectType::Muon && m_Thresholds.size() == 1) {
                M.PtThreshold *= 1.05;
            } else if (M.Object == xAOD::Type::ObjectType::Electron || M.Object == xAOD::Type::ObjectType::Photon) {
                M.PtThreshold += extraThreshold_Egamma;
            } else if (M.Object == xAOD::Type::ObjectType::Muon) {
                M.PtThreshold += extraThreshold_DiMuon;
            } else if (M.Object == xAOD::Type::ObjectType::Tau) {
                M.PtThreshold += extraThreshold_Tau;
            }
        }
        std::sort(m_Thresholds.begin(), m_Thresholds.end(),
                  [](const OfflineMatching& a, const OfflineMatching& b) { return a.PtThreshold > b.PtThreshold; });
    }
    TriggerInterface::FinalStrObjMatching TriggerInterface::FindObjectInTriggerString(const std::string& TriggerString,
                                                                                      const ObjMatchVec& Matching) const {
        FinalStrObjMatching Match;
        ObjMatchVec::const_iterator End_Itr = Matching.end();
        for (unsigned int i = 1; i <= 3; ++i) {
            for (ObjMatchVec::const_iterator Itr = Matching.begin(); Itr != End_Itr; ++Itr) {
                std::string SearchString = "_" + (i == 1 ? "" : std::to_string(i)) + Itr->first;
                size_t Pos = TriggerString.find(SearchString);
                if (Pos < Match.StringPosition) {
                    Match.StringPosition = Pos + SearchString.size();
                    Match.Obj = Itr->second;
                    Match.Multiplicity = i;
                }
            }
        }
        return Match;
    }

    int TriggerInterface::ExtractPtThreshold(std::string& TriggerString, int& M, xAOD::Type::ObjectType& T) {
        M = 0;
        TriggerInterface::ObjMatchVec MatchingObjects{
            StringObjectMatching("e", xAOD::Type::ObjectType::Electron), StringObjectMatching("mu", xAOD::Type::ObjectType::Muon),
            StringObjectMatching("g", xAOD::Type::ObjectType::Photon), StringObjectMatching("tau", xAOD::Type::ObjectType::Tau)};
        FinalStrObjMatching Matching = FindObjectInTriggerString(TriggerString, MatchingObjects);
        T = Matching.Obj;
        if (Matching.StringPosition != std::string::npos) {
            TriggerString = TriggerString.substr(Matching.StringPosition, TriggerString.size());
            M = Matching.Multiplicity - 1;
            float Thrs = atof(TriggerString.substr(0, TriggerString.find("_")).c_str()) * 1000;
            if (TriggerString.find("_") < TriggerString.size())
                TriggerString = TriggerString.substr(TriggerString.find("_"), TriggerString.size());
            else
                TriggerString = "";
            return Thrs;
        }
        return -1;
    }
    bool TriggerInterface::PassTriggerMatching() {
        if (!NeedsTriggerMatching()) { return false; }
        // The matching store is already filled
        // Return whats in there
        if (m_MatchingStore->isAvailable()) { return m_MatchingStore->GetValue(); }

        // The matching store needs to be filled
        if (!m_MatchingStore->Store(false)) { return false; }
        if (!PassTrigger()) {
            if (!m_SaveObjMatching) return false;
            if (NeedsElectronMatching() && !m_TriggerTool->CalibElectrons()->empty())
                m_MatchingDecorator(*m_TriggerTool->CalibElectrons()->at(0)) = 0;
            if (NeedsMuonMatching() && !m_TriggerTool->CalibMuons()->empty()) m_MatchingDecorator(*m_TriggerTool->CalibMuons()->at(0)) = 0;
            if (NeedsTauMatching() && !m_TriggerTool->CalibTaus()->empty()) m_MatchingDecorator(*m_TriggerTool->CalibTaus()->at(0)) = 0;
            if (NeedsPhotonMatching() && !m_TriggerTool->CalibPhotons()->empty())
                m_MatchingDecorator(*m_TriggerTool->CalibPhotons()->at(0)) = 0;

            return false;
        }
        unsigned int NumMatches = 0;
        for (auto& Thrs : m_Thresholds) { Thrs.ObjectMatched = false; }
        if (NeedsElectronMatching()) { NumMatches += MatchObjectsToTrigger(m_TriggerTool->CalibElectrons()); }
        if (NeedsMuonMatching()) { NumMatches += MatchObjectsToTrigger(m_TriggerTool->CalibMuons()); }
        if (NeedsTauMatching()) { NumMatches += MatchObjectsToTrigger(m_TriggerTool->CalibTaus()); }
        if (NeedsPhotonMatching()) { NumMatches += MatchObjectsToTrigger(m_TriggerTool->CalibPhotons()); }
        if (!m_MatchingStore->Store(NumMatches >= m_Thresholds.size())) { return false; }
        return m_MatchingStore->GetValue();
    }
    unsigned int TriggerInterface::MatchObjectsToTrigger(const xAOD::IParticleContainer* calibrated_obj) {
        unsigned int n_matched = 0;
        for (const auto& obj : *calibrated_obj) {
            const xAOD::IParticle* orig_obj = xAOD::getOriginalObject(*obj);
            /// dR matching has already been performed no worries
            if (!isMatchingDone(orig_obj)) {
                m_dec_isMatchingDone(*orig_obj) = true;
                m_MatchingDecorator(*orig_obj) = PassTrigger() && m_trigMatchTool->match({obj}, name());
            }
            if (!isMatched_dR(orig_obj)) continue;
            std::vector<OfflineMatching>::iterator itr = std::find_if(
                m_Thresholds.begin(), m_Thresholds.end(),
                [obj](const OfflineMatching& m) { return !m.ObjectMatched && m.Object == obj->type() && m.PtThreshold < obj->pt(); });
            if (itr == m_Thresholds.end()) continue;
            (*itr).ObjectMatched = true;
            ++n_matched;
        }
        return n_matched;
    }

    bool TriggerInterface::NeedsElectronMatching() const { return m_MatchEle; }
    bool TriggerInterface::NeedsMuonMatching() const { return m_MatchMuo; }
    bool TriggerInterface::NeedsTauMatching() const { return m_MatchTau; }
    bool TriggerInterface::NeedsPhotonMatching() const { return m_MatchPho; }
    bool TriggerInterface::NeedsTriggerMatching() const {
        return NeedsElectronMatching() || NeedsMuonMatching() || NeedsTauMatching() || NeedsPhotonMatching();
    }
    std::string TriggerInterface::PrintMatchingThresholds() const {
        std::string Thrs = " ";
        for (const auto& Trig : m_Thresholds) {
            Thrs += "pt(";
            if (Trig.Object == xAOD::Type::ObjectType::Electron)
                Thrs += "El";
            else if (Trig.Object == xAOD::Type::ObjectType::Muon)
                Thrs += "Mu";
            else if (Trig.Object == xAOD::Type::ObjectType::Photon)
                Thrs += "Pho";
            else if (Trig.Object == xAOD::Type::ObjectType::Tau)
                Thrs += "Tau";
            Thrs += Form(")>%.2f GeV, ", (Trig.PtThreshold / 1000.));
        }
        return Thrs;
    }

    StatusCode TriggerInterface::addTriggerPeriod(unsigned int begin, unsigned int end) {
        if (begin > end) {
            Error("TriggerInteface::addTriggerPeriod()", "How can the start of a period %u come after its end %u", begin, end);
            return StatusCode::FAILURE;
        }
        std::vector<run_range>::const_iterator itr = std::find_if(m_periods.begin(), m_periods.end(), [begin, end](const run_range& r) {
            return (r.first <= begin && r.second >= begin) || (r.first <= end && r.second >= end);
        });
        if (itr != m_periods.end()) {
            Error("TriggerInterface:::addTriggerPeriod()", "%s has detected an overlapping period for run-range %u - %u", name().c_str(),
                  begin, end);
            return StatusCode::FAILURE;
        }
        Info("TriggerInterface:::addTriggerPeriod()", "%s is going to be constrained to runs of data-taking %u - %u", name().c_str(), begin,
             end);

        m_periods.push_back(run_range(begin, end));
        std::sort(m_periods.begin(), m_periods.end());
        m_has_periods = true;
        return StatusCode::SUCCESS;
    }

    //############################################################################################
    //                                      FiredObjectTrigger
    //############################################################################################
    FiredObjectTrigger::FiredObjectTrigger(const std::vector<std::shared_ptr<TriggerInterface>>& trigger_list, unsigned int assoc,
                                           int n_obj) :
        m_assoc(assoc),
        m_num_obj(n_obj),
        m_assoc_triggers(),
        m_dec_is_fired(nullptr),
        m_dec_is_matched(nullptr) {
        for (auto& trig : trigger_list) {
            if (((assoc & Association::Electron) != 0) != trig->NeedsElectronMatching()) continue;
            if (((assoc & Association::Muon) != 0) != trig->NeedsMuonMatching()) continue;
            if (((assoc & Association::Tau) != 0) != trig->NeedsTauMatching()) continue;
            if (((assoc & Association::Photon) != 0) != trig->NeedsPhotonMatching()) continue;

            if (n_obj <= 0 || trig->num_toMatch() == n_obj) { m_assoc_triggers.push_back(trig); }
        }
    }

    bool FiredObjectTrigger::has_triggers() const { return !m_assoc_triggers.empty(); }

    StatusCode FiredObjectTrigger::checkTrigger() {
        bool is_fired = false;
        for (const auto& assoc_trig : m_assoc_triggers) {
            is_fired = assoc_trig->PassTrigger();
            if (is_fired) break;
        }
        return m_dec_is_fired->ConstStore(is_fired);
    }
    StatusCode FiredObjectTrigger::checkMatching() {
        if (m_assoc == 0) return StatusCode::SUCCESS;
        bool matched = false;
        for (const auto& assoc_trig : m_assoc_triggers) {
            matched = assoc_trig->PassTriggerMatching();
            if (matched) break;
        }
        return m_dec_is_matched->Store(matched);
    }
    StatusCode FiredObjectTrigger::make_stores(XAMPP::EventInfo* info) {
        if (!has_triggers()) {
            Error("FiredObjectTrigger()", "No triggers were found");
            return StatusCode::FAILURE;
        }
        std::string store_name;
        if (m_assoc == 0)
            store_name = "MET";
        else {
            if (m_num_obj == 1)
                store_name += "Single";
            else if (m_num_obj == 2)
                store_name += "Di";
            else if (m_num_obj > 0)
                store_name += "Multi";
        }
        if (m_assoc & Association::Electron) store_name += "Elec";
        if (m_assoc & Association::Muon) store_name += "Muon";
        if (m_assoc & Association::Tau) store_name += "Tau";
        if (m_assoc & Association::Photon) store_name += "Photon";
        store_name += "Trig";
        if (!info->NewCommonEventVariable<char>("Is" + store_name + "Passed").isSuccess()) return StatusCode::FAILURE;

        if (m_assoc != 0) {
            if (!info->NewEventVariable<char>("Is" + store_name + "Matched").isSuccess()) return StatusCode::FAILURE;
            m_dec_is_matched = info->GetVariableStorage<char>("Is" + store_name + "Matched");
        }
        m_dec_is_fired = info->GetVariableStorage<char>("Is" + store_name + "Passed");
        return StatusCode::SUCCESS;
    }
    //############################################################################################
    //                                      SUSYTriggerTool
    //############################################################################################
    SUSYTriggerTool::~SUSYTriggerTool() {}

    SUSYTriggerTool::SUSYTriggerTool(const std::string& myname) :
        AsgTool(myname),
        m_XAMPPInfo(),
        m_XAMPPInfoHandle("EventInfoHandler"),
        m_systematics("SystematicsTool"),
        m_susytools("SUSYTools"),
        m_trigDecTool(""),
        m_trigMatchTool(""),
        m_elec_selection("SUSYElectronSelector"),
        m_muon_selection("SUSYMuonSelector"),
        m_phot_selection("SUSYPhotonSelector"),
        m_tau_selection("SUSYTauSelector"),
        m_triggers(),
        m_obj_trig(),
        m_trigger_names(),
        m_init(false),
        m_Pass(false),
        m_NoCut(false),
        m_EmptyTriggerList(false),
        m_StoreObjectMatching(false),
        m_StorePreScaling(false),
        m_MetTrigEmulation(false),
        m_doLVL1Met(true),
        m_doCellMet(true),
        m_doMhtMet(true),
        m_doTopoClMet(true),
        m_doTopoClPufitMet(true),
        m_doTopoClPuetaMet(true),
        m_doMetTriggerPassed(false),
        m_doSingleElectronTriggerPassed(false),
        m_doSingleMuonTriggerPassed(false),
        m_doSinglePhotonTriggerPassed(false),
        m_DecTrigger(nullptr),
        m_DecMatching(nullptr),
        m_useSignalElec(true),
        m_useSignalMuon(true),
        m_useSignalTau(true),
        m_useSignalPhot(true)

    {
        declareProperty("Triggers", m_trigger_names);
        declareProperty("DisableSkimming", m_NoCut);
        declareProperty("StoreMatchingInfo", m_StoreObjectMatching);
        declareProperty("WritePrescaling", m_StorePreScaling);
        declareProperty("MetTrigEmulation", m_MetTrigEmulation);
        declareProperty("LowestUnprescaledMetTrigger", m_doMetTriggerPassed);
        declareProperty("WriteSingleElecObjTrigger", m_doSingleElectronTriggerPassed);
        declareProperty("WriteSingleMuonObjTrigger", m_doSingleMuonTriggerPassed);
        declareProperty("WriteSinglePhotonObjTrigger", m_doSinglePhotonTriggerPassed);
        declareProperty("ElectronSelector", m_elec_selection);
        declareProperty("MuonSelector", m_muon_selection);
        declareProperty("PhotonSelector", m_phot_selection);
        declareProperty("TauSelector", m_tau_selection);
        declareProperty("SystematicsTool", m_systematics);
        declareProperty("TrigDecisionTool", m_trigDecTool);
        declareProperty("TrigMatchTool", m_trigMatchTool);

        declareProperty("SignalElecForMatching", m_useSignalElec);
        declareProperty("SignalMuonForMatching", m_useSignalMuon);
        declareProperty("SignalTauForMatching", m_useSignalTau);
        declareProperty("SignalPhotForMatching", m_useSignalPhot);

        m_susytools.declarePropertyFor(this, "SUSYTools", "The SUSYTools instance");
        m_XAMPPInfoHandle.declarePropertyFor(this, "EventInfoHandler", "The XAMPP EventInfo handle");
    }
    xAOD::ElectronContainer* SUSYTriggerTool::CalibElectrons() const {
        return m_useSignalElec ? m_elec_selection->GetSignalElectrons() : m_elec_selection->GetBaselineElectrons();
    }
    xAOD::MuonContainer* SUSYTriggerTool::CalibMuons() const {
        return m_useSignalMuon ? m_muon_selection->GetSignalMuons() : m_muon_selection->GetBaselineMuons();
    }
    xAOD::PhotonContainer* SUSYTriggerTool::CalibPhotons() const {
        return m_useSignalPhot ? m_phot_selection->GetSignalPhotons() : m_phot_selection->GetBaselinePhotons();
    }
    xAOD::TauJetContainer* SUSYTriggerTool::CalibTaus() const {
        return m_useSignalTau ? m_tau_selection->GetSignalTaus() : m_tau_selection->GetBaselineTaus();
    }

    StatusCode SUSYTriggerTool::initialize() {
        if (m_init) { return StatusCode::SUCCESS; }
        m_init = true;
        ATH_MSG_INFO("Initialising...");
        if (m_NoCut) ATH_MSG_INFO("Disable trigger skimming");
        if (m_trigDecTool.empty() || m_trigMatchTool.empty()) {
            ATH_MSG_INFO("Trigger decision and matching will be taken from SUSYTools");
            ATH_CHECK(m_susytools.retrieve());
            m_trigDecTool = GetCPTool<Trig::TrigDecisionTool>("TrigDecisionTool");
            m_trigMatchTool = GetCPTool<Trig::IMatchingTool>("TrigMatchTool");
        } else {
            ATH_MSG_INFO("Trigger decision and matching tools have been provided by the user");
        }
        ATH_CHECK(m_trigDecTool.retrieve());
        ATH_CHECK(m_trigMatchTool.retrieve());

        ATH_CHECK(m_XAMPPInfoHandle.retrieve());
        m_XAMPPInfo = dynamic_cast<XAMPP::EventInfo*>(m_XAMPPInfoHandle.operator->());

        if (!isData()) { m_StorePreScaling = false; }
        ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<char>("Trigger", m_NoCut));
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<char>("TrigMatching"));

        m_DecTrigger = m_XAMPPInfo->GetVariableStorage<char>("Trigger");
        m_DecMatching = m_XAMPPInfo->GetVariableStorage<char>("TrigMatching");

        TriggerInterface::SaveTriggerPrescaling(m_StorePreScaling);
        TriggerInterface::SaveObjectMatching(m_StoreObjectMatching);

        /// Fill the triggers from the names
        ATH_CHECK(FillTriggerVector(m_trigger_names));

        if (isData()) { m_MetTrigEmulation = false; }
        if (m_MetTrigEmulation) {
            ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<float>("TriggerLVL1Met"));
            ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<float>("TriggerCellMet"));
            ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<float>("TriggerMhtMet"));
            ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<float>("TriggerTopoClMet"));
            ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<float>("TriggerTopoClPufitMet"));
            ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<float>("TriggerTopoClPuetaMet"));
        }
        if (m_doMetTriggerPassed) {
            std::shared_ptr<FiredObjectTrigger> met_trig = std::make_shared<FiredObjectTrigger>(m_triggers, 0);
            if (met_trig->has_triggers()) m_obj_trig.push_back(met_trig);
        }
        if (m_doSingleElectronTriggerPassed) {
            std::shared_ptr<FiredObjectTrigger> single_ele =
                std::make_shared<FiredObjectTrigger>(m_triggers, FiredObjectTrigger::Electron, 1);
            if (single_ele->has_triggers()) m_obj_trig.push_back(single_ele);
        }
        if (m_doSingleMuonTriggerPassed) {
            std::shared_ptr<FiredObjectTrigger> single_muo = std::make_shared<FiredObjectTrigger>(m_triggers, FiredObjectTrigger::Muon, 1);
            if (single_muo->has_triggers()) m_obj_trig.push_back(single_muo);
        }
        if (m_doSinglePhotonTriggerPassed) {
            std::shared_ptr<FiredObjectTrigger> single_pho =
                std::make_shared<FiredObjectTrigger>(m_triggers, FiredObjectTrigger::Photon, 1);
            if (single_pho->has_triggers()) m_obj_trig.push_back(single_pho);
        }

        if (m_triggers.empty())
            m_EmptyTriggerList = true;
        else {
            ATH_MSG_INFO("The following triggers are set in the tool:");
            for (auto Trigger : m_triggers) { ATH_MSG_INFO("  - " << Trigger->name() << Trigger->PrintMatchingThresholds()); }
        }
        for (const auto& trig : m_obj_trig) { ATH_CHECK(trig->make_stores(m_XAMPPInfo)); }
        // Retrieve the object selection tools
        ATH_CHECK(m_elec_selection.retrieve());
        ATH_CHECK(m_muon_selection.retrieve());
        ATH_CHECK(m_phot_selection.retrieve());
        ATH_CHECK(m_tau_selection.retrieve());

        return StatusCode::SUCCESS;
    }
    std::vector<std::string> SUSYTriggerTool::GetTriggerOR(const std::string& trig_string) {
        return SUSYToolsPtr()->GetTriggerOR(trig_string);
    }
    bool SUSYTriggerTool::CheckTrigger() {
        m_Pass = false;
        // which madness drove me to execute these lines?!
        // if (this->msg().level() == MSG::DEBUG) {
        //    const Trig::ChainGroup* Chain =
        //    m_susytools->GetTrigChainGroup(".*"); for (auto &trig :
        //    Chain->getListOfTriggers()) {
        //        ATH_MSG_DEBUG("Trigger in the menue " << trig << " is fired "
        //        << (m_susytools->IsTrigPassed(trig) ? "Yes" : "No"));
        //    }
        //}
        for (auto& Trigg : m_triggers) {
            ATH_MSG_DEBUG("Check trigger " << Trigg->name() << " and perform matching if needed");
            Trigg->NewEvent();
            ATH_MSG_DEBUG("Trigger " << Trigg->name() << " has fired " << (Trigg->PassTrigger() ? "Yes" : "No") << ".");
            m_Pass = Trigg->PassTrigger() || m_Pass;
        }
        if (!m_DecTrigger->ConstStore(m_Pass || m_EmptyTriggerList).isSuccess()) {
            ATH_MSG_FATAL("Could not update trigger information");
            return false;
        }
        if (m_MetTrigEmulation && !MetTriggerEmulation().isSuccess()) {
            ATH_MSG_FATAL("Could not emulate the trigger");
            return false;
        }
        for (const auto& obj : m_obj_trig) {
            if (!obj->checkTrigger().isSuccess()) {
                ATH_MSG_FATAL("Object trigger evaluation failed");
                return false;
            }
        }

        return (m_NoCut || m_EmptyTriggerList || m_Pass);
    }
    bool SUSYTriggerTool::CheckTriggerMatching() {
        bool PassMatching = false;
        for (auto& Trig : m_triggers) { PassMatching = Trig->PassTriggerMatching() || PassMatching; }
        ATH_CHECK(m_DecMatching->Store(PassMatching));
        for (const auto& trig : m_obj_trig) ATH_CHECK(trig->checkMatching());
        return PassMatching;
    }
    bool SUSYTriggerTool::CheckTrigger(const std::string& trigger_name) {
        for (auto Trigg : m_triggers) {
            if (Trigg->name() == trigger_name) return Trigg->PassTrigger();
        }
        ATH_MSG_WARNING("The trigger " << trigger_name << " is unknown to the SUSYTriggerTool. Please check your configs");
        return m_trigDecTool->isPassed(trigger_name);
    }
    StatusCode SUSYTriggerTool::FillTriggerVector(std::vector<std::string>& triggers_vector) {
        for (auto trigger : triggers_vector) {
            // trigger names can have the following formats
            //      <trigger_name>
            //      <triger_name>;a-b;c-d;e-f
            //  where a,b,c,d are the run number to which the trigger should exclusively applied to
            std::shared_ptr<TriggerInterface> Trigg = std::make_shared<TriggerInterface>(trigger.substr(0, trigger.find(";")), this);
            ATH_CHECK(Trigg->initialize(m_XAMPPInfo));
            // Chop the periods from back to forth
            while (trigger.find(";") != std::string::npos) {
                std::string trigger_period = trigger.substr(trigger.rfind(";") + 1, std::string::npos);
                size_t delimiter = trigger_period.find("-");
                if (delimiter == std::string::npos) {
                    ATH_MSG_FATAL(
                        "Invalid run range is given "
                        << trigger
                        << ". Please recall that the format must be like <name>;begin_first-end_first;begin_second-end_second;....");
                    return StatusCode::FAILURE;
                }
                unsigned int begin = atoi(trigger_period.substr(0, delimiter).c_str());
                unsigned int end = atoi(trigger_period.substr(delimiter + 1).c_str());
                ATH_CHECK(Trigg->addTriggerPeriod(begin, end));
                trigger = trigger.substr(0, trigger.rfind(";"));
            }
            m_triggers.push_back(Trigg);
        }
        triggers_vector.clear();
        return StatusCode::SUCCESS;
    }
    bool SUSYTriggerTool::IsMatchedObject(const xAOD::IParticle* p, const std::string& Trig) const {
        std::shared_ptr<TriggerInterface> dec_Trig = GetActiveTrigger(Trig);
        if (!dec_Trig) return false;
        return dec_Trig->isMatched(p);
    }

    StatusCode SUSYTriggerTool::MetTriggerEmulation() {
        static XAMPP::Storage<float>* dec_TriggerLVL1Met = m_XAMPPInfo->GetVariableStorage<float>("TriggerLVL1Met");
        static XAMPP::Storage<float>* dec_TriggerCellMet = m_XAMPPInfo->GetVariableStorage<float>("TriggerCellMet");
        static XAMPP::Storage<float>* dec_TriggerMhtMet = m_XAMPPInfo->GetVariableStorage<float>("TriggerMhtMet");
        static XAMPP::Storage<float>* dec_TriggerTopoClMet = m_XAMPPInfo->GetVariableStorage<float>("TriggerTopoClMet");
        static XAMPP::Storage<float>* dec_TriggerTopoClPufitMet = m_XAMPPInfo->GetVariableStorage<float>("TriggerTopoClPufitMet");
        static XAMPP::Storage<float>* dec_TriggerTopoClPuetaMet = m_XAMPPInfo->GetVariableStorage<float>("TriggerTopoClPuetaMet");
        // the following is taken from
        // https://twiki.cern.ch/twiki/bin/viewauth/Atlas/METTriggerEmulation
        if (m_doLVL1Met && !evtStore()->contains<xAOD::EnergySumRoI>("LVL1EnergySumRoI")) {
            ATH_MSG_WARNING(
                "The file does not contain the EnergySumRoI LVL1EnergySumRoI, "
                "switch "
                "off the retrieving for the rest of the job.");
            m_doLVL1Met = false;
        }
        float l1_met = -1.;
        if (m_doLVL1Met) {
            const xAOD::EnergySumRoI* l1MetObject = nullptr;
            ATH_CHECK(evtStore()->retrieve(l1MetObject, "LVL1EnergySumRoI"));
            float l1_mex = l1MetObject->exMiss();
            float l1_mey = l1MetObject->eyMiss();
            l1_met = sqrt(l1_mex * l1_mex + l1_mey * l1_mey);
        }
        ATH_CHECK(dec_TriggerLVL1Met->ConstStore(l1_met));
        float hltMet = -1.;
        ATH_CHECK(GetHLTMet(hltMet, "HLT_xAOD__TrigMissingETContainer_TrigEFMissingET", m_doCellMet));
        ATH_CHECK(dec_TriggerCellMet->ConstStore(hltMet));
        ATH_CHECK(GetHLTMet(hltMet, "HLT_xAOD__TrigMissingETContainer_TrigEFMissingET_mht", m_doMhtMet));
        ATH_CHECK(dec_TriggerMhtMet->ConstStore(hltMet));
        ATH_CHECK(GetHLTMet(hltMet, "HLT_xAOD__TrigMissingETContainer_TrigEFMissingET_topocl", m_doTopoClMet));
        ATH_CHECK(dec_TriggerTopoClMet->ConstStore(hltMet));
        ATH_CHECK(GetHLTMet(hltMet, "HLT_xAOD__TrigMissingETContainer_TrigEFMissingET_topocl_PUC", m_doTopoClPufitMet));
        ATH_CHECK(dec_TriggerTopoClPufitMet->ConstStore(hltMet));
        ATH_CHECK(GetHLTMet(hltMet, "HLT_xAOD__TrigMissingETContainer_TrigEFMissingET_topocl_PS", m_doTopoClPuetaMet));
        ATH_CHECK(dec_TriggerTopoClPuetaMet->ConstStore(hltMet));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTriggerTool::GetHLTMet(float& met, const std::string& containerName, bool& doContainer) {
        if (!doContainer) {
            met = -1;
            return StatusCode::SUCCESS;
        }
        if (!m_trigDecTool->isPassed("L1_XE50")) {
            met = -1;
            return StatusCode::SUCCESS;
        }
        if (!evtStore()->contains<xAOD::TrigMissingETContainer>(containerName)) {
            ATH_MSG_WARNING("The file does not contain the TrigMissingETContainer "
                            << containerName << ", switch off the retrieving for the rest of the job.");
            met = -1.;
            doContainer = false;
            return StatusCode::SUCCESS;
        }
        const xAOD::TrigMissingETContainer* hltCont = nullptr;
        ATH_CHECK(evtStore()->retrieve(hltCont, containerName));
        XAMPP::FloatAccessor dec_ex("ex"), dec_ey("ey");
        if (dec_ex.isAvailable(*(hltCont->front())) && dec_ey.isAvailable(*(hltCont->front()))) {
            float ex = hltCont->front()->ex();
            float ey = hltCont->front()->ey();
            met = sqrt(ex * ex + ey * ey);
        } else {
            ATH_MSG_WARNING("ex and ey of HLT TrigMissingETContainer not found, returning -1 for GetHLTMet()");
            met = -1;
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTriggerTool::SaveObjectMatching(ParticleStorage* Storage, xAOD::Type::ObjectType Type) {
        if (!m_StoreObjectMatching) {
            ATH_MSG_INFO("Tool not configured to store the matching decsion per object");
            return StatusCode::SUCCESS;
        }
        for (auto& Trig : m_triggers) {
            if (Trig->NeedsElectronMatching() && Type == xAOD::Type::ObjectType::Electron)
                ATH_CHECK(Storage->SaveVariable<char>(Trig->MatchStoreName()));
            if (Trig->NeedsMuonMatching() && Type == xAOD::Type::ObjectType::Muon)
                ATH_CHECK(Storage->SaveVariable<char>(Trig->MatchStoreName()));
            if (Trig->NeedsTauMatching() && Type == xAOD::Type::ObjectType::Tau)
                ATH_CHECK(Storage->SaveVariable<char>(Trig->MatchStoreName()));
            if (Trig->NeedsPhotonMatching() && Type == xAOD::Type::ObjectType::Photon)
                ATH_CHECK(Storage->SaveVariable<char>(Trig->MatchStoreName()));
        }
        return StatusCode::SUCCESS;
    }
    bool SUSYTriggerTool::isData() const { return m_systematics->isData(); }
    ST::SUSYObjDef_xAOD* SUSYTriggerTool::SUSYToolsPtr() {
        ST::SUSYObjDef_xAOD* ST = dynamic_cast<ST::SUSYObjDef_xAOD*>(m_susytools.operator->());
        return ST;
    }
    std::vector<std::shared_ptr<TriggerInterface>> SUSYTriggerTool::GetActiveTriggers() const { return m_triggers; }
    std::shared_ptr<TriggerInterface> SUSYTriggerTool::GetActiveTrigger(const std::string& trig_name) const {
        for (const auto& trig : m_triggers) {
            if (trig->name() == trig_name) return trig;
        }
        return std::shared_ptr<TriggerInterface>();
    }

}  // namespace XAMPP
