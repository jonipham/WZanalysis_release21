#ifndef XAMPPbase_IEventInfo_H
#define XAMPPbase_IEventInfo_H

#include <AsgTools/IAsgTool.h>
#include <XAMPPbase/Defs.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODTracking/Vertex.h>

namespace CP {
    class SystematicSet;
}
namespace XAMPP {
    class ParticleStorage;
    class DataVectorStorage;
    class SystematicGroup;
    class IEventInfo : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(XAMPP::IEventInfo)
    public:
        enum OutputElement {
            Tree = 1,
            Histo = 1 << 1,
        };

        virtual StatusCode initialize() = 0;
        virtual StatusCode LoadInfo() = 0;
        // sets the current systematic such that the underlying variables
        // and branches can be filled
        virtual StatusCode SetSystematic(const CP::SystematicSet* set) = 0;

        // const xAOD::Event Info not decorated with the calculated variables
        virtual const xAOD::EventInfo* GetOrigInfo() const = 0;
        // Current event info having all variables associated with this systematic
        virtual xAOD::EventInfo* GetEventInfo() const = 0;
        // Primary vertex in the event
        virtual const xAOD::Vertex* GetPrimaryVertex() const = 0;
        // Active kinematic systematic
        virtual const CP::SystematicSet* GetSystematic() const = 0;
        // Nominal systematic to be retrieved from systematics tool
        virtual const CP::SystematicSet* GetNominal() const = 0;

        // Does it pass the GRL/ has it vertices / Liquid argon cleaning
        virtual bool PassCleaning() const = 0;
        // Good starting point for MET systemtatics
        virtual StatusCode CopyInfoFromNominal(const CP::SystematicSet* To) = 0;

        /// Functions as a short cut from the event-info itself
        virtual bool isMC() const = 0;
        virtual unsigned long long eventNumber() const = 0;
        virtual int mcChannelNumber() const = 0;
        virtual unsigned int runNumber() const = 0;
        virtual unsigned int randomRunNumber() const = 0;
        // get the MC generator weight.
        // this will apply the outlier correction set by the user
        virtual double GetGenWeight(unsigned int idx = 0) const = 0;
        // the the MC generator weight *without* applying the outlier correction
        virtual double GetRawGenWeight(unsigned int idx = 0) const = 0;

        /// Propagation of the luminosity to the meta-data
        virtual double GetPileUpLuminosity() = 0;
        virtual bool ApplyPileUp() const = 0;

        // Function needed to shift dsid in metadata
        virtual int dsidOffSet() const = 0;
        virtual bool applyDSIDShift() const = 0;

        // Functions to create and retieve particle storages
        virtual StatusCode BookParticleStorage(const std::string& Name, bool StoreMass = false, bool SaveVariations = true,
                                               bool saveTrees = true) = 0;
        // The storage is going to be booked into the common tree and available for all systematic variations
        virtual StatusCode BookCommonParticleStorage(const std::string& Name, bool StoreMass = false, bool saveTrees = true) = 0;

        virtual ParticleStorage* GetParticleStorage(const std::string& Name) const = 0;
        // Functions to create and retrieve Storages associated to any type of
        // xAOD::Container
        virtual StatusCode BookContainerStorage(const std::string& Name, bool SaveVariations = true, bool saveTrees = true) = 0;
        virtual StatusCode BookCommonContainerStorage(const std::string& Name, bool saveTrees = true) = 0;

        /// Retrieves the container storage registered under a given name. ParticleStorages are
        /// returned as well. If the name does not exist nullptr is returned
        virtual DataVectorStorage* GetContainerStorage(const std::string& Name) const = 0;

        /// Retrieve all Container storages registered with this event info class
        virtual std::vector<DataVectorStorage*> GetContainerStorages(unsigned int e) const = 0;

        virtual StatusCode createSystematicGroup(const std::string& name, XAMPP::SelectionObject obj_type) = 0;
        virtual std::shared_ptr<SystematicGroup> getSystematicGroup(const std::string& name) const = 0;
        virtual const std::vector<std::shared_ptr<SystematicGroup>>& getSystematicGroups() const = 0;

        // Method which disables the adding of storages
        virtual void Lock() = 0;
        virtual bool isLocked() const = 0;

        // set a strategy for treating.... 'creative'... sherpa event weights
        typedef enum OutlierWeightStrategy {
            doNothing = 0,    // will not do anything for large weights
            ignoreEvent = 1,  // will remove events with crazy weights
            resetWeight = 2   // will renormalise crazy weights to +/-1 (PMG recommendation)
        } OutlierWeightStrategy;
        virtual int getOutlierWeightStrategy() const = 0;
        // allow clients to ask if the current event shows a pathological weight
        virtual bool isOutlierGenWeight(unsigned int idx = 0) const = 0;
        // check the w directly
        virtual bool isOutlierGenWeight(double w) const = 0;

        virtual ~IEventInfo() {}
    };
}  // namespace XAMPP
#endif
