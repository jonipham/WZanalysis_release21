/// Here we define additional decorations which are
/// defined in case of truth particles.
/// These extend the set defined in the
/// ParticleDecorations class.

#ifndef XAMPPbase__TRUTHDECORATIONS__H
#define XAMPPbase__TRUTHDECORATIONS__H

#include <XAMPPbase/ParticleDecorations.h>

#include <xAODTruth/TruthParticle.h>
#include <xAODTruth/TruthParticleContainer.h>

namespace XAMPP {

    class TruthDecorations : public ParticleDecorations {
    public:
        TruthDecorations();
        virtual void populateDefaults(SG::AuxElement& ipart) override;

        // charge
        Decoration<float> charge;

        // original 4-momentum
        Decoration<float> pt_orig;
        Decoration<float> eta_orig;
        Decoration<float> phi_orig;
        Decoration<float> m_orig;

        // dressed 4-momentum
        Decoration<float> pt_dressed;
        Decoration<float> eta_dressed;
        Decoration<float> phi_dressed;
        Decoration<float> e_dressed;

        // visible 4-momentum for trut taus
        Decoration<double> pt_visible;
        Decoration<double> eta_visible;
        Decoration<double> phi_visible;
        Decoration<double> m_visible;

        // the loaded type of 4-mom
        // convention:
        // 0 = as in IParticle, nothing done yet
        // 1 = loaded the vanilla 4-mom (manually)
        // 2 = loaded the default dressed 4-mom
        // 3 = loaded the visible tau momentum
        // > 3 are free for analysis specific use
        enum class p4LoadStatus { NothingLoaded = 0, Vanilla, Dressed, VisibleTauMom };

        Decoration<char> loaded_p4;

        // is this a tau_had?
        Decoration<char> IsHadronicTau;

        // truth classifier things
        Decoration<int> truthType;
        Decoration<int> truthOrigin;
        // same as above but in different format (as found in DAOD)
        Decoration<unsigned int> classifierParticleType;
        Decoration<unsigned int> classifierParticleOrigin;
        Decoration<ElementLink<xAOD::TruthParticleContainer>> originalTruthParticle;
    };
}  // namespace XAMPP

#endif  // XAMPPbase__TRUTHDECORATIONS__H
