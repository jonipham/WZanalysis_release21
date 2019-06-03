#ifndef XAMPPbase_Cuts_IXX
#define XAMPPbase_Cuts_IXX
#include <XAMPPbase/Cuts.h>

namespace XAMPP {
    template <typename T> bool ICutElement::IsValid(const T& Value) const { return !std::isnan(Value) && !std::isinf(Value); }
    template <typename T, typename T1> bool Cut::initialize(XAMPP::Storage<T>* Store, T1 CutValue, Cut::Relation Rel) {
        if (m_Init) {
            Error("Cut::initialize()", "%s has already been initialized.", GetName().c_str());
            return false;
        }
        ScalarCutElement<T>* ScalarElement = new ScalarCutElement<T>(Store, m_XAMPPInfo);
        m_CutElement = std::unique_ptr<XAMPP::ICutElement>(ScalarElement);
        m_Init = true;
        return ScalarElement->initialize(CutValue, Rel, m_IsRealCut);
    }
    template <typename T1> bool Cut::initialize(XAMPP::Storage<XAMPPmet>* S, T1 CutValue, Cut::Relation Rel) {
        if (m_Init) {
            Error("Cut::initialize()", "%s has already been initialized.", GetName().c_str());
            return false;
        }
        XAMPPMetCutElement* MetCut = new XAMPPMetCutElement(S, m_XAMPPInfo);
        m_CutElement = std::unique_ptr<XAMPP::ICutElement>(MetCut);
        m_Init = true;
        return MetCut->initialize(CutValue, Rel, m_IsRealCut);
    }
    template <typename T>
    bool Cut::ParticleInit(const std::string& Container, const std::string& Variable, unsigned int Nth, T CutValue, Cut::Relation Rel) {
        ParticleCutElement<T>* PartCut = new ParticleCutElement<T>(m_XAMPPInfo, Container, Nth, Variable);
        m_CutElement = std::unique_ptr<XAMPP::ICutElement>(PartCut);
        m_Init = true;
        return PartCut->initialize(CutValue, Rel, m_IsRealCut);
    }

    template <class T>
    ScalarCutElement<T>::ScalarCutElement(XAMPP::Storage<T>* Store, XAMPP::IEventInfo* XAMPPInfo) :
        ICutElement(Store, XAMPPInfo),
        m_CutStore(Store),
        m_CutSkims(true),
        m_CutValue(),
        m_Relation(nullptr) {}
    template <class T> bool ScalarCutElement<T>::PassGreaterThan() const { return m_CutStore->GetValue() > m_CutValue; }
    template <class T> bool ScalarCutElement<T>::PassGreaterEqual() const { return m_CutStore->GetValue() >= m_CutValue; }
    template <class T> bool ScalarCutElement<T>::PassEqual() const { return m_CutStore->GetValue() == m_CutValue; }
    template <class T> bool ScalarCutElement<T>::PassLessEqual() const { return m_CutStore->GetValue() <= m_CutValue; }
    template <class T> bool ScalarCutElement<T>::PassLessThan() const { return m_CutStore->GetValue() < m_CutValue; }
    template <class T> bool ScalarCutElement<T>::ApplyCut(bool ForCutFlow) const {
        if (!IsValid(m_CutStore->GetValue())) {
            Error("ScalarCutElement<T>::ApplyCut()", "The variable %s is not a number in event %u", name().c_str(), EventNumber());
            return false;
        }
        return (ForCutFlow && (this->*m_Relation)()) || (!ForCutFlow && (!m_CutSkims || (this->*m_Relation)()));
    }
    template <class T> bool ScalarCutElement<T>::initialize(T Value, Cut::Relation R, bool IsSkimmingCut) {
        if (m_Relation) {
            Error("ScalarCutElement::initialize()", "The Element using Storage<T> %s is already initialized", name().c_str());
            return false;
        }
        if (!m_CutStore) {
            Error("ScalarCutElement<T>::ApplyCut()", "The EventStore does not exist");
            return false;
        }
        if (R == Cut::Relation::Greater)
            m_Relation = &ScalarCutElement::PassGreaterThan;
        else if (R == Cut::Relation::GreaterEqual)
            m_Relation = &ScalarCutElement::PassGreaterEqual;
        else if (R == Cut::Relation::Equal)
            m_Relation = &ScalarCutElement::PassEqual;
        else if (R == Cut::Relation::LessEqual)
            m_Relation = &ScalarCutElement::PassLessEqual;
        else if (R == Cut::Relation::Less)
            m_Relation = &ScalarCutElement::PassLessThan;
        else {
            Error("SclarCutElement::initialize()", "No valid cut relation has been given");
            return false;
        }
        m_CutValue = Value;
        m_CutSkims = IsSkimmingCut;
        Info("ScalarCutElement::initialize()", "The element has been succesfully initialized using %s %s %s", name().c_str(),
             Cut::RelToString(R).c_str(), std::to_string(Value).c_str());
        return true;
    }
    //#############################################################################
    //                              XAMPPParticlecutElement
    //#############################################################################

    template <class T>
    ParticleCutElement<T>::ParticleCutElement(XAMPP::IEventInfo* XAMPPInfo, const std::string& Particle, unsigned int N_th,
                                              const std::string& Variable) :
        ICutElement(XAMPPInfo->GetParticleStorage(Particle), XAMPPInfo),
        m_ParticleStore(XAMPPInfo->GetParticleStorage(Particle)),
        m_Acc(Variable),
        m_VarName(Variable),
        m_Idx(N_th),
        m_CutValue(-1),
        m_CutSkims(false),
        m_Relation(nullptr) {}
    template <class T> const xAOD::IParticle* ParticleCutElement<T>::Particle() const {
        if (!m_ParticleStore->Container() || m_ParticleStore->Container()->size() <= m_Idx) return nullptr;
        return m_ParticleStore->Container()->at(m_Idx);
    }

    template <class T> bool ParticleCutElement<T>::ApplyCut(bool ForCutFlow) const {
        if (!Particle() || !m_Acc.isAvailable(*Particle())) {
            if (!ForCutFlow && m_CutSkims)
                Warning("ParticleCutElement::ApplyCut()", "Could not retrieve %s in event %u", name().c_str(), EventNumber());
            if (ForCutFlow) return false;
            return !m_CutSkims;
        }
        return (ForCutFlow && (this->*m_Relation)()) || (!ForCutFlow && (!m_CutSkims || (this->*m_Relation)()));
    }
    template <class T> std::string ParticleCutElement<T>::name() const {
        return ICutElement::name() + " " + m_VarName + "[" + std::to_string(m_Idx) + "]";
    }
    template <class T> bool ParticleCutElement<T>::PassGreaterThan() const { return m_Acc(*Particle()) > m_CutValue; }
    template <class T> bool ParticleCutElement<T>::PassGreaterEqual() const { return m_Acc(*Particle()) >= m_CutValue; }
    template <class T> bool ParticleCutElement<T>::PassEqual() const { return m_Acc(*Particle()) == m_CutValue; }
    template <class T> bool ParticleCutElement<T>::PassLessEqual() const { return m_Acc(*Particle()) <= m_CutValue; }
    template <class T> bool ParticleCutElement<T>::PassLessThan() const { return m_Acc(*Particle()) < m_CutValue; }
    template <class T> bool ParticleCutElement<T>::initialize(T Value, Cut::Relation R, bool IsSkimmingCut) {
        if (R == Cut::Relation::Greater)
            m_Relation = &ParticleCutElement::PassGreaterThan;
        else if (R == Cut::Relation::GreaterEqual)
            m_Relation = &ParticleCutElement::PassGreaterEqual;
        else if (R == Cut::Relation::Equal)
            m_Relation = &ParticleCutElement::PassEqual;
        else if (R == Cut::Relation::LessEqual)
            m_Relation = &ParticleCutElement::PassLessEqual;
        else if (R == Cut::Relation::Less)
            m_Relation = &ParticleCutElement::PassLessThan;
        else {
            Error("ParticleCutElement::initialize()", "No valid cut relation has been given");
            return false;
        }
        if (!m_ParticleStore) {
            Error("ParticleCutElement::initialize()", "The particle storage does not exist");
            return false;
        }
        m_CutValue = Value;
        m_CutSkims = IsSkimmingCut;
        Info("ParticleCutElement::initialize()", "The element has been succesfully initialized using %s %s %s", name().c_str(),
             Cut::RelToString(R).c_str(), std::to_string(Value).c_str());
        return true;
    }

}  // namespace XAMPP
#endif
