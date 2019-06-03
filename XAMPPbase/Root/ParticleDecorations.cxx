#include <XAMPPbase/ParticleDecorations.h>

XAMPP::ParticleDecorations::ParticleDecorations() :
    passPreselection("baseline"),
    passBaseline("passOR"),
    passSignal("signal"),
    passIsolation("isol"),
    enterOverlapRemoval("selected") {}

void XAMPP::ParticleDecorations::populateDefaults(SG::AuxElement& ipart) {
    passPreselection.set(ipart, false);
    passBaseline.set(ipart, false);
    passSignal.set(ipart, false);
    passIsolation.set(ipart, false);
    enterOverlapRemoval.set(ipart, false);
}
