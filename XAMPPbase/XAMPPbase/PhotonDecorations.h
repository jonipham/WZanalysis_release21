/// Here we define additional decorations which are
/// defined in case of photons.
/// These extend the set defined in the
/// ParticleDecorations class.

#ifndef XAMPPbase__PHOTONDECORATIONS__H
#define XAMPPbase__PHOTONDECORATIONS__H

#include <XAMPPbase/ParticleDecorations.h>

#include <xAODTruth/TruthParticleContainer.h>

namespace XAMPP {
    class PhotonDecorations : public ParticleDecorations {
    public:
        PhotonDecorations();
        virtual void populateDefaults(SG::AuxElement& ipart) override;
        // At the moment no additonal photon decorations are made inside the frameork
    };
}  // namespace XAMPP

#endif
