#ifndef XAMPPBASE_ParticleSelector_H
#define XAMPPBASE_ParticleSelector_H

#include <AsgTools/AsgTool.h>

#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/ParticleDecorations.h>
// EDM includes mandatory for the Template functions
#include <AsgTools/AnaToolHandle.h>
#include <AsgTools/ToolHandle.h>
#include <XAMPPbase/Defs.h>
#include <xAODBase/ObjectType.h>
#include <xAODCore/ShallowCopy.h>
#include <memory>

namespace CP {
    class SystematicSet;
}
namespace XAMPP {

    class IEventInfo;
    class EventInfo;
    class ISystematics;
    class IPartilceWeightDecorator;

    class ParticleSelector : public asg::AsgTool {
    public:
        ParticleSelector(const std::string& myname);
        virtual ~ParticleSelector();
        virtual StatusCode initialize();

        // Typedefs & enums of the class
        enum LinkStatus { Created, Loaded, Failed };
        typedef std::pair<float, float> EtaRange;
        typedef std::vector<EtaRange> EtaRangeVector;
        enum ScaleFactorMapContains { SignalSf, BaselineSf, SignalAndBaseSf };

    protected:
        ToolHandle<XAMPP::ISystematics> m_systematics;
        XAMPP::EventInfo* m_XAMPPInfo;

        // Set the internatl systematics pointer for container
        // selection during the event loop
        void SetSystematics(const CP::SystematicSet& Set);
        void SetSystematics(const CP::SystematicSet* Set);

        // Name of the main container used by this ParticleSelector
        // Can only be set once and never changed after wards
        void SetContainerKey(const std::string& Key);
        // What kind of object is this Selector.
        // Please consult the XAMPPbase/Defs.h for the
        // different objects
        void SetObjectType(SelectionObject Type);

        // Getter methods for the  ContainerKey and ObjectType()
        const std::string& ContainerKey() const;
        XAMPP::SelectionObject ObjectType() const;

        // Boolean set if ParticleSelector::initialize() is called
        bool isInitialized() const;

        // Is an object type enabled by the user or not
        // Information is distributed by the systematic tool
        bool ProcessObject(XAMPP::SelectionObject T) const;
        // Is the object covered by this selector enabled or not
        bool ProcessObject() const;
        // Current run is data. From the systematics tool
        bool isData() const;

        //  XAMPP follows the philosophy of a three step selection
        //  1) Basic kinematic requirements and quality
        //  2) Overlap removal to resolve ambiguities
        //  3) Signal requirements applied on top, like raising pt cut, tighter object selection

        // The following methods perform the kinematic selections. Their cuts can be set
        // via properties

        // BaselinePtCut, BaselineEtaCut, ExcludeBaselineEta applies for 1-3
        virtual bool PassBaselineKinematics(const xAOD::IParticle& P) const;
        // signalPtCut, SignalEtaCut, ExcludeSignalInEta applies for 3)
        virtual bool PassSignalKinematics(const xAOD::IParticle& P) const;

        // Same as above but they accept a pointer
        bool PassBaselineKinematics(const xAOD::IParticle* P) const;
        bool PassSignalKinematics(const xAOD::IParticle* P) const;

        // These three methods ask whether the object passes the
        // quality or not on top of the kinematic requirements.
        // In general they should be used to perform the selection inside
        // XAMPP. For the quality selection extra decorators are used whose
        // names can be set via properties.

        // Selection 3) using SignalDecorator (signal) + PassSignalKinematics
        virtual bool PassSignal(const xAOD::IParticle& P) const;
        // Selection 2) using BaselineDecorator (passOR) + PassBaselineKinematics
        virtual bool PassBaseline(const xAOD::IParticle& P) const;
        // Selection 1) using PreSelectionDecorator (baseline) + PassBaselineKinematics
        virtual bool PassPreSelection(const xAOD::IParticle& P) const;

        // Extra flag to ask the isolation of electrons/muons/photons
        // using IsolationDecorator (isol) as extra decoration
        bool PassIsolation(const xAOD::IParticle& P) const;

        // Extra flag to ask whether the signal quality and kinematic
        // selection criteria are passed using the SignalDecorator (signal)
        // as input
        bool PassSignalNoOR(const xAOD::IParticle& P) const;

        // Passing methods with pointer as input arguments
        bool PassSignal(const xAOD::IParticle* P) const;
        bool PassBaseline(const xAOD::IParticle* P) const;
        bool PassPreSelection(const xAOD::IParticle* P) const;
        bool PassIsolation(const xAOD::IParticle* P) const;
        bool PassSignalNoOR(const xAOD::IParticle* P) const;
        // Sets baseline/passOR/signal to true or false
        void SetSelectionDecorators(const xAOD::IParticle& P, bool Pass) const;
        // Sets baseline decorator + the input decorartor used by assicoationUtils
        // which can also be set via ORUtilsSelectionDecorator (selected) property
        // Somehow AssociationUtils allows for an hierachy of the OR using integers
        // to me believe as higher the integer as less important is the object in the
        // overlatp removal. The default integer for this selector can be set in
        // via ORUtilsSelectionFlag (1).
        void SetPreSelectionDecorator(const xAOD::IParticle& P, bool Pass) const;
        // Here you can overwrite the decorator explicitly
        void SetOverlapInDecorator(const xAOD::IParticle& P, int Pass) const;
        // Here you can overwrite the isolation decorator explicitly
        void SetIsolationDecorator(const xAOD::IParticle& P, int Pass) const;
        // Sets the flag of the `passOR` decorator
        void SetBaselineDecorator(const xAOD::IParticle& P, bool Pass) const;
        // Sets the flag of the `signal decorator
        void SetSignalDecorator(const xAOD::IParticle& P, bool Pass) const;

        // Raw decorations of the tool
        bool GetPreSelectionDecorator(const xAOD::IParticle& P_PGID) const;
        int GetOverlapInDecorator(const xAOD::IParticle& P) const;

        virtual bool GetBaselineDecorator(const xAOD::IParticle& P_PGID) const;
        virtual bool GetSignalDecorator(const xAOD::IParticle& P_PGID) const;

        // Implementation using the Particle pointers

        void SetSelectionDecorators(const xAOD::IParticle* P, bool Pass) const;
        void SetPreSelectionDecorator(const xAOD::IParticle* P, bool Pass) const;
        void SetOverlapInDecorator(const xAOD::IParticle* P, int Pass) const;
        void SetIsolationDecorator(const xAOD::IParticle* P, int Pass) const;
        void SetBaselineDecorator(const xAOD::IParticle* P, bool Pass) const;
        void SetSignalDecorator(const xAOD::IParticle* P, bool Pass) const;

        bool GetPreSelectionDecorator(const xAOD::IParticle* P_PGID) const;
        int GetOverlapInDecorator(const xAOD::IParticle* P) const;
        bool GetBaselineDecorator(const xAOD::IParticle* P_PGID) const;
        bool GetSignalDecorator(const xAOD::IParticle* P_PGID) const;

        // Helper method to create char AuxElements from a string. The string is emptied afterwards
        StatusCode CreateAuxElements(std::string& name, SelectionAccessor& acc, SelectionDecorator& dec);
        // Exclude eta ranges from the selections, like the crack, etc.
        // The StringVector must be of the form {"-1.5;-1.3", "0;0.1", etc.}
        StatusCode ExtractEtaRanges(StringVector& propertyVector, EtaRangeVector& rangeVector);
        // Returns true if the particle in one of the forbidden ranges
        bool IsInEtaRange(const xAOD::IParticle& P, const EtaRangeVector& ranges) const;

        // have this function in the particle selector for using it
        // independently in JetSelector and TruthSelector
        virtual StatusCode ReclusterJets(const xAOD::IParticleContainer* inputJets, float Rcone, float minPtKt4 = -1,
                                         std::string PreFix = "", float minPtRecl = -1, float rclus = 0, float ptfrac = -1);

        // Creates an view elements container and saves it in the storegate
        template <typename Container> StatusCode ViewElementsContainer(const std::string& Key, Container*& Cont);
        // Retrieves the container from the storegate with the key used above. Optionally the nominal
        // container can be loaded from the storegate
        template <typename Container>
        StatusCode LoadViewElementsContainer(const std::string& Key, Container*& Cont, bool LoadNominal = false) const;
        // Load a generic container from the store gate.
        template <typename Container> StatusCode LoadContainer(const std::string& Key, const Container*& Cont) const;
        template <typename Container> StatusCode LoadContainer(const std::string& Key, Container*& Cont) const;
        // Connect the tool handles with the systematic tool
        template <typename T>
        StatusCode DeclareAsWeightSyst(ToolHandle<T>& handle, XAMPP::SelectionObject O = XAMPP::SelectionObject::Other);
        template <typename T> StatusCode DeclareAsKineSyst(ToolHandle<T>& handle, XAMPP::SelectionObject O = XAMPP::SelectionObject::Other);

        template <typename T>
        StatusCode DeclareAsWeightSyst(asg::AnaToolHandle<T>& handle, XAMPP::SelectionObject O = XAMPP::SelectionObject::Other);
        template <typename T>
        StatusCode DeclareAsKineSyst(asg::AnaToolHandle<T>& handle, XAMPP::SelectionObject O = XAMPP::SelectionObject::Other);

        // Name of the current systematic in the StoreGate
        // Getter method of the basic storename used to save all containers
        // from this selector into the store gate
        const std::string& StoreName() const;
        // Current systematic name set by SetSystematics
        std::string SystName(bool InclUnderScore = true) const;

        // Is a given systematic affecting the kinemtic of this
        // Particle type. Known systematics are distributed from
        // the SystematicsTool.
        bool SystematicAffects(const CP::SystematicSet& Set) const;
        bool SystematicAffects(const CP::SystematicSet* Set) const;

        // This function creates the ShallowCopy links of the particle
        // containers if the current selector is actually affected by the
        // kinematic systematic Otherwise the nominal container is loaded from
        // the StoreGate
        //   LinkStatus::Created --> new shallow copy made
        //   LinkStatus::Loaded --> nominal copy from storegate retrieved
        //   LinkStatus::Failed --> error
        // The flag "linkOriginal" allows the user to steer whether to create the OriginalObjectLink for the copy.
        // This is needed for some detector-level use cases (e.g iso correction),
        // but can be turned off to save CPU if not required (e.g. truth).
        template <typename Container>
        ParticleSelector::LinkStatus CreateContainerLinks(const std::string& Key, Container*& Cont, bool linkOriginal = true);
        template <typename Container>
        ParticleSelector::LinkStatus CreateContainerLinks(const std::string& Key, Container*& Cont,
                                                          xAOD::ShallowAuxContainer*& AuxContainer, bool linkOriginal = true);

        //
        // Helper method to store the particle weights
        //
        // Write the scale-factors of the particle to the tree. To be called after
        // all weights are setup
        StatusCode SaveObjectSF(ParticleStorage* Storage);

        // IParticleWeight decorator as the standard class to handle the scale-factors
        // sf_type -> Full/Reco/ID/Isolation/BTagging
        // content -> SignalSf, BaselineSf, SignalAndBaseSf
        // save -> save what's in there to the tree
        // base_name -> Replace the first 3 letters of the container key by base_name
        StatusCode initIParticleWeight(IPartilceWeightDecorator& weighter, const std::string& sf_type, const CP::SystematicSet* syst_set,
                                       unsigned int content, bool save = true, const std::string& base_name = "");

        // Mapping between the AuxElements for the total Scale-factors to the particle specific ones
        StatusCode initEventSignalSf(XAMPP::Storage<double>*& store_ptr, const std::string& suffix, const CP::SystematicSet* set, bool save,
                                     const std::string& basename = "");
        StatusCode initEventBaselineSf(XAMPP::Storage<double>*& store_ptr, const std::string& suffix, const CP::SystematicSet* set,
                                       bool save, const std::string& basename = "");

        // set up the strings to use in our particle decorations, based on
        // the tool properties.
        // Will also instantiate the decorations provider, *if* it has
        // not already been done so (e.g. by an upstream tool)
        // If an non-null argument is provided, this pointer will
        // be used for our m_particleDecorations member.
        // Use this to set it in inheriting classes.
        void setupDecorations(std::shared_ptr<ParticleDecorations> input = nullptr);

    private:
        float m_baselinePt;
        float m_baselineEta;
        float m_signalPt;
        float m_signalEta;

        StringVector m_baseEtaExcludeProperty;
        EtaRangeVector m_baseEtaExclude;
        StringVector m_signalEtaExcludeProperty;
        EtaRangeVector m_signalEtaExclude;

        bool m_hasBaseToExclude;
        bool m_hasSignalToExclude;

        const CP::SystematicSet* m_ActSys;
        std::string m_ContainerKey;
        std::string m_storeName;
        SelectionObject m_ObjectType;
        bool m_init;

        std::string m_PreSelDecorName;
        SelectionAccessor m_acc_presel;
        SelectionDecorator m_dec_presel;

        std::string m_BaselineDecorName;
        SelectionAccessor m_acc_baseline;
        SelectionDecorator m_dec_baseline;

        std::string m_SignalDecorName;
        SelectionAccessor m_acc_signal;
        SelectionDecorator m_dec_signal;

        std::string m_IsolDecorName;
        SelectionAccessor m_acc_isol;
        SelectionDecorator m_dec_isol;

        std::string m_ORutilsDecorName;
        SelectionAccessor m_acc_ORUtils_in;
        SelectionDecorator m_dec_ORUtils_in;
        int m_ORUtils_InFlag;
        mutable bool m_syst_checked;
        std::vector<XAMPP::Storage<double>*> m_eventSFstores;
        bool m_WriteSFperParticle;
        asg::AnaToolHandle<XAMPP::IEventInfo> m_EvInfoHandle;
        // this packages our decorations.
        // Private since we use object-specific decorations in the inheriting
        // tools, which get their own pointer to the object-specific
        // class (pointing to the same object)
        std::shared_ptr<ParticleDecorations> m_particleDecorations;

        bool checkForValidSystematics() const;

        std::shared_ptr<DoubleDecorator> getParticleSfDecorator(XAMPP::Storage<double>* SF_decorator);
        std::shared_ptr<DoubleAccessor> getParticleSfAccessor(XAMPP::Storage<double>* SF_decorator);
    };

    class IPartilceWeightDecorator {
    public:
        IPartilceWeightDecorator();
        virtual ~IPartilceWeightDecorator();
        // apply the SF if the container is empty
        virtual StatusCode applySF();
        StatusCode initEvent();

        // Get the scale-factor from the particle
        double getSF(const xAOD::IParticle& particle) const;
        bool isSFcalculated(const xAOD::IParticle& particle) const;

        double getBaselineEventSF() const;
        double getSignalEventSF() const;
        bool hasBaselineSF() const;
        bool hasSignalSF() const;

        void setEventSfStores(XAMPP::Storage<double>* base, XAMPP::Storage<double>* signal);
        void setSfDecorators(std::shared_ptr<DoubleDecorator> dec, std::shared_ptr<DoubleAccessor> acc);

    protected:
        // Save the efficiency scale-factor to particle and baseline/signal
        // storage element
        StatusCode saveEventSF(const xAOD::IParticle& particle, double SF, bool isSignal);
        StatusCode saveEventSF(double SF, bool isSignal);
        StatusCode saveBaselineSF(double SF);
        StatusCode saveSignalSF(double SF);

    private:
        bool isStoreLocked(XAMPP::Storage<double>*& event_store) const;
        StatusCode applyEventSF(XAMPP::Storage<double>*& event_store, const double& SF);
        StatusCode applyEventSF(XAMPP::Storage<double>*& event_store);

        // Pointers to attach the SF to each muon itself
        std::shared_ptr<DoubleDecorator> m_part_SF_decor;
        std::shared_ptr<DoubleAccessor> m_part_SF_acc;
        // Decorators to check whether the SF has already
        // been calculated for this particle
        std::unique_ptr<BoolDecorator> m_part_isEval_decor;
        std::unique_ptr<BoolAccessor> m_part_isEval_acc;

        // Event storages to the baseline
        // and signal event SF
        XAMPP::Storage<double>* m_event_base_SF;
        XAMPP::Storage<double>* m_event_signal_SF;
    };

}  // namespace XAMPP
#include <XAMPPbase/ParticleSelector.ixx>
#endif
