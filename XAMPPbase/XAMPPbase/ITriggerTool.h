// ITriggerTool.h

#ifndef XAMPPbase_ITriggerTool_H
#define XAMPPbase_ITriggerTool_H

#include <AsgTools/IAsgTool.h>
#include <xAODBase/ObjectType.h>

#include <XAMPPbase/IElectronSelector.h>
#include <XAMPPbase/IMuonSelector.h>
#include <XAMPPbase/IPhotonSelector.h>
#include <XAMPPbase/ITauSelector.h>

#include <memory>
namespace XAMPP {
    typedef std::vector<EleLink> MatchedEl;
    typedef std::vector<MuoLink> MatchedMuo;
    typedef std::vector<PhoLink> MatchedPho;
    typedef std::vector<TauLink> MatchedTau;

    class ParticleStorage;
    class TriggerInterface;
    class ITriggerTool : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(ITriggerTool)

    public:
        virtual bool CheckTrigger() = 0;
        virtual bool CheckTriggerMatching() = 0;
        virtual bool CheckTrigger(const std::string& trigger_name) = 0;
        virtual bool IsMatchedObject(const xAOD::IParticle* el, const std::string& Trig) const = 0;

        virtual StatusCode SaveObjectMatching(ParticleStorage* Storage, xAOD::Type::ObjectType Type) = 0;

        virtual std::vector<std::shared_ptr<TriggerInterface>> GetActiveTriggers() const = 0;
        virtual std::shared_ptr<TriggerInterface> GetActiveTrigger(const std::string& trig_name) const = 0;

        // Retrieve the OR of triggers
        virtual std::vector<std::string> GetTriggerOR(const std::string& trig_string) = 0;

        virtual ~ITriggerTool() = default;
    };
}  // namespace XAMPP
#endif
