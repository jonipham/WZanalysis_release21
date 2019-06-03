#ifndef XAMPPbase_SUSYElectronSelector_H
#define XAMPPbase_SUSYElectronSelector_H

#include <XAMPPbase/IElectronSelector.h>
#include <XAMPPbase/ITriggerTool.h>
#include <XAMPPbase/SUSYParticleSelector.h>

#include <AsgTools/AnaToolHandle.h>

class IAsgElectronEfficiencyCorrectionTool;

namespace ST {
    class ISUSYObjDef_xAODTool;
}
namespace CP {
    class IIsolationSelectionTool;
}
namespace XAMPP {
    class ElectronWeightDecorator;
    class ElectronWeightHandler;
    class TriggerInterface;
    /// Short the names of the types a bit
    typedef std::shared_ptr<ElectronWeightDecorator> ElectronWeight_Ptr;
    typedef std::map<const CP::SystematicSet*, ElectronWeight_Ptr> ElectronWeightMap;
    typedef std::vector<ElectronWeight_Ptr> ElectronWeight_Vector;
    typedef std::map<const CP::SystematicSet*, ElectronWeight_Vector> ElectronWeight_VectorMap;
    typedef std::shared_ptr<ElectronWeightHandler> ElectronWeightHandler_Ptr;
    typedef IAsgElectronEfficiencyCorrectionTool EleEffTool;
    typedef ToolHandle<IAsgElectronEfficiencyCorrectionTool> EleEffToolHandle;

    class SUSYElectronSelector : public SUSYParticleSelector, virtual public IElectronSelector {
    public:
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(SUSYElectronSelector, XAMPP::IElectronSelector)

        SUSYElectronSelector(const std::string& myname);
        virtual ~SUSYElectronSelector();

        virtual StatusCode initialize();

        virtual StatusCode LoadSelection(const CP::SystematicSet& systset);

        virtual StatusCode LoadContainers();
        virtual StatusCode InitialFill(const CP::SystematicSet& systset);
        virtual StatusCode FillElectrons(const CP::SystematicSet& systset);

        virtual EleLink GetLink(const xAOD::Electron& el) const;
        virtual EleLink GetOrigLink(const xAOD::Electron& el) const;

        virtual const xAOD::ElectronContainer* GetElectronContainer() const { return m_xAODElectrons; }
        virtual xAOD::ElectronContainer* GetElectrons() const { return m_Electrons; }
        virtual xAOD::ElectronContainer* GetPreElectrons() const { return m_PreElectrons; }
        virtual xAOD::ElectronContainer* GetSignalElectrons() const { return m_SignalElectrons; }
        virtual xAOD::ElectronContainer* GetBaselineElectrons() const { return m_BaselineElectrons; }
        virtual xAOD::ElectronContainer* GetSignalNoORElectrons() const { return m_SignalNoORElectrons; }
        virtual xAOD::ElectronContainer* GetCustomElectrons(const std::string& kind) const;

        virtual StatusCode SaveScaleFactor();

        virtual std::shared_ptr<ElectronDecorations> GetElectronDecorations() const { return m_electronDecorations; }

    protected:
        virtual StatusCode CallSUSYTools();
        virtual bool PassPreSelection(const xAOD::IParticle& P) const;
        virtual bool PassBaseline(const xAOD::IParticle& P) const;
        virtual bool GetSignalDecorator(const xAOD::IParticle& P) const;
        StatusCode setupScaleFactors();

    private:
        float m_PreSelectionD0SigCut;
        float m_PreSelectionZ0SinThetaCut;
        bool m_RequireIsoPreSelection;

        float m_BaselineD0SigCut;
        float m_BaselineZ0SinThetaCut;
        bool m_RequireIsoBaseline;

        float m_SignalD0SigCut;
        float m_SignalZ0SinThetaCut;
        bool m_RequireIsoSignal;
        bool m_StoreTruthClassifier;

        const xAOD::ElectronContainer* m_xAODElectrons;

    protected:
        xAOD::ElectronContainer* m_Electrons;
        xAOD::ShallowAuxContainer* m_ElectronsAux;
        xAOD::ElectronContainer* m_PreElectrons;       // before OR
        xAOD::ElectronContainer* m_BaselineElectrons;  // after OR

        xAOD::ElectronContainer* m_SignalNoORElectrons;
        xAOD::ElectronContainer* m_SignalElectrons;
        // Translate the Egamma WP MediumLLH to Egamma jargon
        std::string EG_WP(const std::string& wp) const;
        // Extract from the trigger_string a different isolation
        // WP than the choosen signal one
        // The Trig_Iso_WP is defined if there is an ";<Iso_WP>" appended
        // to the trigger key
        std::string Trig_Iso_WP(const std::string& trig_exp, bool use_signal = true) const;
        // Extract the id WP from the trigger key differing from the
        // signal ID WP. The ID WP is selected if an ":<ID_WP>" can
        // be found in the trigger_key. The order between changing
        // isolation & ID WP does not matter...
        std::string Trig_EG_WP(const std::string& trig_exp, bool use_signal = true) const;

        std::shared_ptr<ElectronDecorations> m_electronDecorations;

        virtual void setupDecorations(std::shared_ptr<ElectronDecorations> input = nullptr);
        // The trigger tool is needed once during the initialization stage to recieve the active triggers for matching
        ToolHandle<ITriggerTool> m_trigger_tool;

    private:
        typedef std::pair<std::string, EleEffToolHandle> TrigSFTool;
        typedef std::map<std::string, EleEffToolHandle> TrigSFTool_Map;

        StatusCode initializeScaleFactors(const std::string& sf_type, EleEffToolHandle& sf_tool, ElectronWeightMap& map,
                                          unsigned int content);
        StatusCode initializeTriggerScaleFactors(TrigSFTool_Map& SFTools, ElectronWeight_VectorMap& SFs, unsigned int content);

        // Methods to initialize the trigger sf tools
        void GetTriggerTokens(std::string trigger_str, std::vector<std::string>& v_trigger15, std::vector<std::string>& v_trigger16,
                              std::vector<std::string>& v_trigger17, std::vector<std::string>& v_trigger18) const;

        StatusCode initializeTriggerSFTools();
        StatusCode initializeTriggerSFTools(TrigSFTool_Map& map, bool use_signal);
        std::string FindBestSFTool(const TrigSFTool_Map& TriggerSFTools, const std::string& TriggerSF);

        std::vector<ElectronWeightHandler_Ptr> m_SF;

        TrigSFTool_Map m_SignalTrig_SF_Tools;
        TrigSFTool_Map m_BaselineTrig_SF_Tools;

        EleEffToolHandle m_Reco_SF_Handle;

        EleEffToolHandle m_SignalId_SF_Handle;
        EleEffToolHandle m_SignalIso_SF_Handle;

        EleEffToolHandle m_BaselineId_SF_Handle;
        EleEffToolHandle m_BaselineIso_SF_Handle;

        bool m_SeparateSF;
        bool m_doRecoSF;
        bool m_doIdSF;
        bool m_doTriggerSF;
        bool m_doIsoSF;
        std::vector<std::string> m_TriggerExp;
        std::vector<std::string> m_TriggerSFConf;
        bool m_StoreMultiTrigSF;
        bool m_writeBaselineSF;

        std::string m_Baseline_Id_WP;
        std::string m_Signal_Id_WP;
        std::string m_EfficiencyMap;

        std::string m_CorrelationModel;
        std::string m_Signal_Iso_WP;
        std::string m_Baseline_Iso_WP;

        ToolHandle<CP::IIsolationSelectionTool> m_iso_tool;
        bool m_force_iso_calc;
    };

    class ElectronWeightDecorator : public IPartilceWeightDecorator {
    public:
        ElectronWeightDecorator();
        virtual ~ElectronWeightDecorator();
        virtual StatusCode initialize();
        // Method to be called to store the SF per each Electron
        virtual StatusCode saveSF(const xAOD::Electron& Electron, bool isSignal);

    protected:
        virtual StatusCode calculateSF(const xAOD::Electron& Electron, double& SF) = 0;
    };
    class ElectronWeight : public ElectronWeightDecorator {
    public:
        ElectronWeight(EleEffToolHandle& SFTool);
        virtual ~ElectronWeight();

    protected:
        virtual StatusCode calculateSF(const xAOD::Electron& Electron, double& SF);

    private:
        EleEffToolHandle m_SFTool;
    };

    class ElectronWeightHandler : public ElectronWeightDecorator {
    public:
        ElectronWeightHandler(const CP::SystematicSet* syst_set);
        const CP::SystematicSet* systematic() const;
        size_t nWeights() const;

        void multipleTriggerSF(bool B = true);

        virtual StatusCode saveSF(const xAOD::Electron& Electron, bool isSignal);
        virtual StatusCode applySF();

        bool append(const ElectronWeightMap& map, const CP::SystematicSet* nominal);
        bool setSignalTriggerSF(const ElectronWeight_VectorMap& map, const CP::SystematicSet* nominal);

    protected:
        virtual StatusCode calculateSF(const xAOD::Electron& Electron, double& SF);

    private:
        const CP::SystematicSet* m_Syst;
        std::vector<ElectronWeight_Ptr> m_Weights;
        bool m_init;

        ElectronWeight_Vector m_signal_trig_SF;
        bool m_multiple_trig_sf;
    };
    class ElectronTriggerSFHandler : public ElectronWeightDecorator {
    public:
        ElectronTriggerSFHandler(const std::string& Trigger, const ToolHandle<ITriggerTool>& trigger_tool, EleEffToolHandle& TriggerSF);
        virtual StatusCode initialize();
        virtual ~ElectronTriggerSFHandler();

    protected:
        virtual StatusCode calculateSF(const xAOD::Electron& Electron, double& SF);

    private:
        bool RetrieveMatchers(const std::string& Trigger);
        bool IsTriggerMachted(const xAOD::Electron& El) const;

        std::string m_TrigStr;
        EleEffToolHandle m_TriggerSFTool;
        ToolHandle<ITriggerTool> m_trig_tool;
        std::vector<std::shared_ptr<TriggerInterface>> m_Triggers;
    };
}  // namespace XAMPP
#endif
