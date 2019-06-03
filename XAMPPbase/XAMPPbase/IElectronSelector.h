#ifndef XAMPPbase_IElectronSelector_H
#define XAMPPbase_IElectronSelector_H

#include <AsgTools/IAsgTool.h>
#include <XAMPPbase/ElectronDecorations.h>

#include <xAODEgamma/Electron.h>
#include <xAODEgamma/ElectronContainer.h>

namespace CP {
    class SystematicSet;
}
namespace XAMPP {
    typedef ElementLink<xAOD::ElectronContainer> EleLink;

    class IElectronSelector : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(IElectronSelector)

    public:
        virtual StatusCode LoadContainers() = 0;
        virtual StatusCode InitialFill(const CP::SystematicSet& systset) = 0;
        virtual StatusCode FillElectrons(const CP::SystematicSet& systset) = 0;

        virtual StatusCode LoadSelection(const CP::SystematicSet& systset) = 0;

        virtual EleLink GetLink(const xAOD::Electron& el) const = 0;
        virtual EleLink GetOrigLink(const xAOD::Electron& el) const = 0;

        virtual const xAOD::ElectronContainer* GetElectronContainer() const = 0;
        /// Returns the unskimmed container of all calibrated Electron objects
        virtual xAOD::ElectronContainer* GetElectrons() const = 0;
        /// The pre container only contains Electronss passing the basic kinematic requirements and the baseline selection WP
        virtual xAOD::ElectronContainer* GetPreElectrons() const = 0;

        /// Baseline Electron candidates are preElectrons surviving the overlap removal procedure
        /// No additional quality requirement is made
        virtual xAOD::ElectronContainer* GetBaselineElectrons() const = 0;

        /// Signal Electrons have to pass all final selection requirements
        virtual xAOD::ElectronContainer* GetSignalElectrons() const = 0;

        /// Signal quality muons are all objects which pass the baseline & signal kinematics
        /// as well as the quality selection criteria. No overlap removal is considered.
        virtual xAOD::ElectronContainer* GetSignalNoORElectrons() const = 0;

        virtual xAOD::ElectronContainer* GetCustomElectrons(const std::string& kind) const = 0;
        virtual StatusCode SaveScaleFactor() = 0;

        virtual std::shared_ptr<ElectronDecorations> GetElectronDecorations() const = 0;

        virtual ~IElectronSelector() = default;
    };
}  // namespace XAMPP
#endif
