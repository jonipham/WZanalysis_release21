#ifndef XAMPPbase_SUSYMuonSelector_H
#define XAMPPbase_SUSYMuonSelector_H

#include <XAMPPbase/IMuonSelector.h>
#include <XAMPPbase/SUSYParticleSelector.h>

namespace CP {
    class IMuonEfficiencyScaleFactors;
    class IMuonTriggerScaleFactors;
    class IIsolationSelectionTool;

}  // namespace CP

namespace XAMPP {

    // Classes to obtain Reco/Iso/Id SF
    class MuonWeight;
    typedef std::shared_ptr<MuonWeight> MuonWeight_Ptr;
    typedef std::map<const CP::SystematicSet*, MuonWeight_Ptr> MuonWeightMap;

    // Class to calculate SFs for each systematic
    class MuonWeightHandler;
    typedef std::shared_ptr<MuonWeightHandler> MuonWeightHandler_Ptr;
    // TriggerSFToolHandlers
    class SUSYMuonTriggerSFHandler;
    typedef std::shared_ptr<SUSYMuonTriggerSFHandler> SUSYMuonTriggerSFHandler_Ptr;
    typedef std::vector<SUSYMuonTriggerSFHandler_Ptr> SUSYMuonTriggerSFHandler_Vector;
    typedef std::map<const CP::SystematicSet*, SUSYMuonTriggerSFHandler_Vector> SUSYMuonTriggerSFHandler_Map;

    class Cut;

    class SUSYMuonSelector : public SUSYParticleSelector, virtual public IMuonSelector {
    public:
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(SUSYMuonSelector, XAMPP::IMuonSelector)

        SUSYMuonSelector(const std::string& myname);
        virtual ~SUSYMuonSelector();

        virtual StatusCode initialize();

        virtual StatusCode LoadContainers();
        virtual StatusCode InitialFill(const CP::SystematicSet& systset);
        virtual StatusCode FillMuons(const CP::SystematicSet& systset);
        virtual StatusCode LoadSelection(const CP::SystematicSet& systset);

        virtual MuoLink GetLink(const xAOD::Muon& mu) const;
        virtual MuoLink GetOrigLink(const xAOD::Muon& mu) const;

        virtual const xAOD::MuonContainer* GetMuonContainer() const { return m_xAODMuons; }
        virtual xAOD::MuonContainer* GetMuons() const { return m_Muons; }
        virtual xAOD::MuonContainer* GetPreMuons() const { return m_PreMuons; }
        virtual xAOD::MuonContainer* GetSignalMuons() const { return m_SignalMuons; }
        virtual xAOD::MuonContainer* GetBaselineMuons() const { return m_BaselineMuons; }
        virtual xAOD::MuonContainer* GetSignalNoORMuons() const { return m_SignalQualMuons; }

        virtual xAOD::MuonContainer* GetCustomMuons(const std::string& kind) const;

        virtual StatusCode SaveScaleFactor();

        virtual std::shared_ptr<MuonDecorations> GetMuonDecorations() const { return m_muonDecorations; }

    protected:
        virtual StatusCode CallSUSYTools();
        StatusCode StoreTruthClassifer(xAOD::Muon& mu) const;
        bool IsBadMuon(const xAOD::Muon& mu) const;
        bool IsCosmicMuon(const xAOD::Muon& mu) const;

        virtual bool PassPreSelection(const xAOD::IParticle& P) const;
        virtual bool PassBaseline(const xAOD::IParticle& P) const;
        virtual bool GetSignalDecorator(const xAOD::IParticle& P) const;

        StatusCode SetupScaleFactors();
        StatusCode SetupSelection();

    private:
        StatusCode initializeSfMap(const std::string& sf_type, ToolHandle<CP::IMuonEfficiencyScaleFactors>& sf_tool, MuonWeightMap& map,
                                   unsigned int content);
        StatusCode initializeTrigSFMap(ToolHandle<CP::IMuonTriggerScaleFactors>& sf_tool, SUSYMuonTriggerSFHandler_Map& map,
                                       bool isBaseline);

        StatusCode initTriggerSFDecorators(const std::vector<std::string>& Triggers, std::vector<SUSYMuonTriggerSFHandler_Ptr>& Decorators,
                                           const CP::SystematicSet* set, unsigned int year, bool isBaseline);

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

    protected:
        const xAOD::MuonContainer* m_xAODMuons;
        xAOD::MuonContainer* m_Muons;
        xAOD::ShallowAuxContainer* m_MuonsAux;

        xAOD::MuonContainer* m_PreMuons;
        xAOD::MuonContainer* m_SignalMuons;
        xAOD::MuonContainer* m_BaselineMuons;
        xAOD::MuonContainer* m_SignalQualMuons;

        std::shared_ptr<MuonDecorations> m_muonDecorations;

        virtual void setupDecorations(std::shared_ptr<MuonDecorations> input = nullptr);

    private:
        bool m_SeparateSF;
        std::vector<MuonWeightHandler_Ptr> m_SF;
        bool m_doRecoSF;
        bool m_doIsoSF;
        bool m_doTTVASF;
        bool m_doTriggerSF;

        std::vector<std::string> m_TriggerExp2015;
        std::vector<std::string> m_TriggerExp2016;
        std::vector<std::string> m_TriggerExp2017;
        std::vector<std::string> m_TriggerExp2018;
        bool m_StoreMultipleTrigSF;
        bool m_writeBaselineSF;

        ToolHandle<CP::IMuonEfficiencyScaleFactors> m_SFtool_Reco;
        ToolHandle<CP::IMuonEfficiencyScaleFactors> m_SFtool_Iso;
        ToolHandle<CP::IMuonEfficiencyScaleFactors> m_SFtool_TTVA;

        ToolHandle<CP::IMuonEfficiencyScaleFactors> m_SFtool_BaseReco;

        ToolHandle<CP::IMuonTriggerScaleFactors> m_SFtool_Trig;
        ToolHandle<CP::IMuonTriggerScaleFactors> m_SFtool_BasTrig;
        std::string m_SFtoolName_Trig;

    protected:
        XAMPP::Storage<int>* m_NumBadMuons;
        XAMPP::Storage<int>* m_NumCosmics;

    private:
        int m_Baseline_Muon_Id;
        int m_Signal_Muon_Id;
        // what is the quality selection of the muon to apply on top of
        // the muon cosmic criterion
        std::string m_quality_Cosmic;
        std::string m_quality_BadMuon;

        ToolHandle<CP::IIsolationSelectionTool> m_iso_tool;
        bool m_force_iso_calc;
    };
    class MuonWeightDecorator : public IPartilceWeightDecorator {
    public:
        MuonWeightDecorator();
        virtual ~MuonWeightDecorator();
        // Method to be called to store the SF per each muon
        virtual StatusCode saveSF(const xAOD::Muon& muon, bool isSignal);

    protected:
        virtual StatusCode calculateSF(const xAOD::Muon& muon, float& SF) = 0;
    };
    class MuonWeight : public MuonWeightDecorator {
    public:
        MuonWeight(ToolHandle<CP::IMuonEfficiencyScaleFactors>& SFTool, XAMPP::EventInfo* info);
        // set the interval in which the SF is to be applied
        void setValidityRangeAbsEta(double min, double max);
        void setValidityRangePt(double min, double max);
        virtual ~MuonWeight();

    protected:
        virtual StatusCode calculateSF(const xAOD::Muon& muon, float& SF);

    private:
        ToolHandle<CP::IMuonEfficiencyScaleFactors>& m_SFTool;
        XAMPP::EventInfo* m_XAMPPInfo;
        double m_validity_eta_min;
        double m_validity_eta_max;
        double m_validity_pt_min;
        double m_validity_pt_max;
    };
    class MuonWeightHandler : public MuonWeightDecorator {
    public:
        MuonWeightHandler(const CP::SystematicSet* syst_set);
        const CP::SystematicSet* systematic() const;
        size_t nWeights() const;

        StatusCode saveBaselineTriggerSF(const xAOD::MuonContainer* muons);
        StatusCode saveSignalTriggerSF(const xAOD::MuonContainer* muons);
        void multipleTriggerSF(bool B = true);

        virtual StatusCode saveSF(const xAOD::Muon& muon, bool isSignal);
        virtual StatusCode applySF();

        bool append(MuonWeightMap& map, const CP::SystematicSet* nominal);
        bool setBaseTriggerSF(SUSYMuonTriggerSFHandler_Map& map, const CP::SystematicSet* nominal);
        bool setSignalTriggerSF(SUSYMuonTriggerSFHandler_Map& map, const CP::SystematicSet* nominal);

    protected:
        virtual StatusCode calculateSF(const xAOD::Muon& muon, float& SF);

    private:
        const CP::SystematicSet* m_Syst;
        std::vector<MuonWeight_Ptr> m_Weights;
        bool m_init;

        SUSYMuonTriggerSFHandler_Vector m_baseline_trig_SF;
        SUSYMuonTriggerSFHandler_Vector m_signal_trig_SF;
        bool m_multiple_trig_sf;
    };

    class SUSYMuonTriggerSFHandler : public IPartilceWeightDecorator {
    public:
        SUSYMuonTriggerSFHandler(const XAMPP::EventInfo* Info, ToolHandle<ST::ISUSYObjDef_xAODTool> ST, const std::string& Trigger,
                                 int year);
        StatusCode initialize();

        // The TriggerSF tool is only called if the associated trigger actually
        // fired...
        void ApplyIfTriggerFired(bool B);
        void DefineTrigger(const std::string& Trigger);

        // Systematics do not affecting the trigger SF are given the nominal
        // tool as reference This calls the TriggerSFTool once per event and
        // passes it to the Signal_TotalSF if necessary
        StatusCode SaveSF(const xAOD::MuonContainer* Muons);
        bool SetBaselineTool(SUSYMuonTriggerSFHandler_Ptr Ref);
        void FilterHandlersFromOtherYears(const SUSYMuonTriggerSFHandler_Vector& handlers);
        bool isAvailable() const;
        const std::string& name() const;
        unsigned int year() const;
        unsigned int nMuons() const;
        ~SUSYMuonTriggerSFHandler();

    private:
        // Calculate the trigger SFs only if the number of baseline does not
        // match the signal muons
        SUSYMuonTriggerSFHandler_Ptr m_BaselineHandler;
        std::vector<SUSYMuonTriggerSFHandler*> m_otherYearHandlers;
        const XAMPP::EventInfo* m_Info;
        ToolHandle<ST::ISUSYObjDef_xAODTool> m_SUSYTools;
        std::string m_SF_string;
        std::string m_Trigger_string;

        unsigned int m_year;
        unsigned int m_n_muons;
        bool m_DependOnTrigger;
        Cut* m_Cut;
    };
}  // namespace XAMPP

#endif
