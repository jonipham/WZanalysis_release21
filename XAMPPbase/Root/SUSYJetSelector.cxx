#include <XAMPPbase/SUSYJetSelector.h>
#include <xAODJet/JetContainerInfo.h>

#include <JetJvtEfficiency/JetJvtEfficiency.h>
#include <xAODBTaggingEfficiency/BTaggingEfficiencyTool.h>

namespace XAMPP {

    //###########################################
    //                  SUSYJetSelector         #
    //###########################################
    SUSYJetSelector::SUSYJetSelector(const std::string& myname) :
        SUSYParticleSelector(myname),
        m_xAODJets(nullptr),
        m_Jets(nullptr),
        m_PreJets(nullptr),
        m_BaselineJets(nullptr),
        m_PreJVTJets(nullptr),
        m_SignalJets(nullptr),
        m_SignalQualJets(nullptr),
        m_PreFatJets(nullptr),
        m_BJets(nullptr),
        m_LightJets(nullptr),
        m_jetDecorations(nullptr),
        m_JvtTool(""),
        m_JetCollectionType(xAOD::JetInput::EMTopo),
        m_decNBad(nullptr),
        m_bJetEtaCut(-1.),
        m_SeparateSF(false),
        m_SF(),
        m_doBTagSF(true),
        m_doJVTSF(true),
        m_doLargeRdecors(true),
        m_Kt10_BaselinePt(0),
        m_Kt10_BaselineEta(-1),
        m_Kt10_SignalPt(0),
        m_Kt10_SignalEta(-1),
        m_Kt02_BaselinePt(0),
        m_Kt02_BaselineEta(-1),
        m_Kt02_SignalPt(0),
        m_Kt02_SignalEta(-1),
        m_Kt02_ORUtils_flag(2),
        m_Kt10_ORUtils_flag(2),
        m_BtagTool("") {
        declareProperty("bJetEtaCut", m_bJetEtaCut);
        declareProperty("SeparateSF",
                        m_SeparateSF);  // if this option is switched on the jet
                                        // SF are saved split into BTag and JVT
        declareProperty("ApplyBTagSF", m_doBTagSF);
        declareProperty("ApplyJVTSF", m_doJVTSF);
        declareProperty("BosonTagging", m_doLargeRdecors);

        declareProperty("AntiKt10_BaselinePt", m_Kt10_BaselinePt);
        declareProperty("AntiKt10_BaselineEta", m_Kt10_BaselineEta);
        declareProperty("AntiKt10_SignalPt", m_Kt10_SignalPt);
        declareProperty("AntiKt10_SignalEta", m_Kt10_SignalEta);
        declareProperty("AntiKt10_ORUtilsInputFlag", m_Kt10_ORUtils_flag);

        declareProperty("AntiKt02_BaselinePt", m_Kt02_BaselinePt);
        declareProperty("AntiKt02_BaselineEta", m_Kt02_BaselineEta);
        declareProperty("AntiKt02_SignalPt", m_Kt02_SignalPt);
        declareProperty("AntiKt02_SignalEta", m_Kt02_SignalEta);
        declareProperty("AntiKt02_ORUtilsInputFlag", m_Kt02_ORUtils_flag);

        declareProperty("JetCollectionType", m_JetCollectionType);
        declareProperty("JvtEfficiencyTool", m_JvtTool);
        declareProperty("BTagEfficiencyTool", m_BtagTool);

        SetObjectType(XAMPP::SelectionObject::Jet);
    }

    SUSYJetSelector::~SUSYJetSelector() {}

    StatusCode SUSYJetSelector::SetupSelection() {
        SetContainerKey("AntiKt4" + xAOD::JetInput::typeName(xAOD::JetInput::Type(m_JetCollectionType)) + "Jets");
        ATH_MSG_INFO("Use " + ContainerKey() + " container for the standard anti-kt4 jets");
        ATH_CHECK(m_XAMPPInfo->NewEventVariable<int>("BadJet", false));
        m_decNBad = m_XAMPPInfo->GetVariableStorage<int>("BadJet");
        m_doBTagSF &= m_systematics->ProcessObject(XAMPP::SelectionObject::BTag);
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYJetSelector::initialize() {
        if (isInitialized()) { return StatusCode::SUCCESS; }

        ATH_CHECK(SUSYParticleSelector::initialize());
        ATH_CHECK(SetupSelection());
        if (!ProcessObject()) { return StatusCode::SUCCESS; }
        setupDecorations();
        m_JvtTool = GetCPTool<CP::IJetJvtEfficiency>("JetJvtEfficiencyTool");
        if (m_BtagTool.empty()) m_BtagTool = GetCPTool<IBTaggingEfficiencyTool>("BTaggingEfficiencyTool");
        ATH_CHECK(SetupScaleFactors());
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYJetSelector::SetupScaleFactors() {
        if (isData()) return StatusCode::SUCCESS;
        ATH_CHECK(m_JvtTool.retrieve());
        ATH_CHECK(m_BtagTool.retrieve());

        JvtJetWeightMap JvtSFs;
        BTagJetWeightMap BTagSFs;

        std::vector<const CP::SystematicSet*> WeightSyst;
        CopyVector(m_systematics->GetWeightSystematics(XAMPP::SelectionObject::BTag), WeightSyst, false);
        CopyVector(m_systematics->GetWeightSystematics(ObjectType()), WeightSyst, false);
        if (WeightSyst.empty()) {
            ATH_MSG_INFO("Declare the weight systematics");
            ATH_CHECK(DeclareAsWeightSyst(m_JvtTool));
            ATH_CHECK(DeclareAsWeightSyst(m_BtagTool, XAMPP::SelectionObject::BTag));
            CopyVector(m_systematics->GetWeightSystematics(XAMPP::SelectionObject::BTag), WeightSyst, false);
            CopyVector(m_systematics->GetWeightSystematics(ObjectType()), WeightSyst, false);
        }

        for (const auto syst_set : WeightSyst) {
            if (m_doJVTSF && XAMPP::ToolIsAffectedBySystematic(m_JvtTool, syst_set)) {
                JvtJetWeight_Ptr Jvt(new JvtJetWeight(m_JvtTool));
                ATH_CHECK(initIParticleWeight(*Jvt, "JVT", syst_set, ScaleFactorMapContains::SignalSf, m_SeparateSF, "Jet"));
                if (JvtSFs.find(syst_set) != JvtSFs.end()) {
                    ATH_MSG_FATAL("The Jvt SF's already exist for systematic " << syst_set->name());
                    return StatusCode::FAILURE;
                }
                JvtSFs.insert(std::pair<const CP::SystematicSet*, JvtJetWeight_Ptr>(syst_set, Jvt));
            }
            if (m_doBTagSF && XAMPP::ToolIsAffectedBySystematic(m_BtagTool, syst_set)) {
                BTagJetWeight_Ptr BTag(new BTagJetWeight(m_BtagTool));
                BTag->SetBJetEtaCut(m_bJetEtaCut);
                ATH_CHECK(initIParticleWeight(*BTag, "BTag", syst_set, ScaleFactorMapContains::SignalSf, m_SeparateSF, "Jet"));
                if (BTagSFs.find(syst_set) != BTagSFs.end()) {
                    ATH_MSG_FATAL("The BTag SF's already exist for systematic " << syst_set->name());
                    return StatusCode::FAILURE;
                }
                BTagSFs.insert(std::pair<const CP::SystematicSet*, BTagJetWeight_Ptr>(syst_set, BTag));
            }
        }
        for (auto syst_set : WeightSyst) {
            JetWeightHandler_Ptr Handler(new JetWeightHandler(syst_set));
            bool append = false;
            if (Handler->append(BTagSFs, m_systematics->GetNominal())) append = true;
            if (Handler->append(JvtSFs, m_systematics->GetNominal())) append = true;
            if (append) {
                ATH_CHECK(initIParticleWeight(*Handler, "", syst_set, ScaleFactorMapContains::SignalSf, true, "Jet"));
                // make sure the handler understands our jet decorations
                Handler->setupDecorations(m_jetDecorations);
                m_SF.push_back(Handler);
            }
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYJetSelector::LoadContainers() {
        if (!ProcessObject()) return StatusCode::SUCCESS;
        for (auto& ScaleFactors : m_SF) { ATH_CHECK(ScaleFactors->initEvent()); }
        return StatusCode::SUCCESS;
    }

    xAOD::JetContainer* SUSYJetSelector::GetCustomJets(const std::string& kind) const {
        xAOD::JetContainer* customJets = nullptr;
        if (LoadViewElementsContainer(kind, customJets).isSuccess()) return customJets;
        return GetBaselineJets();
    }

    StatusCode SUSYJetSelector::InitialFill(const CP::SystematicSet& systset) {
        SetSystematics(systset);
        if (ProcessObject())
            ATH_CHECK(CalibrateJets(ContainerKey(), m_Jets, m_PreJets));
        else {
            ATH_CHECK(ViewElementsContainer("container", m_Jets));
            ATH_CHECK(ViewElementsContainer("presel", m_PreJets));
        }
        ATH_CHECK(ViewElementsContainer("PreFattys", m_PreFatJets));
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYJetSelector::CalibrateJets(const std::string& Key, xAOD::JetContainer*& Container, xAOD::JetContainer*& PreSelected,
                                              const std::string& PreSelName, JetAlgorithm Cone) {
        xAOD::ShallowAuxContainer* AuxContainer = nullptr;

        LinkStatus Link = CreateContainerLinks(Key, Container, AuxContainer);

        if (Link == LinkStatus::Failed)
            return StatusCode::FAILURE;
        else if (Link == LinkStatus::Created) {
            if (Cone == JetAlgorithm::AntiKt10) {
                ATH_CHECK(preCalibCorrection(Container));
                ATH_CHECK(m_susytools->GetFatJets(Container, AuxContainer, false, Key, m_doLargeRdecors));
            } else if (Cone == JetAlgorithm::AntiKt2) {
                ATH_CHECK(m_susytools->GetTrackJets(Container, AuxContainer, false, Key));
            } else if (Cone == JetAlgorithm::AntiKt4) {
                if (m_XAMPPInfo->GetSystematic() == m_systematics->GetNominal()) {
                    ATH_CHECK(m_susytools->GetJets(Container, AuxContainer, false));
                } else {
                    xAOD::JetContainer* nominal_container = nullptr;
                    ATH_CHECK(LoadContainer(name() + "_" + Key, nominal_container));

                    xAOD::JetContainer::const_iterator nominal_begin = nominal_container->begin();
                    xAOD::JetContainer::const_iterator nominal_end = nominal_container->end();

                    xAOD::JetContainer::iterator copy_begin = Container->begin();
                    xAOD::JetContainer::iterator copy_end = Container->end();

                    for (; copy_begin != copy_end && nominal_begin != nominal_end; ++nominal_begin, ++copy_begin) {
                        (**copy_begin) = (**nominal_begin);
                        ATH_CHECK(m_susytools->FillJet(**copy_begin, false));
                        m_susytools->IsBadJet(**copy_begin);
                        m_susytools->IsSignalJet(**copy_begin, -1, 10);
                    }
                }
            }
            ATH_CHECK(ViewElementsContainer(PreSelName.empty() ? "JetPreSel" : PreSelName, PreSelected));
            for (const auto& Jet : *Container) {
                m_jetDecorations->jetAlgorithm.set(*Jet, Cone);
                if (Cone == JetAlgorithm::AntiKt2) {
                    SetSelectionDecorators(*Jet, true);
                    SetOverlapInDecorator(*Jet, m_Kt02_ORUtils_flag);
                }
                if (Cone == JetAlgorithm::AntiKt10 && PassPreSelection(*Jet)) SetOverlapInDecorator(*Jet, m_Kt10_ORUtils_flag);
                if (PassPreSelection(*Jet)) { PreSelected->push_back(Jet); }
            }
            PreSelected->sort(XAMPP::ptsorter);
        } else if (Link == LinkStatus::Loaded) {
            ATH_CHECK(LoadViewElementsContainer((PreSelName.empty() ? "JetPreSel" : PreSelName), PreSelected, true));
        }
        return StatusCode::SUCCESS;
    }
    StatusCode SUSYJetSelector::CalibrateJets(const std::string& Key, xAOD::JetContainer*& PreSelected, const std::string& PreSelName,
                                              JetAlgorithm Cone) {
        xAOD::JetContainer* Container;
        ATH_CHECK(CalibrateJets(Key, Container, PreSelected, PreSelName, Cone));
        return StatusCode::SUCCESS;
    }
    bool SUSYJetSelector::IsBadJet(const xAOD::Jet& jet) const {
        char isBad = false;
        if (!m_jetDecorations->isBadJet.get(jet, isBad)) {
            ATH_MSG_WARNING("Failed to read bad jet flag");
            return false;
        }
        return isBad && SUSYParticleSelector::PassBaseline(jet);
    }
    double SUSYJetSelector::BtagBDT(const xAOD::Jet& jet) const {
        double weight_MV2c10(0.);
        if (jet.btagging() != nullptr) jet.btagging()->MVx_discriminant("MV2c10", weight_MV2c10);
        return weight_MV2c10;
    }
    StatusCode SUSYJetSelector::FillJets(const CP::SystematicSet& systset) {
        SetSystematics(systset);
        ATH_CHECK(ViewElementsContainer("baseline", m_BaselineJets));
        // These jets are there to get the JVT ineffieciencies
        ATH_CHECK(ViewElementsContainer("noJVTsignal", m_PreJVTJets));

        ATH_CHECK(ViewElementsContainer("signal", m_SignalJets));
        ATH_CHECK(ViewElementsContainer("GQobj", m_SignalQualJets));

        ATH_CHECK(ViewElementsContainer("bjet", m_BJets));
        ATH_CHECK(ViewElementsContainer("light", m_LightJets));

        int NBadJets = 0;
        for (const auto& ijet : *m_PreJets) {
            if (IsBadJet(*ijet)) {
                ++NBadJets;
                continue;
            }
            m_jetDecorations->MV2c10.set(*ijet, BtagBDT(*ijet));
            std::vector<int> nTrkVec;
            ijet->getAttribute(xAOD::JetAttribute::NumTrkPt500, nTrkVec);
            m_jetDecorations->nTracks.set(*ijet, nTrkVec.at(m_XAMPPInfo->GetPrimaryVertex()->index()));

            if (PassBaseline(*ijet)) { m_BaselineJets->push_back(ijet); }
            if (!PassSignalKinematics(*ijet)) continue;
            if (PassSignalNoOR(*ijet)) m_SignalQualJets->push_back(ijet);
            m_PreJVTJets->push_back(ijet);
            if (!PassSignal(*ijet)) continue;
            m_SignalJets->push_back(ijet);

            if (IsBJet(*ijet))
                m_BJets->push_back(ijet);
            else
                m_LightJets->push_back(ijet);
        }

        // for top reconstruction, b-jets should be sorted by b-tag weight
        m_BJets->sort(XAMPP::btagweightsorter);
        ATH_CHECK(m_decNBad->Store(NBadJets));

        ATH_MSG_DEBUG("Number of all jets: " << m_Jets->size());
        ATH_MSG_DEBUG("Number of preselected jets: " << m_PreJets->size());
        ATH_MSG_DEBUG("Number of selected baseline jets: " << m_BaselineJets->size());
        ATH_MSG_DEBUG("Number of selected signal jets: " << m_SignalJets->size());
        ATH_MSG_DEBUG("Number of selected b-jets: " << m_BJets->size());
        return StatusCode::SUCCESS;
    }
    bool SUSYJetSelector::IsBJet(const xAOD::Jet& jet) const {
        char isB = false;
        if (!m_jetDecorations->isBJet.get(jet, isB)) {
            ATH_MSG_WARNING("Unable to read b-jet deco");
            return false;
        }
        return (isB && (m_bJetEtaCut < 0 || fabs(jet.eta()) < m_bJetEtaCut));
    }
    StatusCode SUSYJetSelector::ReclusterJets(const xAOD::IParticleContainer* inputJets, float Rcone, float minPtKt4, std::string PreFix,
                                              float minPtRecl, float rclus, float ptfrac) {
        if (rclus != 0.0)
            ATH_MSG_WARNING("The current value of rclus =  " << rclus << " leads to jet reclustering with a trimming procedure \
not recommended by the JetEtmiss group. If you really want to use this as it is, consider to overwrite this virtual method \
in your derived class to suppress this warning.");

        return ParticleSelector::ReclusterJets(inputJets, Rcone, minPtKt4, PreFix, minPtRecl, rclus, ptfrac);
    }

    StatusCode SUSYJetSelector::SaveScaleFactor() {
        if (m_SF.empty()) return StatusCode::SUCCESS;
        const CP::SystematicSet* kineSet = m_systematics->GetCurrent();
        // The JvtEfficiency tool requires truth matching
        // https://gitlab.cern.ch/atlas/athena/blob/21.2/Reconstruction/Jet/JetJvtEfficiency/Root/JetJvtEfficiency.cxx#L159
        // Do the truth matching once per container and not once
        ATH_MSG_DEBUG("Do truth matching for JvtEfficiency");
        if (m_doJVTSF && SystematicAffects(kineSet)) {
            const xAOD::JetContainer* truthJets = nullptr;
            ATH_CHECK(LoadContainer("AntiKt4TruthJets", truthJets));
            // Only for the nominal systematic we need to tag the full Jet
            // Container The others are only tagged once
            xAOD::JetContainer* JetsToTag = kineSet == m_systematics->GetNominal() ? GetPreJets() : GetSignalJetsNoJVT();
            if (!m_SF.empty()) ATH_CHECK((*m_SF.begin())->tagTruth(JetsToTag, truthJets));
        }
        // If only B-tagging is performed then -> pass SignalJets to the SF
        // evaluation
        xAOD::JetContainer* SFjets = m_doJVTSF ? GetSignalJetsNoJVT() : GetSignalJets();
        ATH_MSG_DEBUG("Save SF of " << SFjets->size() << " jets");
        for (auto& JetSF : m_SF) {
            if (kineSet != m_systematics->GetNominal() && JetSF->systematic() != m_systematics->GetNominal()) continue;
            ATH_CHECK(m_systematics->setSystematic(JetSF->systematic()));
            for (auto jet : *SFjets) { ATH_CHECK(JetSF->saveSF(*jet)); }
            ATH_CHECK(JetSF->applySF());
        }
        return StatusCode::SUCCESS;
    }
    bool SUSYJetSelector::isFromAlgorithm(const xAOD::IParticle& jet, XAMPP::JetAlgorithm alg) const {
        int jetAlg = -1;
        if (!m_jetDecorations->jetAlgorithm.get(jet, jetAlg)) {
            ATH_MSG_ERROR("No idea what jet algoritm was used to built the jet");
            PromptParticle(jet);
            return false;
        }
        return jetAlg == alg;
    }
    bool SUSYJetSelector::isFromAlgorithm(const xAOD::IParticle* jet, XAMPP::JetAlgorithm alg) const { return isFromAlgorithm(*jet, alg); }
    bool SUSYJetSelector::PassBaselineKinematics(const xAOD::IParticle& P) const {
        int jetAlg = -1;
        if (!m_jetDecorations->jetAlgorithm.get(P, jetAlg)) {
            ATH_MSG_ERROR("No idea what jet algoritm was used to built the jet");
            PromptParticle(&P);
            return false;
        }
        switch (jetAlg) {
            case JetAlgorithm::AntiKt4: return ParticleSelector::PassBaselineKinematics(P); break;
            case JetAlgorithm::AntiKt2:
                return (P.pt() > m_Kt02_BaselinePt && (m_Kt02_BaselineEta < 0 || fabs(P.eta()) < m_Kt02_BaselineEta));
                break;
            case JetAlgorithm::AntiKt10:
                return (P.pt() > m_Kt10_BaselinePt && (m_Kt10_BaselineEta < 0 || fabs(P.eta()) < m_Kt10_BaselineEta));
                break;
            default: ATH_MSG_WARNING("The current flag is unknown " << jetAlg);
        }
        return false;
    }
    bool SUSYJetSelector::PassSignalKinematics(const xAOD::IParticle& P) const {
        int jetAlg = -1;
        if (!m_jetDecorations->jetAlgorithm.get(P, jetAlg)) {
            ATH_MSG_ERROR("No idea what jet algoritm was used to built the jet");
            PromptParticle(&P);
            return false;
        }
        switch (jetAlg) {
            case JetAlgorithm::AntiKt4: return ParticleSelector::PassSignalKinematics(P); break;
            case JetAlgorithm::AntiKt2:
                return (P.pt() > m_Kt02_SignalPt && (m_Kt02_SignalEta < 0 || fabs(P.eta()) > m_Kt02_SignalEta));
                break;
            case JetAlgorithm::AntiKt10:
                return (P.pt() > m_Kt10_SignalPt && (m_Kt10_SignalEta < 0 || fabs(P.eta()) > m_Kt10_SignalEta));
                break;
            default: ATH_MSG_WARNING("The current flag is unknown " << jetAlg);
        }
        return false;
    }

    void SUSYJetSelector::setupDecorations(std::shared_ptr<JetDecorations> input) {
        // as for the particle selector, use the input if provided, or use a default
        // if not set before.
        if (input) { m_jetDecorations = input; }
        if (!m_jetDecorations) { m_jetDecorations = std::make_shared<JetDecorations>(); }
        // also set up the particle selector decoration members
        ParticleSelector::setupDecorations(m_jetDecorations);
    }

    //##########################################################################
    //                            JetWeightDecorator
    //##########################################################################
    JetWeightDecorator::JetWeightDecorator() : IPartilceWeightDecorator(), m_jetDecorations(nullptr) {}
    JetWeightDecorator::~JetWeightDecorator() {}
    StatusCode JetWeightDecorator::saveSF(const xAOD::Jet& Jet) {
        float SF = 1.;
        if (!isSFcalculated(Jet)) {
            if (!calculateScaleFactor(Jet, SF).isSuccess()) return StatusCode::FAILURE;
        } else
            SF = getSF(Jet);
        return saveEventSF(Jet, SF, true);
    }
    StatusCode JetWeightDecorator::calculateScaleFactor(const xAOD::Jet& Jet, float& SF) {
        if (PassSelection(Jet)) return calculateEfficiencySF(Jet, SF);
        return calculateInefficiencySF(Jet, SF);
    }
    void JetWeightDecorator::setupDecorations(std::shared_ptr<JetDecorations> decos) { m_jetDecorations = decos; }
    //##########################################################################
    //                            BTagJetWeight
    //##########################################################################
    BTagJetWeight::BTagJetWeight(ToolHandle<IBTaggingEfficiencyTool>& SFTool) : JetWeightDecorator(), m_SFTool(SFTool), m_bJetEtaCut(-1) {}
    BTagJetWeight::~BTagJetWeight() {}
    StatusCode BTagJetWeight::calculateEfficiencySF(const xAOD::Jet& Jet, float& SF) {
        // Usually return a failure, but B-tagging has become a pain in the ass
        // at the moment Just ignore it
        if (m_bJetEtaCut > 0 && fabs(Jet.eta()) > m_bJetEtaCut) return StatusCode::SUCCESS;
        if (m_SFTool->getScaleFactor(Jet, SF) == CP::CorrectionCode::Error) return StatusCode::SUCCESS;
        return StatusCode::SUCCESS;
    }
    StatusCode BTagJetWeight::calculateInefficiencySF(const xAOD::Jet& Jet, float& SF) {
        // Usually return a failure, but B-tagging has become a pain in the ass
        // at the moment Just ignore it
        if (m_bJetEtaCut > 0 && fabs(Jet.eta()) > m_bJetEtaCut) return StatusCode::SUCCESS;
        if (m_SFTool->getInefficiencyScaleFactor(Jet, SF) == CP::CorrectionCode::Error) return StatusCode::SUCCESS;
        return StatusCode::SUCCESS;
    }
    bool BTagJetWeight::PassSelection(const xAOD::Jet& jet) const { return m_jetDecorations->isBJet(jet); }
    //##########################################################################
    //                            JvtJetWeight
    //##########################################################################
    JvtJetWeight::JvtJetWeight(ToolHandle<CP::IJetJvtEfficiency>& SFTool) :
        JetWeightDecorator(),
        m_SFTool_Raw_Ptr(nullptr),
        m_SFTool(SFTool) {
        const CP::JetJvtEfficiency* Ptr = dynamic_cast<const CP::JetJvtEfficiency*>(SFTool.operator->());
        m_SFTool_Raw_Ptr = const_cast<CP::JetJvtEfficiency*>(Ptr);
    }
    JvtJetWeight::~JvtJetWeight() {}
    bool JvtJetWeight::PassSelection(const xAOD::Jet& jet) const { return m_jetDecorations->passJVT(jet); }
    StatusCode JvtJetWeight::calculateEfficiencySF(const xAOD::Jet& Jet, float& SF) {
        if (m_SFTool->getEfficiencyScaleFactor(Jet, SF) == CP::CorrectionCode::Error) return StatusCode::FAILURE;
        return StatusCode::SUCCESS;
    }
    StatusCode JvtJetWeight::calculateInefficiencySF(const xAOD::Jet& Jet, float& SF) {
        if (m_SFTool->getInefficiencyScaleFactor(Jet, SF) == CP::CorrectionCode::Error) return StatusCode::FAILURE;
        return StatusCode::SUCCESS;
    }
    StatusCode JvtJetWeight::tagTruth(const xAOD::IParticleContainer* jets, const xAOD::IParticleContainer* truthJets) {
        return m_SFTool_Raw_Ptr->tagTruth(jets, truthJets);
    }
    //##########################################################################
    //                            JetWeightHandler
    //##########################################################################
    JetWeightHandler::JetWeightHandler(const CP::SystematicSet* set) :
        IPartilceWeightDecorator(),
        m_syst(set),
        m_JvtWeighter(),
        m_BTagWeighter() {}
    JetWeightHandler::~JetWeightHandler() {}
    const CP::SystematicSet* JetWeightHandler::systematic() const { return m_syst; }
    StatusCode JetWeightHandler::tagTruth(const xAOD::IParticleContainer* jets, const xAOD::IParticleContainer* truthJets) {
        if (m_JvtWeighter) return m_JvtWeighter->tagTruth(jets, truthJets);
        // Print the warning once per job
        static bool Warned = false;
        if (!Warned) Warning("JetWeightHandler::tagTruth()", "No jvt interfaces given");
        Warned = true;
        return StatusCode::SUCCESS;
    }
    StatusCode JetWeightHandler::saveSF(const xAOD::Jet& Jet) {
        double SF = 1.;
        if (!m_JvtWeighter && !m_BTagWeighter) {
            Error("JetWeightHandler()", "Please give me anything to retrieve a SF from");
            return StatusCode::FAILURE;
        }
        // retrieve the JvtWeight first
        if (m_JvtWeighter) {
            if (!m_JvtWeighter->saveSF(Jet).isSuccess()) return StatusCode::FAILURE;
            SF = m_JvtWeighter->getSF(Jet);
        }
        // If the jet does not pass the JvtSelection it fails the last
        // missing requirement to be a signal jet.
        if (!m_JvtWeighter || m_JvtWeighter->PassSelection(Jet)) {
            if (m_BTagWeighter) {
                if (!m_BTagWeighter->saveSF(Jet).isSuccess()) return StatusCode::FAILURE;
                SF *= m_BTagWeighter->getSF(Jet);
            }
        }
        return saveEventSF(Jet, SF, true);
    }
    bool JetWeightHandler::append(const BTagJetWeightMap& sfs, const CP::SystematicSet* nominal) {
        BTagJetWeightMap::const_iterator Itr = sfs.find(systematic());
        if (Itr != sfs.end()) {
            if (m_BTagWeighter.get() == nullptr) m_BTagWeighter = Itr->second;
            return true;
        } else {
            Itr = sfs.find(nominal);
            if (Itr != sfs.end()) {
                if (m_BTagWeighter.get() == nullptr) m_BTagWeighter = Itr->second;
            }
        }
        return m_JvtWeighter.get() != nullptr;
    }
    bool JetWeightHandler::append(const JvtJetWeightMap& sfs, const CP::SystematicSet* nominal) {
        JvtJetWeightMap::const_iterator Itr = sfs.find(systematic());
        if (Itr != sfs.end()) {
            if (m_JvtWeighter.get() == nullptr) m_JvtWeighter = Itr->second;
            return true;
        } else {
            Itr = sfs.find(nominal);
            if (Itr != sfs.end()) {
                if (m_JvtWeighter.get() == nullptr) m_JvtWeighter = Itr->second;
            }
        }
        return m_BTagWeighter.get() != nullptr;
    }
    StatusCode JetWeightHandler::applySF() {
        if (m_BTagWeighter && !m_BTagWeighter->applySF().isSuccess()) return StatusCode::FAILURE;
        if (m_JvtWeighter && !m_JvtWeighter->applySF().isSuccess()) return StatusCode::FAILURE;
        return IPartilceWeightDecorator::applySF();
    }
    void JetWeightHandler::setupDecorations(std::shared_ptr<JetDecorations> decos) {
        // pass along the decorator
        if (m_BTagWeighter) m_BTagWeighter->setupDecorations(decos);
        if (m_JvtWeighter) m_JvtWeighter->setupDecorations(decos);
    }
}  // namespace XAMPP
