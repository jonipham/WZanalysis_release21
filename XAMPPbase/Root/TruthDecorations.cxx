#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/TruthDecorations.h>

namespace XAMPP {
    TruthDecorations::TruthDecorations() :
        ParticleDecorations(),
        charge("charge"),
        pt_orig("pt_orig"),
        eta_orig("eta_orig"),
        phi_orig("phi_orig"),
        m_orig("m_orig"),
        pt_dressed("pt_dressed"),
        eta_dressed("eta_dressed"),
        phi_dressed("phi_dressed"),
        e_dressed("e_dressed"),
        pt_visible("pt_vis"),
        eta_visible("eta_vis"),
        phi_visible("phi_vis"),
        m_visible("m_vis"),
        loaded_p4("loaded_p4"),
        IsHadronicTau("IsHadronicTau"),
        truthType("truthType"),
        truthOrigin("truthOrigin"),
        classifierParticleType("classifierParticleType"),
        classifierParticleOrigin("classifierParticleOrigin"),
        originalTruthParticle("originalTruthParticle") {}

    void TruthDecorations::populateDefaults(SG::AuxElement& ipart) {
        ParticleDecorations::populateDefaults(ipart);
        loaded_p4.set(ipart, 0);
    }
}  // namespace XAMPP
