#ifndef XAMPPbase_HistoHelpers_IXX
#define XAMPPbase_HistoHelpers_IXX

#include <XAMPPbase/Defs.h>
#include <XAMPPbase/EventStorage.h>
#include <XAMPPbase/HistoBase.h>

namespace XAMPP {
    template <class T> class Storage;
    class ParticleStorage;
    class HistoBase;

    template <class T>
    EventHistoVariable<T>::EventHistoVariable(XAMPP::Storage<T>* Store, HistoBase* Base, const CutFlow* Flow, const std::string& Name) :
        IHistoVariable(Base, Flow, Name),
        m_Store(Store),
        m_Template(nullptr) {}
    template <class T> bool EventHistoVariable<T>::Fill() { return true; }
    template <class T> TH1* EventHistoVariable<T>::GetTemplate() const { return nullptr; }
}  // namespace XAMPP
#endif
