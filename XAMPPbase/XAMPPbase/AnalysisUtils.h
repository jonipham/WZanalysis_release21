#ifndef XAMPPbase_AnalysisUtils_H
#define XAMPPbase_AnalysisUtils_H

#include <xAODBase/IParticleHelpers.h>
#include <xAODParticleEvent/Particle.h>

#include <xAODEgamma/Electron.h>
#include <xAODEgamma/ElectronContainer.h>
#include <xAODEgamma/Photon.h>
#include <xAODEgamma/PhotonContainer.h>
#include <xAODMuon/Muon.h>
#include <xAODMuon/MuonContainer.h>
#include <xAODTau/TauJet.h>
#include <xAODTau/TauJetContainer.h>

#include <xAODTruth/TruthParticle.h>
#include <xAODTruth/TruthParticleContainer.h>

#include <xAODTruth/TruthVertex.h>
#include <xAODTruth/TruthVertexContainer.h>

#include <xAODBTagging/BTagging.h>
#include <xAODJet/Jet.h>
#include <xAODJet/JetContainer.h>

#include <xAODMissingET/MissingET.h>
#include <xAODMissingET/MissingETContainer.h>

#include <TMath.h>
#include <XAMPPbase/Defs.h>
#include <XAMPPbase/EventStorage.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <vector>
#include "FourMomUtils/xAODP4Helpers.h"
#include "PATInterfaces/SystematicSet.h"
#include "TLorentzVector.h"

#include <AsgTools/AnaToolHandle.h>
#include <AsgTools/AsgTool.h>
#include <AsgTools/ToolHandle.h>

namespace XAMPP {
    class EventInfo;
    class IReconstructedParticles;

    double Sign(const double& N);
    template <typename T> int max_bit(const T& number) {
        for (int bit = sizeof(number) * 8 - 1; bit >= 0; --bit) {
            if (number & (1 << bit)) return bit;
        }
        return -1;
    }

    bool ptsorter(const xAOD::IParticle* j1, const xAOD::IParticle* j2);
    bool btagweightsorter(const xAOD::Jet* j1, const xAOD::Jet* j2);
    std::vector<std::string> GetPathResolvedFileList(const std::vector<std::string>& Files);
    std::vector<std::string> ListDirectory(std::string Path, std::string ElementToContain = "", std::string RegExpToMatch = "");

    StatusCode ResetOverlapDecorations(const xAOD::IParticleContainer* Container);
    StatusCode RemoveOverlap(xAOD::IParticleContainer* RemFrom, xAOD::IParticleContainer* RemWith, float dR, bool UseRapidity = true);
    StatusCode RemoveOverlap(xAOD::IParticleContainer* RemFrom, xAOD::IParticleContainer* RemWith,
                             std::function<float(const xAOD::IParticle*, const xAOD::IParticle*)> radiusFunc, bool UseRapidity = true);

    /**
     * @brief      Calculates the symmetric mt2 out of 2 particles and the
     * missing transverse energy
     *
     * @param[in]  P1            First particle
     * @param[in]  P2            Second particle
     * @param      met           Missing transverse energy
     * @param[in]  InvMass       Assumed mass of the invisible particle
     * @param[in]  ParticleMass  Overwrites the 'real' mass of P1 and P2 if
     * needed
     *
     * @return     Symmetric mt2
     */
    float CalculateMT2(const xAOD::IParticle* P1, const xAOD::IParticle* P2, XAMPP::Storage<XAMPPmet>* met, float InvMass = 0,
                       float ParticleMass = -1);
    float CalculateMT2(const xAOD::IParticle* P1, const xAOD::IParticle* P2, const xAOD::MissingET* met, float InvMass = 0,
                       float ParticleMass = -1);
    float CalculateMT2(const xAOD::IParticle& P1, const xAOD::IParticle& P2, XAMPP::Storage<XAMPPmet>* met, float InvMass = 0,
                       float ParticleMass = -1);
    float CalculateMT2(const xAOD::IParticle& P1, const xAOD::IParticle& P2, const xAOD::MissingET* met, float InvMass = 0,
                       float ParticleMass = -1);

    /**
     * @brief      Calculates the asymmetric mt2 out of 2 particles and the
     * missing transverse energy assuming an asymmetric mass distribution among
     * the invisible particles
     *
     * @param[in]  P1            First particle
     * @param[in]  P2            Second particle
     * @param      met           Missing transverse energy
     * @param[in]  InvMass1      Assumed mass of the first invisible particle
     * @param[in]  InvMass2      Assumed mass of the second invisible particle
     * @param[in]  ParticleMass  Overwrites the 'real' mass of P1 and P2 if
     * needed
     *
     * @return     Asymmetric mt2
     */
    float CalculateAMT2(const xAOD::IParticle* P1, const xAOD::IParticle* P2, XAMPP::Storage<XAMPPmet>* met, float InvMass1 = 0,
                        float InvMass2 = 0, float ParticleMass = -1);

    float CalculateAMT2(const xAOD::IParticle* P1, const xAOD::IParticle* P2, const xAOD::MissingET* met, float InvMass1 = 0,
                        float InvMass2 = 0, float ParticleMass = -1);
    float CalculateAMT2(const xAOD::IParticle& P1, const xAOD::IParticle& P2, XAMPP::Storage<XAMPPmet>* met, float InvMass1 = 0,
                        float InvMass2 = 0, float ParticleMass = -1);
    float CalculateAMT2(const xAOD::IParticle& P1, const xAOD::IParticle& P2, const xAOD::MissingET* met, float InvMass1 = 0,
                        float InvMass2 = 0, float ParticleMass = -1);

    Momentum FourMomentum(const std::string& str);
    std::string FourMomentum(Momentum Mom);

    template <typename T> bool ToolIsAffectedBySystematic(const ToolHandle<T>& Handle, const CP::SystematicSet* set) {
        if (set->name().empty()) { return true; }
        for (const auto& sys : *set) {
            if (Handle->isAffectedBySystematic(sys)) return true;
        }
        return false;
    }
    template <typename P> P GetProperty(const std::string& name, const asg::IAsgTool* IAsgTool) {
        const asg::AsgTool* tool = dynamic_cast<const asg::AsgTool*>(IAsgTool);
        if (!tool) return P();
        return (*tool->getProperty<P>(name));
    }

    template <typename P, typename T> P GetProperty(const std::string& name, const ToolHandle<T>& handle) {
        return GetProperty<P>(name, handle.operator->());
    }

    template <typename P, typename T> P GetProperty(const std::string& name, const asg::AnaToolHandle<T>& handle) {
        return GetProperty<P>(name, handle.getHandle());
    }
    int Index(const xAOD::IParticle* P);
    unsigned int GetNthPrime(unsigned int N);

    void FillVectorFromString(std::vector<std::string>& str_vector, std::string& str);
    std::string EraseWhiteSpaces(std::string str);
    std::string ReplaceExpInString(std::string str, const std::string& exp, const std::string& rep);
    std::string ToLower(const std::string& str);
    bool GetLine(std::ifstream& inf, std::string& line);

    MSG::Level setOutputLevel(int m_output_level_int);
    xAOD::IParticle* FindLeadingParticle(xAOD::IParticleContainer* Particles);
    std::string RemoveAllExpInStr(std::string Str, std::string Exp);
    float GetLeadingPt(xAOD::IParticleContainer* Particles);

    bool Overlaps(const xAOD::IParticle& P1, const xAOD::IParticle& P2, float dR, bool UseRapidity = false);
    bool Overlaps(const xAOD::IParticle* P1, const xAOD::IParticle* P2, float dR, bool UseRapidity = false);

    bool IsRecoLepton(const xAOD::IParticle& L);
    bool IsRecoLepton(const xAOD::IParticle* L);

    bool OppositeSign(const xAOD::IParticle& L, const xAOD::IParticle& L1);
    bool OppositeSign(const xAOD::IParticle* L, const xAOD::IParticle* L1);
    bool OppositeSignLead2(const xAOD::IParticleContainer* pc);

    bool SameFlavour(const xAOD::IParticle& L, const xAOD::IParticle& L1);
    bool SameFlavour(const xAOD::IParticle* L, const xAOD::IParticle* L1);

    bool OppositeFlavour(const xAOD::IParticle& L, const xAOD::IParticle& L1);
    bool OppositeFlavour(const xAOD::IParticle* L, const xAOD::IParticle* L1);

    bool IsSame(const xAOD::IParticle& P, const xAOD::IParticle& P1, bool DeRefShallowCopy = false);
    bool IsSame(const xAOD::IParticle* P, const xAOD::IParticle* P1, bool DeRefShallowCopy = false);

    bool IsNotSame(const xAOD::IParticle& P, const xAOD::IParticle& P1);
    bool IsNotSame(const xAOD::IParticle* P, const xAOD::IParticle* P1);

    bool IsSFOS(const xAOD::IParticle& L, const xAOD::IParticle& L1);
    bool IsSFOS(const xAOD::IParticle* L, const xAOD::IParticle* L1);

    bool IsDFOS(const xAOD::IParticle& L, const xAOD::IParticle& L1);
    bool IsDFOS(const xAOD::IParticle* L, const xAOD::IParticle* L1);

    StatusCode RemoveLowMassLeptons(xAOD::IParticleContainer* Leptons, bool (*pair)(const xAOD::IParticle*, const xAOD::IParticle*),
                                    float Upper_Mll, float Lower_Mll = 0.);
    StatusCode RemoveLowMassLeptons(xAOD::IParticleContainer* Leptons, float Upper_Mll, float Lower_Mll = 0.);

    // This function calculates the invariant momenta of P1 and P2 storing it in
    // Candidate. In addition the charge is calculated. Information whether the
    // particle is formed from to same parts and of two IsSignal objects is also
    // stored
    void AddFourMomenta(const xAOD::IParticle* P, const xAOD::IParticle* P1, xAOD::Particle* Candidate,
                        std::function<bool(const xAOD::IParticle*)> signalFunc);

    // These functions loop through the IParticleContainers constructing all
    // possible invariant momenta...
    void ConstructInvariantMomenta(xAOD::IParticleContainer* Leptons, XAMPP::IReconstructedParticles* Constructor,
                                   std::function<bool(const xAOD::IParticle*)> signalFunc, int MaxN = -1);
    void ConstructInvariantMomenta(xAOD::IParticleContainer& Leptons, XAMPP::IReconstructedParticles* Constructor,
                                   std::function<bool(const xAOD::IParticle*)> signalFunc, int MaxN = -1);
    void ConstructInvariantMomenta(xAOD::IParticleContainer& Leptons, xAOD::IParticleContainer::const_iterator Itr,
                                   XAMPP::IReconstructedParticles* Constructor, std::function<bool(const xAOD::IParticle*)> signalFunc,
                                   int MaxN, const xAOD::IParticle* In = nullptr);
    // The entire things with an asg::AnaToolHandle as wrapper again
    void ConstructInvariantMomenta(xAOD::IParticleContainer* Leptons, asg::AnaToolHandle<XAMPP::IReconstructedParticles>& Constructor,
                                   std::function<bool(const xAOD::IParticle*)> signalFunc, int MaxN = -1);
    void ConstructInvariantMomenta(xAOD::IParticleContainer& Leptons, asg::AnaToolHandle<XAMPP::IReconstructedParticles>& Constructor,
                                   std::function<bool(const xAOD::IParticle*)> signalFunc, int MaxN = -1);
    void ConstructInvariantMomenta(xAOD::IParticleContainer& Leptons, xAOD::IParticleContainer::const_iterator Itr,
                                   asg::AnaToolHandle<XAMPP::IReconstructedParticles>& Constructor,
                                   std::function<bool(const xAOD::IParticle*)> signalFunc, int MaxN, const xAOD::IParticle* In = nullptr);

    float Charge(const xAOD::IParticle* p);
    float Charge(const xAOD::IParticle& p);

    // Returns the pdgIds of the Particle
    int TypeToPdgId(const xAOD::IParticle* P);
    int TypeToPdgId(const xAOD::IParticle& P);

    // Computes the Mt between two particles or one particle and the MissingEt
    float ComputeMt(const xAOD::IParticle* v1, XAMPP::Storage<XAMPPmet>* met);
    float ComputeMt(const xAOD::IParticle& v1, XAMPP::Storage<XAMPPmet>* met);

    float ComputeMt(const xAOD::IParticle* v1, const xAOD::MissingET* met);
    float ComputeMt(const xAOD::IParticle& v1, const xAOD::MissingET& met);

    float ComputeMt(const xAOD::IParticle* v1, const xAOD::IParticle* v2);
    float ComputeMt(const xAOD::IParticle& v1, const xAOD::IParticle& v2);

    void CalculateMt(xAOD::IParticleContainer& Particles, XAMPP::Storage<xAOD::MissingET*>* dec_MET);
    void CalculateMt(const xAOD::IParticleContainer* Particles, XAMPP::Storage<xAOD::MissingET*>* dec_MET);

    float RelPt(const xAOD::IParticle* v1, const xAOD::IParticle* v2);
    float RelPt(const xAOD::IParticle& v1, const xAOD::IParticle& v2);

    // Computes the invariant mass of two to four particles
    float InvariantMass(const xAOD::IParticle& P1, const xAOD::IParticle& P2);
    float InvariantMass(const xAOD::IParticle* P1, const xAOD::IParticle* P2);
    float InvariantMass(const xAOD::IParticle& P1, const xAOD::IParticle& P2, const xAOD::IParticle& P3);
    float InvariantMass(const xAOD::IParticle* P1, const xAOD::IParticle* P2, const xAOD::IParticle* P3);
    float InvariantMass(const xAOD::IParticle& P1, const xAOD::IParticle& P2, const xAOD::IParticle& P3, const xAOD::IParticle& P4);
    float InvariantMass(const xAOD::IParticle* P1, const xAOD::IParticle* P2, const xAOD::IParticle* P3, const xAOD::IParticle* P4);

    int GetZVeto(xAOD::IParticleContainer& Particles, float Z_Window);
    int GetZVeto(xAOD::IParticleContainer* Particles, float Z_Window);
    int GetZVeto(xAOD::IParticleContainer* Electrons, xAOD::IParticleContainer* Muons, float Z_Window = 1.e4);

    float CalculateHt(xAOD::IParticleContainer* Container, float MinPt = 0.);

    float CalculateLeptonHt(xAOD::ElectronContainer* Electrons, xAOD::MuonContainer* Muons, xAOD::TauJetContainer* Taus = nullptr);

    const xAOD::IParticle* GetClosestParticle(xAOD::IParticleContainer* Cont, XAMPP::Storage<XAMPPmet>* met,
                                              xAOD::IParticleContainer* Exclude = nullptr);
    const xAOD::IParticle* GetClosestParticle(xAOD::IParticleContainer* Cont, xAOD::MissingET* met,
                                              xAOD::IParticleContainer* Exclude = nullptr);

    const xAOD::IParticle* GetFarestParticle(xAOD::IParticleContainer* Cont, XAMPP::Storage<XAMPPmet>* met,
                                             xAOD::IParticleContainer* Exclude = nullptr);
    const xAOD::IParticle* GetFarestParticle(xAOD::IParticleContainer* Cont, xAOD::MissingET* met,
                                             xAOD::IParticleContainer* Exclude = nullptr);

    const xAOD::IParticle* GetClosestParticle(xAOD::IParticleContainer* Cont, const xAOD::IParticle* P1,
                                              xAOD::IParticleContainer* Exclude = nullptr);
    const xAOD::IParticle* GetFarestParticle(xAOD::IParticleContainer* Cont, const xAOD::IParticle* P1,
                                             xAOD::IParticleContainer* Exclude = nullptr);

    const xAOD::IParticle* GetClosestParticle(const xAOD::IParticleContainer* LookIn, const xAOD::IParticle* RefPart,
                                              bool UseRapidity = false, const xAOD::IParticleContainer* RefContainer = nullptr);

    float ComputeMtMin(xAOD::IParticleContainer* Collection, XAMPP::Storage<XAMPPmet>* met);
    float ComputeMtMin(xAOD::IParticleContainer* Collection, xAOD::MissingET* met);
    float ComputeMtMin(xAOD::IParticleContainer* Collection, const xAOD::IParticle* P1);
    float ComputeMtMax(xAOD::IParticleContainer* Collection, XAMPP::Storage<XAMPPmet>* met);
    float ComputeMtMax(xAOD::IParticleContainer* Collection, xAOD::MissingET* met);
    float ComputeMtMax(xAOD::IParticleContainer* Collection, const xAOD::IParticle* P1);
    float ComputeDPhiMin(xAOD::JetContainer* Jets, XAMPP::Storage<XAMPPmet>* met, unsigned int NJetsToUse = 0);
    float ComputeDPhiMin(xAOD::JetContainer* Jets, xAOD::MissingET* met, unsigned int NJetsToUse = 0);
    bool HasTauCandidate(xAOD::JetContainer* Jets, xAOD::MissingET* met, unsigned int primVtxIdx, float& MtTauCand, int& Ntracks);
    float ComputeMinMt(xAOD::JetContainer* Jets, xAOD::MissingET* met);

    float ComputeJetAngularSep(xAOD::JetContainer* Jets);

    float ComputeAbsDeltaPhi(xAOD::MissingET* met1, xAOD::MissingET* met2);
    float ComputeDeltaPhi(xAOD::MissingET* met1, xAOD::MissingET* met2);

    float ComputeDeltaPhi(XAMPP::Storage<XAMPPmet>* met1, XAMPP::Storage<XAMPPmet>* met2);
    float ComputeAbsDeltaPhi(XAMPP::Storage<XAMPPmet>* met1, XAMPP::Storage<XAMPPmet>* met2);

    bool RecoZfromLeps(xAOD::IParticleContainer* Lep, xAOD::Particle* RecoZ, float MassWindow = 1.e4);

    void GetTopCandidates(xAOD::JetContainer* BJets, xAOD::JetContainer* LightJets, xAOD::Particle* W1Cand, xAOD::Particle* W2Cand,
                          xAOD::Particle* Top1Cand, xAOD::Particle* Top2Cand);

    void GetTopCandidatesDRB4(xAOD::JetContainer* BJets, xAOD::JetContainer* LightJets, xAOD::Particle* W1CandDRB4,
                              xAOD::Particle* W2CandDRB4, xAOD::Particle* Top1CandDRB4, xAOD::Particle* Top2CandDRB4);

    void GetTopCandidatesMinMass(xAOD::JetContainer* BJets, xAOD::JetContainer* LightJets, xAOD::Particle* W1CandMinMass,
                                 xAOD::Particle* W2CandMinMass, xAOD::Particle* Top1CandMinMass, xAOD::Particle* Top2CandMinMass);

    bool IsBJet(const xAOD::IParticle* P);

    float GetChi2FromTopConstruction(float WMass, float TopMass);

    bool isConstituent(const xAOD::IParticle* C, const xAOD::Particle* DiPart);

    bool HasCommonConstructingParticles(const xAOD::IParticle* First, const xAOD::IParticle* Second);

    xAOD::Particle* GetTop(const xAOD::IParticle* W, const xAOD::Jet* B, asg::AnaToolHandle<XAMPP::IReconstructedParticles>& Constructor);
    xAOD::Particle* GetTop(const xAOD::IParticle* W, const xAOD::Jet* B, XAMPP::IReconstructedParticles* Constructor);

    StatusCode GetTopCandidatesChi2(xAOD::JetContainer* SignalJets, const xAOD::JetContainer* BJets,
                                    asg::AnaToolHandle<XAMPP::IReconstructedParticles>& Constructor);
    StatusCode GetTopCandidatesChi2(xAOD::JetContainer* SignalJets, const xAOD::JetContainer* BJets,
                                    XAMPP::IReconstructedParticles* Constructor);

    xAOD::MissingET* GetMET_obj(const std::string& Name, xAOD::MissingETContainer* Cont);

    void PromptParticle(const xAOD::IParticle* Part, std::string AddInfo = "");
    void PromptParticle(const xAOD::IParticle& Part, std::string AddInfo = "");

    bool IsInContainer(const xAOD::IParticle* P, const xAOD::IParticleContainer* Container);
    bool IsInContainer(const xAOD::IParticle& P, const xAOD::IParticleContainer* Container);
    bool IsInContainer(const xAOD::IParticle* P, const xAOD::IParticleContainer& Container);
    bool IsInContainer(const xAOD::IParticle& P, const xAOD::IParticleContainer& Container);

    template <typename T> bool IsInVector(const T& Ele, const std::vector<T>& Vec) {
        for (auto& In : Vec) {
            if (Ele == In) return true;
        }
        return false;
    }
    template <typename T> void CopyVector(const std::vector<T>& From, std::vector<T>& To, bool Clear = true) {
        if (Clear) To.clear();
        if (To.capacity() < From.size()) To.reserve(From.size() + To.capacity());
        for (auto& Ele : From) {
            if (!IsInVector(Ele, To)) To.push_back(Ele);
        }
        To.shrink_to_fit();
    }
    template <typename T> void ClearFromDuplicates(std::vector<T>& toClear) {
        std::vector<T> free;
        CopyVector(toClear, free);
        CopyVector(free, toClear);
    }
    template <typename T> void EraseFromVector(std::vector<T>& vec, std::function<bool(const T&)> func) {
        typename std::vector<T>::iterator itr = std::find_if(vec.begin(), vec.end(), func);
        while (itr != vec.end()) {
            vec.erase(itr);
            itr = std::find_if(vec.begin(), vec.end(), func);
        }
    }
    template <class T> std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
        os << "[";
        for (const auto& i : v) { os << " " << i; }
        os << " ]";
        return os;
    }

    bool IsChargedFlipped(const xAOD::Electron* el);
    // Truth particle functions
    bool isSparticle(const xAOD::TruthParticle& P);
    bool isSparticle(const xAOD::TruthParticle* P);

    bool isGluon(const xAOD::TruthParticle& P);
    bool isGluon(const xAOD::TruthParticle* P);

    bool isEWboson(const xAOD::TruthParticle& P);
    bool isEWboson(const xAOD::TruthParticle* P);

    const xAOD::TruthParticle* getTruthMatchedParticle(const xAOD::IParticle& P);
    const xAOD::TruthParticle* getTruthMatchedParticle(const xAOD::IParticle* P);

    int getParticleTruthOrigin(const xAOD::IParticle& P);
    int getParticleTruthOrigin(const xAOD::IParticle* P);

    int getParticleTruthType(const xAOD::IParticle& P);
    int getParticleTruthType(const xAOD::IParticle* P);

    const xAOD::TruthParticle* GetFirstChainLink(const xAOD::TruthParticle* TruthPart);
    const xAOD::TruthParticle* GetLastChainLink(const xAOD::TruthParticle* TruthPart);
    bool isParticleFromHardProcess(const xAOD::TruthParticle* TruthPart, bool rejectUnknownOrigin = false);

    bool IsInOutGoing(const xAOD::TruthParticle* P);

    float MinDeltaR(const xAOD::IParticle* p, const xAOD::IParticleContainer* pc);

}  // namespace XAMPP
#endif
