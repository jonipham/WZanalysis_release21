/// Here we define additional decorations which are
/// defined in case of jets.
/// These extend the set defined in the
/// ParticleDecorations class.

#ifndef XAMPP__JETDECORATIONS__H
#define XAMPP__JETDECORATIONS__H

#include <XAMPPbase/ParticleDecorations.h>
#include <xAODTruth/TruthParticleContainer.h>

namespace XAMPP {
    class JetDecorations : public ParticleDecorations {
    public:
        JetDecorations();
        virtual void populateDefaults(SG::AuxElement& ipart) override;
        // is this a bad jet?
        Decoration<char> isBadJet;
        // did this jet pass b-tagging?
        Decoration<char> isBJet;
        // does jet pass the JVT?
        Decoration<char> passJVT;
        // number of tracks in jet
        Decoration<int> nTracks;
        // jet algorithm?
        Decoration<int> jetAlgorithm;
        // MV2c10 b-tag weight
        Decoration<double> MV2c10;
        // DAOD-level decoration used for bad jet cleaning
        Decoration<char> DFCommonJets_jetClean_LooseBad;
        // the raw JVT (possibly recomputed) that we cut on
        Decoration<float> rawJVT;
    };
}  // namespace XAMPP
#endif  // XAMPP__JETDECORATIONS__H
