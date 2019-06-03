#include <XAMPPbase/JetDecorations.h>

XAMPP::JetDecorations::JetDecorations() :
    ParticleDecorations(),
    isBadJet("bad"),
    isBJet("bjet"),
    passJVT("passJvt"),
    nTracks("NTrks"),
    jetAlgorithm("JetAlgorithm"),
    MV2c10("MV2c10"),
    DFCommonJets_jetClean_LooseBad("DFCommonJets_jetClean_LooseBad"),
    rawJVT("Jvt") {}

void XAMPP::JetDecorations::populateDefaults(SG::AuxElement& ipart) {
    isBadJet.set(ipart, false);
    isBJet.set(ipart, false);
    nTracks.set(ipart, -1);
    jetAlgorithm.set(ipart, -1);
    MV2c10.set(ipart, -1e9);
    rawJVT.set(ipart, 0.);
    // do not touch flags from the xAOD
}
