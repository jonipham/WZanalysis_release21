#ifndef XAMPPbase_EventInfo_H
#define XAMPPbase_EventInfo_H

#include <XAMPPbase/EventStorage.h>

#include <XAMPPbase/IEventInfo.h>
#include <XAMPPbase/ISystematics.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODTracking/Vertex.h>

#include <AsgTools/AnaToolHandle.h>
#include <AsgTools/AsgTool.h>
#include <AsgTools/IAsgTool.h>
#include <AsgTools/ToolHandle.h>

#include <memory>

namespace CP {
    class SystematicSet;
    class IPileupReweightingTool;
}  // namespace CP
class IGoodRunsListSelectionTool;

namespace XAMPP {
    class ISystematics;
    class TreeBase;
    class EventInfo : public asg::AsgTool, virtual public IEventInfo {
    public:
        EventInfo(const std::string& myname);
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(EventInfo, XAMPP::IEventInfo)

        virtual StatusCode initialize();
        virtual StatusCode LoadInfo();
        virtual StatusCode SetSystematic(const CP::SystematicSet* set);

        virtual const xAOD::EventInfo* GetOrigInfo() const;
        virtual xAOD::EventInfo* GetEventInfo() const;
        virtual const xAOD::Vertex* GetPrimaryVertex() const;
        virtual bool isMC() const;
        virtual unsigned long long eventNumber() const;
        virtual int mcChannelNumber() const;
        virtual unsigned int runNumber() const;
        virtual unsigned int randomRunNumber() const;
        virtual unsigned int dataYear() const;
        virtual const CP::SystematicSet* GetSystematic() const;
        virtual const CP::SystematicSet* GetNominal() const;

        virtual bool PassCleaning() const;

        virtual double GetPileUpLuminosity();
        virtual bool ApplyPileUp() const;

        // Function needed to shift dsid in metadata
        virtual int dsidOffSet() const;
        virtual bool applyDSIDShift() const;

        // Functions to create and retrieve event variables saved to the output
        template <typename T> bool DoesVariableExist(const std::string& Name) const;
        template <typename T> StatusCode NewEventVariable(const std::string& Name, bool saveToTree = true, bool SaveVariations = true);
        template <typename T>
        StatusCode NewCommonEventVariable(const std::string& Name, bool saveToTree = true, bool SaveVariations = true);

        template <typename T> void RemoveVariableFromOutput(const std::string& Name);
        template <typename T> Storage<T>* GetVariableStorage(const std::string& Name) const;
        template <typename T> std::vector<Storage<T>*> GetStorages(unsigned int e) const;

        // Functions to create and retieve particle storages
        virtual StatusCode BookParticleStorage(const std::string& Name, bool StoreMass = false, bool SaveVariations = true,
                                               bool saveTrees = true);

        /// Storage which is piped to the transient particle tree if there is any
        virtual StatusCode BookCommonParticleStorage(const std::string& Name, bool StoreMass = false, bool saveToTree = true);

        virtual ParticleStorage* GetParticleStorage(const std::string& Name) const;

        // Functions to create and retrieve Storages associated to any type of
        // xAOD::Container
        virtual StatusCode BookContainerStorage(const std::string& Name, bool SaveVariations = true, bool saveToTree = true);
        virtual StatusCode BookCommonContainerStorage(const std::string& Name, bool saveTrees = true);

        virtual DataVectorStorage* GetContainerStorage(const std::string& Name) const;

        virtual std::vector<DataVectorStorage*> GetContainerStorages(unsigned int e) const;

        virtual double GetGenWeight(unsigned int idx = 0) const;
        virtual double GetRawGenWeight(unsigned int idx = 0) const;
        virtual StatusCode CopyInfoFromNominal(const CP::SystematicSet* To);

        virtual bool isLocked() const;
        virtual void Lock();
        virtual void LockKeeper();

        virtual StatusCode createSystematicGroup(const std::string& name, XAMPP::SelectionObject obj_type);
        virtual std::shared_ptr<SystematicGroup> getSystematicGroup(const std::string& name) const;
        virtual const std::vector<std::shared_ptr<SystematicGroup>>& getSystematicGroups() const;

        virtual int getOutlierWeightStrategy() const;
        // allow clients to ask if the current event shows a pathological weight
        virtual bool isOutlierGenWeight(unsigned int idx = 0) const;
        virtual bool isOutlierGenWeight(double w) const;
        virtual ~EventInfo();

    private:
        StatusCode FindPrimaryVertex();
        StatusCode GetInfoFromStore(const CP::SystematicSet* set);
        bool returnStorage(IStorage* store, unsigned int bit_mask) const;

        double GetPeriodWeight();
        StatusCode RunPRWTool();
        const xAOD::EventInfo* m_ConstEvtInfo;
        xAOD::EventInfo* m_EvtInfo;
        const xAOD::Vertex* m_primaryVtx;

        const CP::SystematicSet* m_ActSys;
        ToolHandle<XAMPP::ISystematics> m_systematics;
        asg::AnaToolHandle<CP::IPileupReweightingTool> m_prwTool;
        asg::AnaToolHandle<IGoodRunsListSelectionTool> m_GrlTool;
        bool m_ApplyPRW;
        bool m_ApplyGRL;
        bool m_Init;
        bool m_Locked;
        bool m_Filter;
        bool m_RecoFlags;
        bool m_MultiPRWPeriods;
        struct PileUpDecorators {
            XAMPP::Storage<double>* muWeight;
            XAMPP::Storage<float>* AverageCross;
            XAMPP::Storage<unsigned int>* RandomRunNumber;
            XAMPP::Storage<unsigned int>* RandomLumiBlock;
        };
        std::map<const CP::SystematicSet*, PileUpDecorators> m_DecPup;  // PileupWeight
        XAMPP::Storage<int>* m_nNVtx;
        XAMPP::Storage<char>* m_PassGRL;
        XAMPP::Storage<char>* m_PassLArTile;
        XAMPP::Storage<char>* m_HasVtx;
        XAMPP::Storage<float>* m_mu_density;
        /// In case people are messing up the DSIDs of the samples.
        /// The boolean must be activated to shift the DSIDS, however,
        /// it's an very ugly way of tweaking things and not actually
        /// recommended at all since it leads the concept of meta
        /// hillariously ad absurdum
        bool m_shiftDSISD;
        int m_dsidOffSet;

        int m_OutlierStrat;
        double m_outlierWeightThreshold;

        std::vector<std::shared_ptr<SystematicGroup>> m_systGroups;
    };
}  // namespace XAMPP
#include <XAMPPbase/EventInfo.ixx>
#endif
