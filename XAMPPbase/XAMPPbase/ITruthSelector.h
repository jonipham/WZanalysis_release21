#ifndef XAMPPbase_ITruthSelector_H
#define XAMPPbase_ITruthSelector_H

#include <AsgTools/IAsgTool.h>

#include <XAMPPbase/TruthDecorations.h>
#include <xAODJet/JetContainer.h>
#include <xAODTruth/TruthParticle.h>
#include <xAODTruth/TruthParticleContainer.h>

namespace CP {
    class SystematicSet;
}
namespace XAMPP {
    class ITruthSelector : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(ITruthSelector)

    public:
        virtual StatusCode LoadContainers() = 0;

        virtual StatusCode InitialFill(const CP::SystematicSet& systset) = 0;
        virtual StatusCode FillTruth(const CP::SystematicSet& systset) = 0;

        //            virtual bool isTrueTop(const xAOD::TruthParticle *
        //            particle) = 0; virtual bool isTrueW(const
        //            xAOD::TruthParticle * particle) = 0; virtual bool
        //            isTrueZ(const xAOD::TruthParticle * particle) = 0; virtual
        //            bool isTrueSUSY(const xAOD::TruthParticle * particle) = 0;
        virtual int classifyWDecays(const xAOD::TruthParticle* particle) = 0;

        virtual int GetInitialState() = 0;

        virtual xAOD::TruthParticleContainer* GetTruthPreElectrons() const = 0;
        virtual xAOD::TruthParticleContainer* GetTruthBaselineElectrons() const = 0;
        virtual xAOD::TruthParticleContainer* GetTruthSignalElectrons() const = 0;

        virtual xAOD::TruthParticleContainer* GetTruthPreMuons() const = 0;
        virtual xAOD::TruthParticleContainer* GetTruthBaselineMuons() const = 0;
        virtual xAOD::TruthParticleContainer* GetTruthSignalMuons() const = 0;

        virtual xAOD::TruthParticleContainer* GetTruthPrePhotons() const = 0;
        virtual xAOD::TruthParticleContainer* GetTruthBaselinePhotons() const = 0;
        virtual xAOD::TruthParticleContainer* GetTruthSignalPhotons() const = 0;

        virtual xAOD::TruthParticleContainer* GetTruthPreTaus() const = 0;
        virtual xAOD::TruthParticleContainer* GetTruthBaselineTaus() const = 0;
        virtual xAOD::TruthParticleContainer* GetTruthSignalTaus() const = 0;

        virtual xAOD::TruthParticleContainer* GetTruthNeutrinos() const = 0;
        virtual xAOD::TruthParticleContainer* GetTruthPrimaryParticles() const = 0;

        virtual xAOD::JetContainer* GetTruthPreJets() const = 0;
        virtual xAOD::JetContainer* GetTruthBaselineJets() const = 0;
        virtual xAOD::JetContainer* GetTruthSignalJets() const = 0;

        virtual xAOD::JetContainer* GetTruthBJets() const = 0;
        virtual xAOD::JetContainer* GetTruthLightJets() const = 0;
        virtual xAOD::JetContainer* GetTruthFatJets() const = 0;
        virtual xAOD::JetContainer* GetTruthCustomJets(const std::string& kind) const = 0;

        virtual xAOD::TruthParticleContainer* GetTruthParticles() const = 0;
        virtual const xAOD::TruthParticleContainer* GetTruthInContainer() const = 0;

        virtual std::shared_ptr<TruthDecorations> GetTruthDecorations() const = 0;

        virtual StatusCode ReclusterTruthJets(const xAOD::IParticleContainer* inputJets, float Rcone, float minPtKt4 = -1,
                                              std::string PreFix = "", float minPtRecl = -1, float rclus = 0., float ptfrac = -1) = 0;

        virtual ~ITruthSelector() {}
    };
}  // namespace XAMPP
#endif
