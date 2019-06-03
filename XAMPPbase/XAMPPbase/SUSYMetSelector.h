#ifndef XAMPPbase_SUSYMetSelector_H
#define XAMPPbase_SUSYMetSelector_H

#include <SUSYTools/SUSYObjDef_xAOD.h>

#include <METInterface/IMETMaker.h>
#include <METInterface/IMETSystematicsTool.h>

#include <AsgTools/AnaToolHandle.h>
#include <AsgTools/AsgTool.h>
#include <XAMPPbase/EventStorage.h>
#include <XAMPPbase/IMetSelector.h>

#include <xAODMissingET/MissingETAssociationMap.h>
#include <xAODMissingET/MissingETContainer.h>

class IMETMaker;
class IMETSystematicsTool;
class IMETSignificance;
namespace XAMPP {

    class IElectronSelector;
    class IJetSelector;
    class IMuonSelector;
    class IPhotonSelector;
    class ITauSelector;
    class IEventInfo;
    class EventInfo;
    class ISystematics;

    class SUSYMetSelector;
    class MetSignificanceHandler;
    typedef std::shared_ptr<MetSignificanceHandler> MetSignificanceHandler_Ptr;

    class MetSignificanceHandler {
    public:
        // The pointer to the MET SUSYMetSelector is given because, we need to
        // pass the declare property function
        MetSignificanceHandler(const std::string& grp_name, SUSYMetSelector* MetSelector, const std::string& description = "",
                               bool isDisabled = false);
        MetSignificanceHandler(const MetSignificanceHandler&) = delete;
        void operator=(const MetSignificanceHandler&) = delete;

        StatusCode initialize();
        StatusCode fill();
        // The pointer to the pointer where the met container is stored to
        // It can be set at any time of the programme in case that the pointer
        // is only a local variable of the method
        void setMissingEt(xAOD::MissingETContainer*& Met);
        // set the option whether the x_overSqrt_Met / SumHT variables shall be
        // stored
        void storeSqrtVariables(bool B = true);
        void useSofTrackTerm(bool B = true);

        bool isInitialized() const;
        bool isDisabled() const;
        bool isUserConfigured() const;
        std::string group() const;

        // parse the set property methods to the AnaToolHandle.
        template <class T2> StatusCode setProperty(const std::string& property, const T2& value) {
            if (!m_propertySet)
                m_metSignif.setTypeAndName("met::METSignificance/MetSignificanceTool" + std::string(group().empty() ? "" : "_") + group());
            m_propertySet = true;
            return m_metSignif.setProperty(property, value);
        }

    private:
        std::string m_grp_name;
        const SUSYMetSelector* m_met_selector;
        bool m_init;
        bool m_enabled;
        bool m_use_track_term;

        asg::AnaToolHandle<IMETSignificance> m_metSignif;
        ToolHandle<XAMPP::IEventInfo> m_event_info_handle;
        bool m_propertySet;

        xAOD::MissingETContainer** m_met_container;

        XAMPP::Storage<float>* m_Significance;
        XAMPP::Storage<float>* m_Significance_Rho;
        XAMPP::Storage<float>* m_Significance_VarL;

        bool m_store_sqrt_vars;
        XAMPP::Storage<float>* m_OverSqrtSumET;
        XAMPP::Storage<float>* m_OverSqrtHT;
    };

    class SUSYMetSelector : public asg::AsgTool, virtual public IMetSelector {
    public:
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(SUSYMetSelector, XAMPP::IMetSelector)

        SUSYMetSelector(const std::string& myname);
        virtual ~SUSYMetSelector() { ATH_MSG_INFO("Destructor called"); }

        virtual StatusCode initialize();

        virtual StatusCode LoadContainers();
        virtual StatusCode FillMet(const CP::SystematicSet& systset);

        virtual StatusCode SaveScaleFactor();

        virtual xAOD::MissingETContainer* GetCustomMet(const std::string& kind = "") const;

        virtual StatusCode addToInvisible(xAOD::IParticle* particle, const std::string& invis_container);
        virtual StatusCode addToInvisible(xAOD::IParticle& particle, const std::string& invis_container);

        virtual StatusCode addToInvisible(const xAOD::IParticleContainer* container, const std::string& invis_container);
        virtual StatusCode addToInvisible(const xAOD::IParticleContainer& container, const std::string& invis_container);

        virtual xAOD::IParticleContainer* getInvisible(const std::string& invis_container = "") const;

    protected:
        StatusCode CreateContainer(const std::string& name, xAOD::MissingETContainer*& Cont);
        StatusCode SetSystematic(const CP::SystematicSet& systset);
        std::string storeName(const std::string& name) const;

        virtual StatusCode initializeMetSignificance();

    public:
        // These methods need to be public for the Met significance helpers
        std::string softTrackTerm() const;
        std::string softCaloTerm() const;

        // Reference term of the particular objects
        std::string referenceTerm(xAOD::Type::ObjectType Type) const;
        // Final term
        std::string FinalMetTerm() const;
        std::string FinalTrackMetTerm() const;

        // Retrieve the SUSYTools pointer from the Tool Handle
        ST::SUSYObjDef_xAOD* SUSYToolsPtr();

    protected:
        bool isInitialized() const;
        bool IncludePhotons() const;
        bool IncludeTaus() const;

        StatusCode buildMET(xAOD::MissingETContainer*& MET, const std::string& met_name, const std::string& soft_term,
                            const xAOD::ElectronContainer* electrons = nullptr, const xAOD::MuonContainer* muons = nullptr,
                            const xAOD::TauJetContainer* taus = nullptr, const xAOD::PhotonContainer* photons = nullptr, bool doJvt = true);
        StatusCode buildTrackMET(xAOD::MissingETContainer*& MET, const std::string& met_name,
                                 const xAOD::ElectronContainer* electrons = nullptr, const xAOD::MuonContainer* muons = nullptr,
                                 const xAOD::TauJetContainer* taus = nullptr, const xAOD::PhotonContainer* photons = nullptr,
                                 bool doJvt = true);

        MetSignificanceHandler_Ptr newSignificanceTool(const std::string& grp, bool enabled = true);

        XAMPP::EventInfo* m_XAMPPInfo;
        asg::AnaToolHandle<ST::ISUSYObjDef_xAODTool> m_susytools;

    protected:
        template <typename T> ToolHandle<T> GetCPTool(const std::string& name) {
            return ToolHandle<T>(SUSYToolsPtr()->getProperty(name).toString());
        }

    private:
        asg::AnaToolHandle<XAMPP::IEventInfo> m_InfoHandle;
        std::vector<MetSignificanceHandler_Ptr> m_metSignifHandlers;

        MetSignificanceHandler_Ptr m_metSignif;
        MetSignificanceHandler_Ptr m_metSignif_noPUJets;
        MetSignificanceHandler_Ptr m_metSignif_noPUJets_noSoftTerm;
        MetSignificanceHandler_Ptr m_metSignif_dataJER;
        MetSignificanceHandler_Ptr m_metSignif_dataJER_noPUJets;
        MetSignificanceHandler_Ptr m_metSignif_phireso_noPUJets;

        bool m_init;
        bool m_doTrackMet;
        bool m_doMetCST;
        bool m_IncludePhotons;
        bool m_IncludeTaus;

    protected:
        ToolHandle<XAMPP::IElectronSelector> m_elec_selection;
        ToolHandle<XAMPP::IMuonSelector> m_muon_selection;
        ToolHandle<XAMPP::IJetSelector> m_jet_selection;
        ToolHandle<XAMPP::IPhotonSelector> m_phot_selection;
        ToolHandle<XAMPP::ITauSelector> m_tau_selection;
        ToolHandle<XAMPP::ISystematics> m_systematics;

    private:
        StatusCode addContainerToMet(xAOD::MissingETContainer* MET, const xAOD::IParticleContainer* particles, xAOD::Type::ObjectType type,
                                     const xAOD::IParticleContainer* invisible = nullptr);

        StatusCode markInvisible(xAOD::MissingETContainer* MET, const xAOD::IParticleContainer* invisible);

        StatusCode buildMET(xAOD::MissingETContainer* MET, const std::string& softTerm, bool doJvt = true,
                            const xAOD::IParticleContainer* invisible = nullptr, bool build_track = false);

        // xAOD containers to build the calibrated MET from the objects
        xAOD::MissingETContainer* m_MetTST;
        xAOD::MissingETContainer* m_MetCST;
        xAOD::MissingETContainer* m_MetTrack;
        //  Storages to pipe the final MET's into
        XAMPP::Storage<XAMPPmet>* m_sto_MetTST;
        XAMPP::Storage<XAMPPmet>* m_sto_MetCST;
        XAMPP::Storage<XAMPPmet>* m_sto_MetTrack;

        // xAOD containers used to build the MET from
        const xAOD::MissingETContainer* m_xAODMet;
        const xAOD::MissingETAssociationMap* m_xAODMap;
        const xAOD::MissingETContainer* m_xAODTrackMet;

        std::string m_met_key;
        std::string m_met_map_key;
        std::string m_met_track_key;

        std::string m_systName;

        bool m_store_significance;

        // Stuff to calculate the met itself not using SUSYTools
        ToolHandle<IMETMaker> m_metMaker;
        ToolHandle<IMETSystematicsTool> m_metSystTool;

        // Reference terms to get the suff from
        std::string m_EleRefTerm;
        std::string m_MuoRefTerm;
        std::string m_TauRefTerm;
        std::string m_JetRefTerm;
        std::string m_PhoRefTerm;

        std::string m_TrackSoftTerm;
        std::string m_CaloSoftTerm;

        std::string m_FinalMetTerm;
        std::string m_FinalTrackTerm;

        bool m_trkJetsyst;
        bool m_trkMETsyst;
    };
}  // namespace XAMPP
#endif
