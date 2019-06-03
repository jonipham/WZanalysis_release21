#ifndef XAMPPbase_IAnalysisModule_H
#define XAMPPbase_IAnalysisModule_H
#include <AsgTools/IAsgTool.h>

namespace XAMPP {
    class IAnalysisModule : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(XAMPP::IAnalysisModule)
    public:
        virtual StatusCode initialize() = 0;
        virtual StatusCode bookVariables() = 0;
        virtual StatusCode fill() = 0;
        virtual ~IAnalysisModule() {}
    };
}  // namespace XAMPP
#endif
