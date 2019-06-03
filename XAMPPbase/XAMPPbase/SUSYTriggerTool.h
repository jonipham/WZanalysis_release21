#ifndef XAMPPbase_SUSYTriggerTool_H
#define XAMPPbase_SUSYTriggerTool_H

#include <AsgTools/AnaToolHandle.h>
#include <AsgTools/AsgTool.h>
#include <AsgTools/ToolHandle.h>

#include <XAMPPbase/EventStorage.h>
#include <XAMPPbase/ITriggerTool.h>

#include <xAODBase/IParticleHelpers.h>
#include <xAODCore/ShallowCopy.h>

namespace ST {
    class ISUSYObjDef_xAODTool;
    class SUSYObjDef_xAOD;
}  // namespace ST

namespace Trig {
    // Need the TrigDecisionTool directly for getChainGroup, features, and GetPreScale
    class TrigDecisionTool;
    class IMatchingTool;
}  // namespace Trig

namespace XAMPP {

    class SUSYTriggerTool;
    class EventInfo;
    class IEventInfo;
    class ISystematics;

    class TriggerInterface {
    public:
        TriggerInterface(const std::string& Name, SUSYTriggerTool*);
        virtual ~TriggerInterface() = default;

        static void SaveTriggerPrescaling(bool B);
        /// If this flag is true then the information whether the
        /// trigger has fired and is matched is written to the ntuple.
        /// In the other case only the latter is written out
        static void SaveFullTriggerInfo(bool B);
        /// if this is flag is true a dummy matching decision is always assigned
        /// to the particle container since the matching decision is going to be dumped
        /// into the n-tuples. If no trigger fired -> no matching -> usally bail out of the matching
        /// procedure
        static void SaveObjectMatching(bool B);

        StatusCode initialize(XAMPP::EventInfo* Info);

        std::string name() const;
        std::string MatchStoreName() const;
        std::string StoreName() const;

        /// The newEvent evaluates if the trigger has fired
        void NewEvent();

        bool PassTrigger() const;

        /// Evaluates the trigger matching
        /// 1) Objects are matched via dR matching
        /// 2) pt_threshold is then checked
        /// 3) Count how many object pass the particular thresholds
        /// 4)   -> compare against matching requirement
        bool PassTriggerMatching();

        /// Checks whether the trigger also requires some matching
        /// either to electrons muons or photons
        bool NeedsTriggerMatching() const;
        /// Electron trigger matching is enabled if _eXX or _2eXX is found in the name
        bool NeedsElectronMatching() const;
        /// Checks whether the trigger is a muon trigger and requires matching to them
        bool NeedsMuonMatching() const;
        /// Trigger matching for taus
        bool NeedsTauMatching() const;

        /// Finally the photon trigger matching is as well supported
        bool NeedsPhotonMatching() const;

        /// Prints the extracted pt thresholds of the trigger foreach object
        std::string PrintMatchingThresholds() const;
        /// Checks whether the matching is done on a particle
        /// the information is recieved and stored to the original xAOD::IParticle
        bool isMatchingDone(const xAOD::IParticle* P) const;
        bool isMatchingDone(const xAOD::IParticle& P) const;
        /// Checks whether a particle is matched to the trigger in terms of the ordinary
        /// dR matching where no requirement is applied on the pt of the particle
        bool isMatched_dR(const xAOD::IParticle* P) const;
        bool isMatched_dR(const xAOD::IParticle& P) const;

        /// Checks whether the object is really matched to the trigger with applying the full matching chain
        bool isMatched(const xAOD::IParticle* P) const;
        bool isMatched(const xAOD::IParticle& P) const;

        StatusCode addTriggerPeriod(unsigned int begin, unsigned int end);

        int num_toMatch() const;
        int num_toMatch(const XAMPP::SelectionObject obj) const;

    private:
        unsigned int MatchObjectsToTrigger(const xAOD::IParticleContainer* calibrated_obj);
        bool isInPeriod() const;
        // Methods needed during initialization
        typedef std::pair<std::string, xAOD::Type::ObjectType> StringObjectMatching;
        typedef std::vector<StringObjectMatching> ObjMatchVec;
        struct FinalStrObjMatching {
            xAOD::Type::ObjectType Obj = xAOD::Type::ObjectType::Other;
            unsigned int Multiplicity = 0;
            size_t StringPosition = std::string::npos;
        };
        FinalStrObjMatching FindObjectInTriggerString(const std::string& TriggerString, const ObjMatchVec& Matching) const;

        void GetMatchingThresholds();
        int ExtractPtThreshold(std::string& TriggerString, int& M, xAOD::Type::ObjectType& T);
        bool AssignMatching(xAOD::Type::ObjectType T) const;

        std::string m_name;
        SUSYTriggerTool* m_TriggerTool;
        XAMPP::EventInfo* m_XAMPPInfo;

        ToolHandle<Trig::TrigDecisionTool> m_trigDecTool;
        ToolHandle<Trig::IMatchingTool> m_trigMatchTool;

        XAMPP::Storage<char>* m_TriggerStore;
        XAMPP::Storage<char>* m_MatchingStore;
        XAMPP::Storage<float>* m_PreScalingStore;

        CharDecorator m_MatchingDecorator;
        CharAccessor m_MatchingAccessor;

        BoolDecorator m_dec_isMatchingDone;
        BoolAccessor m_acc_isMatchingDone;

        bool m_MatchEle;
        bool m_MatchMuo;
        bool m_MatchTau;
        bool m_MatchPho;

        struct OfflineMatching {
            float PtThreshold = 0.;
            int Object = xAOD::Type::ObjectType::Other;
            bool ObjectMatched = false;
        };
        std::vector<OfflineMatching> m_Thresholds;

        typedef std::pair<unsigned int, unsigned int> run_range;
        std::vector<run_range> m_periods;
        bool m_has_periods;

        static bool m_SavePrescaling;
        static bool m_saveFullTrigInfo;
        static bool m_SaveObjMatching;
    };

    /// Helper class to handle whether any trigger associated to
    /// an object e.g. electrons, muons, photons has fired
    class FiredObjectTrigger {
    public:
        enum Association {
            Electron = 1,
            Muon = 1 << 1,
            Photon = 1 << 2,
            Tau = 1 << 3,
        };
        FiredObjectTrigger(const std::vector<std::shared_ptr<TriggerInterface>>& trigger_list, unsigned int assoc, int n_obj = -1);

        FiredObjectTrigger(const FiredObjectTrigger&) = delete;
        void operator=(const FiredObjectTrigger&) = delete;

        bool has_triggers() const;

        StatusCode checkTrigger();
        StatusCode checkMatching();
        StatusCode make_stores(XAMPP::EventInfo* info);

    private:
        unsigned int m_assoc;
        int m_num_obj;
        std::vector<std::shared_ptr<TriggerInterface>> m_assoc_triggers;
        XAMPP::Storage<char>* m_dec_is_fired;
        XAMPP::Storage<char>* m_dec_is_matched;
    };

    class SUSYTriggerTool : public asg::AsgTool, virtual public ITriggerTool {
    public:
        virtual ~SUSYTriggerTool();
        SUSYTriggerTool(const std::string& myname);
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(SUSYTriggerTool, XAMPP::ITriggerTool)

        virtual StatusCode initialize();
        // Checks the triggers at the beginning of the event
        virtual bool CheckTrigger();
        virtual bool CheckTriggerMatching();

        // These functions can be called after the Trigger matching has been
        // performed
        virtual bool IsMatchedObject(const xAOD::IParticle* p, const std::string& Trig) const;

        virtual bool CheckTrigger(const std::string& trigger_name);
        virtual StatusCode SaveObjectMatching(ParticleStorage* Storage, xAOD::Type::ObjectType Type);

        virtual xAOD::ElectronContainer* CalibElectrons() const;
        virtual xAOD::MuonContainer* CalibMuons() const;
        virtual xAOD::PhotonContainer* CalibPhotons() const;
        virtual xAOD::TauJetContainer* CalibTaus() const;

        virtual std::vector<std::shared_ptr<TriggerInterface>> GetActiveTriggers() const;
        virtual std::shared_ptr<TriggerInterface> GetActiveTrigger(const std::string& trig_name) const;

        virtual std::vector<std::string> GetTriggerOR(const std::string& trig_string);

    protected:
        bool isData() const;
        StatusCode MetTriggerEmulation();
        StatusCode GetHLTMet(float& met, const std::string& containerName, bool& doContainer);
        StatusCode FillTriggerVector(std::vector<std::string>& triggers_vector);

        enum LinkStatus { Created, Loaded, Failed };
        //  This method is used to create shallow copies from the
        //  xAOD::TriggerContainers They are *not* recommended to use for
        //  xAOD::IParticleContainers. Refer back to the proper
        //  ParticleSelectors to do ShallowCopies from ParticleContainers
        template <typename TriggerContainer> LinkStatus CreateContainerLinks(const std::string& key, TriggerContainer*& container);
        template <typename Container> StatusCode ViewElementsContainer(const std::string& Key, Container*& Cont);

        template <typename T> ToolHandle<T> GetCPTool(const std::string& name);

        ST::SUSYObjDef_xAOD* SUSYToolsPtr();

        XAMPP::EventInfo* m_XAMPPInfo;

    private:
        asg::AnaToolHandle<XAMPP::IEventInfo> m_XAMPPInfoHandle;
        ToolHandle<XAMPP::ISystematics> m_systematics;

    protected:
        asg::AnaToolHandle<ST::ISUSYObjDef_xAODTool> m_susytools;

        ToolHandle<Trig::TrigDecisionTool> m_trigDecTool;
        ToolHandle<Trig::IMatchingTool> m_trigMatchTool;

        ToolHandle<XAMPP::IElectronSelector> m_elec_selection;
        ToolHandle<XAMPP::IMuonSelector> m_muon_selection;
        ToolHandle<XAMPP::IPhotonSelector> m_phot_selection;
        ToolHandle<XAMPP::ITauSelector> m_tau_selection;

        std::vector<std::shared_ptr<TriggerInterface>> m_triggers;
        std::vector<std::shared_ptr<FiredObjectTrigger>> m_obj_trig;
        std::vector<std::string> m_trigger_names;

        bool m_init;
        bool m_Pass;
        bool m_NoCut;
        bool m_EmptyTriggerList;

        /// If this flag is turned into true. The information whether
        /// the triggers are dR matched are saved to the n-tuples if
        /// the SaveObjectMatching(ParticleStorage* Storage, xAOD::Type::ObjectType Type)
        /// method is called during initializeEventVariables() in SUSYAnalysisHelper
        bool m_StoreObjectMatching;
        bool m_StoreFullTriggerInfo;

        bool m_StorePreScaling;
        bool m_MetTrigEmulation;

        bool m_doLVL1Met;
        bool m_doCellMet;
        bool m_doMhtMet;
        bool m_doTopoClMet;
        bool m_doTopoClPufitMet;
        bool m_doTopoClPuetaMet;

        ///  If each of the flag is turned into true
        ///  then two dedicated branches are written to the
        ///  n-tuple indicating whether any of e.g. electron
        ///  trigger has fired in this round. Please be aware
        ///  that only the triggers on the list are considered.
        ///  If you like to use the lowest unprescaled trigger only
        ///  please use the `constrainToPeriods` method inside your
        ///  JobOptions...
        bool m_doMetTriggerPassed;
        bool m_doSingleElectronTriggerPassed;
        bool m_doSingleMuonTriggerPassed;
        bool m_doSinglePhotonTriggerPassed;

        XAMPP::Storage<char>* m_DecTrigger;
        XAMPP::Storage<char>* m_DecMatching;
        XAMPP::Storage<char>* m_DecMetTrigger;

        /// Flags to choose which container is going to be returned for the
        /// object matching of the triggers. If the flag is set to false then
        /// the baseline container is used instead
        bool m_useSignalElec;
        bool m_useSignalMuon;
        bool m_useSignalTau;
        bool m_useSignalPhot;
    };
}  // namespace XAMPP
#include <XAMPPbase/SUSYTriggerTool.ixx>
#endif
