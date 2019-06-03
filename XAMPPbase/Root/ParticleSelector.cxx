#include <PATInterfaces/SystematicSet.h>
#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/ISystematics.h>
#include <XAMPPbase/ParticleSelector.h>

// for jet reclustering
#include <xAODJet/JetAuxContainer.h>
#include <xAODJet/JetContainer.h>
#include "JetRec/JetDumper.h"
#include "JetRec/JetFinder.h"
#include "JetRec/JetFromPseudojet.h"
#include "JetRec/JetRecTool.h"
#include "JetRec/JetSplitter.h"
#include "JetRec/JetToolRunner.h"
#include "JetRec/PseudoJetGetter.h"
#include "fastjet/ClusterSequence.hh"
#include "fastjet/PseudoJet.hh"
#include "fastjet/tools/Filter.hh"

namespace XAMPP {
    ParticleSelector::ParticleSelector(const std::string& myname) :
        AsgTool(myname),
        m_systematics("SystematicsTool"),
        m_XAMPPInfo(nullptr),
        m_baselinePt(0.),
        m_baselineEta(-1.),
        m_signalPt(0.),
        m_signalEta(-1.),
        m_baseEtaExcludeProperty(),
        m_baseEtaExclude(),
        m_signalEtaExcludeProperty(),
        m_signalEtaExclude(),
        m_hasBaseToExclude(false),
        m_hasSignalToExclude(false),
        m_ActSys(nullptr),
        m_ContainerKey(),
        m_storeName(),
        m_ObjectType(XAMPP::SelectionObject::Other),
        m_init(false),
        m_PreSelDecorName("baseline"),
        m_BaselineDecorName("passOR"),
        m_SignalDecorName("signal"),
        m_IsolDecorName("isol"),
        m_ORutilsDecorName("selected"),
        m_ORUtils_InFlag(1),
        m_syst_checked(false),
        m_eventSFstores(),
        m_WriteSFperParticle(false),
        m_EvInfoHandle("EventInfoHandler"),
        m_particleDecorations(nullptr) {
        declareProperty("PreSelectionDecorator", m_PreSelDecorName);
        declareProperty("IsolationDecorator", m_IsolDecorName);

        declareProperty("BaselineDecorator", m_BaselineDecorName);
        declareProperty("SignalDecorator", m_SignalDecorName);

        declareProperty("ORUtilsSelectionDecorator", m_ORutilsDecorName);
        declareProperty("ORUtilsSelectionFlag", m_ORUtils_InFlag);

        declareProperty("BaselinePtCut", m_baselinePt);
        declareProperty("BaselineEtaCut", m_baselineEta);
        declareProperty("ExcludeBaselineEta", m_baseEtaExcludeProperty);

        declareProperty("SignalPtCut", m_signalPt);
        declareProperty("SignalEtaCut", m_signalEta);
        declareProperty("ExcludeSignalEta", m_signalEtaExcludeProperty);

        declareProperty("SystematicsTool", m_systematics);

        declareProperty("DecorateSFs", m_WriteSFperParticle);
        m_EvInfoHandle.declarePropertyFor(this, "EventInfoHandler", "The XAMPP EventInfo handle");
    }

    ParticleSelector::~ParticleSelector() { ATH_MSG_DEBUG("Destructor called"); }

    StatusCode ParticleSelector::CreateAuxElements(std::string& name, SelectionAccessor& acc, SelectionDecorator& dec) {
        if (name.empty()) {
            ATH_MSG_ERROR("You cannot have an empty decorator name");
            return StatusCode::FAILURE;
        }
        acc = SelectionAccessor(new CharAccessor(name));
        dec = SelectionDecorator(new CharDecorator(name));
        name.clear();
        return StatusCode::SUCCESS;
    }
    XAMPP::SelectionObject ParticleSelector::ObjectType() const { return m_ObjectType; }

    bool ParticleSelector::ProcessObject(XAMPP::SelectionObject T) const { return m_systematics->ProcessObject(T); }

    bool ParticleSelector::ProcessObject() const { return ProcessObject(ObjectType()); }

    StatusCode ParticleSelector::initialize() {
        ATH_MSG_INFO("Initialising...");
        if (isInitialized()) {
            ATH_MSG_WARNING("The tool is already initialized");
            return StatusCode::SUCCESS;
        }
        ATH_CHECK(m_systematics.retrieve());
        ATH_CHECK(m_EvInfoHandle.retrieve());
        m_XAMPPInfo = dynamic_cast<XAMPP::EventInfo*>(m_EvInfoHandle.operator->());

        if (!m_systematics->SystematicsFixed()) { ATH_MSG_DEBUG("Not all systematics are initialized yet"); }
        if (m_ObjectType == XAMPP::SelectionObject::Other) {
            ATH_MSG_FATAL("Object type not set");
            return StatusCode::FAILURE;
        } else if (!ProcessObject()) {
            ATH_MSG_INFO("Selector has been deactivated");
            m_init = true;
            return StatusCode::SUCCESS;
        }

        // create the particle decorations, if it has not already been done (for example in an upstream tool)
        setupDecorations();

        ATH_CHECK(ExtractEtaRanges(m_baseEtaExcludeProperty, m_baseEtaExclude));
        ATH_CHECK(ExtractEtaRanges(m_signalEtaExcludeProperty, m_signalEtaExclude));

        m_hasBaseToExclude = !m_baseEtaExclude.empty();
        m_hasSignalToExclude = !m_signalEtaExclude.empty();

        m_init = true;
        if (m_hasBaseToExclude || m_baselinePt > 0. || m_baselineEta > 0.) {
            ATH_MSG_INFO("Select baseline objects with p_{T} >" << m_baselinePt / 1.e3 << " GeV and |eta| <" << m_baselineEta << ".");
            if (m_hasBaseToExclude) {
                ATH_MSG_INFO("Exclude objects which are within:");
                for (auto& range : m_baseEtaExclude) ATH_MSG_INFO("    *** " << range.first << "< eta <" << range.second << ".");
            }
        }
        if (m_hasSignalToExclude || m_signalPt > 0. || m_signalEta > 0.) {
            ATH_MSG_INFO("Select signal objects with p_{T} >" << m_signalPt / 1.e3 << " GeV and |eta| <" << m_signalEta << ".");
            if (m_hasSignalToExclude) {
                ATH_MSG_INFO("Exclude objects which are within:");
                for (auto& range : m_signalEtaExclude) ATH_MSG_INFO("    *** " << range.first << "< eta <" << range.second << ".");
            }
        }

        return StatusCode::SUCCESS;
    }

    bool ParticleSelector::isInitialized() const { return m_init; }

    void ParticleSelector::SetSystematics(const CP::SystematicSet* Set) {
        if (!m_XAMPPInfo->SetSystematic(Set).isSuccess())
            ATH_MSG_WARNING("Could not set the systematic " << Set->name() << " to EventInfo");
        if (m_ActSys == Set) return;
        m_ActSys = Set;
        m_storeName = name() + SystName();
        ATH_MSG_DEBUG("Loaded systematic. Update StoreName to " << StoreName());
    }

    void ParticleSelector::SetContainerKey(const std::string& Key) {
        if (Key.empty() || !m_ContainerKey.empty()) {
            ATH_MSG_ERROR("Could not update the container key '" << m_ContainerKey << "' to '" << Key << "'");
            return;
        }
        m_ContainerKey = Key;
    }

    void ParticleSelector::SetObjectType(SelectionObject Type) {
        if (Type == SelectionObject::Other || m_ObjectType != SelectionObject::Other) {
            ATH_MSG_ERROR("Could not set the selection type of the selector");
        } else {
            m_ObjectType = Type;
        }
    }

    const std::string& ParticleSelector::StoreName() const { return m_storeName; }

    const std::string& ParticleSelector::ContainerKey() const { return m_ContainerKey; }

    std::string ParticleSelector::SystName(bool InclUnderScore) const {
        if (!m_ActSys) {
            ATH_MSG_WARNING("No current systematic defined");
            return "";
        }
        if (m_ActSys->name().empty() || !InclUnderScore) { return m_ActSys->name(); }
        return "_" + m_ActSys->name();
    }

    void ParticleSelector::SetSystematics(const CP::SystematicSet& Set) { SetSystematics(&Set); }

    bool ParticleSelector::SystematicAffects(const CP::SystematicSet* Set) const {
        bool Affects = IsInVector(Set, m_systematics->GetKinematicSystematics(m_ObjectType));
        ATH_MSG_DEBUG("Systematic " << Set->name() << " affects tool " << (Affects ? "yes" : "no"));
        return Affects;
    }

    bool ParticleSelector::isData() const { return m_systematics->isData(); }

    bool ParticleSelector::SystematicAffects(const CP::SystematicSet& Set) const { return SystematicAffects(&Set); }

    bool ParticleSelector::PassBaselineKinematics(const xAOD::IParticle* P) const { return PassBaselineKinematics(*P); }

    bool ParticleSelector::PassSignalKinematics(const xAOD::IParticle* P) const { return PassSignalKinematics(*P); }

    bool ParticleSelector::PassBaselineKinematics(const xAOD::IParticle& P) const {
        return (P.pt() > m_baselinePt && (m_baselineEta < 0. || fabs(P.eta()) < m_baselineEta)) &&
               (!m_hasBaseToExclude || !IsInEtaRange(P, m_baseEtaExclude));
    }

    bool ParticleSelector::PassSignalKinematics(const xAOD::IParticle& P) const {
        return (P.pt() > m_signalPt && (m_signalEta < 0. || fabs(P.eta()) < m_signalEta)) &&
               (!m_hasSignalToExclude || !IsInEtaRange(P, m_signalEtaExclude));
    }

    bool ParticleSelector::PassPreSelection(const xAOD::IParticle& P) const {
        return PassBaselineKinematics(P) && m_particleDecorations->passPreselection(P);
    }
    bool ParticleSelector::PassPreSelection(const xAOD::IParticle* P) const { return PassPreSelection(*P); }

    bool ParticleSelector::PassIsolation(const xAOD::IParticle& P) const {
        char pass = false;
        if (!m_particleDecorations->passIsolation.get(P, pass)) {
            ATH_MSG_ERROR("Isolation decoration is not set");
            PromptParticle(P);
            return false;
        }
        return pass;
    }
    bool ParticleSelector::PassSignalNoOR(const xAOD::IParticle* P) const { return PassSignalNoOR(*P); }

    bool ParticleSelector::PassSignalNoOR(const xAOD::IParticle& P) const { return GetSignalDecorator(P) && PassSignalKinematics(P); }

    bool ParticleSelector::PassIsolation(const xAOD::IParticle* P) const { return PassIsolation(*P); }
    bool ParticleSelector::PassBaseline(const xAOD::IParticle& P) const { return GetBaselineDecorator(P) && PassPreSelection(P); }
    bool ParticleSelector::PassBaseline(const xAOD::IParticle* P) const { return PassBaseline(*P); }

    bool ParticleSelector::PassSignal(const xAOD::IParticle& P) const {
        return GetSignalDecorator(P) && PassBaseline(P) && PassSignalKinematics(P);
    }
    bool ParticleSelector::PassSignal(const xAOD::IParticle* P) const { return PassSignal(*P); }

    void ParticleSelector::SetPreSelectionDecorator(const xAOD::IParticle& P, bool Pass) const {
        m_particleDecorations->passPreselection.set(P, Pass);
        m_particleDecorations->enterOverlapRemoval.set(P, (Pass ? m_ORUtils_InFlag : 0));
    }
    void ParticleSelector::SetPreSelectionDecorator(const xAOD::IParticle* P, bool Pass) const { SetPreSelectionDecorator(*P, Pass); }

    void ParticleSelector::SetSelectionDecorators(const xAOD::IParticle& P, bool Pass) const {
        SetSignalDecorator(P, Pass);
        SetPreSelectionDecorator(P, Pass);
        SetBaselineDecorator(P, Pass);
    }
    void ParticleSelector::SetSelectionDecorators(const xAOD::IParticle* P, bool Pass) const { SetSelectionDecorators(*P, Pass); }

    void ParticleSelector::SetBaselineDecorator(const xAOD::IParticle& P, bool Pass) const {
        m_particleDecorations->passBaseline.set(P, Pass);
    }
    void ParticleSelector::SetBaselineDecorator(const xAOD::IParticle* P, bool Pass) const { SetBaselineDecorator(*P, Pass); }

    void ParticleSelector::SetSignalDecorator(const xAOD::IParticle& P, bool Pass) const { m_particleDecorations->passSignal.set(P, Pass); }
    void ParticleSelector::SetSignalDecorator(const xAOD::IParticle* P, bool Pass) const { SetSignalDecorator(*P, Pass); }

    void ParticleSelector::SetOverlapInDecorator(const xAOD::IParticle& P, int Pass) const {
        m_particleDecorations->enterOverlapRemoval.set(P, Pass);
    }
    void ParticleSelector::SetOverlapInDecorator(const xAOD::IParticle* P, int Pass) const { SetOverlapInDecorator(*P, Pass); }

    void ParticleSelector::SetIsolationDecorator(const xAOD::IParticle& P, int Pass) const {
        m_particleDecorations->passIsolation.set(P, Pass);
    }
    void ParticleSelector::SetIsolationDecorator(const xAOD::IParticle* P, int Pass) const { SetIsolationDecorator(*P, Pass); }

    bool ParticleSelector::GetPreSelectionDecorator(const xAOD::IParticle& P) const {
        char pass = false;
        if (!m_particleDecorations->passPreselection.get(P, pass)) {
            ATH_MSG_ERROR("Preselection decoration is not set");
            PromptParticle(P);
            return false;
        }
        return pass;
    }
    bool ParticleSelector::GetBaselineDecorator(const xAOD::IParticle& P) const {
        char pass = false;
        if (!m_particleDecorations->passBaseline.get(P, pass)) {
            ATH_MSG_ERROR("Baseline decoration is not set");
            PromptParticle(P);
            return false;
        }
        return pass;
    }
    bool ParticleSelector::GetSignalDecorator(const xAOD::IParticle& P) const {
        char pass = false;
        if (!m_particleDecorations->passSignal.get(P, pass)) {
            ATH_MSG_ERROR("Signal decoration is not set");
            PromptParticle(P);
            return false;
        }
        return pass;
    }
    int ParticleSelector::GetOverlapInDecorator(const xAOD::IParticle& P) const {
        char val = 0;
        if (!m_particleDecorations->passPreselection.get(P, val)) {
            ATH_MSG_ERROR("Overlap decoration is not set");
            PromptParticle(P);
            return false;
        }
        return val;
    }
    bool ParticleSelector::GetPreSelectionDecorator(const xAOD::IParticle* P_PGID) const { return GetPreSelectionDecorator(*P_PGID); }
    int ParticleSelector::GetOverlapInDecorator(const xAOD::IParticle* P) const { return GetOverlapInDecorator(*P); }
    bool ParticleSelector::GetBaselineDecorator(const xAOD::IParticle* P_PGID) const { return GetBaselineDecorator(*P_PGID); }
    bool ParticleSelector::GetSignalDecorator(const xAOD::IParticle* P_PGID) const { return GetSignalDecorator(*P_PGID); }

    StatusCode ParticleSelector::ExtractEtaRanges(StringVector& propertyVector, EtaRangeVector& rangeVector) {
        // Since Gaudi does not support the interface to vectors of pairs
        //
        for (auto& range : propertyVector) {
            if (range.find(";") == std::string::npos) {
                ATH_MSG_ERROR("The eta range " << range << " does not contain seperation string \";\". Please update");
                return StatusCode::FAILURE;
            }
            float eta1 = atof(range.substr(0, range.find(";")).c_str());
            float eta2 = atof(range.substr(range.find(";") + 1, range.size()).c_str());
            if (eta1 == eta2) {
                ATH_MSG_ERROR("Please give a range and not a point in " << range);
                return StatusCode::FAILURE;
            } else if (eta1 > eta2)
                rangeVector.push_back(EtaRange(eta2, eta1));
            else
                rangeVector.push_back(EtaRange(eta1, eta2));
        }
        std::sort(rangeVector.begin(), rangeVector.end(), [](const EtaRange& a, const EtaRange& b) { return a.first < b.first; });
        bool hasDuplicates = rangeVector.size() > 1;
        // remove overlapping ranges
        while (hasDuplicates) {
            hasDuplicates = false;
            EtaRangeVector::iterator begin = rangeVector.begin();
            for (EtaRangeVector::iterator itr = begin + 1; itr != rangeVector.end(); ++itr) {
                for (EtaRangeVector::iterator itr1 = begin; itr1 != itr; ++itr1) {
                    // The upper interval limit from the first range
                    // overlaps with the low border from the second range
                    //  -2.7 --  -2.4
                    //  -2.5 --  -1.6
                    if (itr->first <= itr1->second) {
                        itr1->second = itr->second;
                        rangeVector.erase(itr);
                        hasDuplicates = true;
                        break;
                    }
                }
                if (hasDuplicates) break;
            }
        }
        propertyVector.clear();
        return StatusCode::SUCCESS;
    }
    bool ParticleSelector::IsInEtaRange(const xAOD::IParticle& P, const EtaRangeVector& ranges) const {
        for (const auto& rng : ranges) {
            if (rng.first <= P.eta() && P.eta() <= rng.second) return true;
        }
        return false;
    }
    StatusCode ParticleSelector::ReclusterJets(const xAOD::IParticleContainer* inputJets, float Rcone, float minPtKt4, std::string PreFix,
                                               float minPtRecl, float rclus, float ptfrac) {
        if (m_systematics->AffectsOnlyMET(m_systematics->GetCurrent())) return StatusCode::SUCCESS;
        ATH_MSG_DEBUG("Starting Jet Reclustering with Rcone = " << Rcone);
        static SG::AuxElement::Decorator<int> dec_constituents("constituents");
        xAOD::JetContainer* FatJets = new xAOD::JetContainer();
        xAOD::JetAuxContainer* FatJetsAux = new xAOD::JetAuxContainer();
        FatJets->setStore(FatJetsAux);

        if (minPtKt4 == -1.) minPtKt4 = m_signalPt;  // using signal pt cut if not set from outside

        std::vector<fastjet::PseudoJet> v_pseudoJets;
        unsigned int jetIdx = 0;
        for (const auto& ijet : *inputJets) {
            if (ijet->p4().Pt() >= minPtKt4) {
                fastjet::PseudoJet myjet(ijet->p4().Px(), ijet->p4().Py(), ijet->p4().Pz(), ijet->p4().E());
                myjet.set_user_index(jetIdx);  // able to trace back
                v_pseudoJets.push_back(myjet);
            }
            ++jetIdx;
        }
        ATH_MSG_DEBUG("Vector of pseudo jets filled ");

        fastjet::Strategy strategy = fastjet::Best;
        fastjet::RecombinationScheme recomb_scheme = fastjet::E_scheme;

        fastjet::JetDefinition jet_def(fastjet::antikt_algorithm, Rcone, recomb_scheme, strategy);
        ATH_MSG_DEBUG("Jet Definition set ");

        // Execute the clustering algorithm and retrieve the output
        fastjet::ClusterSequence cs(v_pseudoJets, jet_def);
        ATH_MSG_DEBUG("Cluster sequence done");
        // std::vector<fastjet::PseudoJet> cs_result =
        // fastjet::sorted_by_pt(cs.inclusive_jets(150000.));
        std::vector<fastjet::PseudoJet> cs_result = cs.inclusive_jets(minPtRecl);
        ATH_MSG_DEBUG("Vector of pseudojets done and sorted");

        fastjet::Filter trimmer(fastjet::JetDefinition(fastjet::kt_algorithm, rclus), fastjet::SelectorPtFractionMin(ptfrac));
        for (fastjet::PseudoJet& pjet : cs_result) {
            fastjet::PseudoJet pj;
            if (rclus != 0)
                pj = trimmer(pjet);
            else {
                std::vector<fastjet::PseudoJet> pj_vec;
                for (fastjet::PseudoJet& psubjet : pjet.constituents()) {
                    if (psubjet.pt() > ptfrac * pjet.pt()) pj_vec.push_back(psubjet);
                }
                pj = fastjet::join(pj_vec);
            }
            if ((minPtRecl > 0) && (pj.pt() <= minPtRecl)) continue;
            xAOD::Jet* myJet = new xAOD::Jet();
            FatJets->push_back(myJet);
            xAOD::JetFourMom_t FourVec;
            ROOT::Math::LorentzVector<ROOT::Math::PxPyPzM4D<double> >::Scalar E = pj.e();
            ROOT::Math::LorentzVector<ROOT::Math::PxPyPzM4D<double> >::Scalar Px = pj.px();
            ROOT::Math::LorentzVector<ROOT::Math::PxPyPzM4D<double> >::Scalar Py = pj.py();
            ROOT::Math::LorentzVector<ROOT::Math::PxPyPzM4D<double> >::Scalar Pz = pj.pz();
            FourVec.SetPxPyPzE(Px, Py, Pz, E);
            myJet->setJetP4(FourVec);
            dec_constituents(*myJet) = pj.constituents().size();
            for (fastjet::PseudoJet& pjcons : pj.constituents()) { myJet->addConstituent(inputJets->at(pjcons.user_index())); }
        }
        v_pseudoJets.clear();
        cs_result.clear();

        FatJets->sort(XAMPP::ptsorter);

        // set the name of the FatJet container in order to access them via
        // GetCustomJets(std::string kind)
        std::string store = Form("%s%.1f%s", (PreFix + "FatJet").c_str(), Rcone, StoreName().c_str());
        ATH_CHECK(evtStore()->record(FatJets, store));
        ATH_CHECK(evtStore()->record(FatJetsAux, store + "Aux."));

        ATH_MSG_DEBUG("JetReclustering done with PtThreshold [MeV] = " << minPtKt4 << " and R = " << Rcone);
        return StatusCode::SUCCESS;
    }
    bool ParticleSelector::checkForValidSystematics() const {
        if (m_syst_checked) return true;
        if (!m_systematics->SystematicsFixed()) {
            ATH_MSG_FATAL("Systematics are not fixed yet");
            return false;
        } else if (m_systematics->GetKinematicSystematics(m_ObjectType).empty()) {
            ATH_MSG_FATAL("No systematic has been defined");
            return false;
        }

        m_syst_checked = true;
        return true;
    }
    // Functionallity to store the scale-factors
    StatusCode ParticleSelector::SaveObjectSF(ParticleStorage* Storage) {
        if (isData() || !m_WriteSFperParticle) return StatusCode::SUCCESS;
        for (auto& SF : m_eventSFstores) {
            if (!SF->SaveTrees()) continue;
            std::string sf_decorName = ReplaceExpInString(Storage->name(), "Weight", "EffSF");
            ATH_CHECK(Storage->SaveDouble(sf_decorName, SF->SaveVariations()));
        }
        return StatusCode::SUCCESS;
    }
    std::shared_ptr<DoubleDecorator> ParticleSelector::getParticleSfDecorator(XAMPP::Storage<double>* SF_storage) {
        if (SF_storage == nullptr) {
            ATH_MSG_ERROR("No event scale factor has been given");
            return std::shared_ptr<DoubleDecorator>();
        }
        std::string sf_decorName = ReplaceExpInString(SF_storage->name(), "Weight", "EffSF");
        if (!IsInVector(SF_storage, m_eventSFstores)) m_eventSFstores.push_back(SF_storage);
        return std::shared_ptr<DoubleDecorator>(new DoubleDecorator(sf_decorName));
    }
    std::shared_ptr<DoubleAccessor> ParticleSelector::getParticleSfAccessor(XAMPP::Storage<double>* SF_storage) {
        if (SF_storage == nullptr) {
            ATH_MSG_ERROR("No event scale factor has been given");
            return std::shared_ptr<DoubleAccessor>();
        }
        std::string sf_decorName = ReplaceExpInString(SF_storage->name(), "Weight", "EffSF");
        if (!IsInVector(SF_storage, m_eventSFstores)) m_eventSFstores.push_back(SF_storage);
        return std::shared_ptr<DoubleAccessor>(new DoubleAccessor(sf_decorName));
    }
    StatusCode ParticleSelector::initEventSignalSf(XAMPP::Storage<double>*& store_ptr, const std::string& suffix,
                                                   const CP::SystematicSet* set, bool save, const std::string& basename) {
        std::string VarName =
            (basename.empty() ? ContainerKey().substr(0, 3) : basename) + "Weight" + suffix + (set->name().size() ? "_" : "") + set->name();
        if (!m_XAMPPInfo->DoesVariableExist<double>(VarName))
            ATH_CHECK(m_XAMPPInfo->NewEventVariable<double>(VarName, save,
                                                            set == m_systematics->GetNominal()));  // Weights are saved to the
                                                                                                   // trees only the nominal
                                                                                                   // gets passed to all trees
        store_ptr = m_XAMPPInfo->GetVariableStorage<double>(VarName);
        if (!store_ptr) return StatusCode::FAILURE;
        return StatusCode::SUCCESS;
    }
    StatusCode ParticleSelector::initEventBaselineSf(XAMPP::Storage<double>*& store_ptr, const std::string& suffix,
                                                     const CP::SystematicSet* set, bool save, const std::string& basename) {
        std::string VarName = (basename.empty() ? ContainerKey().substr(0, 3) : basename) + "BaseWeight" + suffix +
                              (set->name().size() ? "_" : "") + set->name();
        if (!m_XAMPPInfo->DoesVariableExist<double>(VarName))
            ATH_CHECK(m_XAMPPInfo->NewEventVariable<double>(VarName, save,
                                                            set == m_systematics->GetNominal()));  // Weights are saved to the
                                                                                                   // trees only the nominal
                                                                                                   // gets passed to all trees
        store_ptr = m_XAMPPInfo->GetVariableStorage<double>(VarName);
        if (!store_ptr) return StatusCode::FAILURE;
        return StatusCode::SUCCESS;
    }
    StatusCode ParticleSelector::initIParticleWeight(IPartilceWeightDecorator& weighter, const std::string& sf_type,
                                                     const CP::SystematicSet* syst_set, unsigned int content, bool save,
                                                     const std::string& base_name) {
        XAMPP::Storage<double>* BaseStore = nullptr;
        XAMPP::Storage<double>* SignalStore = nullptr;

        if (content == ScaleFactorMapContains::SignalAndBaseSf) {
            ATH_CHECK(initEventSignalSf(SignalStore, sf_type, syst_set, save, base_name));
            ATH_CHECK(initEventBaselineSf(BaseStore, sf_type, syst_set, save, base_name));
            weighter.setSfDecorators(getParticleSfDecorator(SignalStore), getParticleSfAccessor(SignalStore));
        } else if (content == ScaleFactorMapContains::BaselineSf) {
            ATH_CHECK(initEventBaselineSf(BaseStore, sf_type, syst_set, save, base_name));
            weighter.setSfDecorators(getParticleSfDecorator(BaseStore), getParticleSfAccessor(BaseStore));
        } else if (content == ScaleFactorMapContains::SignalSf) {
            ATH_CHECK(initEventSignalSf(SignalStore, sf_type, syst_set, save, base_name));
            weighter.setSfDecorators(getParticleSfDecorator(SignalStore), getParticleSfAccessor(SignalStore));
        } else {
            ATH_MSG_FATAL("Unknown what " << sf_type << " contains");
            return StatusCode::FAILURE;
        }
        weighter.setEventSfStores(BaseStore, SignalStore);
        return StatusCode::SUCCESS;
    }

    void ParticleSelector::setupDecorations(std::shared_ptr<ParticleDecorations> in_ptr) {
        // a user-specified argument takes precedence.
        if (in_ptr) { m_particleDecorations = in_ptr; }
        // if we do not yet have an instance, set it here
        if (!m_particleDecorations) { m_particleDecorations = std::make_shared<ParticleDecorations>(); }
        m_particleDecorations->passPreselection.setDecorationString(m_PreSelDecorName);
        m_particleDecorations->passBaseline.setDecorationString(m_BaselineDecorName);
        m_particleDecorations->passSignal.setDecorationString(m_SignalDecorName);
        m_particleDecorations->passIsolation.setDecorationString(m_IsolDecorName);
        m_particleDecorations->enterOverlapRemoval.setDecorationString(m_ORutilsDecorName);
    }

    //#############################################################
    //                IPartilceWeightDecorator
    //#############################################################
    IPartilceWeightDecorator::IPartilceWeightDecorator() :
        m_part_SF_decor(),
        m_part_SF_acc(),
        m_part_isEval_decor(),
        m_part_isEval_acc(),
        m_event_base_SF(nullptr),
        m_event_signal_SF(nullptr) {}
    IPartilceWeightDecorator::~IPartilceWeightDecorator() {}
    StatusCode IPartilceWeightDecorator::applyEventSF(XAMPP::Storage<double>*& event_store, const double& SF) {
        if (isStoreLocked(event_store)) return StatusCode::SUCCESS;
        if (!event_store->isAvailable()) return event_store->Store(SF);
        return event_store->Store(SF * event_store->GetValue());
    }
    StatusCode IPartilceWeightDecorator::initEvent() {
        if (m_event_base_SF && !m_event_base_SF->isAvailable() && !m_event_base_SF->ConstStore(1.).isSuccess()) return StatusCode::FAILURE;
        if (m_event_signal_SF && !m_event_signal_SF->isAvailable() && !m_event_signal_SF->ConstStore(1.).isSuccess())
            return StatusCode::FAILURE;
        return StatusCode::SUCCESS;
    }
    StatusCode IPartilceWeightDecorator::applyEventSF(XAMPP::Storage<double>*& event_store) {
        if (isStoreLocked(event_store)) return StatusCode::SUCCESS;
        m_part_isEval_decor->operator()(*event_store->XAMPPInfo()->GetEventInfo()) = true;
        if (event_store->isAvailable()) return StatusCode::SUCCESS;
        return event_store->Store(1.);
    }
    bool IPartilceWeightDecorator::isStoreLocked(XAMPP::Storage<double>*& event_store) const {
        if (event_store == nullptr) return true;
        if (!event_store->isAvailable()) return false;
        return (m_part_isEval_acc->isAvailable(*event_store->XAMPPInfo()->GetEventInfo()) &&
                m_part_isEval_acc->operator()(*event_store->XAMPPInfo()->GetEventInfo()) == true);
    }
    void IPartilceWeightDecorator::setEventSfStores(XAMPP::Storage<double>* base, XAMPP::Storage<double>* signal) {
        m_event_base_SF = base;
        m_event_signal_SF = signal;
        std::string is_saved = "SfIsApplied";
        if (m_event_signal_SF)
            is_saved = ReplaceExpInString(m_event_signal_SF->name(), "Weight", "SfIsApplied");
        else if (m_event_base_SF)
            is_saved = ReplaceExpInString(m_event_base_SF->name(), "Weight", "SfIsApplied");
        else
            Warning("IPartilceWeightDecorator::setEventSfStores()", "No storage has been given");
        m_part_isEval_decor = std::unique_ptr<BoolDecorator>(new BoolDecorator(is_saved));
        m_part_isEval_acc = std::unique_ptr<BoolAccessor>(new BoolAccessor(is_saved));
    }
    void IPartilceWeightDecorator::setSfDecorators(std::shared_ptr<DoubleDecorator> dec, std::shared_ptr<DoubleAccessor> acc) {
        m_part_SF_decor = dec;
        m_part_SF_acc = acc;
    }
    bool IPartilceWeightDecorator::isSFcalculated(const xAOD::IParticle& particle) const {
        return m_part_isEval_acc->isAvailable(particle) && m_part_isEval_acc->operator()(particle) == true;
    }

    double IPartilceWeightDecorator::getSF(const xAOD::IParticle& particle) const {
        if (!isSFcalculated(particle)) {
            Warning("IPartilceWeightDecorator::getSF()", "no SF saved");
            return DBL_MAX;
        }
        return m_part_SF_acc->operator()(particle);
    }
    StatusCode IPartilceWeightDecorator::applySF() {
        if (!applyEventSF(m_event_base_SF).isSuccess()) return StatusCode::FAILURE;
        return applyEventSF(m_event_signal_SF);
    }
    StatusCode IPartilceWeightDecorator::saveEventSF(const xAOD::IParticle& particle, double SF, bool isSignal) {
        m_part_SF_decor->operator()(particle) = SF;
        m_part_isEval_decor->operator()(particle) = true;
        return saveEventSF(SF, isSignal);
    }
    StatusCode IPartilceWeightDecorator::saveEventSF(double SF, bool isSignal) {
        if (!applyEventSF(m_event_base_SF, SF).isSuccess()) return StatusCode::FAILURE;
        if (isSignal && !applyEventSF(m_event_signal_SF, SF).isSuccess()) return StatusCode::FAILURE;
        return StatusCode::SUCCESS;
    }
    StatusCode IPartilceWeightDecorator::saveBaselineSF(double SF) { return applyEventSF(m_event_base_SF, SF); }
    StatusCode IPartilceWeightDecorator::saveSignalSF(double SF) { return applyEventSF(m_event_signal_SF, SF); }
    bool IPartilceWeightDecorator::hasBaselineSF() const { return m_event_base_SF && m_event_base_SF->isAvailable(); }
    bool IPartilceWeightDecorator::hasSignalSF() const { return m_event_signal_SF && m_event_signal_SF->isAvailable(); }
    double IPartilceWeightDecorator::getBaselineEventSF() const {
        if (!hasBaselineSF()) {
            Warning("getBaselineEventSF()", "No baseline SF defined");
            return DBL_MAX;
        }
        return m_event_base_SF->GetValue();
    }
    double IPartilceWeightDecorator::getSignalEventSF() const {
        if (!hasSignalSF()) {
            Warning("getSignalEventSF()", "No signal SF defined");
            return DBL_MAX;
        }
        return m_event_signal_SF->GetValue();
    }

}  // namespace XAMPP
