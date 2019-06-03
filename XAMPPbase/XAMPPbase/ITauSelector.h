#ifndef XAMPPbase_ITauSelector_H
#define XAMPPbase_ITauSelector_H

#include <AsgTools/IAsgTool.h>

#include <XAMPPbase/TauDecorations.h>
#include <xAODTau/TauJet.h>
#include <xAODTau/TauJetContainer.h>
namespace CP {
    class SystematicSet;
}

namespace XAMPP {
    typedef ElementLink<xAOD::TauJetContainer> TauLink;

    class ITauSelector : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(ITauSelector)

    public:
        virtual StatusCode LoadContainers() = 0;
        virtual StatusCode InitialFill(const CP::SystematicSet& systset) = 0;
        virtual StatusCode FillTaus(const CP::SystematicSet& systset) = 0;

        virtual TauLink GetLink(const xAOD::TauJet& tau) const = 0;
        virtual TauLink GetOrigLink(const xAOD::TauJet& tau) const = 0;

        virtual const xAOD::TauJetContainer* GetTauContainer() const = 0;

        /// Returns the unskimmed container of all calibrated tau objects
        virtual xAOD::TauJetContainer* GetTaus() const = 0;
        /// The pre container only contains taus passing the basic kinematic requirements and the baseline selection WP
        virtual xAOD::TauJetContainer* GetPreTaus() const = 0;

        /// Baseline Tau candidates are preTaus surviving the overlap removal procedure
        /// No additional quality requirement is made
        virtual xAOD::TauJetContainer* GetBaselineTaus() const = 0;

        /// Signal taus have to pass all final selection requirements
        virtual xAOD::TauJetContainer* GetSignalTaus() const = 0;

        /// Signal quality taus are all taus which pass the baseline & signal kinematics
        /// as well as the quality selection criteria. No overlap removal is considered.
        virtual xAOD::TauJetContainer* GetSignalNoORTaus() const = 0;

        virtual xAOD::TauJetContainer* GetCustomTaus(const std::string& kind) const = 0;

        virtual StatusCode SaveScaleFactor() = 0;
        virtual ~ITauSelector() {}
        virtual std::shared_ptr<TauDecorations> GetTauDecorations() const = 0;
    };
}  // namespace XAMPP
#endif
