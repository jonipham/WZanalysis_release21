#ifndef XAMPPbase_SUSYTruthSelector_H
#define XAMPPbase_SUSYTruthSelector_H

#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/ITruthSelector.h>
#include <XAMPPbase/ParticleSelector.h>

namespace XAMPP {
    class SUSYTruthSelector : public ParticleSelector, virtual public ITruthSelector {
    public:
        ASG_TOOL_CLASS(SUSYTruthSelector, XAMPP::ITruthSelector)

        SUSYTruthSelector(const std::string& myname);
        virtual ~SUSYTruthSelector() {}

        virtual int GetInitialState();
        virtual StatusCode initialize();

        virtual StatusCode LoadContainers();
        virtual StatusCode InitialFill(const CP::SystematicSet& systset);
        virtual StatusCode FillTruth(const CP::SystematicSet& systset);

        virtual bool isTrueTop(const xAOD::TruthParticle* particle);
        virtual bool isTrueW(const xAOD::TruthParticle* particle);
        virtual bool isTrueZ(const xAOD::TruthParticle* particle);
        virtual bool isTrueSUSY(const xAOD::TruthParticle* particle);
        virtual bool isTrueTau(const xAOD::TruthParticle* particle);
        virtual int classifyWDecays(const xAOD::TruthParticle* particle);
        virtual xAOD::TruthParticleContainer* GetTruthPreElectrons() const { return m_PreElectrons; }
        virtual xAOD::TruthParticleContainer* GetTruthBaselineElectrons() const { return m_BaselineElectrons; }
        virtual xAOD::TruthParticleContainer* GetTruthSignalElectrons() const { return m_SignalElectrons; }
        virtual xAOD::TruthParticleContainer* GetTruthPreMuons() const { return m_PreMuons; }
        virtual xAOD::TruthParticleContainer* GetTruthBaselineMuons() const { return m_BaselineMuons; }
        virtual xAOD::TruthParticleContainer* GetTruthSignalMuons() const { return m_SignalMuons; }
        virtual xAOD::TruthParticleContainer* GetTruthPrePhotons() const { return m_PrePhotons; }
        virtual xAOD::TruthParticleContainer* GetTruthBaselinePhotons() const { return m_BaselinePhotons; }
        virtual xAOD::TruthParticleContainer* GetTruthSignalPhotons() const { return m_SignalPhotons; }
        virtual xAOD::TruthParticleContainer* GetTruthPreTaus() const { return m_PreTaus; }
        virtual xAOD::TruthParticleContainer* GetTruthBaselineTaus() const { return m_BaselineTaus; }
        virtual xAOD::TruthParticleContainer* GetTruthSignalTaus() const { return m_SignalTaus; }
        virtual xAOD::TruthParticleContainer* GetTruthNeutrinos() const { return m_Neutrinos; }
        virtual xAOD::TruthParticleContainer* GetTruthPrimaryParticles() const { return m_InitialStatePart; }
        virtual xAOD::JetContainer* GetTruthPreJets() const { return m_PreJets; }
        virtual xAOD::JetContainer* GetTruthBaselineJets() const { return m_BaselineJets; }
        virtual xAOD::JetContainer* GetTruthSignalJets() const { return m_SignalJets; }
        virtual xAOD::JetContainer* GetTruthBJets() const { return m_BJets; }
        virtual xAOD::JetContainer* GetTruthLightJets() const { return m_LightJets; }
        virtual xAOD::JetContainer* GetTruthFatJets() const { return m_InitialFatJets; }
        virtual xAOD::JetContainer* GetTruthCustomJets(const std::string& kind) const;

        virtual xAOD::TruthParticleContainer* GetTruthParticles() const { return m_Particles; }
        virtual const xAOD::TruthParticleContainer* GetTruthInContainer() const { return m_xAODTruthParticles; }
        virtual StatusCode ReclusterTruthJets(const xAOD::IParticleContainer* inputJets, float Rcone, float minPtKt4 = -1,
                                              std::string PreFix = "", float minPtRecl = -1, float rclus = -1, float ptfrac = -1);

        virtual std::shared_ptr<TruthDecorations> GetTruthDecorations() const { return m_truthDecorations; }
        // set the truth decorations object
        virtual void setupDecorations(std::shared_ptr<TruthDecorations> inptr = nullptr);

    protected:
        bool isTRUTH3() const;

        virtual bool PassBaselineKinematics(const xAOD::IParticle& P) const;
        virtual bool PassSignalKinematics(const xAOD::IParticle& P) const;

        virtual bool PassSignal(const xAOD::IParticle& P) const;
        virtual bool PassBaseline(const xAOD::IParticle& P) const;
        virtual bool PassPreSelection(const xAOD::IParticle& P) const;

        virtual void InitDecorators(xAOD::TruthParticle* T, bool Pass = true);
        virtual void InitDecorators(const xAOD::Jet* J, bool Pass);

        template <typename Cont> void InitContainer(Cont* Input, Cont* PreSel) {
            for (auto ipart : *Input) {
                InitDecorators(ipart, PassBaselineKinematics(*ipart));
                if (PassPreSelection(*ipart)) PreSel->push_back(ipart);
            }
            PreSel->sort(XAMPP::ptsorter);
        }
        virtual bool IsGenParticle(const xAOD::TruthParticle* Truthpart) const;
        virtual bool IsInitialStateParticle(const xAOD::TruthParticle* truth);
        virtual bool ConsiderParticle(xAOD::TruthParticle* particle);
        virtual bool IsBJet(const xAOD::IParticle* j);

        void FillBaselineContainer(xAOD::TruthParticleContainer* Pre, xAOD::TruthParticleContainer* Baseline);
        void FillSignalContainer(xAOD::TruthParticleContainer* Baseline, xAOD::TruthParticleContainer* Signal);

        StatusCode FillTruthParticleContainer();
        StatusCode RetrieveParticleContainer(xAOD::TruthParticleContainer*& Particles, bool FromStoreGate, const std::string& GateKey,
                                             const std::string& ViewElementsKey, bool linkOriginal = true);
        void LoadDressedMomentum(xAOD::TruthParticle* Truth);
        void LoadVisibleMomentum(xAOD::TruthParticle* Truth);
        void DressVanillaMomentum(const xAOD::TruthParticle* Truth);
        bool doTruthJets() const;
        bool doTruthParticles() const;

        std::string TauKey() const;
        std::string ElectronKey() const;
        std::string MuonKey() const;
        std::string PhotonKey() const;

        std::string NeutrinoKey() const;
        std::string BosonKey() const;
        std::string BSMKey() const;
        std::string TopKey() const;
        std::string JetKey() const;

        struct ObjectDefinition {
            ObjectDefinition() {
                BaselinePt = SignalPt = 0;
                BaselineEta = SignalEta = -1;
                doObject = hasContainer = true;
                hasBaseEtaToExclude = hasSignalEtaToExclude = false;
            }
            float BaselinePt;
            float BaselineEta;

            float SignalPt;
            float SignalEta;

            StringVector ExclBaseEta_Property;
            StringVector ExclSignalEta_Property;

            EtaRangeVector BaseEtaExclude;
            EtaRangeVector SignalEtaExclude;
            bool hasBaseEtaToExclude;
            bool hasSignalEtaToExclude;

            std::string ContainerKey;
            bool hasContainer;
            bool doObject;
        };

        void declare(ObjectDefinition& obj, const std::string& as);
        StatusCode init(ObjectDefinition& obj, const std::string& as);

        inline bool RequirePreselFromHardProc() const { return m_RequirePreselFromHardProc; }
        inline bool RequireBaseFromHardProc() const { return m_RequireBaseFromHardProc; }
        inline bool RequireSignalFromHardProc() const { return m_RequireSignalFromHardProc; }

        inline bool RequirePreSelTauHad() const { return m_RequirePreSelTauHad; }
        inline bool RequireBaseTauHad() const { return m_RequireBaseTauHad; }
        inline bool RequireSignalTauHad() const { return m_RequireSignalTauHad; }

        inline bool RejectUnknownOrigin() const { return m_rejectUnknownOrigin; }

    private:
        bool BaselineKinematics(const xAOD::IParticle& P, const ObjectDefinition& obj) const;
        bool SignalKinematics(const xAOD::IParticle& P, const ObjectDefinition& obj) const;

        ObjectDefinition m_MuonDefs;
        ObjectDefinition m_ElectronDefs;
        ObjectDefinition m_TauDefs;
        ObjectDefinition m_JetDefs;
        ObjectDefinition m_PhotonDefs;
        ObjectDefinition m_NeutrinoDefs;

        float m_BJetPtCut;
        float m_BJetEtaCut;

        bool m_RequirePreselFromHardProc;
        bool m_RequireBaseFromHardProc;
        bool m_RequireSignalFromHardProc;

        bool m_RequirePreSelTauHad;
        bool m_RequireBaseTauHad;
        bool m_RequireSignalTauHad;

    protected:
        const xAOD::JetContainer* m_xAODTruthJets;

        const xAOD::TruthParticleContainer* m_xAODTruthParticles;
        const xAOD::MissingETContainer* m_xAODTruthMet;
        const xAOD::TruthParticleContainer* m_xAODTruthBSM;
        const xAOD::TruthParticleContainer* m_xAODTruthBoson;
        const xAOD::TruthParticleContainer* m_xAODTruthTop;

        xAOD::TruthParticleContainer* m_InitialParticles;

        xAOD::JetContainer* m_InitialJets;
        xAOD::JetContainer* m_InitialFatJets;

        xAOD::TruthParticleContainer* m_InitialElectrons;
        xAOD::TruthParticleContainer* m_InitialMuons;
        xAOD::TruthParticleContainer* m_InitialPhotons;
        xAOD::TruthParticleContainer* m_InitialTaus;
        xAOD::TruthParticleContainer* m_InitialNeutrinos;

        xAOD::JetContainer* m_PreJets;
        xAOD::JetContainer* m_BaselineJets;
        xAOD::JetContainer* m_SignalJets;
        xAOD::JetContainer* m_BJets;
        xAOD::JetContainer* m_LightJets;

        xAOD::TruthParticleContainer* m_Particles;

        xAOD::TruthParticleContainer* m_PreElectrons;
        xAOD::TruthParticleContainer* m_BaselineElectrons;
        xAOD::TruthParticleContainer* m_SignalElectrons;
        xAOD::TruthParticleContainer* m_PreMuons;
        xAOD::TruthParticleContainer* m_BaselineMuons;
        xAOD::TruthParticleContainer* m_SignalMuons;
        xAOD::TruthParticleContainer* m_PrePhotons;
        xAOD::TruthParticleContainer* m_BaselinePhotons;
        xAOD::TruthParticleContainer* m_SignalPhotons;
        xAOD::TruthParticleContainer* m_PreTaus;
        xAOD::TruthParticleContainer* m_BaselineTaus;
        xAOD::TruthParticleContainer* m_SignalTaus;
        xAOD::TruthParticleContainer* m_Neutrinos;
        xAOD::TruthParticleContainer* m_InitialStatePart;

        std::shared_ptr<TruthDecorations> m_truthDecorations;

    private:
        bool m_doTruthParticles;
        bool m_doTruthTop;
        bool m_doTruthBoson;
        bool m_doTruthSUSY;
        bool m_isTRUTH3;
        bool m_rejectUnknownOrigin;
        // flag to steer the filling of the SUSY process (save CPU if off)
        bool m_doSUSYProcess;

        std::string m_BosonKey;
        std::string m_BSMKey;
        std::string m_TopKey;

        bool m_useVisTauP4;
    };
}  // namespace XAMPP
#endif
