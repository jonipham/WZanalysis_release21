#ifndef XAMPPbase_ISystematics_H
#define XAMPPbase_ISystematics_H

#include <AsgTools/IAsgTool.h>
#include <XAMPPbase/Defs.h>
#include <xAODBase/ObjectType.h>
#include <vector>

namespace CP {
    class SystematicSet;
}

namespace XAMPP {
    // Service interface class to handle extra tools setup by the users in  a
    // common way
    class ISystematicToolService {
    public:
        virtual StatusCode resetSystematic() = 0;
        virtual StatusCode setSystematic(const CP::SystematicSet* Set) = 0;
        virtual StatusCode initialize() = 0;
        virtual std::string name() const = 0;
        virtual ~ISystematicToolService() {}
    };
    class ISystematics : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(ISystematics)

    public:
        virtual std::vector<const CP::SystematicSet*> GetAllSystematics() const = 0;
        virtual std::vector<const CP::SystematicSet*> GetKinematicSystematics(
            XAMPP::SelectionObject T = XAMPP::SelectionObject::Other) const = 0;
        virtual std::vector<const CP::SystematicSet*> GetWeightSystematics(XAMPP::SelectionObject T = SelectionObject::Other) const = 0;

        virtual const CP::SystematicSet* GetNominal() const = 0;
        virtual const CP::SystematicSet* GetCurrent() const = 0;

        virtual bool AffectsOnlyMET(const CP::SystematicSet* Set) const = 0;
        virtual bool ProcessObject(XAMPP::SelectionObject T) const = 0;
        virtual bool isData() const = 0;
        virtual bool isAF2() const = 0;

        virtual StatusCode resetSystematics() = 0;
        virtual StatusCode setSystematic(const CP::SystematicSet* Set) = 0;

        virtual StatusCode InsertKinematicSystematic(const CP::SystematicSet& set,
                                                     XAMPP::SelectionObject T = XAMPP::SelectionObject::Other) = 0;
        virtual StatusCode InsertWeightSystematic(const CP::SystematicSet& set,
                                                  XAMPP::SelectionObject T = XAMPP::SelectionObject::Other) = 0;
        virtual StatusCode InsertSystematicToolService(ISystematicToolService* Service) = 0;

        virtual bool SystematicsFixed() const = 0;
        virtual StatusCode FixSystematics() = 0;
        virtual ~ISystematics() {}
    };

}  // namespace XAMPP
#endif
