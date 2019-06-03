/// Here we define additional decorations which are
/// defined in case of electrons.
/// These extend the set defined in the
/// ParticleDecorations class.

#ifndef XAMPPbase__ELECTRONDECORATIONS__H
#define XAMPPbase__ELECTRONDECORATIONS__H

#include <XAMPPbase/ParticleDecorations.h>

#include <xAODTruth/TruthParticleContainer.h>

namespace XAMPP {

    class ElectronDecorations : public ParticleDecorations {
    public:
        ElectronDecorations();
        virtual void populateDefaults(SG::AuxElement& ipart) override;

        // impact parameters
        // z0 sin theta w.r.t PV
        Decoration<float> z0sinTheta;
        // significance of transverse impact parameter w.r.t PV
        Decoration<float> d0sig;
        // raw value of transverse impact parameter w.r.t PV
        Decoration<float> d0raw;
        // pass signal WP (if different from baseline)
        Decoration<char> passSignalWorkingPoint;

        // truth matching
        // truth matching from the xAOD.
        Decoration<ElementLink<xAOD::TruthParticleContainer>> truthParticleLink;
        // truth type of the electron track
        Decoration<int> truthType;
        // truth origin of the electron track
        Decoration<int> truthOrigin;
    };
}  // namespace XAMPP

#endif  // XAMPPbase__ELECTRONDECORATIONS__H
