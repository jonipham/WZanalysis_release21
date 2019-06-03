#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/Cuts.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/EventStorage.h>

namespace XAMPP {
    //########################################################################################
    //                          ICutElement
    //########################################################################################

    ICutElement::ICutElement(XAMPP::IStorage* Store, XAMPP::IEventInfo* Info) : m_Store(Store), m_Info(Info) {}
    std::string ICutElement::name() const { return m_Store ? m_Store->name() : "Not defined"; }
    unsigned int ICutElement::EventNumber() const { return m_Info->GetOrigInfo()->eventNumber(); }

    //########################################################################################
    //                          Cut
    //########################################################################################
    Cut::Cut() : Cut(nullptr, "", CutType::CutInDef, false) {}
    std::string Cut::RelToString(Relation R) {
        if (R == Relation::NoRel) return "NoRel";
        if (R == Relation::Greater) return ">";
        if (R == Relation::Equal) return "=";
        if (R == Relation::Less) return "<";
        if (R == Relation::GreaterEqual) return ">=";
        if (R == Relation::LessEqual) return "<=";
        return "";
    }
    Cut::Cut(XAMPP::EventInfo* Info, const std::string& N, CutType T, bool Real) :
        m_Name(N),
        m_Type(T),
        m_IsRealCut(Real),
        m_Debug(false),
        m_CutElement(),
        m_XAMPPInfo(Info),
        m_Init(false),
        m_Connected() {
        StorageKeeper::GetInstance()->AttachCut(this);
    }
    Cut::Cut(const std::string& N, CutType T, bool Real) : Cut(nullptr, N, T, Real) {}
    void Cut::SetName(const std::string& N) {
        if (!N.empty()) m_Name = N;
    }
    void Cut::Debug(bool B) {
        Info("Cut::Debug()", "Setting debug mode to %u for %s", (int)B, GetName().c_str());
        m_Debug = B;
    }

    Cut::~Cut() {
        StorageKeeper::GetInstance()->DettachCut(this);
        for (auto& C : m_Connected) {
            if (C) C->Disconnect(this);
        }
    }
    Cut* Cut::combine(Cut* cut, Combine C) {
        if (C == Combine::inDef) {
            std::cout << "Cut::combine() -- ERROR: " << GetName() << " Only Combine::AND or Combine::OR are allowed as options"
                      << std::endl;
            return nullptr;
        }
        if (cut == this) {
            std::cout << "Cut::combine() -- ERROR: " << GetName() << " You are trying to combine a cut with itself... Not clever"
                      << std::endl;
            return nullptr;
        }
        if (cut == nullptr) {
            std::cout << "Cut::combine() -- ERROR: " << GetName() << " The cut to combine is given as a nullptr pointer." << std::endl;
            return nullptr;
        }
        if (C == Combine::AND) return new CombCutAND(this, cut);
        return new CombCutOR(this, cut);
    }
    Cut::Relation Cut::GetCondition(std::string& Cond) {
        Relation C = Relation::NoRel;
        Cond.erase(std::remove_if(Cond.begin(), Cond.end(), isspace), Cond.end());
        if (Cond.find(">=") < Cond.size())
            C = Relation::GreaterEqual;
        else if (Cond.find("<=") < Cond.size())
            C = Relation::LessEqual;
        else if (Cond.find("<") < Cond.size())
            C = Relation::Less;
        else if (Cond.find(">") < Cond.size())
            C = Relation::Greater;
        else if (Cond.find("=") < Cond.size())
            C = Relation::Equal;
        Cond.erase(std::remove(Cond.begin(), Cond.end(), '<'), Cond.end());
        Cond.erase(std::remove(Cond.begin(), Cond.end(), '='), Cond.end());
        Cond.erase(std::remove(Cond.begin(), Cond.end(), '>'), Cond.end());
        return C;
    }

    bool Cut::initialize(std::string VarName, std::string Cond) {
        if (m_Type == CutType::CutInDef) {
            Error("Cut::initialize()", "The CuType is not defined for %s", GetName().c_str());
            return false;
        }
        if (!m_XAMPPInfo) {
            Error("Cut::initialize()", "Could not retrieve the XAMPPInfo");
            return false;
        }
        Relation C = GetCondition(Cond);
        if (m_Type == CutPartInt || m_Type == CutPartFloat || m_Type == CutPartChar || m_Type == CutPartBool) {
            if (VarName.find(" ") == std::string::npos) {
                Error("Cut::initialize()",
                      "The variable %s does not contain any space. Please give in the format <Container> <Variable>[Idx] ",
                      VarName.c_str());
                return false;
            }
            int Idx = -1;
            std::string Container = VarName.substr(0, VarName.find(" "));

            VarName = VarName.substr(VarName.find(" ") + 1, VarName.size());
            if (VarName.find("[") != std::string::npos) {
                std::string Index = VarName.substr(VarName.rfind("[") + 1, VarName.size());
                Index = Index.substr(0, Index.size() - 1);
                Idx = atoi(Index.c_str());
                VarName = VarName.substr(0, VarName.find("["));
            }
            if (Idx == -1) {
                Error("Cut::initialize()", "The variable %s is invalid. Please give in the format <Container> <Variable>[Idx] ",
                      VarName.c_str());
                return false;
            }
            Momentum Mom = FourMomentum(VarName);
            if (Mom) {
                if (m_Type != CutPartFloat)
                    Warning("Cut::initialize()",
                            "The Cut is not of 'CutPartFloat'. But you want to cut "
                            "on p4()");
                return ParticleInit(Container, Mom, Idx, atof(Cond.c_str()), C);
            }
            if (m_Type == CutPartInt)
                return ParticleInit<int>(Container, VarName, Idx, atoi(Cond.c_str()), C);
            else if (m_Type == CutPartChar)
                return ParticleInit<char>(Container, VarName, Idx, atoi(Cond.c_str()), C);
            else if (m_Type == CutPartBool) {
                bool Boolean = Cond.find("true") != std::string::npos;
                if (!Boolean && Cond.find("false") == std::string::npos) {
                    Error("Cut::initialize()",
                          "Could not interpret what you've given as cutting "
                          "condition %s. Please type either 'false' or 'true' for a boolean cut",
                          Cond.c_str());
                    return false;
                }
                return ParticleInit<bool>(Container, VarName, Idx, Boolean, C);
            }
            return ParticleInit<float>(Container, VarName, Idx, atof(Cond.c_str()), C);
        }
        if (m_Type == CutXAMPPmet)
            return initialize(m_XAMPPInfo->GetVariableStorage<xAOD::MissingET*>(VarName), atof(Cond.c_str()), C);
        else if (m_Type == CutType::CutInt || m_Type == CutType::CutChar) {
            int Value = atoi(Cond.c_str());
            if (m_Type == CutType::CutInt)
                return initialize(m_XAMPPInfo->GetVariableStorage<int>(VarName), Value, C);
            else
                return Cut::initialize(m_XAMPPInfo->GetVariableStorage<char>(VarName), Value, C);
        } else if (m_Type == CutType::CutBool) {
            bool Boolean = Cond.find("true") != std::string::npos;
            if (!Boolean && Cond.find("false") == std::string::npos) {
                Error("Cut::initialize()",
                      "Could not interpret what you've given as cutting "
                      "condition %s. Please type either 'false' or 'true' for a boolean cut",
                      Cond.c_str());
                return false;
            }
            return initialize(m_XAMPPInfo->GetVariableStorage<bool>(VarName), Boolean, C);
        }
        float FloatValue = atof(Cond.c_str());
        return initialize(m_XAMPPInfo->GetVariableStorage<float>(VarName), FloatValue, C);
    }
    bool Cut::IsInitialized() const {
        if (!m_Init) Error("Cut::IsInitialized()", "%s has not been initialized ", GetName().c_str());
        return m_Init;
    }
    unsigned int Cut::EventNumber() const { return m_XAMPPInfo->GetOrigInfo()->eventNumber(); }
    bool Cut::ApplyCut(bool ForCutFlow) {
        bool Pass = m_CutElement->ApplyCut(ForCutFlow);
        if (Pass && m_Debug) { Info("Cut::ApplyCut()", "%s passed in event %u", GetName().c_str(), EventNumber()); }
        return Pass;
    }
    std::string Cut::GetName() const { return m_Name.empty() ? (m_CutElement ? m_CutElement->name() : "") : m_Name; }
    void Cut::Disconnect(Cut* Other) {
        if (m_Connected.empty() || Other == this) return;
        for (auto& C : m_Connected) {
            if (C == Other) C = nullptr;
        }
    }
    void Cut::Connect(Cut* C) {
        if (C != this && !IsInVector(C, m_Connected)) { m_Connected.push_back(C); }
    }
    bool Cut::ParticleInit(const std::string& Container, Momentum Mom, unsigned int Nth, float CutValue, Cut::Relation Rel) {
        ParticleFourVectorCutElement* Cut = new ParticleFourVectorCutElement(m_XAMPPInfo, Container, Nth, Mom);
        m_CutElement = std::unique_ptr<XAMPP::ICutElement>(Cut);
        m_Init = true;
        return Cut->initialize(CutValue, Rel, m_IsRealCut);
    }

    //########################################################################################
    //                          CompositeCut
    //########################################################################################
    CompositeCut::CompositeCut(Cut* C1, Cut* C2) : Cut(), m_Cut1(C1), m_Cut2(C2) {
        C1->Connect(this);
        C2->Connect(this);
    }
    CompositeCut::~CompositeCut() {
        if (m_Cut1) delete m_Cut1;
        if (m_Cut2) delete m_Cut2;
    }
    void CompositeCut::Disconnect(Cut* C) {
        Cut::Disconnect(C);
        if (C == m_Cut1) m_Cut1 = nullptr;
        if (C == m_Cut2) m_Cut2 = nullptr;
    }
    bool CompositeCut::IsInitialized() const { return m_Cut1->IsInitialized() && m_Cut2->IsInitialized(); }

    //########################################################################################
    //                          CombCutAND
    //########################################################################################
    CombCutAND::CombCutAND(Cut* C1, Cut* C2) : CompositeCut(C1, C2) {}
    bool CombCutAND::ApplyCut(bool ForCutFlow) { return m_Cut1->ApplyCut(ForCutFlow) && m_Cut2->ApplyCut(ForCutFlow); }
    std::string CombCutAND::GetName() const {
        if (m_Name.size()) return m_Name;
        return "(" + m_Cut1->GetName() + " && " + m_Cut2->GetName() + ")";
    }
    //########################################################################################
    //                          CombCutOR
    //########################################################################################

    CombCutOR::CombCutOR(Cut* C1, Cut* C2) : CompositeCut(C1, C2) {}
    bool CombCutOR::ApplyCut(bool ForCutFlow) { return m_Cut1->ApplyCut(ForCutFlow) || m_Cut2->ApplyCut(ForCutFlow); }
    std::string CombCutOR::GetName() const {
        if (m_Name.size()) return m_Name;
        return "(" + m_Cut1->GetName() + " || " + m_Cut2->GetName() + ")";
    }

    //########################################################################################
    //                          XAMPPMetCutElement
    //########################################################################################
    bool XAMPPMetCutElement::ApplyCut(bool ForCutFlow) const {
        if (!IsValid(GetMet())) {
            Error("XAMPPMetCutElement::ApplyCut()", "The variable %s is not a number in event %u", name().c_str(), EventNumber());
            return false;
        }
        return (ForCutFlow && (this->*m_Relation)()) || (!ForCutFlow && (!m_CutSkims || (this->*m_Relation)()));
    }
    bool XAMPPMetCutElement::initialize(float Value, Cut::Relation R, bool IsSkimmingCut) {
        if (m_Relation) {
            Error("XAMPPMetCutElement::initialize()", "The Element using Storage<T> %s is already initialized", name().c_str());
            return false;
        }
        if (R == Cut::Relation::Greater)
            m_Relation = &XAMPPMetCutElement::PassGreaterThan;
        else if (R == Cut::Relation::GreaterEqual)
            m_Relation = &XAMPPMetCutElement::PassGreaterEqual;
        else if (R == Cut::Relation::Equal)
            m_Relation = &XAMPPMetCutElement::PassEqual;
        else if (R == Cut::Relation::LessEqual)
            m_Relation = &XAMPPMetCutElement::PassLessEqual;
        else if (R == Cut::Relation::Less)
            m_Relation = &XAMPPMetCutElement::PassLessThan;
        else {
            Error("XAMPPMetCutElement::initialize()", "No valid cut relation has been given");
            return false;
        }
        m_CutValue = Value;
        m_CutSkims = IsSkimmingCut;
        Info("XAMPPMetCutElement::initialize()", "The element has been succesfully initialized using %s %s %s", name().c_str(),
             Cut::RelToString(R).c_str(), std::to_string(Value).c_str());
        return true;
    }

    XAMPPMetCutElement::XAMPPMetCutElement(XAMPP::Storage<XAMPPmet>* Store, XAMPP::IEventInfo* XAMPPInfo) :
        ICutElement(Store, XAMPPInfo),
        m_MetStore(Store),
        m_CutValue(-1),
        m_CutSkims(true),
        m_Relation(nullptr) {}
    float XAMPPMetCutElement::GetMet() const {
        if (m_MetStore->isAvailable()) { return m_MetStore->GetValue()->met(); }
        Warning("XAMPPMetCutElement::GetMet()", "No met has been saved in Storage %s", name().c_str());
        return NAN;
    }
    bool XAMPPMetCutElement::PassGreaterThan() const { return GetMet() > m_CutValue; }
    bool XAMPPMetCutElement::PassGreaterEqual() const { return GetMet() >= m_CutValue; }
    bool XAMPPMetCutElement::PassEqual() const { return GetMet() == m_CutValue; }
    bool XAMPPMetCutElement::PassLessThan() const { return GetMet() < m_CutValue; }
    bool XAMPPMetCutElement::PassLessEqual() const { return GetMet() <= m_CutValue; }

    //########################################################################################
    //                          ParticleFourVectorCutElement
    //########################################################################################
    bool ParticleFourVectorCutElement::ApplyCut(bool ForCutFlow) const {
        if (!Particle() || !m_Momentum) {
            if (!ForCutFlow && m_CutSkims)
                Warning("ParticleCutElement::ApplyCut()", "Could not retrieve %s in event %u", name().c_str(), EventNumber());
            if (ForCutFlow) return false;
            return !m_CutSkims;
        }
        return (ForCutFlow && (this->*m_Relation)()) || (!ForCutFlow && (!m_CutSkims || (this->*m_Relation)()));
        return false;
    }
    bool ParticleFourVectorCutElement::initialize(float Value, Cut::Relation R, bool IsSkimmingCut) {
        if (R == Cut::Relation::Greater)
            m_Relation = &ParticleFourVectorCutElement::PassGreaterThan;
        else if (R == Cut::Relation::GreaterEqual)
            m_Relation = &ParticleFourVectorCutElement::PassGreaterEqual;
        else if (R == Cut::Relation::Equal)
            m_Relation = &ParticleFourVectorCutElement::PassEqual;
        else if (R == Cut::Relation::LessEqual)
            m_Relation = &ParticleFourVectorCutElement::PassLessEqual;
        else if (R == Cut::Relation::Less)
            m_Relation = &ParticleFourVectorCutElement::PassLessThan;
        else {
            Error("ParticleFourVectorCutElement::initialize()", "No valid cut relation has been given");
            return false;
        }
        if (!m_ParticleStorage) {
            Error("ParticleFourVectorCutElement::initialize()", "The particle storage does not exist");
            return false;
        }
        m_CutSkims = IsSkimmingCut;
        m_CutValue = Value;
        Info("ParticleFourVectorCutElement::initialize()", "The element has been succesfully initialized using %s %s %s", name().c_str(),
             Cut::RelToString(R).c_str(), std::to_string(Value).c_str());

        return true;
    }
    const xAOD::IParticle* ParticleFourVectorCutElement::Particle() const {
        if (!m_ParticleStorage || m_ParticleStorage->Container()->size() <= m_Idx) return nullptr;
        return m_ParticleStorage->Container()->at(m_Idx);
    }
    ParticleFourVectorCutElement::ParticleFourVectorCutElement(XAMPP::IEventInfo* XAMPPInfo, const std::string& Particle, unsigned int Nth,
                                                               Momentum Mom) :
        ICutElement(XAMPPInfo->GetParticleStorage(Particle), XAMPPInfo),
        m_ParticleStorage(XAMPPInfo->GetParticleStorage(Particle)),
        m_Momentum(Mom),
        m_Relation(nullptr),
        m_Idx(Nth),
        m_CutValue(-1),
        m_CutSkims(false) {}

    bool ParticleFourVectorCutElement::PassGreaterThan() const { return (Particle()->*m_Momentum)() > m_CutValue; }
    bool ParticleFourVectorCutElement::PassGreaterEqual() const { return (Particle()->*m_Momentum)() >= m_CutValue; }
    bool ParticleFourVectorCutElement::PassEqual() const { return (Particle()->*m_Momentum)() == m_CutValue; }
    bool ParticleFourVectorCutElement::PassLessThan() const { return (Particle()->*m_Momentum)() < m_CutValue; }
    bool ParticleFourVectorCutElement::PassLessEqual() const { return (Particle()->*m_Momentum)() <= m_CutValue; }
    std::string ParticleFourVectorCutElement::name() const {
        return ICutElement::name() + " " + FourMomentum(m_Momentum) + "[" + std::to_string(m_Idx) + "]";
        ;
    }
}  // namespace XAMPP
