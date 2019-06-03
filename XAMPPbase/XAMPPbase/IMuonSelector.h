// IMuonSelector.h

#ifndef XAMPPBASE_IMuonSelector_H
#define XAMPPBASE_IMuonSelector_H

#include <AsgTools/IAsgTool.h>

#include <XAMPPbase/MuonDecorations.h>
#include <xAODMuon/Muon.h>
#include <xAODMuon/MuonContainer.h>

namespace CP {
    class SystematicSet;
}

namespace XAMPP {
    typedef ElementLink<xAOD::MuonContainer> MuoLink;

    class IMuonSelector : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(IMuonSelector)

    public:
        virtual StatusCode LoadContainers() = 0;
        virtual StatusCode InitialFill(const CP::SystematicSet& systset) = 0;
        virtual StatusCode FillMuons(const CP::SystematicSet& systset) = 0;
        virtual StatusCode LoadSelection(const CP::SystematicSet& systset) = 0;

        virtual MuoLink GetLink(const xAOD::Muon& mu) const = 0;
        virtual MuoLink GetOrigLink(const xAOD::Muon& mu) const = 0;
        virtual const xAOD::MuonContainer* GetMuonContainer() const = 0;

        /// Returns the unskimmed container of all calibrated Muon objects
        virtual xAOD::MuonContainer* GetMuons() const = 0;
        /// The pre container only contains Muonss passing the basic kinematic requirements and the baseline selection WP
        virtual xAOD::MuonContainer* GetPreMuons() const = 0;

        /// Baseline Muon candidates are preMuons surviving the overlap removal procedure
        /// No additional quality requirement is made
        virtual xAOD::MuonContainer* GetBaselineMuons() const = 0;

        /// Signal Muons have to pass all final selection requirements
        virtual xAOD::MuonContainer* GetSignalMuons() const = 0;

        /// Signal quality muons are all objects which pass the baseline & signal kinematics
        /// as well as the quality selection criteria. No overlap removal is considered.
        virtual xAOD::MuonContainer* GetSignalNoORMuons() const = 0;

        virtual xAOD::MuonContainer* GetCustomMuons(const std::string& kind) const = 0;

        virtual StatusCode SaveScaleFactor() = 0;

        virtual std::shared_ptr<MuonDecorations> GetMuonDecorations() const = 0;

        virtual ~IMuonSelector() = default;
    };
}  // namespace XAMPP
#endif
