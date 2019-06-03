/// This defines the decorations we can expect
/// to find in every particle within the
/// XAMPP framework.
/// They mainly represent abstract stages
/// in the object selection.
/// This is then extended in the specific
/// classes for each particle type,
/// and optionally within the analysis specific code.
///
/// The hierarchy assumed here is:
/// Preselection (WP, kinematics,...)
///     |   // via overlap removal
///     V
///  Baseline
///     |   // via more signal cuts
///     V
///   Signal
///     |  // via isolation cuts
///     V
///  Isolated

#ifndef XAMPPBASE_PARTICLEDECORATIONS__H
#define XAMPPBASE_PARTICLEDECORATIONS__H

#include <XAMPPbase/DecorationInterface.h>

namespace XAMPP {
    class ParticleDecorations {
    public:
        // set decorations to initial names compatible with SUSYTools.
        // we can later change this via the setDecorationString
        // methods of our members below.
        ParticleDecorations();
        // this will set *all* the provided decorations to false,
        // in order to ensure a starting set of information.
        virtual void populateDefaults(SG::AuxElement& ipart);

        // did we pass the baseline lepton cuts *except* for overlap removal?
        Decoration<char> passPreselection;

        // did we pass the baseline object selection *including* overlap removal?
        Decoration<char> passBaseline;

        // did we pass additional signal object cuts?
        Decoration<char> passSignal;

        // did we pass isolation requirements?
        Decoration<char> passIsolation;

        // should this particle be considered in the overlap removal tool?
        Decoration<char> enterOverlapRemoval;
    };
}  // namespace XAMPP

#endif  // XAMPPBASE_PARTICLEDECORATIONS__H
