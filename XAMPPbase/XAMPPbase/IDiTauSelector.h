#ifndef XAMPPbase_IDiTauSelector_H
#define XAMPPbase_IDiTauSelector_H

#include <AsgTools/IAsgTool.h>

#include <xAODTau/DiTauJet.h>
#include <xAODTau/DiTauJetContainer.h>

namespace CP {
    class SystematicSet;
}

namespace XAMPP {

    class IDiTauSelector : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(IDiTauSelector)

    public:
        virtual StatusCode LoadContainers() = 0;
        virtual StatusCode InitialFill(const CP::SystematicSet& systset) = 0;
        virtual StatusCode FillDiTaus(const CP::SystematicSet& systset) = 0;

        virtual const xAOD::DiTauJetContainer* GetDiTauContainer() const = 0;
        virtual xAOD::DiTauJetContainer* GetDiTaus() const = 0;
        virtual xAOD::DiTauJetContainer* GetPreDiTaus() const = 0;
        virtual xAOD::DiTauJetContainer* GetSignalDiTausNoOR() const = 0;
        virtual xAOD::DiTauJetContainer* GetSignalDiTaus() const = 0;
        virtual xAOD::DiTauJetContainer* GetBaselineDiTaus() const = 0;
        virtual xAOD::DiTauJetContainer* GetCustomDiTaus(const std::string& kind) const = 0;

        virtual ~IDiTauSelector() {}
    };
}  // namespace XAMPP
#endif
