#ifndef XAMPPbase_SUSYTruthAnalysisHelper_H
#define XAMPPbase_SUSYTruthAnalysisHelper_H

#include <XAMPPbase/SUSYAnalysisHelper.h>
namespace XAMPP {
    class SUSYTruthAnalysisHelper : public SUSYAnalysisHelper {
    public:
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(SUSYTruthAnalysisHelper, XAMPP::IAnalysisHelper)

        SUSYTruthAnalysisHelper(const std::string& myname);
        virtual ~SUSYTruthAnalysisHelper() {}

        virtual StatusCode LoadContainers();
        virtual StatusCode FillInitialObjects(const CP::SystematicSet* systset);
        virtual StatusCode FillObjects(const CP::SystematicSet* systset);
        virtual StatusCode RemoveOverlap();

        virtual bool EventCleaning() const { return true; }
        virtual bool CheckTrigger() { return true; }
        virtual StatusCode initialize();

    protected:
        virtual StatusCode initializeAnalysisTools() { return StatusCode::SUCCESS; }
        virtual StatusCode initializeEventVariables();
        virtual StatusCode ComputeEventVariables();
        virtual StatusCode initializeObjectTools();
        virtual StatusCode FillEventWeights() { return StatusCode::SUCCESS; }
        virtual bool PassObjectCleaning(const CP::SystematicSet*) const;
    };
}  // namespace XAMPP
#endif
