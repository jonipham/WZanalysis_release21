#include <XAMPPbase/ElectronDecorations.h>

XAMPP::ElectronDecorations::ElectronDecorations() :
    ParticleDecorations(),
    z0sinTheta("z0sinTheta"),
    d0sig("d0sig"),
    d0raw("d0raw"),
    passSignalWorkingPoint("passSignalWorkingPoint"),
    truthParticleLink("truthParticleLink"),
    truthType("truthType"),
    truthOrigin("truthOrigin") {}

void XAMPP::ElectronDecorations::populateDefaults(SG::AuxElement& ipart) {
    ParticleDecorations::populateDefaults(ipart);
    z0sinTheta.set(ipart, 0);
    d0sig.set(ipart, 0);
    d0raw.set(ipart, 0);
    passSignalWorkingPoint.set(ipart, false);
    // do *not* overwrite potentially existing truth match info here!
}
