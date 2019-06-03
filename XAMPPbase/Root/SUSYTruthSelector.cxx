#include <SUSYTools/SUSYCrossSection.h>
#include <SUSYTools/SUSYObjDef_xAOD.h>

#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/IMetSelector.h>
#include <XAMPPbase/SUSYTruthSelector.h>
#include "fastjet/ClusterSequence.hh"
#include "fastjet/PseudoJet.hh"

namespace XAMPP {

    void SUSYTruthSelector::declare(ObjectDefinition& obj, const std::string& as) {
        declareProperty(Form("TruthBaseline%sPtCut", as.c_str()), obj.BaselinePt);
        declareProperty(Form("TruthBaseline%sEtaCut", as.c_str()), obj.BaselineEta);
        declareProperty(Form("TruthSignal%sPtCut", as.c_str()), obj.SignalPt);
        declareProperty(Form("TruthSignal%sEtaCut", as.c_str()), obj.SignalEta);

        declareProperty(Form("ExcludeBaseline%sInEta", as.c_str()), obj.ExclBaseEta_Property);
        declareProperty(Form("ExcludeSignal%sInEta", as.c_str()), obj.ExclSignalEta_Property);
        declareProperty(Form("%sContainer", as.c_str()), obj.ContainerKey);
        declareProperty(Form("do%ss", as.c_str()), obj.doObject);
    }
    SUSYTruthSelector::SUSYTruthSelector(const std::string& myname) :
        ParticleSelector(myname),
        m_MuonDefs(),
        m_ElectronDefs(),
        m_TauDefs(),
        m_JetDefs(),
        m_PhotonDefs(),
        m_NeutrinoDefs(),
        m_BJetPtCut(-1.),
        m_BJetEtaCut(-1.),
        m_RequirePreselFromHardProc(false),
        m_RequireBaseFromHardProc(false),
        m_RequireSignalFromHardProc(false),
        m_RequirePreSelTauHad(true),
        m_RequireBaseTauHad(true),
        m_RequireSignalTauHad(true),
        m_xAODTruthJets(nullptr),
        m_xAODTruthParticles(nullptr),
        m_xAODTruthMet(nullptr),
        m_xAODTruthBSM(nullptr),
        m_xAODTruthBoson(nullptr),
        m_xAODTruthTop(nullptr),
        m_InitialParticles(nullptr),
        m_InitialJets(nullptr),
        m_InitialFatJets(nullptr),
        m_InitialElectrons(nullptr),
        m_InitialMuons(nullptr),
        m_InitialPhotons(nullptr),
        m_InitialTaus(nullptr),
        m_InitialNeutrinos(nullptr),
        m_PreJets(nullptr),
        m_BaselineJets(nullptr),
        m_SignalJets(nullptr),
        m_BJets(nullptr),
        m_LightJets(nullptr),
        m_Particles(nullptr),
        m_PreElectrons(nullptr),
        m_BaselineElectrons(nullptr),
        m_SignalElectrons(nullptr),
        m_PreMuons(nullptr),
        m_BaselineMuons(nullptr),
        m_SignalMuons(nullptr),
        m_PrePhotons(nullptr),
        m_BaselinePhotons(nullptr),
        m_SignalPhotons(nullptr),
        m_PreTaus(nullptr),
        m_BaselineTaus(nullptr),
        m_SignalTaus(nullptr),
        m_Neutrinos(nullptr),
        m_InitialStatePart(nullptr),
        m_doTruthParticles(false),
        m_isTRUTH3(false),
        m_rejectUnknownOrigin(false),
        m_doSUSYProcess(true),
        m_BosonKey(""),
        m_BSMKey(""),
        m_TopKey(""),
        m_useVisTauP4(true) {
        SetContainerKey("TruthParticles");
        SetObjectType(XAMPP::SelectionObject::TruthParticle);
        // Kinematic properties of the particles
        declare(m_MuonDefs, "Muon");
        declare(m_ElectronDefs, "Electron");
        declare(m_TauDefs, "Tau");
        declare(m_JetDefs, "Jet");
        declare(m_PhotonDefs, "Photon");
        declare(m_NeutrinoDefs, "Neutrino");

        declareProperty("TruthBJetPtCut", m_BJetPtCut);
        declareProperty("TruthBJetEtaCut", m_BJetEtaCut);

        declareProperty("PreselectionHardProcess", m_RequirePreselFromHardProc);
        declareProperty("BaselineHardProcess", m_RequireBaseFromHardProc);
        declareProperty("SignalHardProcess", m_RequireSignalFromHardProc);

        declareProperty("OnlySelectHadronicTau", m_RequirePreSelTauHad);
        declareProperty("OnlySelectHadronicBaseTau", m_RequireBaseTauHad);
        declareProperty("OnlySelectHadronicSignalTau", m_RequireSignalTauHad);
        declareProperty("UseVisibleTauMomentum", m_useVisTauP4);
        // Run Options
        declareProperty("BosonContainer", m_BosonKey);
        declareProperty("BSMContainer", m_BSMKey);
        declareProperty("TopContainer", m_TopKey);

        declareProperty("doTruthParticles", m_doTruthParticles);
        declareProperty("isTRUTH3", m_isTRUTH3);
        declareProperty("rejectUnknownOrigin", m_rejectUnknownOrigin);
        declareProperty("fillSUSYProcess", m_doSUSYProcess);
    }
    StatusCode SUSYTruthSelector::init(ObjectDefinition& obj, const std::string& as) {
        ATH_MSG_INFO("Load object definitions of " << as << ".");
        ATH_CHECK(ExtractEtaRanges(obj.ExclBaseEta_Property, obj.BaseEtaExclude));
        ATH_CHECK(ExtractEtaRanges(obj.ExclSignalEta_Property, obj.SignalEtaExclude));
        obj.hasBaseEtaToExclude = !obj.BaseEtaExclude.empty();
        obj.hasSignalEtaToExclude = !obj.SignalEtaExclude.empty();
        obj.hasContainer = !obj.ContainerKey.empty();
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTruthSelector::initialize() {
        if (isInitialized()) { return StatusCode::SUCCESS; }
        ATH_CHECK(ParticleSelector::initialize());
        if (!ProcessObject()) { return StatusCode::SUCCESS; }
        if (ProcessObject(XAMPP::SelectionObject::MissingET)) {
            ATH_CHECK(m_XAMPPInfo->NewCommonEventVariable<XAMPPmet>("TruthMET", true));
        }
        ATH_CHECK(init(m_MuonDefs, "Muon"));
        ATH_CHECK(init(m_ElectronDefs, "Electron"));
        ATH_CHECK(init(m_TauDefs, "Tau"));
        ATH_CHECK(init(m_JetDefs, "Jet"));
        ATH_CHECK(init(m_PhotonDefs, "Photon"));
        ATH_CHECK(init(m_NeutrinoDefs, "Neutrino"));

        setupDecorations();

        m_doTruthTop = !m_TopKey.empty();
        m_doTruthBoson = !m_BosonKey.empty();
        m_doTruthSUSY = !m_BSMKey.empty();

        return StatusCode::SUCCESS;
    }

    StatusCode SUSYTruthSelector::LoadContainers() {
        if (ProcessObject(XAMPP::SelectionObject::MissingET)) { ATH_CHECK(LoadContainer("MET_Truth", m_xAODTruthMet)); }
        if (m_doTruthParticles) ATH_CHECK(LoadContainer(ContainerKey(), m_xAODTruthParticles));
        if (doTruthJets()) ATH_CHECK(LoadContainer(JetKey(), m_xAODTruthJets));
        // These containers may be available in the derivations
        //        if (m_doTruthTop) ATH_CHECK(LoadContainer(m_TopKey,
        //        m_xAODTruthTop));
        if (!m_BSMKey.empty()) ATH_CHECK(LoadContainer(m_BSMKey, m_xAODTruthBSM));
        if (!m_BosonKey.empty()) ATH_CHECK(LoadContainer(m_BosonKey, m_xAODTruthBoson));

        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTruthSelector::InitialFill(const CP::SystematicSet& systset) {
        if (!SystematicAffects(systset)) return StatusCode::SUCCESS;
        SetSystematics(systset);
        if (ProcessObject(XAMPP::SelectionObject::MissingET)) {
            static XAMPP::Storage<XAMPPmet>* s_MET = m_XAMPPInfo->GetVariableStorage<XAMPPmet>("TruthMET");
            xAOD::MissingET* MET = XAMPP::GetMET_obj("NonInt", const_cast<xAOD::MissingETContainer*>(m_xAODTruthMet));
            ATH_CHECK(s_MET->Store(MET));
        }
        if (m_doTruthParticles && CreateContainerLinks(ContainerKey(), m_InitialParticles) == LinkStatus::Failed)
            return StatusCode::FAILURE;
        if (doTruthJets()) {
            if (CreateContainerLinks(JetKey(), m_InitialJets) == LinkStatus::Failed) return StatusCode::FAILURE;
        } else
            ATH_CHECK(ViewElementsContainer("PreKt4InitJets", m_InitialJets));

        ATH_CHECK(FillTruthParticleContainer());

        ATH_CHECK(ViewElementsContainer("preJets", m_PreJets));
        ATH_CHECK(ViewElementsContainer("baselineJets", m_BaselineJets));
        ATH_CHECK(ViewElementsContainer("signalTrJets", m_SignalJets));
        ATH_CHECK(ViewElementsContainer("bjetJets", m_BJets));
        ATH_CHECK(ViewElementsContainer("lightJets", m_LightJets));

        ATH_CHECK(ViewElementsContainer("preElec", m_PreElectrons));
        ATH_CHECK(ViewElementsContainer("baselineElec", m_BaselineElectrons));
        ATH_CHECK(ViewElementsContainer("signalElec", m_SignalElectrons));
        ATH_CHECK(ViewElementsContainer("preMuon", m_PreMuons));
        ATH_CHECK(ViewElementsContainer("baselineMuon", m_BaselineMuons));
        ATH_CHECK(ViewElementsContainer("signalMuon", m_SignalMuons));
        ATH_CHECK(ViewElementsContainer("prePhoton", m_PrePhotons));
        ATH_CHECK(ViewElementsContainer("baselinePhoton", m_BaselinePhotons));
        ATH_CHECK(ViewElementsContainer("signalPhoton", m_SignalPhotons));
        ATH_CHECK(ViewElementsContainer("preTau", m_PreTaus));
        ATH_CHECK(ViewElementsContainer("baselineTau", m_BaselineTaus));
        ATH_CHECK(ViewElementsContainer("signalTau", m_SignalTaus));
        ATH_CHECK(ViewElementsContainer("signalNeut", m_Neutrinos));

        InitContainer(m_InitialJets, m_PreJets);
        InitContainer(m_InitialElectrons, m_PreElectrons);
        InitContainer(m_InitialMuons, m_PreMuons);
        InitContainer(m_InitialPhotons, m_PrePhotons);
        InitContainer(m_InitialTaus, m_PreTaus);
        InitContainer(m_InitialNeutrinos, m_Neutrinos);

        return StatusCode::SUCCESS;
    }
    void SUSYTruthSelector::InitDecorators(xAOD::TruthParticle* T, bool Pass) {
        m_truthDecorations->charge.set(T, T->charge());
        if (m_useVisTauP4) LoadVisibleMomentum(T);
        SetSelectionDecorators(*T, Pass);
        if (T->isTau() && m_RequireBaseTauHad) {
            char isTauHad = false;
            if (Pass && !(m_truthDecorations->IsHadronicTau.get(T, isTauHad) && isTauHad)) {
                SetOverlapInDecorator(*T, false);
                SetBaselineDecorator(*T, false);
            }
        }
    }
    void SUSYTruthSelector::InitDecorators(const xAOD::Jet* J, bool Pass) { SetSelectionDecorators(*J, Pass); }

    bool SUSYTruthSelector::IsGenParticle(const xAOD::TruthParticle* particle) const {
        // The barcode requirement only selects particles generated by the
        // generator NOT by GEANT4
        // https://svnweb.cern.ch/trac/atlasoff/browser/Generators/TruthUtils/trunk/TruthUtils/TruthParticleHelpers.h
        return (!particle || particle->isGenSpecific() || particle->barcode() >= 200000);
    }
    bool SUSYTruthSelector::ConsiderParticle(xAOD::TruthParticle* particle) {
        // final state particle
        if (IsGenParticle(particle))
            return false;
        else if (particle->status() == 1) {
            if (particle->isLepton() || particle->isPhoton() || isSparticle(particle)) return true;
        } else if (particle->isHiggs())
            return true;
        else if (isTrueSUSY(particle))
            return true;
        else if (isTrueZ(particle))
            return true;
        else if (isTrueW(particle))
            return true;
        else if (isTrueTop(particle))
            return true;
        else if (isTrueTau(particle))
            return true;
        return false;
    }
    bool SUSYTruthSelector::IsInitialStateParticle(const xAOD::TruthParticle* particle) {
        return isTrueSUSY(particle) || isEWboson(particle) || particle->isQuark();
    }
    StatusCode SUSYTruthSelector::FillTruthParticleContainer() {
        // Sort everything out from the truth particle container
        ATH_CHECK(ViewElementsContainer("skimmedTruth", m_Particles));
        ATH_CHECK(RetrieveParticleContainer(m_InitialElectrons, m_ElectronDefs.hasContainer, ElectronKey(), "initialElec"));
        ATH_CHECK(RetrieveParticleContainer(m_InitialMuons, m_MuonDefs.hasContainer, MuonKey(), "initialMuon"));
        ATH_CHECK(RetrieveParticleContainer(m_InitialPhotons, m_PhotonDefs.hasContainer, PhotonKey(), "initialPhoton"));
        ATH_CHECK(RetrieveParticleContainer(m_InitialTaus, m_TauDefs.hasContainer, TauKey(), "initialTaus"));
        ATH_CHECK(RetrieveParticleContainer(m_InitialNeutrinos, m_NeutrinoDefs.hasContainer, NeutrinoKey(), "initalNeutrinos"));
        ATH_CHECK(RetrieveParticleContainer(m_InitialStatePart, m_doTruthSUSY, BSMKey(), "InitialState"));

        if (m_doTruthParticles) {
            for (const auto& particle : *m_InitialParticles) {
                if (ConsiderParticle(particle)) {
                    InitDecorators(particle, true);
                    m_Particles->push_back(particle);
                    if (!m_ElectronDefs.hasContainer && particle->isElectron())
                        m_InitialElectrons->push_back(particle);
                    else if (!m_MuonDefs.hasContainer && particle->isMuon())
                        m_InitialMuons->push_back(particle);
                    else if (!m_PhotonDefs.hasContainer && particle->isPhoton())
                        m_InitialPhotons->push_back(particle);
                    else if (!m_NeutrinoDefs.hasContainer && particle->isNeutrino())
                        m_InitialNeutrinos->push_back(particle);
                    else if (!m_TauDefs.hasContainer && particle->isTau())
                        m_InitialTaus->push_back(particle);
                    else if (!m_doTruthSUSY && IsInitialStateParticle(particle))
                        m_InitialStatePart->push_back(particle);
                }
            }
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYTruthSelector::RetrieveParticleContainer(xAOD::TruthParticleContainer*& Particles, bool FromStoreGate,
                                                            const std::string& GateKey, const std::string& ViewElementsKey,
                                                            bool linkOriginal) {
        if (FromStoreGate) {
            if (CreateContainerLinks(GateKey, Particles, linkOriginal) == LinkStatus::Failed)
                return StatusCode::FAILURE;
            else
                return StatusCode::SUCCESS;
        } else
            ATH_CHECK(ViewElementsContainer(ViewElementsKey, Particles));

        return StatusCode::SUCCESS;
    }

    StatusCode SUSYTruthSelector::FillTruth(const CP::SystematicSet& systset) {
        if (!SystematicAffects(systset)) return StatusCode::SUCCESS;
        SetSystematics(systset);

        for (const auto& truj_it : *m_PreJets) {
            bool IsB = IsBJet(truj_it);
            if (!PassBaseline(*truj_it)) continue;
            m_BaselineJets->push_back(truj_it);
            if (!PassSignal(*truj_it)) continue;
            m_SignalJets->push_back(truj_it);
            if (IsB)
                m_BJets->push_back(truj_it);
            else
                m_LightJets->push_back(truj_it);
        }

        FillBaselineContainer(m_PreElectrons, m_BaselineElectrons);
        FillBaselineContainer(m_PreMuons, m_BaselineMuons);
        FillBaselineContainer(m_PrePhotons, m_BaselinePhotons);
        FillBaselineContainer(m_PreTaus, m_BaselineTaus);

        FillSignalContainer(m_BaselineElectrons, m_SignalElectrons);
        FillSignalContainer(m_BaselineMuons, m_SignalMuons);
        FillSignalContainer(m_BaselinePhotons, m_SignalPhotons);
        FillSignalContainer(m_BaselineTaus, m_SignalTaus);
        return StatusCode::SUCCESS;
    }
    bool SUSYTruthSelector::IsBJet(const xAOD::IParticle* j) {
        static IntAccessor acc_HardonTruthLabel("HadronConeExclTruthLabelID");
        static IntAccessor acc_ConeTruthLabel("ConeTruthLabelID");
        static IntAccessor acc_PartonTruthLabel("PartonTruthLabelID");
        static IntAccessor acc_GhostBHadron("GhostBHadronsFinalCount");
        static IntAccessor acc_GhostCHadron("GhostCHadronsFinalCount");

        static CharDecorator dec_bjet("bjet");

        if (j->type() != xAOD::Type::ObjectType::Jet) {
            ATH_MSG_WARNING("The IsBJet function is meant for jets, return false!");
            return false;
        }
        int flavor = 0;
        if (acc_HardonTruthLabel.isAvailable(*j))
            flavor = acc_HardonTruthLabel(*j);
        else if (acc_ConeTruthLabel.isAvailable(*j))
            flavor = acc_ConeTruthLabel(*j);
        else if (acc_PartonTruthLabel.isAvailable(*j))
            flavor = abs(acc_PartonTruthLabel(*j));
        else if (acc_GhostBHadron.isAvailable(*j)) {
            if (acc_GhostBHadron(*j) > 0)
                flavor = 5;
            else if (acc_GhostCHadron(*j) > 0)
                flavor = 4;
            else
                flavor = 1;
        }
        bool isB = (flavor == 5) && PassSignal(*j);
        dec_bjet(*j) = isB;
        return isB;
    }

    xAOD::JetContainer* SUSYTruthSelector::GetTruthCustomJets(const std::string& kind) const {
        xAOD::JetContainer* customJets = nullptr;
        if (LoadViewElementsContainer(kind, customJets).isSuccess()) return customJets;
        return m_BaselineJets;
    }

    StatusCode SUSYTruthSelector::ReclusterTruthJets(const xAOD::IParticleContainer* inputJets, float Rcone, float minPtKt4,
                                                     std::string PreFix, float minPtRecl, float rclus, float ptfrac) {
        if (rclus != 0.0)
            ATH_MSG_WARNING("The current value of rclus =  " << rclus << " leads to jet reclustering with a trimming procedure \
not recommended by the JetEtmiss group. If you really want to use this as it is, consider to overwrite this virtual method \
in your derived class to suppress this warning.");

        return ParticleSelector::ReclusterJets(inputJets, Rcone, minPtKt4, PreFix, minPtRecl, rclus, ptfrac);
    }

    void SUSYTruthSelector::FillBaselineContainer(xAOD::TruthParticleContainer* Pre, xAOD::TruthParticleContainer* Baseline) {
        for (const auto it : *Pre) {
            m_truthDecorations->truthType.set(*it, getParticleTruthType(it));
            m_truthDecorations->truthOrigin.set(*it, getParticleTruthOrigin(it));
            if (!PassBaseline(*it)) continue;
            Baseline->push_back(it);
        }
    }
    void SUSYTruthSelector::FillSignalContainer(xAOD::TruthParticleContainer* Baseline, xAOD::TruthParticleContainer* Signal) {
        for (const auto it : *Baseline) {
            if (!PassSignal(*it)) continue;
            Signal->push_back(it);
        }
    }

    bool SUSYTruthSelector::isTrueTop(const xAOD::TruthParticle* particle) {
        if (!particle->isTop() || !particle->hasDecayVtx() || particle->isGenSpecific()) return false;
        unsigned int nW(0), nb(0);
        // Find the Top children
        for (size_t c = 0; c < particle->nChildren(); ++c) {
            const xAOD::TruthParticle* child = particle->decayVtx()->outgoingParticle(c);
            if (!child) continue;
            if (child->isTop())
                return false;
            else if (child->isW()) {
                ATH_MSG_DEBUG("Found a W child of the top, pdgID = " << child->pdgId());
                ++nW;
            } else if (child->absPdgId() == 5) {
                ATH_MSG_DEBUG("Found a b-quark child of the top, pdgID = " << child->pdgId());
                ++nb;
            }
        }
        ATH_MSG_DEBUG("Found n_b-quark = " << nb << "and nW = " << nW << " children of the top.");
        return (nb == 1 && nW == 1);
    }

    int SUSYTruthSelector::classifyWDecays(const xAOD::TruthParticle* particle) {
        if (!particle->isW() || !particle->hasDecayVtx() || particle->isGenSpecific()) return WDecayModes::Unspecified;
        unsigned int nq(0), ne(0), nm(0), nthad(0), ntlep(0), nnu(0);
        // Find the W children
        ATH_MSG_DEBUG("Check W with pdgID = " << particle->pdgId() << ".");
        for (size_t c = 0; c < particle->nChildren(); ++c) {
            const xAOD::TruthParticle* child = particle->decayVtx()->outgoingParticle(c);
            if (!child) continue;
            if (child->isW()) return classifyWDecays(child);  // WDecayModes::Unspecified;
            if (child->isQuark()) {
                ATH_MSG_DEBUG("Found a quark child of the W, pdgID = " << child->pdgId());
                ++nq;
            } else if (child->isElectron()) {
                ATH_MSG_DEBUG("Found an electron child of the W, pdgID = " << child->pdgId());
                ++ne;
            } else if (child->isMuon()) {
                ATH_MSG_DEBUG("Found a muon child of the W, pdgID = " << child->pdgId());
                ++nm;
            } else if (isTrueTau(child)) {
                ATH_MSG_DEBUG("Found a tau child of the W, pdgID = " << child->pdgId());
                char isTauHad = false;
                if (!m_truthDecorations->IsHadronicTau.get(child, isTauHad) || !isTauHad)
                    ++ntlep;
                else
                    ++nthad;
            } else if (child->isNeutrino()) {
                ATH_MSG_DEBUG("Found a neutrino child of the W, pdgID = " << child->pdgId());
                ++nnu;
            }
            ATH_MSG_DEBUG("nq = " << nq << " ne = " << ne << " nm = " << nm << " ntlep = " << ntlep << " nthad = " << nthad
                                  << " nnu = " << nnu << " children of the W");
        }
        if (nq == 2) return WDecayModes::Hadronic;
        if (ne == 1 && nnu == 1) return WDecayModes::ElecNeut;
        if (nm == 1 && nnu == 1) return WDecayModes::MuonNeut;
        if (ntlep == 1 && nnu == 1) return WDecayModes::HadTauNeut;
        if (nthad == 1 && nnu == 1)
            return WDecayModes::LepTauNeut;
        else
            return WDecayModes::Unspecified;
    }
    // This function defines true taus as hadronically decaying taus
    bool SUSYTruthSelector::isTrueTau(const xAOD::TruthParticle* particle) {
        if (!particle->isTau() || !particle->hasDecayVtx() || particle->isGenSpecific()) return false;
        m_truthDecorations->IsHadronicTau.set(particle, true);
        const xAOD::TruthParticle* Neutrino = nullptr;
        int nC = 0;
        for (size_t c = 0; c < particle->nChildren(); ++c) {
            ++nC;
            const xAOD::TruthParticle* child = particle->decayVtx()->outgoingParticle(c);
            if (!child) continue;
            if (child->isTau()) {
                m_truthDecorations->IsHadronicTau.set(particle, false);
                return false;
            }
            if (child->isNeutrino()) Neutrino = child;
            if (child->isChLepton()) { m_truthDecorations->IsHadronicTau.set(particle, false); }
        }
        return (Neutrino != nullptr && nC > 1);
    }
    bool SUSYTruthSelector::isTrueW(const xAOD::TruthParticle* particle) { return classifyWDecays(particle) != WDecayModes::Unspecified; }

    bool SUSYTruthSelector::isTrueZ(const xAOD::TruthParticle* particle) {
        if (!particle->isZ() || !particle->hasDecayVtx() || particle->isGenSpecific()) return false;
        int nC(0), pdgid_C1(0);
        // Find the Z children
        for (size_t c = 0; c < particle->nChildren(); ++c) {
            const xAOD::TruthParticle* child = particle->decayVtx()->outgoingParticle(c);
            if (!child) continue;
            if (child->isZ())
                return false;
            else if (child->isQuark() || child->isChLepton() || child->isNeutrino()) {
                ATH_MSG_DEBUG("Found a fermion child of the Z, pdgID = " << child->pdgId());
                if (pdgid_C1 == 0) {
                    ++nC;
                    pdgid_C1 = child->absPdgId();
                } else if (child->absPdgId() == pdgid_C1)
                    ++nC;
            }
        }
        return nC == 2;
    }

    bool SUSYTruthSelector::isTrueSUSY(const xAOD::TruthParticle* particle) {
        if (!XAMPP::isSparticle(*particle) || particle->isGenSpecific() || (particle != XAMPP::GetLastChainLink(particle))) return false;
        return true;
    }
    int SUSYTruthSelector::GetInitialState() {
        static IntAccessor acc_proc("SUSY_procID");
        // skip if this is not requested by the user - save some CPU...
        if (!m_doSUSYProcess) return 0;
        if (acc_proc.isAvailable(*m_XAMPPInfo->GetOrigInfo())) { return acc_proc(*m_XAMPPInfo->GetOrigInfo()); }

        int State(0), pdgId1(0), pdgId2(0);
        const xAOD::TruthParticleContainer* C = nullptr;
        if (m_xAODTruthParticles)
            C = m_xAODTruthParticles;
        else if (m_xAODTruthBSM)
            C = m_xAODTruthBSM;
        else if (m_xAODTruthBoson)
            C = m_xAODTruthBoson;
        else if (m_xAODTruthTop)
            C = m_xAODTruthTop;
        if (!C) {
            ATH_MSG_WARNING(
                "No input container found to read the final state from. Return "
                "0");
            return State;
        }
        if (!C->empty() && !ST::SUSYObjDef_xAOD::FindSusyHardProc(C, pdgId1, pdgId2, isTRUTH3()))
            ATH_MSG_WARNING(
                "Could not determine the initial SUSY process. May be its "
                "background");
        else if ((pdgId1) || (pdgId2))
            State = SUSY::finalState(pdgId1, pdgId2);
        return State;
    }
    bool SUSYTruthSelector::isTRUTH3() const { return m_isTRUTH3; }
    void SUSYTruthSelector::LoadDressedMomentum(xAOD::TruthParticle* Truth) {
        if (!Truth->isChLepton() || Truth->isTau()) {
            ATH_MSG_DEBUG("The truth particle with pt: " << Truth->pt() / 1.e3 << " GeV, eta:" << Truth->eta() << ", phi: " << Truth->phi()
                                                         << ", pdgId:" << Truth->pdgId());
            return;
        }
        char loaded_p4 = 0;
        float e_aux = 0;
        float pt_aux = 0;
        float eta_aux = 0;
        float phi_aux = 0;
        if (!m_truthDecorations->loaded_p4.get(Truth, loaded_p4) || loaded_p4 != (char)TruthDecorations::p4LoadStatus::Dressed) {
            DressVanillaMomentum(Truth);
            if (m_truthDecorations->e_dressed.get(Truth, e_aux) && m_truthDecorations->eta_dressed.get(Truth, eta_aux) &&
                m_truthDecorations->phi_dressed.get(Truth, phi_aux) && m_truthDecorations->pt_dressed.get(Truth, pt_aux)) {
                m_truthDecorations->loaded_p4.set(Truth, (char)TruthDecorations::p4LoadStatus::Dressed);
                TLorentzVector V;
                V.SetPtEtaPhiE(pt_aux, eta_aux, phi_aux, e_aux);
                Truth->setPx(V.Px());
                Truth->setPy(V.Py());
                Truth->setPz(V.Pz());
                Truth->setE(V.E());
                Truth->setM(V.M());
            }
        }
    }
    void SUSYTruthSelector::LoadVisibleMomentum(xAOD::TruthParticle* Truth) {
        if (!Truth->isTau()) {
            ATH_MSG_DEBUG("The truth particle with pt: " << Truth->pt() / 1.e3 << " GeV, eta:" << Truth->eta() << ", phi: " << Truth->phi()
                                                         << ", pdgId:" << Truth->pdgId());
            return;
        }
        char loaded_p4 = 0;
        double m_aux = 0;
        double pt_aux = 0;
        double eta_aux = 0;
        double phi_aux = 0;

        if (!m_truthDecorations->loaded_p4.get(Truth, loaded_p4) || loaded_p4 != (char)TruthDecorations::p4LoadStatus::VisibleTauMom) {
            DressVanillaMomentum(Truth);
            if (m_truthDecorations->m_visible.get(Truth, m_aux) && m_truthDecorations->eta_visible.get(Truth, eta_aux) &&
                m_truthDecorations->phi_visible.get(Truth, phi_aux) && m_truthDecorations->pt_visible.get(Truth, pt_aux)) {
                m_truthDecorations->loaded_p4.set(Truth, (char)TruthDecorations::p4LoadStatus::VisibleTauMom);
                TLorentzVector V;
                V.SetPtEtaPhiM(pt_aux, eta_aux, phi_aux, m_aux);
                Truth->setPx(V.Px());
                Truth->setPy(V.Py());
                Truth->setPz(V.Pz());
                Truth->setE(V.E());
                Truth->setM(V.M());
            }
        }
    }
    void SUSYTruthSelector::DressVanillaMomentum(const xAOD::TruthParticle* Truth) {
        // avail && 1 : return
        // not avail: dress
        // avail and not 0: complain
        char loaded = 0;
        bool gotIt = m_truthDecorations->loaded_p4.get(Truth, loaded);
        if (gotIt) {
            // already loaded vanilla
            if (loaded == (char)TruthDecorations::p4LoadStatus::Vanilla)
                return;
            else if (loaded != (char)TruthDecorations::p4LoadStatus::NothingLoaded) {
                ATH_MSG_WARNING("The truth particle with pt: " << Truth->pt() / 1.e3 << " GeV, eta:" << Truth->eta()
                                                               << ", phi: " << Truth->phi() << ", pdgId:" << Truth->pdgId()
                                                               << " has not the vanilla momentum loaded");
                return;
            }
        }
        m_truthDecorations->loaded_p4.set(Truth, (char)TruthDecorations::p4LoadStatus::Vanilla);
        m_truthDecorations->m_orig.set(Truth, Truth->m());
        m_truthDecorations->pt_orig.set(Truth, Truth->pt());
        m_truthDecorations->eta_orig.set(Truth, Truth->eta());
        m_truthDecorations->phi_orig.set(Truth, Truth->phi());
    }

    bool SUSYTruthSelector::BaselineKinematics(const xAOD::IParticle& P, const ObjectDefinition& obj) const {
        if (!obj.doObject) return false;
        if (P.pt() < obj.BaselinePt) return false;
        if (obj.BaselineEta > 0 && fabs(P.eta()) > obj.BaselineEta) return false;
        if (obj.hasBaseEtaToExclude && IsInEtaRange(P, obj.BaseEtaExclude)) return false;
        return true;
    }
    bool SUSYTruthSelector::SignalKinematics(const xAOD::IParticle& P, const ObjectDefinition& obj) const {
        if (!obj.doObject) return false;
        if (P.pt() < obj.SignalPt) return false;
        if (obj.SignalEta > 0 && fabs(P.eta()) > obj.SignalEta) return false;
        if (obj.hasBaseEtaToExclude && IsInEtaRange(P, obj.BaseEtaExclude)) return false;
        return true;
    }

    bool SUSYTruthSelector::PassBaselineKinematics(const xAOD::IParticle& P) const {
        if (P.type() == xAOD::Type::ObjectType::Jet)
            return BaselineKinematics(P, m_JetDefs);
        else if (P.type() == xAOD::Type::ObjectType::TruthParticle) {
            unsigned int pdgId = fabs(TypeToPdgId(P));
            // Electrons
            if (pdgId == 11) return BaselineKinematics(P, m_ElectronDefs);
            // Muons
            else if (pdgId == 13)
                return BaselineKinematics(P, m_MuonDefs);
            // Taus
            else if (pdgId == 15)
                return BaselineKinematics(P, m_TauDefs);
            // Neutrinos
            else if (pdgId == 12 || pdgId == 14 || pdgId == 16)
                return BaselineKinematics(P, m_NeutrinoDefs);
            // Photons
            else if (pdgId == 22)
                return BaselineKinematics(P, m_PhotonDefs);
        }
        return true;
    }
    bool SUSYTruthSelector::PassSignalKinematics(const xAOD::IParticle& P) const {
        if (P.type() == xAOD::Type::ObjectType::Jet)
            return SignalKinematics(P, m_JetDefs);
        else if (P.type() == xAOD::Type::ObjectType::TruthParticle) {
            unsigned int pdgId = fabs(TypeToPdgId(P));
            // Electrons
            if (pdgId == 11) return SignalKinematics(P, m_ElectronDefs);
            // Muons
            else if (pdgId == 13)
                return SignalKinematics(P, m_MuonDefs);
            // Taus
            else if (pdgId == 15)
                return SignalKinematics(P, m_TauDefs);
            // Neutrinos
            else if (pdgId == 12 || pdgId == 14 || pdgId == 16)
                return SignalKinematics(P, m_NeutrinoDefs);
            // Photons
            else if (pdgId == 22)
                return SignalKinematics(P, m_PhotonDefs);
        }
        return true;
    }
    bool SUSYTruthSelector::PassSignal(const xAOD::IParticle& P) const {
        if (!ParticleSelector::PassSignal(P)) return false;
        if (m_RequireSignalTauHad && fabs(TypeToPdgId(P)) == 15) {
            char isTauHad = false;
            if (!m_truthDecorations->IsHadronicTau.get(&P, isTauHad) || !isTauHad) { return false; }
        }
        if (P.type() == xAOD::Type::ObjectType::Jet || !m_RequireSignalFromHardProc) return true;
        const xAOD::TruthParticle* truthPart = dynamic_cast<const xAOD::TruthParticle*>(&P);
        if (!truthPart)
            PromptParticle(truthPart, "What is that for a particle???");
        else
            return isParticleFromHardProcess(truthPart, m_rejectUnknownOrigin);
        return false;
    }
    bool SUSYTruthSelector::PassBaseline(const xAOD::IParticle& P) const {
        if (!ParticleSelector::PassBaseline(P)) return false;
        if (m_RequireBaseTauHad && fabs(TypeToPdgId(P)) == 15) {
            char isTauHad = false;
            if (!m_truthDecorations->IsHadronicTau.get(&P, isTauHad) || !isTauHad) { return false; }
        }
        if (P.type() == xAOD::Type::ObjectType::Jet || !m_RequireBaseFromHardProc) return true;

        // Check if the particle originates from the HardProcess or is a fake
        const xAOD::TruthParticle* truthPart = dynamic_cast<const xAOD::TruthParticle*>(&P);
        if (!truthPart)
            PromptParticle(truthPart, "What is that for a particle???");
        else
            return isParticleFromHardProcess(truthPart, m_rejectUnknownOrigin);
        return false;
    }
    bool SUSYTruthSelector::PassPreSelection(const xAOD::IParticle& P) const {
        if (!ParticleSelector::PassPreSelection(P)) return false;
        if (P.type() == xAOD::Type::ObjectType::Jet) return true;
        if (m_RequirePreSelTauHad && fabs(TypeToPdgId(P)) == 15) {
            char isTauHad = false;
            if (!m_truthDecorations->IsHadronicTau.get(&P, isTauHad) || !isTauHad) { return false; }
        }
        if (!m_RequirePreselFromHardProc) return true;
        // Check if the particle originates from the HardProcess or is a fake
        const xAOD::TruthParticle* truthPart = dynamic_cast<const xAOD::TruthParticle*>(&P);
        if (!truthPart)
            PromptParticle(truthPart, "What is that for a particle???");
        else
            return isParticleFromHardProcess(truthPart, m_rejectUnknownOrigin);

        return false;
    }
    bool SUSYTruthSelector::doTruthJets() const { return m_JetDefs.doObject; }
    bool SUSYTruthSelector::doTruthParticles() const { return m_doTruthParticles; }
    std::string SUSYTruthSelector::TauKey() const { return m_TauDefs.ContainerKey; }
    std::string SUSYTruthSelector::ElectronKey() const { return m_ElectronDefs.ContainerKey; }
    std::string SUSYTruthSelector::MuonKey() const { return m_MuonDefs.ContainerKey; }
    std::string SUSYTruthSelector::PhotonKey() const { return m_PhotonDefs.ContainerKey; }
    std::string SUSYTruthSelector::NeutrinoKey() const { return m_NeutrinoDefs.ContainerKey; }
    std::string SUSYTruthSelector::BosonKey() const { return m_BosonKey; }
    std::string SUSYTruthSelector::BSMKey() const { return m_BSMKey; }
    std::string SUSYTruthSelector::TopKey() const { return m_TopKey; }
    std::string SUSYTruthSelector::JetKey() const { return m_JetDefs.ContainerKey; }

    void SUSYTruthSelector::setupDecorations(std::shared_ptr<TruthDecorations> input) {
        // as for the particle selector, use the input if provided, or use a default
        // if not set before.
        if (input) { m_truthDecorations = input; }
        if (!m_truthDecorations) { m_truthDecorations = std::make_shared<TruthDecorations>(); }
        // also set up the particle selector decoration members
        ParticleSelector::setupDecorations(m_truthDecorations);
    }
}  // namespace XAMPP
