#include <XAMPPbase/MuonDecorations.h>

XAMPP::MuonDecorations::MuonDecorations() :
    ParticleDecorations(),
    isCosmicMuon("cosmic"),
    enterCosmicSelection(passBaseline.getDecorationString()),
    isBadMuon("bad"),
    enterBadMuonSelection(passBaseline.getDecorationString()),
    z0sinTheta("z0sinTheta"),
    d0sig("d0sig"),
    d0raw("d0raw"),
    truthParticleLink("truthParticleLink"),
    truthType("truthType"),
    truthOrigin("truthOrigin") {}

void XAMPP::MuonDecorations::populateDefaults(SG::AuxElement& ipart) {
    ParticleDecorations::populateDefaults(ipart);
    isCosmicMuon.set(ipart, false);
    isBadMuon.set(ipart, false);
    enterCosmicSelection.set(ipart, false);
    enterBadMuonSelection.set(ipart, false);
    z0sinTheta.set(ipart, 0);
    d0sig.set(ipart, 0);
    d0raw.set(ipart, 0);
    // do *not* overwrite potentially existing truth match info here!
}
