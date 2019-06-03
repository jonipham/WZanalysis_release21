#ifndef XAMPPbase_IJetSelector_H
#define XAMPPbase_IJetSelector_H

#include <AsgTools/IAsgTool.h>
#include <XAMPPbase/Defs.h>
#include <XAMPPbase/JetDecorations.h>
#include <xAODJet/JetAuxContainer.h>
#include <xAODJet/JetContainer.h>
namespace CP {
    class SystematicSet;
}

namespace XAMPP {

    class IJetSelector : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(IJetSelector)

    public:
        virtual StatusCode LoadContainers() = 0;
        virtual StatusCode InitialFill(const CP::SystematicSet& systset) = 0;
        virtual StatusCode FillJets(const CP::SystematicSet& systset) = 0;

        // returns all (calibrated) xAOD::Jets in order to calculate the missing
        // transverse energy
        virtual xAOD::JetContainer* GetJets() const = 0;

        // returns jets with a very basic selection for the overlap removal
        virtual xAOD::JetContainer* GetPreJets(int Radius = 4) const = 0;

        // returns jets which passed the overlap removal
        virtual xAOD::JetContainer* GetBaselineJets(int Radius = 4) const = 0;

        // returns a selection of 'final' jets to be used for further analysis
        virtual xAOD::JetContainer* GetSignalJets(int Radius = 4) const = 0;
        virtual xAOD::JetContainer* GetSignalNoORJets(int Radius = 4) const = 0;

        virtual xAOD::JetContainer* GetBJets() const = 0;
        virtual xAOD::JetContainer* GetLightJets() const = 0;
        virtual xAOD::JetContainer* GetCustomJets(const std::string& kind) const = 0;

        virtual StatusCode ReclusterJets(const xAOD::IParticleContainer* inputJets, float Rcone, float minPtKt4 = -1,
                                         std::string PreFix = "", float minPtRecl = -1, float rclus = 0., float ptfrac = -1) = 0;
        virtual StatusCode SaveScaleFactor() = 0;

        virtual std::shared_ptr<JetDecorations> GetJetDecorations() const = 0;

        virtual ~IJetSelector() {}
    };
}  // namespace XAMPP
#endif
