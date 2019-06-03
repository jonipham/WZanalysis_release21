#ifndef XAMPPbase_IPhotonSelector_H
#define XAMPPbase_IPhotonSelector_H

#include <AsgTools/IAsgTool.h>
#include <XAMPPbase/PhotonDecorations.h>
#include <xAODEgamma/Photon.h>
#include <xAODEgamma/PhotonContainer.h>
namespace CP {
    class SystematicSet;
}

namespace XAMPP {

    typedef ElementLink<xAOD::PhotonContainer> PhoLink;
    class IPhotonSelector : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(IPhotonSelector)

    public:
        virtual StatusCode LoadContainers() = 0;
        virtual StatusCode InitialFill(const CP::SystematicSet& systset) = 0;
        virtual StatusCode FillPhotons(const CP::SystematicSet& systset) = 0;

        virtual PhoLink GetLink(const xAOD::Photon& ph) const = 0;
        virtual PhoLink GetOrigLink(const xAOD::Photon& ph) const = 0;
        virtual const xAOD::PhotonContainer* GetPhotonContainer() const = 0;

        /// Returns the unskimmed container of all calibrated photon objects
        virtual xAOD::PhotonContainer* GetPhotons() const = 0;
        /// The pre container only contains photons passing the basic kinematic requirements and the baseline selection WP
        virtual xAOD::PhotonContainer* GetPrePhotons() const = 0;

        /// Baseline photon candidates are preTaus surviving the overlap removal procedure
        /// No additional quality requirement is made
        virtual xAOD::PhotonContainer* GetBaselinePhotons() const = 0;

        /// Signal photons have to pass all final selection requirements
        virtual xAOD::PhotonContainer* GetSignalPhotons() const = 0;

        /// Signal quality taus are all taus which pass the baseline & signal kinematics
        /// as well as the quality selection criteria. No overlap removal is considered.
        virtual xAOD::PhotonContainer* GetSignalNoORPhotons() const = 0;

        virtual xAOD::PhotonContainer* GetCustomPhotons(const std::string& kind) const = 0;
        virtual StatusCode SaveScaleFactor() = 0;
        virtual ~IPhotonSelector() {}
        virtual std::shared_ptr<PhotonDecorations> GetPhotonDecorations() const = 0;
    };
}  // namespace XAMPP
#endif
