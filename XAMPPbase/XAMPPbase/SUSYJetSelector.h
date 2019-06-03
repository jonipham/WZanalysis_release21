#ifndef XAMPPbase_SUSYJetSelector_H
#define XAMPPbase_SUSYJetSelector_H

#include <XAMPPbase/IJetSelector.h>
#include <XAMPPbase/SUSYParticleSelector.h>

class IBTaggingEfficiencyTool;
namespace CP {
    class IJetJvtEfficiency;
    class JetJvtEfficiency;
}  // namespace CP
namespace XAMPP {

    class BTagJetWeight;
    typedef std::shared_ptr<BTagJetWeight> BTagJetWeight_Ptr;
    typedef std::map<const CP::SystematicSet*, BTagJetWeight_Ptr> BTagJetWeightMap;

    class JvtJetWeight;
    typedef std::shared_ptr<JvtJetWeight> JvtJetWeight_Ptr;
    typedef std::map<const CP::SystematicSet*, JvtJetWeight_Ptr> JvtJetWeightMap;

    class JetWeightHandler;
    typedef std::shared_ptr<JetWeightHandler> JetWeightHandler_Ptr;

    class SUSYJetSelector : public SUSYParticleSelector, virtual public IJetSelector {
    public:
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(SUSYJetSelector, XAMPP::IJetSelector)

        SUSYJetSelector(const std::string& myname);
        virtual ~SUSYJetSelector();

        virtual StatusCode initialize();

        virtual StatusCode LoadContainers();
        virtual StatusCode InitialFill(const CP::SystematicSet& systset);
        virtual StatusCode FillJets(const CP::SystematicSet& systset);

        virtual xAOD::JetContainer* GetJets() const { return m_Jets; }
        virtual xAOD::JetContainer* GetPreJets(int Radius = 4) const {
            if (Radius == 10) return m_PreFatJets;
            return m_PreJets;
        }
        virtual xAOD::JetContainer* GetBaselineJets(int = 4) const { return m_BaselineJets; }
        virtual xAOD::JetContainer* GetSignalJets(int = 4) const { return m_SignalJets; }
        virtual xAOD::JetContainer* GetSignalNoORJets(int = 4) const { return m_SignalQualJets; }

        virtual xAOD::JetContainer* GetBJets() const { return m_BJets; }
        virtual xAOD::JetContainer* GetLightJets() const { return m_LightJets; }
        virtual xAOD::JetContainer* GetSignalJetsNoJVT() const { return m_PreJVTJets; }
        virtual xAOD::JetContainer* GetCustomJets(const std::string& kind) const;

        virtual StatusCode ReclusterJets(const xAOD::IParticleContainer* inputJets, float Rcone, float minPtKt4 = -1,
                                         std::string PreFix = "", float minPtRecl = -1, float rclus = -1, float ptfrac = -1);
        virtual StatusCode SaveScaleFactor();

        virtual std::shared_ptr<JetDecorations> GetJetDecorations() const { return m_jetDecorations; }

    protected:
        virtual StatusCode CalibrateJets(const std::string& Key, xAOD::JetContainer*& Container, xAOD::JetContainer*& PreSelected,
                                         const std::string& PreSelName = "", JetAlgorithm Cone = JetAlgorithm::AntiKt4);
        virtual StatusCode CalibrateJets(const std::string& Key, xAOD::JetContainer*& PreSelected, const std::string& PreSelName = "",
                                         JetAlgorithm Cone = JetAlgorithm::AntiKt4);

        virtual bool IsBadJet(const xAOD::Jet& jet) const;
        virtual bool IsBJet(const xAOD::Jet& jet) const;
        virtual double BtagBDT(const xAOD::Jet& jet) const;

        virtual bool PassBaselineKinematics(const xAOD::IParticle& P) const;
        virtual bool PassSignalKinematics(const xAOD::IParticle& P) const;

        bool isFromAlgorithm(const xAOD::IParticle& jet, XAMPP::JetAlgorithm alg) const;
        bool isFromAlgorithm(const xAOD::IParticle* jet, XAMPP::JetAlgorithm alg) const;

        virtual StatusCode preCalibCorrection(xAOD::JetContainer*) { return StatusCode::SUCCESS; };

        StatusCode SetupScaleFactors();
        StatusCode SetupSelection();

        const xAOD::JetContainer* m_xAODJets;
        xAOD::JetContainer* m_Jets;

        xAOD::JetContainer* m_PreJets;
        xAOD::JetContainer* m_BaselineJets;

        xAOD::JetContainer* m_PreJVTJets;
        xAOD::JetContainer* m_SignalJets;
        xAOD::JetContainer* m_SignalQualJets;

        xAOD::JetContainer* m_PreFatJets;

        xAOD::JetContainer* m_BJets;
        xAOD::JetContainer* m_LightJets;

        virtual void setupDecorations(std::shared_ptr<JetDecorations> input = nullptr);
        std::shared_ptr<JetDecorations> m_jetDecorations;
        ToolHandle<CP::IJetJvtEfficiency> m_JvtTool;

    private:
        int m_JetCollectionType;
        XAMPP::Storage<int>* m_decNBad;
        float m_bJetEtaCut;

        bool m_SeparateSF;
        std::vector<JetWeightHandler_Ptr> m_SF;

        bool m_doBTagSF;
        bool m_doJVTSF;
        bool m_doLargeRdecors;

        float m_Kt10_BaselinePt;
        float m_Kt10_BaselineEta;
        float m_Kt10_SignalPt;
        float m_Kt10_SignalEta;

        float m_Kt02_BaselinePt;
        float m_Kt02_BaselineEta;
        float m_Kt02_SignalPt;
        float m_Kt02_SignalEta;

        // What is the default input flag to be assigned for association utils
        int m_Kt02_ORUtils_flag;
        int m_Kt10_ORUtils_flag;
        ToolHandle<IBTaggingEfficiencyTool> m_BtagTool;
    };
    class JetWeightDecorator : public IPartilceWeightDecorator {
    public:
        JetWeightDecorator();
        virtual ~JetWeightDecorator();
        // Method to be called to store the SF per each Jet
        virtual StatusCode saveSF(const xAOD::Jet& Jet);
        virtual bool PassSelection(const xAOD::Jet& jet) const = 0;
        // we need to pass along the decorations of the Jet Selector.
        // Hence, *no* default null arg here!
        virtual void setupDecorations(std::shared_ptr<JetDecorations> input);

    protected:
        virtual StatusCode calculateScaleFactor(const xAOD::Jet& Jet, float& SF);

        virtual StatusCode calculateEfficiencySF(const xAOD::Jet& Jet, float& SF) = 0;
        virtual StatusCode calculateInefficiencySF(const xAOD::Jet& Jet, float& SF) = 0;
        std::shared_ptr<JetDecorations> m_jetDecorations;
    };
    class BTagJetWeight : public JetWeightDecorator {
    public:
        BTagJetWeight(ToolHandle<IBTaggingEfficiencyTool>& SFTool);
        virtual ~BTagJetWeight();
        virtual bool PassSelection(const xAOD::Jet& jet) const;
        void SetBJetEtaCut(float eta) { m_bJetEtaCut = eta; }

    protected:
        virtual StatusCode calculateEfficiencySF(const xAOD::Jet& Jet, float& SF);
        virtual StatusCode calculateInefficiencySF(const xAOD::Jet& Jet, float& SF);

    private:
        ToolHandle<IBTaggingEfficiencyTool> m_SFTool;
        float m_bJetEtaCut;
    };
    class JvtJetWeight : public JetWeightDecorator {
    public:
        JvtJetWeight(ToolHandle<CP::IJetJvtEfficiency>& SFTool);
        virtual ~JvtJetWeight();
        virtual bool PassSelection(const xAOD::Jet& jet) const;
        StatusCode tagTruth(const xAOD::IParticleContainer* jets, const xAOD::IParticleContainer* truthJets);

    protected:
        virtual StatusCode calculateEfficiencySF(const xAOD::Jet& Jet, float& SF);
        virtual StatusCode calculateInefficiencySF(const xAOD::Jet& Jet, float& SF);

    private:
        CP::JetJvtEfficiency* m_SFTool_Raw_Ptr;
        ToolHandle<CP::IJetJvtEfficiency> m_SFTool;
    };
    class JetWeightHandler : public IPartilceWeightDecorator {
    public:
        JetWeightHandler(const CP::SystematicSet* set);
        virtual ~JetWeightHandler();
        const CP::SystematicSet* systematic() const;
        // Truth Tagging for the JVT SFs
        StatusCode tagTruth(const xAOD::IParticleContainer* jets, const xAOD::IParticleContainer* truthJets);
        virtual StatusCode saveSF(const xAOD::Jet& Jet);
        virtual StatusCode applySF();
        bool append(const BTagJetWeightMap& sfs, const CP::SystematicSet* nominal);
        bool append(const JvtJetWeightMap& sfs, const CP::SystematicSet* nominal);
        // we need to pass along the decorations of the Jet Selector.
        // Hence, *no* default null arg here!
        void setupDecorations(std::shared_ptr<JetDecorations> decos);

    private:
        const CP::SystematicSet* m_syst;
        JvtJetWeight_Ptr m_JvtWeighter;
        BTagJetWeight_Ptr m_BTagWeighter;
    };
}  // namespace XAMPP
#endif
