#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/PhotonDecorations.h>

namespace XAMPP {
    PhotonDecorations::PhotonDecorations() : ParticleDecorations() {}
    void PhotonDecorations::populateDefaults(SG::AuxElement& ipart) { ParticleDecorations::populateDefaults(ipart); }
}  // namespace XAMPP
