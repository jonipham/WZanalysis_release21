#ifndef XAMPPbase_Defs_H
#define XAMPPbase_Defs_H

#include <xAODMissingET/MissingET.h>
namespace XAMPP {
    typedef xAOD::MissingET* XAMPPmet;
    typedef double (xAOD::IParticle::*Momentum)() const;

    typedef SG::AuxElement::Accessor<char> CharAccessor;
    typedef SG::AuxElement::Decorator<char> CharDecorator;

    typedef std::unique_ptr<CharAccessor> SelectionAccessor;
    typedef std::unique_ptr<CharDecorator> SelectionDecorator;

    typedef SG::AuxElement::Accessor<float> FloatAccessor;
    typedef SG::AuxElement::Decorator<float> FloatDecorator;

    typedef SG::AuxElement::Accessor<double> DoubleAccessor;
    typedef SG::AuxElement::Decorator<double> DoubleDecorator;

    typedef SG::AuxElement::Accessor<int> IntAccessor;
    typedef SG::AuxElement::Decorator<int> IntDecorator;

    typedef SG::AuxElement::Accessor<unsigned int> UIntAccessor;
    typedef SG::AuxElement::Decorator<unsigned int> UIntDecorator;

    typedef SG::AuxElement::Accessor<bool> BoolAccessor;
    typedef SG::AuxElement::Decorator<bool> BoolDecorator;

    typedef std::vector<std::string> StringVector;
    typedef std::vector<float> FloatVector;
    typedef std::vector<int> IntVector;
    typedef std::vector<double> DoubleVector;

    enum SelectionObject {
        Other = 0,
        Jet = 2,
        TrackParticle = 4,
        Electron = 6,
        Photon = 7,
        Muon = 8,
        Tau = 9,
        BTag = 102,
        TruthParticle = 201,
        MissingET = 301,
        EventWeight = 302,
        RecoParticle = 960,
        DiTau = 980
    };
    enum ZVeto { Pass = 1, Fail2Lep = 1 << 1, Fail3Lep = 1 << 2, Fail4Lep = 1 << 3, Fail2LepPhot = 1 << 4 };
    enum Stop_RecoTypes {
        None = 0,
        TopWCandidate1,         // distinguish top and W by PdgId (6 vs. 24)
        TopWCandidate2,         // distinguish top and W by PdgId (6 vs. 24)
        DRB4TopWCandidate1,     // distinguish top and W by PdgId (6 vs. 24)
        DRB4TopWCandidate2,     // distinguish top and W by PdgId (6 vs. 24)
        MinMassTopWCandidate1,  // distinguish top and W by PdgId (6 vs. 24)
        MinMassTopWCandidate2,  // distinguish top and W by PdgId (6 vs. 24)
        Chi2TopWCandidate1,     // distinguish top and W by PdgId (6 vs. 24)
        Chi2TopWCandidate2      // distinguish top and W by PdgId (6 vs. 24)
    };
    constexpr float Z_MASS = 91.1876 * 1e3;
    constexpr float W_MASS = 80387.;
    constexpr float W_WIDTH = 2085;
    constexpr float Top_MASS = 173210.;
    constexpr float MeVToGeV = 1. / 1000.;

    enum WDecayModes {
        Unspecified = 0,
        Hadronic = 1,
        ElecNeut = 2,
        MuonNeut = 3,
        HadTauNeut = 4,
        LepTauNeut = 5,
    };
    enum JetAlgorithm { AntiKt2 = 0, AntiKt4, AntiKt10, RScan };

}  // namespace XAMPP
#endif
