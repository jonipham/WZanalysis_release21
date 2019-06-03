#ifndef XAMPPbase_Cuts_H
#define XAMPPbase_Cuts_H
#include <XAMPPbase/Defs.h>
#include <XAMPPbase/EventInfo.h>
#include <xAODMissingET/MissingETContainer.h>
#include <iostream>
#include <string>

namespace XAMPP {
    class EventInfo;
    class ICutElement {
    public:
        virtual ~ICutElement() {}

        virtual bool ApplyCut(bool ForCutFlow = false) const = 0;
        virtual std::string name() const;

    protected:
        ICutElement(XAMPP::IStorage* Store, XAMPP::IEventInfo* Info);
        template <class T> bool IsValid(const T& Value) const;
        unsigned int EventNumber() const;

    private:
        XAMPP::IStorage* m_Store;
        XAMPP::IEventInfo* m_Info;
    };
    class Cut {
    public:
        enum CutType {
            // Generic data types for event variables
            CutInt,
            CutFloat,
            CutChar,
            CutBool,
            // Generic data types for particles
            CutPartInt,
            CutPartFloat,
            CutPartChar,
            CutPartBool,
            // Missing Et
            CutXAMPPmet,
            CutInDef
        };
        enum Combine { inDef, AND, OR };
        enum Relation { NoRel, Greater, Equal, Less, GreaterEqual, LessEqual };
        Cut(XAMPP::EventInfo*, const std::string& N, CutType T, bool Real);
        Cut(const std::string& N, CutType T, bool Real);
        virtual ~Cut();
        Cut* combine(Cut* cut, Combine C = Combine::inDef);
        template <typename T1> bool initialize(XAMPP::Storage<XAMPPmet>* S, T1 CutValue, Cut::Relation Rel);

        template <typename T, typename T1> bool initialize(XAMPP::Storage<T>* Store, T1 CutValue, Cut::Relation Rel);
        template <typename T>
        bool ParticleInit(const std::string& Container, const std::string& Variable, unsigned int Nth, T CutValue, Cut::Relation Rel);
        bool ParticleInit(const std::string& Container, Momentum Mom, unsigned int Nth, float CutValue, Cut::Relation Rel);

        bool initialize(std::string VarName, std::string Relation);

        void SetName(const std::string& N);

        virtual bool ApplyCut(bool ForCutFlow = false);
        virtual std::string GetName() const;
        static std::string RelToString(Relation R);
        void Debug(bool B = true);
        virtual bool IsInitialized() const;

        virtual void Disconnect(Cut* C);
        void Connect(Cut* C);

    protected:
        Cut();
        unsigned int EventNumber() const;
        Relation GetCondition(std::string& Cond);
        std::string m_Name;
        CutType m_Type;
        bool m_IsRealCut;
        bool m_Debug;
        std::unique_ptr<XAMPP::ICutElement> m_CutElement;
        XAMPP::EventInfo* m_XAMPPInfo;

    private:
        bool m_Init;
        // Vector of all conditions to which the Cut itself is connected to
        std::vector<Cut*> m_Connected;
    };
    class CompositeCut : public Cut {
    public:
        CompositeCut(Cut* C1, Cut* C2);
        virtual ~CompositeCut();
        virtual bool IsInitialized() const;
        virtual void Disconnect(Cut* C);

    protected:
        Cut* m_Cut1;
        Cut* m_Cut2;
    };
    class CombCutAND : public CompositeCut {
    public:
        CombCutAND(Cut* C1, Cut* C2);
        virtual bool ApplyCut(bool ForCutFlow = false);
        virtual std::string GetName() const;
        virtual ~CombCutAND() {}
    };
    class CombCutOR : public CompositeCut {
    public:
        CombCutOR(Cut* C1, Cut* C2);
        virtual bool ApplyCut(bool ForCutFlow = false);
        virtual std::string GetName() const;
        virtual ~CombCutOR() {}
    };
    // For general usecases
    template <class T> class ScalarCutElement : public ICutElement {
    public:
        ScalarCutElement(XAMPP::Storage<T>* Store, XAMPP::IEventInfo* XAMPPInfo);
        virtual bool initialize(T Value, Cut::Relation R, bool IsSkimmingCut);
        virtual bool ApplyCut(bool ForCutFlow = false) const;

    protected:
        typedef bool (ScalarCutElement::*Application)() const;
        XAMPP::Storage<T>* m_CutStore;
        bool m_CutSkims;
        T m_CutValue;
        Application m_Relation;
        bool PassGreaterThan() const;
        bool PassGreaterEqual() const;
        bool PassEqual() const;
        bool PassLessThan() const;
        bool PassLessEqual() const;
    };
    // Something for XAMPPmet
    class XAMPPMetCutElement : public ICutElement {
    public:
        virtual bool ApplyCut(bool ForCutFlow = false) const;
        virtual bool initialize(float Value, Cut::Relation R, bool IsSkimmingCut);
        XAMPPMetCutElement(XAMPP::Storage<XAMPPmet>* Store, XAMPP::IEventInfo* XAMPPInfo);

    protected:
        typedef bool (XAMPPMetCutElement::*Application)() const;
        XAMPP::Storage<XAMPPmet>* m_MetStore;

        float m_CutValue;
        bool m_CutSkims;
        Application m_Relation;

        float GetMet() const;
        bool PassGreaterThan() const;
        bool PassGreaterEqual() const;
        bool PassEqual() const;
        bool PassLessThan() const;
        bool PassLessEqual() const;
    };
    class ParticleFourVectorCutElement : public ICutElement {
    public:
        virtual bool ApplyCut(bool ForCutFlow = false) const;
        virtual bool initialize(float Value, Cut::Relation, bool IsSkimmingCut);
        virtual std::string name() const;
        ParticleFourVectorCutElement(XAMPP::IEventInfo* XAMPPInfo, const std::string& Particle, unsigned int Nth, Momentum Mom);

    protected:
        const xAOD::IParticle* Particle() const;

        typedef bool (ParticleFourVectorCutElement::*Application)() const;

        ParticleStorage* m_ParticleStorage;
        Momentum m_Momentum;
        Application m_Relation;
        unsigned int m_Idx;
        float m_CutValue;
        bool m_CutSkims;

        bool PassGreaterThan() const;
        bool PassGreaterEqual() const;
        bool PassEqual() const;
        bool PassLessThan() const;
        bool PassLessEqual() const;
    };
    template <class T> class ParticleCutElement : public ICutElement {
    public:
        virtual bool ApplyCut(bool ForCutFlow = false) const;
        virtual bool initialize(T Value, Cut::Relation, bool IsSkimmingCut);
        ParticleCutElement(XAMPP::IEventInfo* XAMPPInfo, const std::string& Particle, unsigned int N_th, const std::string& Variable);
        virtual std::string name() const;

    protected:
        typedef bool (ParticleCutElement::*Application)() const;
        const xAOD::IParticle* Particle() const;

        ParticleStorage* m_ParticleStore;
        SG::AuxElement::Accessor<T> m_Acc;
        std::string m_VarName;
        unsigned int m_Idx;
        T m_CutValue;
        bool m_CutSkims;
        Application m_Relation;
        bool PassGreaterThan() const;
        bool PassGreaterEqual() const;
        bool PassEqual() const;
        bool PassLessThan() const;
        bool PassLessEqual() const;
    };

}  // namespace XAMPP
#include <XAMPPbase/Cuts.ixx>
#endif
