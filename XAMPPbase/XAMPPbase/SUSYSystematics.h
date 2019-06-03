#ifndef SUSYSystematics_H
#define SUSYSystematics_H

#include <AsgTools/AnaToolHandle.h>
#include <AsgTools/AsgTool.h>
#include <AsgTools/ToolHandle.h>

#include <SUSYTools/ISUSYObjDef_xAODTool.h>
#include <XAMPPbase/ISystematics.h>

namespace XAMPP {
    class SUSYSystematics : public asg::AsgTool, virtual public ISystematics {
    public:
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(SUSYSystematics, XAMPP::ISystematics)

        SUSYSystematics(const std::string& myname);
        virtual ~SUSYSystematics();

        virtual std::vector<const CP::SystematicSet*> GetAllSystematics() const;
        virtual std::vector<const CP::SystematicSet*> GetKinematicSystematics(
            XAMPP::SelectionObject T = XAMPP::SelectionObject::Other) const;
        virtual std::vector<const CP::SystematicSet*> GetWeightSystematics(XAMPP::SelectionObject T = XAMPP::SelectionObject::Other) const;

        virtual const CP::SystematicSet* GetNominal() const;
        virtual const CP::SystematicSet* GetCurrent() const;

        virtual StatusCode resetSystematics();
        virtual StatusCode setSystematic(const CP::SystematicSet* Set);

        virtual bool AffectsOnlyMET(const CP::SystematicSet* Set) const;
        virtual bool ProcessObject(XAMPP::SelectionObject T) const;
        virtual bool isData() const;
        virtual bool isAF2() const;

        virtual StatusCode InsertKinematicSystematic(const CP::SystematicSet& set,
                                                     XAMPP::SelectionObject T = XAMPP::SelectionObject::Other);
        virtual StatusCode InsertWeightSystematic(const CP::SystematicSet& set, XAMPP::SelectionObject T = XAMPP::SelectionObject::Other);
        virtual StatusCode InsertSystematicToolService(XAMPP::ISystematicToolService* Service);

        virtual bool SystematicsFixed() const;
        virtual StatusCode FixSystematics();

    private:
        const CP::SystematicSet* CreateCopy(const CP::SystematicSet& Set);

        void PromptSystList(std::vector<const CP::SystematicSet*>& List, const std::string& Type) const;
        void AppendSystematic(std::vector<const CP::SystematicSet*>& Systematics, const CP::SystematicSet* set);
        void CopySystematics(std::vector<const CP::SystematicSet*>& From, std::vector<const CP::SystematicSet*>& To);

        std::vector<std::shared_ptr<CP::SystematicSet>> m_syst_all;

        std::vector<const CP::SystematicSet*> m_syst_kin;
        std::vector<const CP::SystematicSet*> m_syst_kin_ele;
        std::vector<const CP::SystematicSet*> m_syst_kin_muo;
        std::vector<const CP::SystematicSet*> m_syst_kin_jet;
        std::vector<const CP::SystematicSet*> m_syst_kin_tau;
        std::vector<const CP::SystematicSet*> m_syst_kin_pho;
        std::vector<const CP::SystematicSet*> m_syst_kin_met;
        std::vector<const CP::SystematicSet*> m_syst_kin_trk;
        std::vector<const CP::SystematicSet*> m_syst_kin_tru;

        std::vector<const CP::SystematicSet*> m_syst_weight;
        std::vector<const CP::SystematicSet*> m_syst_weight_elec;
        std::vector<const CP::SystematicSet*> m_syst_weight_muon;
        std::vector<const CP::SystematicSet*> m_syst_weight_tau;
        std::vector<const CP::SystematicSet*> m_syst_weight_phot;
        std::vector<const CP::SystematicSet*> m_syst_weight_jet;
        std::vector<const CP::SystematicSet*> m_syst_weight_btag;
        std::vector<const CP::SystematicSet*> m_syst_weight_met;
        std::vector<const CP::SystematicSet*> m_syst_weight_trk;
        std::vector<const CP::SystematicSet*> m_syst_weight_tru;
        std::vector<const CP::SystematicSet*> m_syst_weight_evtweight;

        const CP::SystematicSet* m_empty_syst;
        const CP::SystematicSet* m_act_syst;

        bool m_doNoElecs;
        bool m_doNoMuons;
        bool m_doNoJets;
        bool m_doNoTaus;
        bool m_doNoPhotons;
        bool m_doNoDiTaus;
        bool m_doNoBtag;
        bool m_doNoMet;
        bool m_doNoTracks;
        bool m_doNoTruth;
        bool m_doSyst;
        bool m_doWeights;

        std::vector<std::shared_ptr<XAMPP::ISystematicToolService>> m_Tools;
        std::vector<std::string> m_excluded_syst;
        bool m_init;
        bool m_isData;
        bool m_isAF2;
    };

    class SUSYToolsSystematicToolHandle : virtual public ISystematicToolService {
    public:
        SUSYToolsSystematicToolHandle(ToolHandle<ST::ISUSYObjDef_xAODTool>& SusyTools);
        SUSYToolsSystematicToolHandle(asg::AnaToolHandle<ST::ISUSYObjDef_xAODTool>& SusyTools);
        SUSYToolsSystematicToolHandle(ST::ISUSYObjDef_xAODTool* SusyTools);

        virtual StatusCode resetSystematic();
        virtual StatusCode setSystematic(const CP::SystematicSet* Set);
        virtual StatusCode initialize();
        virtual std::string name() const;
        virtual ~SUSYToolsSystematicToolHandle();

    private:
        void FatalMSG(const std::string& MSG) const;
        void InfoMSG(const std::string& MSG) const;
        bool systematicAffectsMET(const ST::SystInfo& isys) const;

        bool systematicAffects(const ST::SystInfo& isys, xAOD::Type::ObjectType t) const;
        bool isKineSys(const ST::SystInfo& isys) const;
        bool isWeightSys(const ST::SystInfo& isys) const;

        ST::ISUSYObjDef_xAODTool* m_SUSYTools_ptr;
        asg::AnaToolHandle<ST::ISUSYObjDef_xAODTool> m_anaSUSYTools;
        ToolHandle<ST::ISUSYObjDef_xAODTool> m_SUSYTools;
        ToolHandle<XAMPP::ISystematics> m_systematic;
        bool m_init;
        std::vector<xAOD::Type::ObjectType> m_objTypes;
    };

}  // namespace XAMPP
#endif
