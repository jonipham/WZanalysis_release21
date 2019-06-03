#include <PathResolver/PathResolver.h>
#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/ReconstructedParticles.h>
#include <xAODBase/IParticleHelpers.h>
#include <xAODEgamma/EgammaTruthxAODHelpers.h>
#include <xAODEgamma/ElectronxAODHelpers.h>

#include <MCTruthClassifier/MCTruthClassifierDefs.h>
#include <TruthUtils/PIDHelpers.h>
#include <boost/filesystem.hpp>  // loop over folder and its containing elements
#include <boost/regex.hpp>       // regular expression matching
#include <iostream>              // std::cout
#include <locale>                // std::locale, std::tolower
#include <string>                // std::string

#include <CalcGenericMT2/MT2_ROOT.h>

namespace XAMPP {
    double Sign(const double& N) { return N != 0 ? std::fabs(N) / N : 0; }
    std::string RemoveAllExpInStr(std::string Str, std::string Exp) { return ReplaceExpInString(Str, Exp, ""); }
    std::string ReplaceExpInString(std::string str, const std::string& exp, const std::string& rep) {
        size_t ExpPos = str.find(exp);
        if (ExpPos == std::string::npos) return str;
        size_t ExpLen = exp.size();
        str = str.substr(0, ExpPos) + rep + str.substr(ExpPos + ExpLen, std::string::npos);
        if (str.find(exp) != std::string::npos) return ReplaceExpInString(str, exp, rep);
        return str;
    }
    bool ptsorter(const xAOD::IParticle* j1, const xAOD::IParticle* j2) { return (j1->pt() > j2->pt()); }
    Momentum FourMomentum(const std::string& str) {
        if (ToLower(str) == "rapidity") return &xAOD::IParticle::rapidity;
        if (ToLower(str) == "pt") return &xAOD::IParticle::pt;
        if (ToLower(str) == "eta") return &xAOD::IParticle::eta;
        if (ToLower(str) == "phi") return &xAOD::IParticle::phi;
        if (ToLower(str) == "e") return &xAOD::IParticle::e;
        if (ToLower(str) == "m") return &xAOD::IParticle::m;
        return nullptr;
    }
    std::string FourMomentum(Momentum Mom) {
        if (Mom == &xAOD::IParticle::pt) return "pt";
        if (Mom == &xAOD::IParticle::eta) return "eta";
        if (Mom == &xAOD::IParticle::phi) return "phi";
        if (Mom == &xAOD::IParticle::m) return "m";
        if (Mom == &xAOD::IParticle::e) return "e";
        if (Mom == &xAOD::IParticle::rapidity) return "rapidity";
        return "Unknown";
    }
    std::string ToLower(const std::string& str) {
        std::string low = str;
        std::locale loc;

        for (int i = str.length() - 1; i >= 0; --i) { low[i] = std::tolower(str[i], loc); }
        return low;
    }
    bool btagweightsorter(const xAOD::Jet* j1, const xAOD::Jet* j2) {
        static DoubleAccessor dec_MV2c10("MV2c10");
        return (dec_MV2c10(*j1) > dec_MV2c10(*j2));
    }

    std::vector<std::string> GetPathResolvedFileList(const std::vector<std::string>& FileList) {
        std::vector<std::string> ResolvedList;
        for (auto& File : FileList) {
            std::string F = PathResolverFindCalibFile(File);
            if (!F.empty()) ResolvedList.push_back(F);
        }
        return ResolvedList;
    }

    std::vector<std::string> ListDirectory(std::string Path, std::string ElementToContain, std::string RegExpToMatch) {
        boost::filesystem::path p(Path);
        if (!boost::filesystem::exists(p)) throw std::runtime_error("Could not list files in directory " + Path);

        std::vector<std::string> List;
        for (auto&& x : boost::filesystem::directory_iterator(p)) {
            // check if token called ElementToContain is present in the file name
            std::string filename = x.path().filename().string();
            if (!(ElementToContain.size() == 0 || filename.find(ElementToContain) < filename.size())) continue;
            // check if file name matches regular expression
            if (RegExpToMatch.size() > 0) {
                boost::regex expr{RegExpToMatch};
                if (!boost::regex_match(filename, expr)) continue;
            }
            List.push_back(x.path().string());
        }
        return List;
    }

    float Charge(const xAOD::IParticle* p) {
        static FloatAccessor acc_charge("charge");
        if (acc_charge.isAvailable(*p)) return acc_charge(*p);
        return 0.f;
    }
    int TypeToPdgId(const xAOD::IParticle& P) { return TypeToPdgId(&P); }
    int TypeToPdgId(const xAOD::IParticle* P) {
        static IntAccessor acc_PdgId("pdgId");
        if (acc_PdgId.isAvailable(*P)) {
            return acc_PdgId(*P);
        } else if (P->type() == xAOD::Type::ObjectType::Jet) {
            return 2;
        } else if (P->type() == xAOD::Type::ObjectType::Electron) {
            return -11 * Charge(P);
        } else if (P->type() == xAOD::Type::ObjectType::Muon) {
            return -13 * Charge(P);
        } else if (P->type() == xAOD::Type::ObjectType::Tau) {
            return -15 * Charge(P);
        } else if (P->type() == xAOD::Type::ObjectType::Photon) {
            return 22;
        }
        return P->type();
    }
    void FillVectorFromString(std::vector<std::string>& str_vector, std::string& str) {
        if (str.find(",") != std::string::npos && str.size() > 1) {
            str = str + ",";  // adding comma in the end to make the 'while'
                              // working for the last element
            while (str.find(",") < str.size()) {
                std::size_t Pos = str.find(",");
                std::string Data = str.substr(0, Pos);
                if (!Data.empty() && !IsInVector(Data, str_vector)) str_vector.push_back(Data);
                str = str.substr(Pos + 1, str.size());
            }
        } else if (!str.empty() && !IsInVector(str, str_vector))
            str_vector.push_back(str);
        str.clear();
    }
    bool GetLine(std::ifstream& inf, std::string& line) {
        if (!std::getline(inf, line)) return false;
        line = EraseWhiteSpaces(line);
        if (line.find("#") == 0 || line.size() < 1) return GetLine(inf, line);
        return true;
    }
    std::string EraseWhiteSpaces(std::string str) {
        str.erase(std::remove(str.begin(), str.end(), '\t'), str.end());
        if (str.find(" ") == 0) return EraseWhiteSpaces(str.substr(1, str.size()));
        if (str.size() > 0 && str.find(" ") == str.size() - 1) return EraseWhiteSpaces(str.substr(0, str.size() - 1));
        return str;
    }
    MSG::Level setOutputLevel(int m_output_level_int) {
        if (m_output_level_int == 1) return MSG::VERBOSE;
        if (m_output_level_int == 2) return MSG::DEBUG;
        if (m_output_level_int == 3) return MSG::INFO;
        if (m_output_level_int == 4) return MSG::WARNING;
        if (m_output_level_int == 5) return MSG::ERROR;
        if (m_output_level_int == 6) return MSG::FATAL;
        return MSG::INFO;
    }
    xAOD::IParticle* FindLeadingParticle(xAOD::IParticleContainer* Particles) {
        xAOD::IParticle* P = nullptr;
        float leadingPt = -1.;
        if (Particles) {
            for (auto C : *Particles) {
                if (C->pt() > leadingPt) {
                    leadingPt = C->pt();
                    P = C;
                }
            }
        }
        return P;
    }
    float GetLeadingPt(xAOD::IParticleContainer* Particles) {
        float leadingPt = -1.;
        if (Particles) {
            for (auto P : *Particles)
                if (P->pt() > leadingPt) leadingPt = P->pt();
        }
        return leadingPt;
    }
    StatusCode ResetOverlapDecorations(const xAOD::IParticleContainer* Container) {
        static CharDecorator dec_passOR("passOR");
        if (!Container) {
            Error("ResetOverlapDecorations", "No container given");
            return StatusCode::FAILURE;
        }
        for (const auto& Particle : *Container) dec_passOR(*Particle) = true;
        return StatusCode::SUCCESS;
    }
    StatusCode RemoveOverlap(xAOD::IParticleContainer* RemFrom, xAOD::IParticleContainer* RemWith, float dR, bool UseRapidity) {
        return RemoveOverlap(RemFrom, RemWith, [dR](const xAOD::IParticle*, const xAOD::IParticle*) { return dR; }, UseRapidity);
    }
    StatusCode RemoveOverlap(xAOD::IParticleContainer* RemFrom, xAOD::IParticleContainer* RemWith,
                             std::function<float(const xAOD::IParticle*, const xAOD::IParticle*)> radiusFunc, bool UseRapidity) {
        static CharDecorator dec_passOR("passOR");
        static CharAccessor acc_passOR("passOR");

        if (!RemFrom || !RemWith) {
            Error("RemoveOverlap", "One of the particle containers is not given");
            return StatusCode::FAILURE;
        }
        if (RemFrom->size() == 0 || RemWith->size() == 0) return StatusCode::SUCCESS;
        for (const auto& From : *RemFrom) {
            if (!acc_passOR(*From)) continue;
            for (const auto& With : *RemWith) {
                if (acc_passOR(*With) && Overlaps(*From, *With, radiusFunc(From, With), UseRapidity)) {
                    dec_passOR(*From) = false;
                    break;
                }
            }
        }
        return StatusCode::SUCCESS;
    }
    float CalculateMT2(const xAOD::IParticle* P1, const xAOD::IParticle* P2, XAMPP::Storage<XAMPPmet>* met, float InvMass,
                       float ParticleMass) {
        return CalculateMT2(*P1, *P2, met, InvMass, ParticleMass);
    }
    float CalculateMT2(const xAOD::IParticle* P1, const xAOD::IParticle* P2, const xAOD::MissingET* met, float InvMass,
                       float ParticleMass) {
        return CalculateMT2(*P1, *P2, met, InvMass, ParticleMass);
    }
    float CalculateMT2(const xAOD::IParticle& P1, const xAOD::IParticle& P2, XAMPP::Storage<XAMPPmet>* met, float InvMass,
                       float ParticleMass) {
        return CalculateMT2(P1, P2, met->GetValue(), InvMass, ParticleMass);
    }
    float CalculateMT2(const xAOD::IParticle& P1, const xAOD::IParticle& P2, const xAOD::MissingET* met, float InvMass,
                       float ParticleMass) {
        return CalculateAMT2(P1, P2, met, InvMass, InvMass, ParticleMass);
    }

    float CalculateAMT2(const xAOD::IParticle* P1, const xAOD::IParticle* P2, XAMPP::Storage<XAMPPmet>* met, float InvMass1, float InvMass2,
                        float ParticleMass) {
        return CalculateAMT2(*P1, *P2, met, InvMass1, InvMass2, ParticleMass);
    }
    float CalculateAMT2(const xAOD::IParticle* P1, const xAOD::IParticle* P2, const xAOD::MissingET* met, float InvMass1, float InvMass2,
                        float ParticleMass) {
        return CalculateAMT2(*P1, *P2, met, InvMass1, InvMass2, ParticleMass);
    }
    float CalculateAMT2(const xAOD::IParticle& P1, const xAOD::IParticle& P2, XAMPP::Storage<XAMPPmet>* met, float InvMass1, float InvMass2,
                        float ParticleMass) {
        return CalculateAMT2(P1, P2, met->GetValue(), InvMass1, InvMass2, ParticleMass);
    }
    float CalculateAMT2(const xAOD::IParticle& P1, const xAOD::IParticle& P2, const xAOD::MissingET* met, float InvMass1, float InvMass2,
                        float ParticleMass) {
        if (!met) return -1;
        TLorentzVector visa, visb, met_tlv;
        visa.SetPtEtaPhiM(P1.pt(), P1.eta(), P1.phi(), (ParticleMass != -1) ? ParticleMass : P1.m());
        visb.SetPtEtaPhiM(P2.pt(), P2.eta(), P2.phi(), (ParticleMass != -1) ? ParticleMass : P2.m());
        met_tlv.SetPxPyPzE(met->mpx(), met->mpy(), 0, met->met());
        return ComputeMT2(visa, visb, met_tlv, InvMass1, InvMass2).Compute();
    }

    bool Overlaps(const xAOD::IParticle& P1, const xAOD::IParticle& P2, float dR, bool UseRapidity) {
        return Overlaps(&P1, &P2, dR, UseRapidity);
    }
    bool Overlaps(const xAOD::IParticle* P1, const xAOD::IParticle* P2, float dR, bool UseRapidity) {
        return !IsSame(P1, P2) && xAOD::P4Helpers::deltaR2(P1, P2, UseRapidity) < dR * dR;
    }
    bool IsRecoLepton(const xAOD::IParticle& L) { return IsRecoLepton(&L); }
    bool IsRecoLepton(const xAOD::IParticle* L) {
        return (L->type() == xAOD::Type::ObjectType::Electron || L->type() == xAOD::Type::ObjectType::Muon ||
                L->type() == xAOD::Type::ObjectType::Tau);
    }
    bool SameFlavour(const xAOD::IParticle& L, const xAOD::IParticle& L1) { return SameFlavour(&L1, &L); }
    bool OppositeFlavour(const xAOD::IParticle& L, const xAOD::IParticle& L1) { return OppositeFlavour(&L, &L1); }
    bool OppositeFlavour(const xAOD::IParticle* L, const xAOD::IParticle* L1) { return IsNotSame(L, L1) && !SameFlavour(L, L1); }
    bool SameFlavour(const xAOD::IParticle* L, const xAOD::IParticle* L1) {
        if (IsSame(L, L1) || L->type() != L1->type()) return false;
        if (IsRecoLepton(L)) return true;
        if (L->type() == xAOD::Type::ObjectType::TruthParticle)
            return std::fabs(TypeToPdgId(L)) == std::fabs(TypeToPdgId(L1)) && MC::PID::isChLepton(TypeToPdgId(L));

        return false;
    }
    bool OppositeSignLead2(const xAOD::IParticleContainer* pc) {
        if (pc->size() < 2) {
            Warning("OppositeSign()", "Particle containers has too few elements");
            return false;
        }
        return OppositeSign(pc->at(0), pc->at(1));
    }
    bool OppositeSign(const xAOD::IParticle& L, const xAOD::IParticle& L1) { return OppositeSign(&L, &L1); }
    bool OppositeSign(const xAOD::IParticle* L, const xAOD::IParticle* L1) {
        if (IsSame(L, L1)) return false;
        if (Charge(L) * Charge(L1) < 0.) return true;
        return false;
    }
    bool IsSame(const xAOD::IParticle& P, const xAOD::IParticle& P1, bool DeRefShallowCopy) { return IsSame(&P, &P1, DeRefShallowCopy); }
    bool IsSame(const xAOD::IParticle* P, const xAOD::IParticle* P1, bool DeRefShallowCopy) {
        if (P == P1) { return true; }
        if (DeRefShallowCopy) {
            const xAOD::IParticle* OrigP1 = xAOD::getOriginalObject(*P1);
            const xAOD::IParticle* OrigP = xAOD::getOriginalObject(*P);
            if (OrigP == OrigP1) { return OrigP != nullptr; }
            if (OrigP == P1 || OrigP1 == P) { return true; }
        }
        return false;
    }
    bool IsSFOS(const xAOD::IParticle& L, const xAOD::IParticle& L1) { return IsSFOS(&L1, &L); }
    bool IsSFOS(const xAOD::IParticle* L, const xAOD::IParticle* L1) { return !IsSame(L, L1) && OppositeSign(L, L1) && SameFlavour(L, L1); }
    bool IsDFOS(const xAOD::IParticle& L, const xAOD::IParticle& L1) { return IsDFOS(&L, &L1); }
    bool IsDFOS(const xAOD::IParticle* L, const xAOD::IParticle* L1) {
        return IsNotSame(L, L1) && OppositeSign(L, L1) && OppositeFlavour(L, L1);
    }
    bool IsNotSame(const xAOD::IParticle& P, const xAOD::IParticle& P1) { return IsNotSame(&P, &P1); }
    bool IsNotSame(const xAOD::IParticle* P, const xAOD::IParticle* P1) { return !IsSame(P, P1); }
    StatusCode RemoveLowMassLeptons(xAOD::IParticleContainer* Leptons, bool (*pair)(const xAOD::IParticle*, const xAOD::IParticle*),
                                    float Upper_Mll, float Lower_Mll) {
        static CharDecorator dec_passLMR("passLMR");
        static CharAccessor acc_passOR("passOR");
        if (!Leptons) {
            Error("RemoveLowMassLeptons", "No lepton container given");
            return StatusCode::FAILURE;
        }
        for (xAOD::IParticleContainer::const_iterator L = Leptons->begin(); L != Leptons->end(); ++L) {
            if (!acc_passOR(**L)) continue;
            for (xAOD::IParticleContainer::const_iterator L1 = Leptons->begin(); L1 != L; ++L1) {
                if (!acc_passOR(**L1) || IsSame(*L, *L1) || !pair(*L, *L1)) continue;
                float M = InvariantMass(*L, *L1);
                if (M > Lower_Mll && M < Upper_Mll) {
                    dec_passLMR(**L1) = false;
                    dec_passLMR(**L) = false;
                }
            }
        }
        return StatusCode::SUCCESS;
    }
    StatusCode RemoveLowMassLeptons(xAOD::IParticleContainer* Leptons, float Upper_Mll, float Lower_Mll) {
        return RemoveLowMassLeptons(Leptons, IsNotSame, Upper_Mll, Lower_Mll);
    }
    float InvariantMass(const xAOD::IParticle* P1, const xAOD::IParticle* P2) {
        TLorentzVector DiPart = P1->p4() + P2->p4();
        return DiPart.M();
    }
    float InvariantMass(const xAOD::IParticle& P1, const xAOD::IParticle& P2) { return InvariantMass(&P1, &P2); }
    float InvariantMass(const xAOD::IParticle* P1, const xAOD::IParticle* P2, const xAOD::IParticle* P3) {
        TLorentzVector TriPart = P1->p4() + P2->p4() + P3->p4();
        return TriPart.M();
    }
    float InvariantMass(const xAOD::IParticle& P1, const xAOD::IParticle& P2, const xAOD::IParticle& P3) {
        return InvariantMass(&P1, &P2, &P3);
    }
    float InvariantMass(const xAOD::IParticle* P1, const xAOD::IParticle* P2, const xAOD::IParticle* P3, const xAOD::IParticle* P4) {
        TLorentzVector FourPart = P1->p4() + P2->p4() + P3->p4() + P4->p4();
        return FourPart.M();
    }
    float InvariantMass(const xAOD::IParticle& P1, const xAOD::IParticle& P2, const xAOD::IParticle& P3, const xAOD::IParticle& P4) {
        return InvariantMass(&P1, &P2, &P3, &P4);
    }

    int GetZVeto(xAOD::IParticleContainer& Particles, float Z_Window) {
        xAOD::IParticleContainer::const_iterator begin_itr = Particles.begin();
        xAOD::IParticleContainer::const_iterator end_itr = Particles.end();
        unsigned int Veto = ZVeto::Pass;
        for (xAOD::IParticleContainer::const_iterator L = begin_itr; L != end_itr; ++L) {
            for (xAOD::IParticleContainer::const_iterator L1 = L + 1; L1 != end_itr; ++L1) {
                bool TwoLep = IsSFOS(*L, *L1);
                if (TwoLep && fabs(InvariantMass(*L, *L1) - Z_MASS) < Z_Window) { Veto = Veto | ZVeto::Fail2Lep; }
                // Three Lepton invariant masses
                for (xAOD::IParticleContainer::const_iterator L2 = L1 + 1; L2 != end_itr; ++L2) {
                    // Now possible is e+e- mu+  // mu+ e+ e- // e+ mu+ e-
                    bool ThreeLep = (TwoLep || IsSFOS(*L1, *L2) || IsSFOS(*L, *L2));
                    if (ThreeLep && fabs(InvariantMass(*L, *L1, *L2) - Z_MASS) < Z_Window) {
                        // Test if one of the particles is a photon
                        if (TypeToPdgId(*L) == 22 || TypeToPdgId(*L1) == 22 || TypeToPdgId(*L2) == 22) {
                            Veto = Veto | ZVeto::Fail2LepPhot;
                        } else {
                            Veto = Veto | ZVeto::Fail3Lep;
                        }
                    }
                    for (xAOD::IParticleContainer::const_iterator L3 = L2 + 1; L3 != end_itr; ++L3) {
                        // Four Lepton invariant masses
                        // mu mu e e ||
                        bool SFOSSFOS = (IsSFOS(*L, *L1) && IsSFOS(*L2, *L3)) || (IsSFOS(*L, *L2) && IsSFOS(*L1, *L3)) ||
                                        (IsSFOS(*L, *L3) && IsSFOS(*L2, *L1));
                        if (SFOSSFOS && fabs(InvariantMass(*L, *L1, *L2, *L3) - Z_MASS) < Z_Window) { Veto = Veto | ZVeto::Fail4Lep; }
                    }
                }
            }
        }
        return Veto;
    }
    int GetZVeto(xAOD::IParticleContainer* Particles, float Z_Window) { return GetZVeto(*Particles, Z_Window); }
    float RelPt(const xAOD::IParticle& v1, const xAOD::IParticle& v2) { return RelPt(&v1, &v2); }
    float RelPt(const xAOD::IParticle* v1, const xAOD::IParticle* v2) {
        if (IsSame(v1, v2) || v1 == nullptr || v2 == nullptr) return 0;
        return (v1->p4() - v2->p4()).Pt();
    }
    int GetZVeto(xAOD::IParticleContainer* Electrons, xAOD::IParticleContainer* Muons, float Z_Window) {
        xAOD::IParticleContainer Lep(SG::VIEW_ELEMENTS);
        for (const auto& E : *Electrons) Lep.push_back(E);
        for (const auto& M : *Muons) Lep.push_back(M);
        return GetZVeto(Lep, Z_Window);
    }
    float ComputeMt(const xAOD::IParticle& v1, XAMPP::Storage<XAMPPmet>* met) { return ComputeMt(&v1, met); }
    float ComputeMt(const xAOD::IParticle& v1, const xAOD::IParticle& v2) { return ComputeMt(&v1, &v2); }
    float ComputeMt(const xAOD::IParticle& v1, const xAOD::MissingET& met) { return ComputeMt(&v1, &met); }
    float ComputeMt(const xAOD::IParticle* v1, const xAOD::IParticle* v2) {
        if (IsSame(v1, v2)) return 0.;
        return TMath::Sqrt(2 * (v1->pt() * v2->pt() - v1->p4().Px() * v2->p4().Px() - v1->p4().Py() * v2->p4().Py()));
    }
    float ComputeMt(const xAOD::IParticle* v1, XAMPP::Storage<XAMPPmet>* met) {
        if (met->GetValue()) return ComputeMt(v1, met->GetValue());
        return -1.;
    }
    float ComputeMt(const xAOD::IParticle* v1, const xAOD::MissingET* met) {
        return TMath::Sqrt(2 * (v1->pt() * met->met() - v1->p4().Px() * met->mpx() - v1->p4().Py() * met->mpy()));
    }
    float CalculateHt(xAOD::IParticleContainer* Container, float MinPt) {
        float Ht = 0;
        if (Container)
            for (auto Part : *Container)
                if (Part->pt() > MinPt) Ht += Part->pt();
        return Ht;
    }
    float CalculateLeptonHt(xAOD::ElectronContainer* Electrons, xAOD::MuonContainer* Muons, xAOD::TauJetContainer* Taus) {
        return CalculateHt(Electrons) + CalculateHt(Muons) + CalculateHt(Taus);
    }
    float ComputeMtMin(xAOD::IParticleContainer* Collection, XAMPP::Storage<XAMPPmet>* met) {
        if (met->GetValue()) return ComputeMtMin(Collection, met->GetValue());
        return -1.;
    }
    float ComputeMtMin(xAOD::IParticleContainer* Collection, xAOD::MissingET* met) {
        float MtMin = -1.;
        const xAOD::IParticle* closestPart = GetClosestParticle(Collection, met);
        if (closestPart) return ComputeMt(closestPart, met);
        return MtMin;
    }
    float ComputeMtMin(xAOD::IParticleContainer* Collection, const xAOD::IParticle* P1) {
        float MtMin = -1.;
        const xAOD::IParticle* closestPart = GetClosestParticle(Collection, P1);
        if (closestPart) return ComputeMt(closestPart, P1);
        return MtMin;
    }
    float ComputeMtMax(xAOD::IParticleContainer* Collection, XAMPP::Storage<XAMPPmet>* met) {
        if (met->GetValue()) return ComputeMtMax(Collection, met->GetValue());
        return -1.;
    }
    float ComputeMtMax(xAOD::IParticleContainer* Collection, xAOD::MissingET* met) {
        float MtMin = -1.;
        const xAOD::IParticle* farestPart = GetFarestParticle(Collection, met);
        if (farestPart) return ComputeMt(farestPart, met);
        return MtMin;
    }
    float ComputeMtMax(xAOD::IParticleContainer* Collection, const xAOD::IParticle* P1) {
        float MtMin = -1.;
        const xAOD::IParticle* farestPart = GetFarestParticle(Collection, P1);
        if (farestPart) return ComputeMt(farestPart, P1);
        return MtMin;
    }
    float ComputeJetAngularSep(xAOD::JetContainer* Jets) {
        float dR_min_jets = 99.;
        if (Jets && Jets->size() > 1) {
            for (const auto& J : *Jets) {
                const xAOD::IParticle* NearJet = GetClosestParticle(Jets, J);
                float dR = xAOD::P4Helpers::deltaR(NearJet, J);
                if (dR_min_jets > dR) dR_min_jets = dR;
            }
        }
        return dR_min_jets;
    }

    const xAOD::IParticle* GetClosestParticle(xAOD::IParticleContainer* Cont, xAOD::MissingET* met, xAOD::IParticleContainer* Exclude) {
        const xAOD::IParticle* RetPart = nullptr;
        double minDPhi = 99.;
        if (Cont && met) {
            for (const auto& P : *Cont) {
                if (Exclude && IsInContainer(P, *Exclude)) continue;
                float dPhi = fabs(xAOD::P4Helpers::deltaPhi(P, met));
                if (dPhi < minDPhi) {
                    RetPart = P;
                    minDPhi = dPhi;
                }
            }
        }
        return RetPart;
    }
    const xAOD::IParticle* GetClosestParticle(xAOD::IParticleContainer* Cont, XAMPP::Storage<XAMPPmet>* met,
                                              xAOD::IParticleContainer* Exclude) {
        if (met) return GetClosestParticle(Cont, met->GetValue(), Exclude);
        return nullptr;
    }
    const xAOD::IParticle* GetClosestParticle(xAOD::IParticleContainer* Cont, const xAOD::IParticle* P1,
                                              xAOD::IParticleContainer* Exclude) {
        const xAOD::IParticle* RetPart = nullptr;
        double minDPhi = 99.;
        if (Cont && P1) {
            for (const auto& P : *Cont) {
                if (Exclude && IsInContainer(P, *Exclude)) continue;
                float dPhi = fabs(xAOD::P4Helpers::deltaPhi(P, P1));
                if (dPhi < minDPhi) {
                    RetPart = P;
                    minDPhi = dPhi;
                }
            }
        }
        return RetPart;
    }
    const xAOD::IParticle* GetFarestParticle(xAOD::IParticleContainer* Cont, xAOD::MissingET* met, xAOD::IParticleContainer* Exclude) {
        const xAOD::IParticle* RetPart = nullptr;
        double maxDPhi = -99.;
        if (Cont && met) {
            for (const auto& P : *Cont) {
                if (Exclude && IsInContainer(P, *Exclude)) continue;
                float dPhi = fabs(xAOD::P4Helpers::deltaPhi(P, met));
                if (dPhi > maxDPhi) {
                    RetPart = P;
                    maxDPhi = dPhi;
                }
            }
        }
        return RetPart;
    }
    const xAOD::IParticle* GetFarestParticle(xAOD::IParticleContainer* Cont, XAMPP::Storage<XAMPPmet>* met,
                                             xAOD::IParticleContainer* Exclude) {
        if (met) return GetFarestParticle(Cont, met->GetValue(), Exclude);
        return nullptr;
    }
    const xAOD::IParticle* GetFarestParticle(xAOD::IParticleContainer* Cont, const xAOD::IParticle* P1, xAOD::IParticleContainer* Exclude) {
        const xAOD::IParticle* RetPart = nullptr;
        double maxDPhi = -99.;
        if (Cont && P1) {
            for (const auto& P : *Cont) {
                if (Exclude && IsInContainer(P, *Exclude)) continue;
                float dPhi = fabs(xAOD::P4Helpers::deltaPhi(P, P1));
                if (dPhi > maxDPhi) {
                    RetPart = P;
                    maxDPhi = dPhi;
                }
            }
        }
        return RetPart;
    }
    float ComputeDPhiMin(xAOD::JetContainer* Jets, XAMPP::Storage<XAMPPmet>* met, unsigned int NJetsToUse) {
        if (met->GetValue()) return ComputeDPhiMin(Jets, met->GetValue(), NJetsToUse);
        return -1.;
    }
    float ComputeDPhiMin(xAOD::JetContainer* Jets, xAOD::MissingET* met, unsigned int NJetsToUse) {
        float minDPhi = 99.;
        xAOD::Jet* closestJet = nullptr;
        if (Jets && met) {
            if (NJetsToUse == 0)
                NJetsToUse = (unsigned int)(Jets->size());
            else
                NJetsToUse = NJetsToUse > Jets->size() ? Jets->size() : NJetsToUse;
            unsigned int curJetIdx = 0;
            for (auto jet : *Jets) {
                ++curJetIdx;
                if (curJetIdx > NJetsToUse) break;
                float dP = fabs(xAOD::P4Helpers::deltaPhi(jet, met));
                if (dP < minDPhi) {
                    closestJet = jet;
                    minDPhi = dP;
                }
            }
            if (closestJet) return xAOD::P4Helpers::deltaPhi(closestJet, met);
        }
        return -FLT_MAX;
    }
    bool HasTauCandidate(xAOD::JetContainer* Jets, xAOD::MissingET* met, unsigned int primVtxIdx, float& MtTauCand, int& Ntracks) {
        static CharDecorator dec_bjet("bjet");
        static CharDecorator dec_isTauCand("isTauCand");
        float minAbsDPhi = FLT_MAX;
        bool foundCandidate = false;
        for (auto jet : *Jets) {
            dec_isTauCand(*jet) = false;
            float AbsDPhi = fabs(TVector2::Phi_mpi_pi(jet->phi() - met->phi()));
            if (AbsDPhi < 0.2 * TMath::Pi() && !dec_bjet(*jet) && fabs(jet->eta()) < 2.5) {
                std::vector<int> nTrkVec;
                jet->getAttribute(xAOD::JetAttribute::NumTrkPt500, nTrkVec);
                if (nTrkVec[primVtxIdx] < 5) {
                    foundCandidate = true;
                    dec_isTauCand(*jet) = true;
                    if (AbsDPhi < minAbsDPhi) {
                        minAbsDPhi = AbsDPhi;
                        MtTauCand = ComputeMt(*jet, *met);
                        Ntracks = nTrkVec[0];
                    }
                }
                nTrkVec.clear();
            }
        }
        return foundCandidate;
    }
    float ComputeMinMt(xAOD::JetContainer* Jets, xAOD::MissingET* met) {
        float MtMin = -1.;
        float minMt = FLT_MAX;
        for (auto jet : *Jets) {
            float currentMt = ComputeMt(*jet, *met);
            if (currentMt < minMt) { minMt = currentMt; }
        }
        if (minMt < FLT_MAX) return minMt;
        return MtMin;
    }
    float ComputeDeltaPhi(xAOD::MissingET* met1, xAOD::MissingET* met2) {
        if (met1 && met2) return xAOD::P4Helpers::deltaPhi(met1->phi(), met2->phi());
        return 99.;
    }
    float ComputeDeltaPhi(XAMPP::Storage<XAMPPmet>* met1, XAMPP::Storage<XAMPPmet>* met2) {
        if (met1 && met2) return ComputeDeltaPhi(met1->GetValue(), met2->GetValue());
        return 99.;
    }
    float ComputeAbsDeltaPhi(XAMPP::Storage<XAMPPmet>* met1, XAMPP::Storage<XAMPPmet>* met2) { return fabs(ComputeDeltaPhi(met1, met2)); }
    float ComputeAbsDeltaPhi(xAOD::MissingET* met1, xAOD::MissingET* met2) { return fabs(ComputeDeltaPhi(met1, met2)); }
    const xAOD::IParticle* GetClosestParticle(const xAOD::IParticleContainer* LookIn, const xAOD::IParticle* RefPart, bool UseRapidity,
                                              const xAOD::IParticleContainer* RefContainer) {
        const xAOD::IParticle* Ret = nullptr;
        float MinDR = 6.e4;
        for (const auto C : *LookIn) {
            if (!Overlaps(RefPart, C, MinDR, UseRapidity)) continue;
            Ret = C;
            MinDR = xAOD::P4Helpers::deltaR2(RefPart, C, UseRapidity);
        }
        if (Ret && RefContainer) {
            if (!IsInContainer(RefPart, *RefContainer)) {
                Warning("GetClosestParticle()",
                        "You want the particles to be closest to each other. "
                        "However, "
                        "the particle is not in the reference container given");
            } else {
                const xAOD::IParticle* Check = GetClosestParticle(RefContainer, Ret, UseRapidity);
                // It is not the best pair
                if (!IsSame(Check, RefPart)) { return nullptr; }
            }
        }
        return Ret;
    }
    bool RecoZfromLeps(xAOD::IParticleContainer* Leps, xAOD::Particle* RecoZ, float MassWindow) {
        static xAOD::IParticleContainer UsedLeps(SG::VIEW_ELEMENTS);
        static IntDecorator dec_Origin("Origin");
        xAOD::IParticle* usedLep;
        xAOD::IParticle* usedLep1;
        TLorentzVector MyZ;
        bool HasZ = false;
        dec_Origin(*RecoZ) = -1;
        for (xAOD::IParticleContainer::iterator L = Leps->begin(); L != Leps->end(); ++L) {
            if (IsInContainer(*L, UsedLeps)) continue;
            for (xAOD::IParticleContainer::iterator L1 = L + 1; L1 != Leps->end(); ++L1) {
                if (!IsSFOS(**L, **L1) || IsInContainer(*L1, UsedLeps)) continue;
                TLorentzVector ZCand = (*L)->p4() + (*L1)->p4();
                float W = fabs(ZCand.M() - Z_MASS);
                if (W < MassWindow && W < fabs(MyZ.M() - Z_MASS)) {
                    MyZ = ZCand;
                    HasZ = true;
                    usedLep = (*L);
                    usedLep1 = (*L1);
                    dec_Origin(*RecoZ) = TypeToPdgId(*L);
                }
            }
        }
        RecoZ->setPxPyPzE(MyZ.Px(), MyZ.Py(), MyZ.Pz(), MyZ.E());
        RecoZ->setPdgId(23);
        RecoZ->setCharge(0.);
        if (HasZ) {
            UsedLeps.push_back(usedLep);
            UsedLeps.push_back(usedLep1);
        } else
            UsedLeps.clear();
        return HasZ;
    }
    void GetTopCandidates(xAOD::JetContainer* BJets, xAOD::JetContainer* LightJets, xAOD::Particle* W1Cand, xAOD::Particle* W2Cand,
                          xAOD::Particle* Top1Cand, xAOD::Particle* Top2Cand) {
        TLorentzVector MyTop1;
        TLorentzVector MyW1;
        TLorentzVector MyTop2;
        TLorentzVector MyW2;
        if (BJets->size() > 0 && (BJets->size() + LightJets->size()) > 2) {
            std::vector<TLorentzVector> myBJets;
            std::vector<TLorentzVector> myLightJets;

            int ibj = 0;
            for (auto bjet : *BJets) {
                if (ibj < 2) {
                    myBJets.push_back(bjet->p4());
                } else {
                    myLightJets.push_back(bjet->p4());
                }
                ++ibj;
            }

            for (auto jet : *LightJets) myLightJets.push_back(jet->p4());

            // cannot build top candidate if 2 bjets and only 1 light jets
            if ((myBJets.size() > 0) && (myLightJets.size() > 1)) {
                double DRmin = DBL_MAX;
                unsigned int j1W1 = 1000;
                unsigned int j2W1 = 1000;
                unsigned int j3T1 = 1000;
                unsigned int j1W2 = 1000;
                unsigned int j2W2 = 1000;
                unsigned int j3T2 = 1000;

                for (unsigned int jet1 = 0; jet1 < myLightJets.size(); ++jet1) {
                    for (unsigned int jet2 = jet1 + 1; jet2 < myLightJets.size(); ++jet2) {
                        double DR = myLightJets[jet1].DeltaR(myLightJets[jet2]);
                        if (DR < DRmin) {
                            DRmin = DR;
                            j1W1 = jet1;
                            j2W1 = jet2;
                        }
                    }
                }

                MyW1 = (myLightJets[j1W1] + myLightJets[j2W1]);

                DRmin = DBL_MAX;

                for (unsigned int jet1 = 0; jet1 < myBJets.size(); ++jet1) {
                    double DR = MyW1.DeltaR(myBJets[jet1]);
                    if (DR < DRmin) {
                        DRmin = DR;
                        j3T1 = jet1;
                    }
                }

                MyTop1 = (MyW1 + myBJets[j3T1]);

                // now the second top candidate
                if (myLightJets.size() > 3) {
                    DRmin = DBL_MAX;
                    for (unsigned int jet1 = 0; jet1 < myLightJets.size(); ++jet1) {
                        if (jet1 == j1W1 || jet1 == j2W1) continue;
                        for (unsigned int jet2 = jet1 + 1; jet2 < myLightJets.size(); ++jet2) {
                            if (jet2 == j1W1 || jet2 == j2W1) continue;
                            double DR = myLightJets[jet1].DeltaR(myLightJets[jet2]);
                            if (DR < DRmin) {
                                DRmin = DR;
                                j1W2 = jet1;
                                j2W2 = jet2;
                            }
                        }
                    }

                    MyW2 = (myLightJets[j1W2] + myLightJets[j2W2]);

                    if (myBJets.size() > 1) {
                        DRmin = DBL_MAX;
                        for (unsigned int jet1 = 0; jet1 < myBJets.size(); ++jet1) {
                            if (jet1 == j3T1) continue;
                            double DR = MyW2.DeltaR(myBJets[jet1]);
                            if (DR < DRmin) {
                                DRmin = DR;
                                j3T2 = jet1;
                            }
                        }

                        MyTop2 = (MyW2 + myBJets[j3T2]);

                        //      // as an alternative to TopCand2, only take
                        //      second b-jet and closest light jet DRmin =
                        //      DBL_MAX; j3T2 = 1000; j1W2 = 1000; j2W2 = 1000;
                        //      for (unsigned int bjet1 = 0; bjet1 <
                        //      myBJets.size(); ++bjet1)
                        //      {
                        //          if (bjet1 == j3T1) continue;
                        //          for (unsigned int jet1 = 0; jet1 <
                        //          myLightJets.size();
                        //          ++jet1) {
                        //              if (jet1 == j1W1 || jet1 == j2W1)
                        //              continue; double DR =
                        //              myLightJets[jet1].DeltaR(myBJets[bjet1]);
                        //              if (DR < DRmin) {
                        //                  DRmin = DR;
                        //                  j3T2 = bjet1;
                        //                  j1W2 = jet1;
                        //              }
                        //          }
                        //      }
                        //      m_eventInfo->TopCand2BPlusdRmin = (myBJets[j3T2]
                        //      + myLightJets[j1W2]);
                        //      // for now, also save top reconstructed with the
                        //      other light jet for (unsigned int jet1 = 0; jet1
                        //      < myLightJets.size();
                        //      ++jet1) {
                        //          if (jet1 == j1W1 || jet1 == j2W1 || jet1 ==
                        //          j1W2) continue; j2W2 = jet1;
                        //      }
                        //      m_eventInfo->TopCand2BPlusdRmax = (myBJets[j3T2]
                        //      + myLightJets[j2W2]);
                    }
                }
            }
        }
        static IntDecorator dec_RecoType("RecoType");
        static CharDecorator dec_WellDefined("WellDefined");
        if (MyW1.M() != 0.) {
            W1Cand->setPxPyPzE(MyW1.Px(), MyW1.Py(), MyW1.Pz(), MyW1.E());
            W1Cand->setPdgId(24);
            dec_RecoType(*W1Cand) = Stop_RecoTypes::TopWCandidate1;
            dec_WellDefined(*W1Cand) = true;
        } else
            dec_WellDefined(*W1Cand) = false;
        if (MyW2.M() != 0.) {
            W2Cand->setPxPyPzE(MyW2.Px(), MyW2.Py(), MyW2.Pz(), MyW2.E());
            W2Cand->setPdgId(24);
            dec_RecoType(*W2Cand) = Stop_RecoTypes::TopWCandidate2;
            dec_WellDefined(*W2Cand) = true;
        } else
            dec_WellDefined(*W2Cand) = false;
        if (MyTop1.M() != 0.) {
            Top1Cand->setPxPyPzE(MyTop1.Px(), MyTop1.Py(), MyTop1.Pz(), MyTop1.E());
            Top1Cand->setPdgId(6);
            dec_RecoType(*Top1Cand) = Stop_RecoTypes::TopWCandidate1;
            dec_WellDefined(*Top1Cand) = true;
        } else
            dec_WellDefined(*Top1Cand) = false;
        if (MyTop2.M() != 0.) {
            Top2Cand->setPxPyPzE(MyTop2.Px(), MyTop2.Py(), MyTop2.Pz(), MyTop2.E());
            Top2Cand->setPdgId(6);
            dec_RecoType(*Top2Cand) = Stop_RecoTypes::TopWCandidate2;
            dec_WellDefined(*Top2Cand) = true;
        } else
            dec_WellDefined(*Top2Cand) = false;
        // could also add support for W with PdgId 24
    }

    void GetTopCandidatesDRB4(xAOD::JetContainer* BJets, xAOD::JetContainer* LightJets, xAOD::Particle* W1CandDRB4,
                              xAOD::Particle* W2CandDRB4, xAOD::Particle* Top1CandDRB4, xAOD::Particle* Top2CandDRB4) {
        TLorentzVector MyTop1;
        TLorentzVector MyW1;
        TLorentzVector MyTop2;
        TLorentzVector MyW2;
        if (BJets->size() > 0 && (BJets->size() + LightJets->size()) > 2) {
            std::vector<TLorentzVector> myBJets;
            std::vector<TLorentzVector> myLightJets;

            int ibj = 0;
            for (auto bjet : *BJets) {
                if (ibj < 2) {
                    myBJets.push_back(bjet->p4());
                } else {
                    myLightJets.push_back(bjet->p4());
                }
                ++ibj;
            }

            for (auto jet : *LightJets) myLightJets.push_back(jet->p4());

            // cannot build top candidate if 2 bjets and only 1 light jets
            if ((myBJets.size() > 0) && (myLightJets.size() > 1)) {
                double DRmin = DBL_MAX;
                unsigned int j1W1 = 1000;
                unsigned int j2W1 = 1000;
                unsigned int j3T1 = 1000;
                unsigned int j1W2 = 1000;
                unsigned int j2W2 = 1000;
                unsigned int j3T2 = 1000;

                unsigned int NLightJets = 4;
                if (myLightJets.size() < 4) NLightJets = myLightJets.size();

                for (unsigned int jet1 = 0; jet1 < NLightJets; ++jet1) {
                    for (unsigned int jet2 = jet1 + 1; jet2 < NLightJets; ++jet2) {
                        double DR = myLightJets[jet1].DeltaR(myLightJets[jet2]);
                        if (DR < DRmin) {
                            DRmin = DR;
                            j1W1 = jet1;
                            j2W1 = jet2;
                        }
                    }
                }

                MyW1 = (myLightJets[j1W1] + myLightJets[j2W1]);

                DRmin = DBL_MAX;

                for (unsigned int jet1 = 0; jet1 < myBJets.size(); ++jet1) {
                    double DR = MyW1.DeltaR(myBJets[jet1]);
                    if (DR < DRmin) {
                        DRmin = DR;
                        j3T1 = jet1;
                    }
                }

                MyTop1 = (MyW1 + myBJets[j3T1]);

                // now the second top candidate
                if (myLightJets.size() > 3) {
                    DRmin = DBL_MAX;
                    for (unsigned int jet1 = 0; jet1 < NLightJets; ++jet1) {
                        if (jet1 == j1W1 || jet1 == j2W1) continue;
                        for (unsigned int jet2 = jet1 + 1; jet2 < NLightJets; ++jet2) {
                            if (jet2 == j1W1 || jet2 == j2W1) continue;
                            double DR = myLightJets[jet1].DeltaR(myLightJets[jet2]);
                            if (DR < DRmin) {
                                DRmin = DR;
                                j1W2 = jet1;
                                j2W2 = jet2;
                            }
                        }
                    }

                    MyW2 = (myLightJets[j1W2] + myLightJets[j2W2]);

                    if (myBJets.size() > 1) {
                        DRmin = DBL_MAX;
                        for (unsigned int jet1 = 0; jet1 < myBJets.size(); ++jet1) {
                            if (jet1 == j3T1) continue;
                            double DR = MyW2.DeltaR(myBJets[jet1]);
                            if (DR < DRmin) {
                                DRmin = DR;
                                j3T2 = jet1;
                            }
                        }

                        MyTop2 = (MyW2 + myBJets[j3T2]);
                    }
                }
            }
        }
        static IntDecorator dec_RecoType("RecoType");
        static CharDecorator dec_WellDefined("WellDefined");
        if (MyW1.M() != 0.) {
            W1CandDRB4->setPxPyPzE(MyW1.Px(), MyW1.Py(), MyW1.Pz(), MyW1.E());
            W1CandDRB4->setPdgId(24);
            dec_RecoType(*W1CandDRB4) = Stop_RecoTypes::DRB4TopWCandidate1;
            dec_WellDefined(*W1CandDRB4) = true;
        } else
            dec_WellDefined(*W1CandDRB4) = false;
        if (MyW2.M() != 0.) {
            W2CandDRB4->setPxPyPzE(MyW2.Px(), MyW2.Py(), MyW2.Pz(), MyW2.E());
            W2CandDRB4->setPdgId(24);
            dec_RecoType(*W2CandDRB4) = Stop_RecoTypes::DRB4TopWCandidate2;
            dec_WellDefined(*W2CandDRB4) = true;
        } else
            dec_WellDefined(*W2CandDRB4) = false;
        if (MyTop1.M() != 0.) {
            Top1CandDRB4->setPxPyPzE(MyTop1.Px(), MyTop1.Py(), MyTop1.Pz(), MyTop1.E());
            Top1CandDRB4->setPdgId(6);
            dec_RecoType(*Top1CandDRB4) = Stop_RecoTypes::DRB4TopWCandidate1;
            dec_WellDefined(*Top1CandDRB4) = true;
        } else
            dec_WellDefined(*Top1CandDRB4) = false;
        if (MyTop2.M() != 0.) {
            Top2CandDRB4->setPxPyPzE(MyTop2.Px(), MyTop2.Py(), MyTop2.Pz(), MyTop2.E());
            Top2CandDRB4->setPdgId(6);
            dec_RecoType(*Top2CandDRB4) = Stop_RecoTypes::DRB4TopWCandidate2;
            dec_WellDefined(*Top2CandDRB4) = true;
        } else
            dec_WellDefined(*Top2CandDRB4) = false;
    }

    void GetTopCandidatesMinMass(xAOD::JetContainer* BJets, xAOD::JetContainer* LightJets, xAOD::Particle* W1CandMinMass,
                                 xAOD::Particle* W2CandMinMass, xAOD::Particle* Top1CandMinMass, xAOD::Particle* Top2CandMinMass) {
        TLorentzVector MyTop1;
        TLorentzVector MyW1;
        TLorentzVector MyTop2;
        TLorentzVector MyW2;
        if (BJets->size() > 0 && (BJets->size() + LightJets->size()) > 2) {
            std::vector<TLorentzVector> myBJets;
            std::vector<TLorentzVector> myLightJets;

            int ibj = 0;
            for (auto bjet : *BJets) {
                if (ibj < 2) {
                    myBJets.push_back(bjet->p4());
                } else {
                    myLightJets.push_back(bjet->p4());
                }
                ++ibj;
            }

            for (auto jet : *LightJets) myLightJets.push_back(jet->p4());

            // cannot build top candidate if 2 bjets and only 1 light jets
            if ((myBJets.size() > 0) && (myLightJets.size() > 1)) {
                double Mmin = DBL_MAX;
                unsigned int j1W1 = 1000;
                unsigned int j2W1 = 1000;
                unsigned int j3T1 = 1000;
                unsigned int j1W2 = 1000;
                unsigned int j2W2 = 1000;
                unsigned int j3T2 = 1000;

                unsigned int NJets = myLightJets.size();
                for (unsigned int jet1 = 0; jet1 < NJets; ++jet1) {
                    for (unsigned int jet2 = jet1 + 1; jet2 < NJets; ++jet2) {
                        double M = (myLightJets[jet1] + myLightJets[jet2]).M();
                        if (fabs(M - 80385.) < Mmin) {
                            Mmin = fabs(M - 80385.);
                            j1W1 = jet1;
                            j2W1 = jet2;
                        }
                    }
                }

                MyW1 = (myLightJets[j1W1] + myLightJets[j2W1]);

                double DRmin = DBL_MAX;

                for (unsigned int jet1 = 0; jet1 < myBJets.size(); ++jet1) {
                    double DR = (myLightJets[j1W1] + myLightJets[j2W1]).DeltaR(myBJets[jet1]);
                    if (DR < DRmin) {
                        DRmin = DR;
                        j3T1 = jet1;
                    }
                }

                MyTop1 = (MyW1 + myBJets[j3T1]);

                if (myLightJets.size() > 3) {
                    Mmin = DBL_MAX;
                    for (unsigned int jet1 = 0; jet1 < NJets; ++jet1) {
                        if (jet1 == j1W1 || jet1 == j2W1) continue;
                        for (unsigned int jet2 = jet1 + 1; jet2 < NJets; ++jet2) {
                            if (jet2 == j1W1 || jet2 == j2W1) continue;
                            double M = (myLightJets[jet1] + myLightJets[jet2]).M();
                            if (fabs(M - 80385.) < Mmin) {
                                Mmin = fabs(M - 80385.);
                                j1W2 = jet1;
                                j2W2 = jet2;
                            }
                        }
                    }

                    MyW2 = (myLightJets[j1W2] + myLightJets[j2W2]);

                    if (myBJets.size() > 1) {
                        DRmin = DBL_MAX;
                        for (unsigned int jet1 = 0; jet1 < myBJets.size(); ++jet1) {
                            if (jet1 == j3T1) continue;
                            double DR = (myLightJets[j1W2] + myLightJets[j2W2]).DeltaR(myBJets[jet1]);
                            if (DR < DRmin) {
                                DRmin = DR;
                                j3T2 = jet1;
                            }
                        }

                        MyTop2 = (MyW2 + myBJets[j3T2]);
                    }
                }
            }
        }

        static IntDecorator dec_RecoType("RecoType");
        static CharDecorator dec_WellDefined("WellDefined");
        if (MyW1.M() != 0.) {
            W1CandMinMass->setPxPyPzE(MyW1.Px(), MyW1.Py(), MyW1.Pz(), MyW1.E());
            W1CandMinMass->setPdgId(24);
            dec_RecoType(*W1CandMinMass) = Stop_RecoTypes::MinMassTopWCandidate1;
            dec_WellDefined(*W1CandMinMass) = true;
        } else
            dec_WellDefined(*W1CandMinMass) = false;
        if (MyW2.M() != 0.) {
            W2CandMinMass->setPxPyPzE(MyW2.Px(), MyW2.Py(), MyW2.Pz(), MyW2.E());
            W2CandMinMass->setPdgId(24);
            dec_RecoType(*W2CandMinMass) = Stop_RecoTypes::MinMassTopWCandidate2;
            dec_WellDefined(*W2CandMinMass) = true;
        } else
            dec_WellDefined(*W2CandMinMass) = false;
        if (MyTop1.M() != 0.) {
            Top1CandMinMass->setPxPyPzE(MyTop1.Px(), MyTop1.Py(), MyTop1.Pz(), MyTop1.E());
            Top1CandMinMass->setPdgId(6);
            dec_RecoType(*Top1CandMinMass) = Stop_RecoTypes::MinMassTopWCandidate1;
            dec_WellDefined(*Top1CandMinMass) = true;
        } else
            dec_WellDefined(*Top1CandMinMass) = false;
        if (MyTop2.M() != 0.) {
            Top2CandMinMass->setPxPyPzE(MyTop2.Px(), MyTop2.Py(), MyTop2.Pz(), MyTop2.E());
            Top2CandMinMass->setPdgId(6);
            dec_RecoType(*Top2CandMinMass) = Stop_RecoTypes::MinMassTopWCandidate2;
            dec_WellDefined(*Top2CandMinMass) = true;
        } else
            dec_WellDefined(*Top2CandMinMass) = false;
    }
    float GetChi2FromTopConstruction(float WMass, float TopMass) {
        return TMath::Power(WMass - XAMPP::W_MASS, 2) / XAMPP::W_MASS + TMath::Power(TopMass - XAMPP::Top_MASS, 2) / XAMPP::Top_MASS;
    }
    bool IsBJet(const xAOD::IParticle* P) {
        static CharAccessor dec_bjet("bjet");
        if (!dec_bjet.isAvailable(*P)) return false;
        return dec_bjet(*P);
    }
    xAOD::Particle* GetTop(const xAOD::IParticle* W, const xAOD::Jet* B, XAMPP::IReconstructedParticles* Constructor) {
        if (!B) return nullptr;
        static FloatDecorator dec_Chi2("Chi2");
        xAOD::Particle* Top = Constructor->CreateEmptyParticle();
        AddFourMomenta(W, B, Top, &IsBJet);
        Top->setPdgId(6);
        dec_Chi2(*Top) = GetChi2FromTopConstruction(W->m(), Top->m());
        return Top;
    }
    xAOD::Particle* GetTop(const xAOD::IParticle* W, const xAOD::Jet* B, asg::AnaToolHandle<XAMPP::IReconstructedParticles>& Constructor) {
        return GetTop(W, B, Constructor.operator->());
    }
    StatusCode GetTopCandidatesChi2(xAOD::JetContainer* SignalJets, const xAOD::JetContainer* BJets,
                                    asg::AnaToolHandle<XAMPP::IReconstructedParticles>& Constructor) {
        return GetTopCandidatesChi2(SignalJets, BJets, Constructor.operator->());
    }

    StatusCode GetTopCandidatesChi2(xAOD::JetContainer* SignalJets, const xAOD::JetContainer* BJets,
                                    XAMPP::IReconstructedParticles* Constructor) {
        // we need at least 2 b-jets
        if (BJets->size() < 2) return StatusCode::SUCCESS;
        const xAOD::Jet* Bjet1 = BJets->size() > 0 ? BJets->at(0) : nullptr;
        const xAOD::Jet* Bjet2 = BJets->size() > 1 ? BJets->at(1) : nullptr;
        // the other b-jets are put into the light jet vector
        xAOD::IParticleContainer LightJets(SG::VIEW_ELEMENTS);
        xAOD::IParticleContainer WCandidates(SG::VIEW_ELEMENTS);
        for (auto Jet : *SignalJets) {
            if (IsSame(Bjet1, Jet) || IsSame(Bjet2, Jet))
                continue;
            else {
                LightJets.push_back(Jet);
                // Boosted W decays can result into 1 single jet...
                WCandidates.push_back(Jet);
            }
        }
        // create a sub-container in RecoCandidates which contains all W
        // candidates
        if (!Constructor->CreateSubContainer("WCandidates").isSuccess()) return StatusCode::FAILURE;
        // add candidates made from 2 light jets
        ConstructInvariantMomenta(LightJets, Constructor, &IsBJet, 2);
        for (auto W : *Constructor->GetSubContainer("WCandidates")) {
            WCandidates.push_back(W);
            W->setPdgId(24);
            Constructor->WellDefined(W, false);
        }
        Constructor->DetachSubContainer();
        // create a sub-container in RecoCandidates which contains all top
        // candidates
        if (!Constructor->CreateSubContainer("TopCandidates").isSuccess()) return StatusCode::FAILURE;

        static IntDecorator dec_RecoType("RecoType");
        static FloatDecorator dec_Chi2("Chi2");
        if (WCandidates.empty()) return StatusCode::SUCCESS;
        xAOD::IParticleContainer::const_iterator WEnd = WCandidates.end();
        xAOD::IParticleContainer::const_iterator WBegin = WCandidates.begin();
        float Chi2Min = FLT_MAX;
        xAOD::Particle* TopCandChi2_1 = nullptr;
        xAOD::Particle* TopCandChi2_2 = nullptr;
        xAOD::IParticle* WCandChi2_1 = nullptr;
        xAOD::IParticle* WCandChi2_2 = nullptr;
        for (xAOD::IParticleContainer::const_iterator W1 = WBegin + 1; W1 != WEnd; ++W1) {
            xAOD::Particle* TopW1B1 = GetTop(*W1, Bjet1, Constructor);
            xAOD::Particle* TopW1B2 = GetTop(*W1, Bjet2, Constructor);
            for (xAOD::IParticleContainer::const_iterator W2 = WBegin; W2 != W1; ++W2) {
                if (HasCommonConstructingParticles(*W1, *W2)) { continue; }
                xAOD::Particle* TopW2B2 = GetTop(*W2, Bjet2, Constructor);
                xAOD::Particle* TopW2B1 = GetTop(*W2, Bjet1, Constructor);
                float Chi2_1 = dec_Chi2(*TopW1B1) + dec_Chi2(*TopW2B2);
                float Chi2_2 = dec_Chi2(*TopW1B2) + dec_Chi2(*TopW2B1);
                if ((Chi2_1 < Chi2Min) || (Chi2_2 < Chi2Min)) {
                    if (TopCandChi2_1) Constructor->WellDefined(TopCandChi2_1, false);
                    if (TopCandChi2_2) Constructor->WellDefined(TopCandChi2_2, false);
                    WCandChi2_1 = *W1;
                    WCandChi2_2 = *W2;
                    if (Chi2_1 < Chi2_2) {
                        Constructor->WellDefined(TopW1B2, false);
                        Constructor->WellDefined(TopW2B1, false);
                        TopCandChi2_1 = TopW1B1;
                        TopCandChi2_2 = TopW2B2;
                        Chi2Min = Chi2_1;
                    } else {
                        Constructor->WellDefined(TopW1B1, false);
                        Constructor->WellDefined(TopW2B2, false);
                        TopCandChi2_1 = TopW1B2;
                        TopCandChi2_2 = TopW2B1;
                        Chi2Min = Chi2_2;
                    }
                } else {
                    Constructor->WellDefined(TopW1B1, false);
                    Constructor->WellDefined(TopW2B2, false);
                    Constructor->WellDefined(TopW1B2, false);
                    Constructor->WellDefined(TopW2B1, false);
                }
            }
        }
        if (TopCandChi2_1) {
            Constructor->WellDefined(TopCandChi2_1, true);
            dec_RecoType(*TopCandChi2_1) = Stop_RecoTypes::Chi2TopWCandidate1;
        }
        if (TopCandChi2_2) {
            Constructor->WellDefined(TopCandChi2_2, true);
            dec_RecoType(*TopCandChi2_2) = Stop_RecoTypes::Chi2TopWCandidate2;
        }
        if (WCandChi2_1 && WCandChi2_1->type() == xAOD::Type::ObjectType::Particle) {
            Constructor->WellDefined(WCandChi2_1, true);
            dec_RecoType(*WCandChi2_1) = Stop_RecoTypes::Chi2TopWCandidate1;
        }
        if (WCandChi2_2 && WCandChi2_2->type() == xAOD::Type::ObjectType::Particle) {
            Constructor->WellDefined(WCandChi2_2, true);
            dec_RecoType(*WCandChi2_2) = Stop_RecoTypes::Chi2TopWCandidate2;
        }
        Constructor->DetachSubContainer();
        return StatusCode::SUCCESS;
    }

    const xAOD::TruthParticle* getTruthMatchedParticle(const xAOD::IParticle& P) {
        static SG::AuxElement::Accessor<ElementLink<xAOD::TruthParticleContainer>> tO("truthParticleLink");
        if (P.type() == xAOD::Type::ObjectType::TruthParticle) return dynamic_cast<const xAOD::TruthParticle*>(&P);
        if (!tO.isAvailable(P)) return nullptr;
        ElementLink<xAOD::TruthParticleContainer> xT = tO(P);
        if (xT.isValid()) return (*xT);
        return nullptr;
    }
    const xAOD::TruthParticle* getTruthMatchedParticle(const xAOD::IParticle* P) { return getTruthMatchedParticle(*P); }
    int getParticleTruthOrigin(const xAOD::IParticle& P) {
        static IntAccessor tO("truthOrigin");
        static UIntAccessor acc_classOrigin("classifierParticleOrigin");
        static UIntAccessor acc_partOrigin("particleOrigin");

        if (acc_classOrigin.isAvailable(P)) {
            return acc_classOrigin(P);
        } else if (acc_partOrigin.isAvailable(P)) {
            return acc_partOrigin(P);
        } else if (!tO.isAvailable(P))
            return -1;
        return tO(P);
    }
    int getParticleTruthOrigin(const xAOD::IParticle* P) { return getParticleTruthOrigin(*P); }
    int getParticleTruthType(const xAOD::IParticle& P) {
        static IntAccessor acc_truthType("truthType");
        static UIntAccessor acc_classType("classifierParticleType");
        static UIntAccessor acc_partType("particleType");

        if (acc_classType.isAvailable(P)) {
            return acc_classType(P);
        } else if (acc_partType.isAvailable(P)) {
            return acc_partType(P);
        } else if (!acc_truthType.isAvailable(P))
            return -1;
        return acc_truthType(P);
    }
    int getParticleTruthType(const xAOD::IParticle* P) { return getParticleTruthType(*P); }

    // Types which are not included in the xAOD::TruthParticleClass
    bool isGluon(const xAOD::TruthParticle& P) { return (P.absPdgId() == 21); }
    bool isSparticle(const xAOD::TruthParticle& P) {
        return ((P.absPdgId() >= 1000001 && P.absPdgId() <= 1000039) || (P.absPdgId() >= 2000001 && P.absPdgId() <= 2000015));
    }
    bool isGluon(const xAOD::TruthParticle* P) { return isGluon(*P); }
    bool isSparticle(const xAOD::TruthParticle* P) { return isSparticle(*P); }
    bool isEWboson(const xAOD::TruthParticle& P) { return (P.isW() || P.isZ() || P.isHiggs() /*|| P.isPhoton()*/); }
    bool isEWboson(const xAOD::TruthParticle* P) { return isEWboson(*P); }
    void PromptParticle(const xAOD::IParticle& Part, std::string AddInfo) { PromptParticle(&Part, AddInfo); }
    void PromptParticle(const xAOD::IParticle* Part, std::string AddInfo) {
        if (Part == nullptr) {
            Warning("PromptParticle()", "Null ptr given");
            return;
        }
        std::string PartType;
        if (Part->type() == xAOD::Type::ObjectType::Electron)
            PartType = "Electron";
        else if (Part->type() == xAOD::Type::ObjectType::Muon)
            PartType = "Muon";
        else if (Part->type() == xAOD::Type::ObjectType::Tau)
            PartType = "Tau";
        else if (Part->type() == xAOD::Type::ObjectType::Jet)
            PartType = "Jet";
        else if (Part->type() == xAOD::Type::ObjectType::Photon)
            PartType = "Photon";
        else if (Part->type() == xAOD::Type::ObjectType::TrackParticle)
            PartType = "Track";
        else if (Part->type() == xAOD::Type::ObjectType::CaloCluster)
            PartType = "CaloCluster";
        else if (Part->type() == xAOD::Type::ObjectType::Particle)
            PartType = "Reconstructed";
        else if (Part->type() == xAOD::Type::ObjectType::TruthParticle) {
            PartType = "Truth_";
            const xAOD::TruthParticle* Tpart = dynamic_cast<const xAOD::TruthParticle*>(Part);
            if (Tpart != nullptr) {
                if (Tpart->isElectron() == true)
                    PartType += "Electron";
                else if (Tpart->isMuon() == true)
                    PartType += "Muon";
                else if (Tpart->isTau() == true)
                    PartType += "Tau";
                else if (Tpart->isW() == true)
                    PartType += "W-Boson";
                else if (Tpart->isZ() == true)
                    PartType += "Z-Boson";
                else if (Tpart->isNeutrino() == true)
                    PartType += "Neutrino";
                else if (Tpart->isPhoton() == true)
                    PartType += "Photon";
                else if (Tpart->isHiggs() == true)
                    PartType += "Higgs";
                else if (Tpart->absPdgId() == 1)
                    PartType += "Down";
                else if (Tpart->absPdgId() == 2)
                    PartType += "Up";
                else if (Tpart->absPdgId() == 3)
                    PartType += "Strange";
                else if (Tpart->absPdgId() == 4)
                    PartType += "Charm";
                else if (Tpart->absPdgId() == 5)
                    PartType += "Bottom";
                else if (Tpart->absPdgId() == 6)
                    PartType += "Top";
                else if (Tpart->absPdgId() == 21)
                    PartType += "Gluon";
                else if (Tpart->absPdgId() == 2212)
                    PartType += "Proton";
                else if (Tpart->absPdgId() == 1000001 || Tpart->absPdgId() == 2000001)
                    PartType += "Sdown";
                else if (Tpart->absPdgId() == 1000002 || Tpart->absPdgId() == 2000002)
                    PartType += "Sup";
                else if (Tpart->absPdgId() == 1000003 || Tpart->absPdgId() == 2000003)
                    PartType += "Sstrange";
                else if (Tpart->absPdgId() == 1000004 || Tpart->absPdgId() == 2000004)
                    PartType += "Scharm";
                else if (Tpart->absPdgId() == 1000005 || Tpart->absPdgId() == 2000005)
                    PartType += "Sbottom";
                else if (Tpart->absPdgId() == 1000006 || Tpart->absPdgId() == 2000006)
                    PartType += "Stop";
                else if (Tpart->absPdgId() == 1000021)
                    PartType += "Gluino";
                else if (Tpart->absPdgId() == 1000022)
                    PartType += "Bino";
                else if (Tpart->absPdgId() == 1000023)
                    PartType += "Zino";
                else if (Tpart->absPdgId() == 1000024)
                    PartType += "Wino";
                AddInfo = Form(
                    "pdgId: %i   Barcode: %i Status: %i truthOrigin %i, "
                    "truthType: "
                    "%i, "
                    "isHardProc: %i %s ",
                    Tpart->pdgId(), Tpart->barcode(), Tpart->status(), getParticleTruthOrigin(Tpart), getParticleTruthType(Tpart),
                    isParticleFromHardProcess(Tpart), AddInfo.c_str());
            }
        }
        Info(PartType.c_str(), Form("m: %.2f GeV    pt: %.2f GeV    eta: %.4f   phi: %.4f  Q: "
                                    "%.1f %s",
                                    Part->p4().M() / 1000., (Part->pt() / 1000.), Part->eta(), Part->phi(), Charge(Part), AddInfo.c_str()));
    }
    xAOD::MissingET* GetMET_obj(const std::string& Name, xAOD::MissingETContainer* Cont) {
        xAOD::MissingET* MET = nullptr;
        if (Cont == nullptr) return MET;
        xAOD::MissingETContainer::iterator met_it = Cont->find(Name);
        if (met_it == Cont->end())
            Error("GetMET_obj()", Form("Could not find any MET object named %s in the container", Name.c_str()));
        else
            MET = (*met_it);
        return MET;
    }
    const xAOD::TruthParticle* GetFirstChainLink(const xAOD::TruthParticle* TruthPart) {
        if (!TruthPart) {
            Error("GetFirstChainLink", "No TruthParticle was given");
            return nullptr;
        }
        if (TruthPart->hasProdVtx()) {
            const xAOD::TruthVertex* Vtx = TruthPart->prodVtx();
            for (size_t n = 0; n < Vtx->nIncomingParticles(); ++n) {
                const xAOD::TruthParticle* P = Vtx->incomingParticle(n);
                if (P && P != TruthPart && P->pdgId() == TruthPart->pdgId() && !IsSame(P, TruthPart, true) && !IsInOutGoing(P))
                    return GetFirstChainLink(P);
            }
        }
        return TruthPart;
    }
    const xAOD::TruthParticle* GetLastChainLink(const xAOD::TruthParticle* TruthPart) {
        if (!TruthPart) {
            Error("GetLastChainLink", "No TruthParticle was given");
            return nullptr;
        }
        for (size_t c = 0; c < TruthPart->nChildren(); ++c) {
            const xAOD::TruthParticle* P = TruthPart->child(c);
            if (P && P->pdgId() == TruthPart->pdgId() && !IsSame(P, TruthPart, true) && !IsInOutGoing(P)) return GetLastChainLink(P);
        }
        return TruthPart;
    }
    bool isParticleFromHardProcess(const xAOD::TruthParticle* TruthPart, bool rejectUnknownOrigin) {
        if (TruthPart == nullptr) {
            Error("IsParticleFromHardProcess()", "No TruthParticle was given");
            return false;
        }
        int truthOrigin = getParticleTruthOrigin(TruthPart);
        int truthType = getParticleTruthType(TruthPart);
        // https://gitlab.cern.ch/atlas/athena/blob/master/PhysicsAnalysis/MCTruthClassifier/MCTruthClassifier/MCTruthClassifierDefs.h
        if (truthType != -1) {
            if (truthType == MCTruthPartClassifier::ParticleType::IsoElectron) return true;
            if (truthType == MCTruthPartClassifier::ParticleType::IsoPhoton) return true;
            if (truthType == MCTruthPartClassifier::ParticleType::IsoMuon) return true;
            if (truthType == MCTruthPartClassifier::ParticleType::IsoTau) return true;
        }
        if (rejectUnknownOrigin && truthOrigin == 0) return false;  // this gives cutflow agreement with SimpleAnalysis
        if (truthOrigin != -1) {
            if (truthOrigin == MCTruthPartClassifier::ParticleOrigin::SUSY)
                return true;
            else if (truthOrigin == MCTruthPartClassifier::ParticleOrigin::top)
                return true;  // 10
            else if (truthOrigin == MCTruthPartClassifier::ParticleOrigin::WBoson)
                return true;  // 12
            else if (truthOrigin == MCTruthPartClassifier::ParticleOrigin::ZBoson)
                return true;  // 13
            else if (truthOrigin == MCTruthPartClassifier::ParticleOrigin::Higgs)
                return true;  // 14
            else if (truthOrigin == MCTruthPartClassifier::ParticleOrigin::HiggsMSSM)
                return true;  // 15
            else if (truthOrigin == MCTruthPartClassifier::ParticleOrigin::HeavyBoson)
                return true;  // 16
            else if (truthOrigin == MCTruthPartClassifier::ParticleOrigin::WBosonLRSM)
                return true;  // 17
            else if (truthOrigin == MCTruthPartClassifier::ParticleOrigin::SUSY)
                return true;  // 22
            else if (truthOrigin == MCTruthPartClassifier::ParticleOrigin::DiBoson)
                return true;  // 43
            else if (truthOrigin == MCTruthPartClassifier::ParticleOrigin::ZorHeavyBoson)
                return true;  // 43
        }
        TruthPart = GetFirstChainLink(TruthPart);
        if (TruthPart->hasProdVtx()) {
            const xAOD::TruthVertex* Vtx = TruthPart->prodVtx();
            for (size_t n = 0; n < Vtx->nIncomingParticles(); ++n) {
                const xAOD::TruthParticle* P = Vtx->incomingParticle(n);
                if (P->isLepton() || P->isW() || P->isZ() || (P->isPhoton() && !TruthPart->isElectron()))
                    return isParticleFromHardProcess(P);
                else if (P->isTop() || P->isQuark() || P->isHiggs() || isSparticle(P) || isGluon(P))
                    return true;  // No doubt whether these are from the Primary
                                  // process
            }
        }
        return false;
    }
    bool IsInOutGoing(const xAOD::TruthParticle* P) {
        if (!P) {
            Error("IsInOutGoing", "NoTruthParticle was given");
            return false;
        }
        if (P->hasProdVtx()) {
            const xAOD::TruthVertex* Vtx = P->prodVtx();
            for (size_t n = 0; n < Vtx->nIncomingParticles(); ++n)
                if (IsSame(P, Vtx->incomingParticle(n))) return true;
        }
        if (P->hasDecayVtx()) {
            const xAOD::TruthVertex* Vtx = P->decayVtx();
            for (size_t n = 0; n < Vtx->nOutgoingParticles(); ++n)
                if (IsSame(P, Vtx->outgoingParticle(n))) return true;
        }
        return false;
    }
    float MinDeltaR(const xAOD::IParticle* p, const xAOD::IParticleContainer* pc) {
        const xAOD::IParticle* cP = GetClosestParticle(pc, p);
        if (!cP) return -1;
        return xAOD::P4Helpers::deltaR(cP, p);
    }
    bool IsChargedFlipped(const xAOD::Electron* el) {
        const xAOD::TruthParticle* Tel = xAOD::EgammaHelpers::getBkgElectronMother(el);
        if (!Tel) {
            Warning("IsChargedFlipped()", "No associated truth particle can be found");
            return false;
        }
        return el->charge() * Tel->charge() < 0;
    }

    void ConstructInvariantMomenta(xAOD::IParticleContainer& Leptons, XAMPP::IReconstructedParticles* Constructor,
                                   std::function<bool(const xAOD::IParticle*)> signalFunc, int MaxN) {
        ConstructInvariantMomenta(Leptons, Leptons.end(), Constructor, signalFunc, MaxN);
    }
    void ConstructInvariantMomenta(xAOD::IParticleContainer* Leptons, XAMPP::IReconstructedParticles* Constructor,
                                   std::function<bool(const xAOD::IParticle*)> signalFunc, int MaxN) {
        ConstructInvariantMomenta(*Leptons, Constructor, signalFunc, MaxN);
    }
    int Index(const xAOD::IParticle* P) {
        static IntAccessor acc_Index("Index");
        if (acc_Index.isAvailable(*P)) return acc_Index(*P);
        return GetNthPrime(P->index());
    }
    void AddFourMomenta(const xAOD::IParticle* P, const xAOD::IParticle* P1, xAOD::Particle* Candidate,
                        std::function<bool(const xAOD::IParticle*)> signalFunc) {
        static CharDecorator acc_IsSF("SameFlavour");
        static CharDecorator dec_IsSF("SameFlavour");

        static IntAccessor acc_NLep("nConstituents");
        static IntDecorator dec_NLep("nConstituents");

        static IntDecorator dec_Index("Index");
        static CharDecorator dec_signal("signal");
        static SG::AuxElement::Decorator<const xAOD::IParticle*> dec_Ptr1("BuildPtr1");
        static SG::AuxElement::Decorator<const xAOD::IParticle*> dec_Ptr2("BuildPtr2");

        TLorentzVector V = P->p4() + P1->p4();
        Candidate->setPxPyPzE(V.Px(), V.Py(), V.Pz(), V.E());
        float Q = Charge(P) + Charge(P1);
        Candidate->setCharge(Q);
        dec_signal(*Candidate) = signalFunc(P) * signalFunc(P1);
        dec_Index(*Candidate) = Index(P) * Index(P1);

        dec_Ptr1(*Candidate) = P;
        dec_Ptr2(*Candidate) = P1;

        Candidate->setPdgId(TypeToPdgId(P) * TypeToPdgId(P1));
        // That is just a dummy value
        if (P->type() != xAOD::Type::ObjectType::Particle && P1->type() != xAOD::Type::ObjectType::Particle) {
            dec_IsSF(*Candidate) = SameFlavour(*P, *P1);
            dec_NLep(*Candidate) = 2;
        } else if ((P->type() == xAOD::Type::ObjectType::Particle && P1->type() != xAOD::Type::ObjectType::Particle) ||
                   (P->type() != xAOD::Type::ObjectType::Particle && P1->type() == xAOD::Type::ObjectType::Particle)) {
            dec_NLep(*Candidate) = (P->type() == xAOD::Type::ObjectType::Particle ? acc_NLep(*P) : acc_NLep(*P1)) + 1;
            dec_IsSF(*Candidate) = (P->type() == xAOD::Type::ObjectType::Particle ? acc_IsSF(*P) && TypeToPdgId(P) % TypeToPdgId(P1) == 0
                                                                                  : acc_IsSF(*P1) && TypeToPdgId(P1) % TypeToPdgId(P) == 0);
        }
    }
    void ConstructInvariantMomenta(xAOD::IParticleContainer& Leptons, xAOD::IParticleContainer::const_iterator end,
                                   XAMPP::IReconstructedParticles* Constructor, std::function<bool(const xAOD::IParticle*)> signalFunc,
                                   int MaxN, const xAOD::IParticle* In) {
        static IntAccessor acc_NLep("nConstituents");

        static UIntDecorator dec_bitmask("containerMask");
        static UIntAccessor acc_bitmask("containerMask");

        for (xAOD::IParticleContainer::const_iterator L = Leptons.begin(); L != end; ++L) {
            if (!In) {
                ConstructInvariantMomenta(Leptons, L, Constructor, signalFunc, MaxN, *L);
            } else {
                // The end iterator represents the last particle added to the list of invariant particles
                // The position of L is the new particle going to be added next to the container
                unsigned int bitmask = 1 << (end - Leptons.begin()) | 1 << (L - Leptons.begin());
                // In case the input particle is already a combined one. The bitmask is set.
                if (acc_bitmask.isAvailable(*In)) bitmask |= acc_bitmask(*In);
                xAOD::Particle* InvLep = Constructor->CreateEmptyParticle();
                AddFourMomenta(*L, In, InvLep, signalFunc);
                dec_bitmask(*InvLep) = bitmask;
                if (MaxN < 0 || acc_NLep(*InvLep) < MaxN) ConstructInvariantMomenta(Leptons, L, Constructor, signalFunc, MaxN, InvLep);
            }
        }
    }
    unsigned int GetNthPrime(unsigned int N) {
        static std::vector<unsigned int> Primes{2, 3, 5};
        unsigned int TestPrime = Primes.back() + 2;
        while (N >= Primes.size()) {
            bool IsPrime = true;
            // Binomial (N,K) == Binomial(N, N-K) -> K <= N-K
            // Prime proof based on AKP
            // (x-1)^{p} - x^{p} + 1 -> sum {k=0}^{p-1}
            // Binomial(P,k)-1^{k}x^{p-k}
            //            for (unsigned int K=1; K <= TestPrime-K; ++K){
            //               unsigned int B = TMath::Binomial(TestPrime,K);
            //               if (B % TestPrime !=0){
            //                   IsPrime = false;
            //                   break;
            //               }
            //            }
            for (auto& P : Primes) {
                if (TestPrime % P == 0) {
                    IsPrime = false;
                    break;
                }
            }
            if (IsPrime) { Primes.push_back(TestPrime); }
            // The number has to be odd so the next one is even -> never prime
            TestPrime = TestPrime + 2;
        }
        return Primes.at(N);
    }
    bool HasCommonConstructingParticles(const xAOD::IParticle* First, const xAOD::IParticle* Second) {
        if (IsSame(First, Second)) return true;
        static SG::AuxElement::Accessor<const xAOD::IParticle*> acc_Ptr1("BuildPtr1");
        static SG::AuxElement::Accessor<const xAOD::IParticle*> acc_Ptr2("BuildPtr2");

        if (First->type() == xAOD::Type::ObjectType::Particle || Second->type() == xAOD::Type::ObjectType::Particle) {
            // The first particle must be a reconstructed particle
            if (Second->type() != xAOD::Type::ObjectType::Particle) {
                return (acc_Ptr1(*First) == Second || acc_Ptr2(*First) == Second);
            }
            // In that case it is the second
            else if (First->type() != xAOD::Type::ObjectType::Particle) {
                return HasCommonConstructingParticles(Second, First);
            }
            if (HasCommonConstructingParticles(acc_Ptr1(*First), acc_Ptr1(*Second))) return true;
            if (HasCommonConstructingParticles(acc_Ptr1(*First), acc_Ptr2(*Second))) return true;
            if (HasCommonConstructingParticles(acc_Ptr2(*First), acc_Ptr1(*Second))) return true;
            if (HasCommonConstructingParticles(acc_Ptr2(*First), acc_Ptr2(*Second))) return true;
        }
        return false;
    }
    void ConstructInvariantMomenta(xAOD::IParticleContainer* Leptons, asg::AnaToolHandle<XAMPP::IReconstructedParticles>& Constructor,
                                   std::function<bool(const xAOD::IParticle*)> signalFunc, int MaxN) {
        ConstructInvariantMomenta(Leptons, Constructor.operator->(), signalFunc, MaxN);
    }
    void ConstructInvariantMomenta(xAOD::IParticleContainer& Leptons, asg::AnaToolHandle<XAMPP::IReconstructedParticles>& Constructor,
                                   std::function<bool(const xAOD::IParticle*)> signalFunc, int MaxN) {
        ConstructInvariantMomenta(Leptons, Constructor.operator->(), signalFunc, MaxN);
    }
    void ConstructInvariantMomenta(xAOD::IParticleContainer& Leptons, xAOD::IParticleContainer::const_iterator Itr,
                                   asg::AnaToolHandle<XAMPP::IReconstructedParticles>& Constructor,
                                   std::function<bool(const xAOD::IParticle*)> signalFunc, int MaxN, const xAOD::IParticle* In) {
        ConstructInvariantMomenta(Leptons, Itr, Constructor.operator->(), signalFunc, MaxN, In);
    }
    void CalculateMt(xAOD::IParticleContainer& Particles, XAMPP::Storage<xAOD::MissingET*>* dec_MET) { CalculateMt(&Particles, dec_MET); }
    void CalculateMt(const xAOD::IParticleContainer* Particles, XAMPP::Storage<xAOD::MissingET*>* dec_MET) {
        static FloatDecorator dec_MT("MT");
        for (const auto& P : *Particles) dec_MT(*P) = ComputeMt(P, dec_MET);
    }

    bool IsInContainer(const xAOD::IParticle* P, const xAOD::IParticleContainer* Container) { return IsInContainer(P, *Container); }
    bool IsInContainer(const xAOD::IParticle& P, const xAOD::IParticleContainer* Container) { return IsInContainer(&P, Container); }
    bool IsInContainer(const xAOD::IParticle& P, const xAOD::IParticleContainer& Container) { return IsInContainer(&P, Container); }
    bool IsInContainer(const xAOD::IParticle* P, const xAOD::IParticleContainer& Container) {
        if (Container.empty()) return false;
        for (auto in_container : Container) {
            if (IsSame(in_container, P, true)) return true;
        }
        return false;
    }

    bool isConstituent(const xAOD::IParticle* C, const xAOD::Particle* DiPart) {
        static SG::AuxElement::Accessor<const xAOD::IParticle*> acc_Ptr1("BuildPtr1");
        static SG::AuxElement::Accessor<const xAOD::IParticle*> acc_Ptr2("BuildPtr2");
        if (DiPart == nullptr) {
            Warning("isConstituent()", "No particle given");
            return false;
        }
        if (acc_Ptr1(*DiPart) == C) return true;
        if (acc_Ptr2(*DiPart) == C) return true;
        return false;
    }

}  // namespace XAMPP
