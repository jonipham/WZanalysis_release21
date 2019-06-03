/// Here we define additional decorations which are
/// defined in case of muons.
/// These extend the set defined in the
/// ParticleDecorations class.

#ifndef XAMPPbase__MUONDECORATIONS__H
#define XAMPPbase__MUONDECORATIONS__H

#include <XAMPPbase/ParticleDecorations.h>

#include <xAODTruth/TruthParticleContainer.h>

namespace XAMPP {

    class MuonDecorations : public ParticleDecorations {
    public:
        MuonDecorations();
        virtual void populateDefaults(SG::AuxElement& ipart) override;
        // check if a muon is classified as a cosmic candidate
        // (typically done by SUSYTools based on d0/z0)
        Decoration<char> isCosmicMuon;
        // check if a muon passes the criteria used when looking
        // for cosmic muon candidates.
        // defaults to the baseline lepton flag.
        Decoration<char> enterCosmicSelection;
        // check if a muon is a "bad MET muon" likely to have
        // a poorly reconstructed momentum.
        // Used for event veto, typically set by SUSYTools
        Decoration<char> isBadMuon;
        // check if a muon passes the criteria used when looking
        // for bad muon candidates.
        // defaults to the baseline lepton flag.
        Decoration<char> enterBadMuonSelection;

        // impact parameters
        // z0 sin theta w.r.t PV
        Decoration<float> z0sinTheta;
        // significance of transverse impact parameter w.r.t PV
        Decoration<float> d0sig;
        // raw value of transverse impact parameter w.r.t PV
        Decoration<float> d0raw;

        // truth matching
        // truth matching based on the track particle.
        Decoration<ElementLink<xAOD::TruthParticleContainer>> truthParticleLink;
        // truth type of the muon track particle
        Decoration<int> truthType;
        // truth origin of the muon track particle
        Decoration<int> truthOrigin;
    };
}  // namespace XAMPP

#endif
