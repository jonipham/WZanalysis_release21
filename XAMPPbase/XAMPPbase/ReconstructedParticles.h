#ifndef XAMPPbase_ReconstructedParticles_H
#define XAMPPbase_ReconstructedParticles_H

#include <AsgTools/AsgTool.h>
#include <AsgTools/IAsgTool.h>

#include <XAMPPbase/ParticleSelector.h>
#include <xAODParticleEvent/Particle.h>
#include <xAODParticleEvent/ParticleAuxContainer.h>
#include <xAODParticleEvent/ParticleContainer.h>
namespace CP {
    class SystematicSet;
}
namespace XAMPP {
    class EventInfo;

    class IReconstructedParticles : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(IReconstructedParticles)
    public:
        virtual StatusCode PrepareContainer(const CP::SystematicSet* Set) = 0;

        virtual xAOD::Particle* CreateEmptyParticle() = 0;
        virtual xAOD::ParticleContainer* GetContainer() = 0;
        virtual void WellDefined(xAOD::IParticle* P, bool W = true) = 0;
        virtual StatusCode CreateSubContainer(const std::string& Name) = 0;
        virtual xAOD::ParticleContainer* GetSubContainer(const std::string& Name) = 0;
        virtual StatusCode LoadSubContainer(const std::string& Name) = 0;
        virtual void DetachSubContainer() = 0;
        virtual bool HasSubContainer(const std::string& name) const = 0;

        virtual ~IReconstructedParticles() {}
    };

    class ReconstructedParticles : public ParticleSelector, virtual public IReconstructedParticles {
    public:
        ReconstructedParticles(std::string myname);
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(ReconstructedParticles, XAMPP::IReconstructedParticles)
        //
        virtual ~ReconstructedParticles();
        virtual StatusCode initialize();
        virtual StatusCode PrepareContainer(const CP::SystematicSet* Set);

        virtual xAOD::Particle* CreateEmptyParticle();
        virtual xAOD::ParticleContainer* GetContainer();
        virtual void WellDefined(xAOD::IParticle* P, bool W = true);
        virtual StatusCode CreateSubContainer(const std::string& Name);
        virtual xAOD::ParticleContainer* GetSubContainer(const std::string& Name);
        virtual StatusCode LoadSubContainer(const std::string& Name);
        virtual void DetachSubContainer();
        virtual bool HasSubContainer(const std::string& name) const;

    protected:
        SelectionDecorator m_DecWell;
        SelectionAccessor m_AccWell;

        xAOD::ParticleContainer* m_Container;
        xAOD::ParticleContainer* m_AutoFillContainer;
        xAOD::ParticleContainer* m_ViewCont;
        xAOD::ParticleAuxContainer* m_AuxContainer;
    };
}  // namespace XAMPP
#endif
