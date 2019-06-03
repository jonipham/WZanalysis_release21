#ifndef XAMPPbase_ToolHandleSystematics_IXX
#define XAMPPbase_ToolHandleSystematics_IXX

#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/ToolHandleSystematics.h>

#include "PATInterfaces/SystematicsUtil.h"
namespace XAMPP {

    template <class T>
    ToolHandleSystematics<T>::ToolHandleSystematics(T* Pointer, const std::string& SystTool, XAMPP::SelectionObject Kine,
                                                    XAMPP::SelectionObject Weight) :
        m_pointer(Pointer),
        m_anaHandle(""),
        m_handle(Pointer->name()),
        m_systematics(SystTool),
        m_affectKine(Kine),
        m_affectWeight(Weight) {}
    template <class T>
    ToolHandleSystematics<T>::ToolHandleSystematics(T* Pointer, XAMPP::SelectionObject Kine, XAMPP::SelectionObject Weight) :
        ToolHandleSystematics(Pointer, "SystematicsTool", Kine, Weight) {}
    template <class T>
    ToolHandleSystematics<T>::ToolHandleSystematics(asg::AnaToolHandle<T>& AnaHandle, const std::string& SystTool,
                                                    XAMPP::SelectionObject Kine, XAMPP::SelectionObject Weight) :
        m_pointer(nullptr),
        m_anaHandle(AnaHandle),
        m_handle(AnaHandle.getHandle()),
        m_systematics(SystTool),
        m_affectKine(Kine),
        m_affectWeight(Weight) {}
    template <class T>
    ToolHandleSystematics<T>::ToolHandleSystematics(asg::AnaToolHandle<T>& AnaHandle, XAMPP::SelectionObject Kine,
                                                    XAMPP::SelectionObject Weight) :
        ToolHandleSystematics(AnaHandle, "SystematicsTool", Kine, Weight) {}

    template <class T>
    ToolHandleSystematics<T>::ToolHandleSystematics(ToolHandle<T> Handle, XAMPP::SelectionObject Kine, XAMPP::SelectionObject Weight) :
        ToolHandleSystematics(Handle, "SystematicsTool", Kine, Weight) {}
    template <class T>
    ToolHandleSystematics<T>::ToolHandleSystematics(ToolHandle<T> Handle, const std::string& SystTool, XAMPP::SelectionObject Kine,
                                                    XAMPP::SelectionObject Weight) :
        m_pointer(nullptr),
        m_anaHandle(""),
        m_handle(Handle),
        m_systematics(SystTool),
        m_affectKine(Kine),
        m_affectWeight(Weight) {}
    template <class T> StatusCode ToolHandleSystematics<T>::resetSystematic() { return setSystematic(m_systematics->GetNominal()); }
    template <class T> StatusCode ToolHandleSystematics<T>::setSystematic(const CP::SystematicSet* Set) {
        if (!ToolIsAffectedBySystematic(m_handle, Set)) { return StatusCode::SUCCESS; }
        if (m_handle->applySystematicVariation(*Set) != CP::SystematicCode::Ok) { return StatusCode::FAILURE; }
        return StatusCode::SUCCESS;
    }
    template <class T> StatusCode ToolHandleSystematics<T>::initialize() {
        if (!m_handle.retrieve().isSuccess()) {
            Error(name().c_str(), "No valid instance of the tool handle found");
            return StatusCode::FAILURE;
        }
        if (!m_systematics.retrieve().isSuccess()) {
            Error(name().c_str(), "Failed to retrieve the ISystematicsTool");
            return StatusCode::FAILURE;
        }
        if (!m_systematics->InsertSystematicToolService(this)) {
            Error(name().c_str(), "Could not append to ISystematicService");
            return StatusCode::FAILURE;
        }
        if (m_affectKine != XAMPP::SelectionObject::Other) {
            for (const auto& set : CP::make_systematics_vector(m_handle->recommendedSystematics())) {
                if (!m_systematics->InsertKinematicSystematic(set, m_affectKine)) {
                    Error(name().c_str(), "Failed to initialize systematic %s", set.name().c_str());
                    return StatusCode::FAILURE;
                }
            }
        }
        if (m_affectWeight != XAMPP::SelectionObject::Other) {
            for (const auto& set : CP::make_systematics_vector(m_handle->recommendedSystematics())) {
                if (!m_systematics->InsertWeightSystematic(set, m_affectWeight)) {
                    Error(name().c_str(), "Failed to initialize systematic %s", set.name().c_str());
                    return StatusCode::FAILURE;
                }
            }
        }
        return StatusCode::SUCCESS;
    }
    template <class T> std::string ToolHandleSystematics<T>::name() const { return m_handle.name(); }
    template <class T> ToolHandleSystematics<T>::~ToolHandleSystematics() {
        if (m_pointer) delete m_pointer;
    }
    template <class T> template <typename P> StatusCode ToolHandleSystematics<T>::setProperty(const std::string& Property, P Value) {
        asg::AsgTool* AsgPtr = dynamic_cast<asg::AsgTool*>(m_handle.operator->());
        if (!AsgPtr) {
            Error(name().c_str(), "The tool is not an AsgTool");
            return StatusCode::FAILURE;
        }
        return AsgPtr->setProperty(Property, Value);
    }
    template <class T> StatusCode ToolHandleSystematics<T>::initTool() { return m_handle->initialize(); }
}  // namespace XAMPP
#endif
