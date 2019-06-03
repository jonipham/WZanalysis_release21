/// Here we define additional decorations which are
/// defined in case of muons.
/// These extend the set defined in the
/// ParticleDecorations class.

#ifndef XAMPPbase__TAUDECORATIONS__H
#define XAMPPbase__TAUDECORATIONS__H

#include <XAMPPbase/ParticleDecorations.h>

#include <xAODTruth/TruthParticleContainer.h>

namespace XAMPP {

    class TauDecorations : public ParticleDecorations {
    public:
        TauDecorations();
        virtual void populateDefaults(SG::AuxElement& ipart) override;

        // truth type of the tau candidate
        Decoration<int> truthType;
        // truth origin of the tau candidate
        Decoration<int> truthOrigin;
        // How many tracks are associated with the tau candidate
        Decoration<int> numTracks;
        // Check the baseline and signalID seperateley
        Decoration<char> passBaselineID;
        Decoration<char> passSignalID;

        // Truth label ID of the partons inside associated jet
        Decoration<int> partonTruthLabelID;
        // Truth label of the entire cone of the associated jet
        Decoration<int> coneTruthLabelID;
        // Tau truth type. Apparently the tau WG has their own truth type
        Decoration<int> tauTruthType;
    };
}  // namespace XAMPP

#endif
