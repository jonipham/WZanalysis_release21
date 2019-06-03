#include <XAMPPbase/TauDecorations.h>

XAMPP::TauDecorations::TauDecorations() :
    ParticleDecorations(),
    truthType("truthType"),
    truthOrigin("truthOrigin"),
    numTracks("NTrks"),
    passBaselineID("baselineID"),
    passSignalID("signalID"),
    partonTruthLabelID("PartonTruthLabelID"),
    coneTruthLabelID("ConeTruthLabelID"),
    tauTruthType("truthTauType") {}

void XAMPP::TauDecorations::populateDefaults(SG::AuxElement& ipart) {
    ParticleDecorations::populateDefaults(ipart);

    // do *not* overwrite potentially existing truth match info here!
}
