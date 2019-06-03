// IMetSelection.h

#ifndef XAMPPbase_IMetSelection_H
#define XAMPPbase_IMetSelection_H

#include <AsgTools/IAsgTool.h>

#include <xAODMissingET/MissingETAuxContainer.h>
#include <xAODMissingET/MissingETContainer.h>

namespace CP {
    class SystematicSet;
}

namespace XAMPP {
    class IMetSelector : virtual public asg::IAsgTool {
    public:
        ASG_TOOL_INTERFACE(IMetSelector)

        virtual StatusCode LoadContainers() = 0;
        virtual StatusCode SaveScaleFactor() = 0;
        virtual StatusCode FillMet(const CP::SystematicSet& systset) = 0;

        virtual xAOD::MissingETContainer* GetCustomMet(const std::string& kind = "") const = 0;

        //  Adds the particle to an existing invisible IParticleContainer, which
        //  is created on the fly The name of the container must match the name
        //  of the MET where the particles are marked as invisible
        virtual StatusCode addToInvisible(xAOD::IParticle* particle, const std::string& invis_container) = 0;
        virtual StatusCode addToInvisible(xAOD::IParticle& particle, const std::string& invis_container) = 0;
        //  Same principle applied to the methods using the containers. Instead
        //  of retrieving the container every-time from the StoreGate per call,
        //  the container is retrieved once            //
        virtual StatusCode addToInvisible(const xAOD::IParticleContainer* container, const std::string& invis_container) = 0;
        virtual StatusCode addToInvisible(const xAOD::IParticleContainer& container, const std::string& invis_container) = 0;

        virtual xAOD::IParticleContainer* getInvisible(const std::string& invis_container = "") const = 0;

        virtual ~IMetSelector() {}
    };
}  // namespace XAMPP
#endif
