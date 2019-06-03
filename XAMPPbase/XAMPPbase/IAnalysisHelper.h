// IAnalysisHelper.h
#ifndef IAnalysisHelper_H
#define IAnalysisHelper_H

#include <XAMPPbase/Defs.h>
#include "AsgTools/IAsgTool.h"
namespace CP {
    class SystematicSet;
}

namespace XAMPP {
    class IAnalysisHelper : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(IAnalysisHelper)

    public:
        virtual bool AcceptEvent() = 0;

        virtual bool EventCleaning() const = 0;
        virtual bool CleanObjects(const CP::SystematicSet* systset) = 0;

        virtual StatusCode LoadContainers() = 0;
        virtual StatusCode FillInitialObjects(const CP::SystematicSet* systset) = 0;
        virtual StatusCode RemoveOverlap() = 0;
        virtual StatusCode FillObjects(const CP::SystematicSet* systset) = 0;
        virtual StatusCode CheckCutFlow(const CP::SystematicSet* systset) = 0;
        virtual StatusCode finalize() = 0;

        virtual bool CheckTrigger() = 0;

        virtual StatusCode FillEvent(const CP::SystematicSet* sys) = 0;
        // call GetMCXsec before GetMCFilterEff/GetMCkFactor/GetMCXsectTimesEff
        // since it computes the SUSY::finalState
        virtual unsigned int finalState() = 0;
        virtual double GetMCXsec(unsigned int mc_channel_number, unsigned int finalState = 0) = 0;
        // this allows to set an externally provided XS uncertainty, if is is defined.
        // the convention is to return the absolute relative variations.
        // the bool ref tells the client if a XS error is defined or not (if false, the following double refs are ignored).
        // For example, for a 3% down and 4% upward uncertainty, return 0.03 and 0.04.
        virtual void GetMCXsecErrors(bool& error_exists, double& rel_err_down, double& rel_err_up, unsigned int mc_channel_number,
                                     unsigned int finalState = 0) = 0;
        virtual double GetMCFilterEff(unsigned int mc_channel_number, unsigned int finalState = 0) = 0;
        virtual double GetMCkFactor(unsigned int mc_channel_number, unsigned int finalState = 0) = 0;
        virtual double GetMCXsectTimesEff(unsigned int mc_channel_number, unsigned int finalState = 0) = 0;

        virtual ~IAnalysisHelper() {}
    };
}  // namespace XAMPP
#endif
