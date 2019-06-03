#ifndef XAMPPbase_SUSYTauSelector_H
#define XAMPPbase_SUSYTauSelector_H

#include <TauAnalysisTools/Enums.h>
#include <TauAnalysisTools/TauEfficiencyCorrectionsTool.h>
#include <XAMPPbase/ITauSelector.h>
#include <XAMPPbase/ITriggerTool.h>
#include <XAMPPbase/SUSYParticleSelector.h>
#include <XAMPPbase/TauDecorations.h>

namespace TauAnalysisTools {
    class ITauTruthMatchingTool;
    class ITauSelectionTool;
}  // namespace TauAnalysisTools
namespace XAMPP {
    class TauWeightDecorator;
    class TauWeightHandler;
    class TriggerInterface;

    typedef TauAnalysisTools::ITauEfficiencyCorrectionsTool TauEffiTool;
    typedef ToolHandle<TauEffiTool> TauEffiToolHandle;
    typedef std::shared_ptr<TauWeightDecorator> TauWeight_Ptr;
    typedef std::map<const CP::SystematicSet*, TauWeight_Ptr> TauWeightMap;
    typedef std::vector<TauWeight_Ptr> TauWeight_Vector;
    typedef std::map<const CP::SystematicSet*, TauWeight_Vector> TauWeight_VectorMap;
    typedef std::shared_ptr<TauWeightHandler> TauWeightHandler_Ptr;
    typedef std::map<TauAnalysisTools::EfficiencyCorrectionType, TauEffiToolHandle> TauEffiToolMap;
    typedef std::map<std::string, TauEffiToolHandle> TauTriggEffiToolMap;

    std::string to_string(TauAnalysisTools::EfficiencyCorrectionType);

    class SUSYTauSelector : public SUSYParticleSelector, virtual public ITauSelector {
    public:
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(SUSYTauSelector, XAMPP::ITauSelector)

        SUSYTauSelector(const std::string& myname);
        virtual ~SUSYTauSelector();

        virtual StatusCode initialize();

        virtual StatusCode LoadContainers();
        virtual StatusCode InitialFill(const CP::SystematicSet& systset);
        virtual StatusCode FillTaus(const CP::SystematicSet& systset);

        virtual TauLink GetLink(const xAOD::TauJet& tau) const;
        virtual TauLink GetOrigLink(const xAOD::TauJet& tau) const;

        virtual const xAOD::TauJetContainer* GetTauContainer() const;

        virtual xAOD::TauJetContainer* GetTaus() const;
        virtual xAOD::TauJetContainer* GetPreTaus() const;
        virtual xAOD::TauJetContainer* GetSignalTaus() const;
        virtual xAOD::TauJetContainer* GetBaselineTaus() const;
        virtual xAOD::TauJetContainer* GetSignalNoORTaus() const;
        virtual xAOD::TauJetContainer* GetCustomTaus(const std::string& kind) const;

        virtual StatusCode SaveScaleFactor();
        virtual std::shared_ptr<TauDecorations> GetTauDecorations() const;

    protected:
        virtual StatusCode CallSUSYTools();
        virtual void setupDecorations(std::shared_ptr<TauDecorations> input = nullptr);

        StatusCode StoreTruthClassifer(xAOD::TauJet& tau);

        const xAOD::TauJetContainer* m_xAODTaus;

        xAOD::TauJetContainer* m_Taus;
        xAOD::ShallowAuxContainer* m_TausAux;

        xAOD::TauJetContainer* m_PreTaus;       // before OR
        xAOD::TauJetContainer* m_BaselineTaus;  // after OR
        xAOD::TauJetContainer* m_SignalTaus;
        xAOD::TauJetContainer* m_SignalQualTaus;

        std::shared_ptr<TauDecorations> m_tauDecorations;

        asg::AnaToolHandle<TauAnalysisTools::ITauTruthMatchingTool> m_TruthMatching;

    private:
        // Methods to initialize the Tau efficiency SFs
        StatusCode initializeTauEfficiencySFTools(TauEffiToolMap& SfTools, ToolHandle<TauAnalysisTools::ITauSelectionTool>& selectionTool);
        StatusCode initializeScaleFactors(TauEffiToolMap& SfTools, std::vector<TauWeightMap>& sf_types, unsigned int content);
        StatusCode initializeScaleFactors(const std::string& sf_type, TauEffiToolHandle& sf_tool, TauWeightMap& map, unsigned int content);

        // Methods to initialize the TauTriggerSFs
        StatusCode initializeTriggerEfficiencySFTools(TauTriggEffiToolMap& SfTools,
                                                      ToolHandle<TauAnalysisTools::ITauSelectionTool>& selectionTool);
        StatusCode initializeTriggerScaleFactors(TauTriggEffiToolMap& SFTools, TauWeight_VectorMap& SF_Map, unsigned int content);

        std::vector<TauWeightHandler_Ptr> m_SF;
        ToolHandle<TauAnalysisTools::ITauSelectionTool> m_BaseTauSelectionTool;
        ToolHandle<TauAnalysisTools::ITauSelectionTool> m_SignalTauSelectionTool;
        ToolHandle<ITriggerTool> m_trigger_tool;

        TauEffiToolMap m_BaselineSF_Tools;
        TauEffiToolMap m_SignalSF_Tools;

        TauTriggEffiToolMap m_Baseline_TrigSF_Tools;
        TauTriggEffiToolMap m_Signal_TrigSF_Tools;

        bool m_SeparateSF;
        bool m_doIdSF;
        bool m_doTrigSF;

        std::vector<std::string> m_TriggerExp;
        bool m_StoreMultiTriggerSf;
        bool m_RequireTrigMatchForSF;
        bool m_writeBaselineSF;
        bool m_writeBaselineTrigSF;
        bool m_StoreTruthClassifier;
        bool m_ignoreBaseTauIDTool;
    };

    class TauWeightDecorator : public IPartilceWeightDecorator {
    public:
        TauWeightDecorator();
        virtual ~TauWeightDecorator();
        virtual StatusCode initialize();
        // Method to be called to store the SF per each Tau
        virtual StatusCode saveSF(const xAOD::TauJet& Tau, bool isSignal);

    protected:
        virtual StatusCode calculateSF(const xAOD::TauJet& Tau, double& SF) = 0;
    };
    class TauWeight : public TauWeightDecorator {
    public:
        TauWeight(TauEffiToolHandle& SFTool);
        virtual ~TauWeight();

    protected:
        virtual StatusCode calculateSF(const xAOD::TauJet& Tau, double& SF);

    private:
        TauEffiToolHandle m_SFTool;
    };

    class TauWeightHandler : public TauWeightDecorator {
    public:
        TauWeightHandler(const CP::SystematicSet* syst_set);
        const CP::SystematicSet* systematic() const;
        size_t nWeights() const;

        void multipleTriggerSF(bool B = true);

        virtual StatusCode saveSF(const xAOD::TauJet& Tau, bool isSignal);
        virtual StatusCode applySF();

        bool append(const TauWeightMap& map, const CP::SystematicSet* nominal);
        bool setSignalTriggerSF(const TauWeight_VectorMap& map, const CP::SystematicSet* nominal);

    protected:
        virtual StatusCode calculateSF(const xAOD::TauJet& Tau, double& SF);

    private:
        const CP::SystematicSet* m_Syst;
        std::vector<TauWeight_Ptr> m_Weights;
        bool m_init;

        TauWeight_Vector m_signal_trig_SF;
        bool m_multiple_trig_sf;
    };

    class TauTriggerSFHandler : public TauWeightDecorator {
    public:
        TauTriggerSFHandler(const std::string& Trigger, const ToolHandle<ITriggerTool>& TriggerTool, TauEffiToolHandle& TriggerSF);

        virtual StatusCode initialize();
        virtual ~TauTriggerSFHandler();
        void requireMatching(bool B = true);

    protected:
        virtual StatusCode calculateSF(const xAOD::TauJet& Tau, double& SF);

    private:
        bool RetrieveMatchers(const std::string& Trigger);
        bool IsTriggerMachted(const xAOD::TauJet& Tau) const;

        std::string m_TrigStr;
        TauEffiToolHandle m_TriggerSFTool;

        ToolHandle<ITriggerTool> m_trigger_tool;
        std::vector<std::shared_ptr<TriggerInterface>> m_Triggers;
        bool m_requireMatching;
    };
}  // namespace XAMPP
#endif
