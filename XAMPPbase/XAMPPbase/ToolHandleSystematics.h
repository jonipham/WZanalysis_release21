#ifndef XAMPPbase_ToolHandleSystematics_H
#define XAMPPbase_ToolHandleSystematics_H

#include <XAMPPbase/Defs.h>
#include <XAMPPbase/ISystematics.h>

#include <AsgTools/AnaToolHandle.h>
#include <AsgTools/ToolHandle.h>

#include <PATInterfaces/SystematicCode.h>
#include <PATInterfaces/SystematicSet.h>

namespace XAMPP {
    template <class T> class ToolHandleSystematics : virtual public ISystematicToolService {
    public:
        virtual StatusCode resetSystematic();
        virtual StatusCode setSystematic(const CP::SystematicSet* Set);
        virtual StatusCode initialize();
        StatusCode initTool();
        virtual std::string name() const;

        template <typename P> StatusCode setProperty(const std::string& Property, P Value);

        ToolHandleSystematics(T* Pointer, XAMPP::SelectionObject Kine = XAMPP::SelectionObject::Other,
                              XAMPP::SelectionObject Weight = XAMPP::SelectionObject::Other);
        ToolHandleSystematics(T* Pointer, const std::string& SystTool, XAMPP::SelectionObject Kine = XAMPP::SelectionObject::Other,
                              XAMPP::SelectionObject Weight = XAMPP::SelectionObject::Other);

        ToolHandleSystematics(ToolHandle<T> Handle, const std::string& SystTool,
                              XAMPP::SelectionObject Kine = XAMPP::SelectionObject::Other,
                              XAMPP::SelectionObject Weight = XAMPP::SelectionObject::Other);
        ToolHandleSystematics(ToolHandle<T> Handle, XAMPP::SelectionObject Kine = XAMPP::SelectionObject::Other,
                              XAMPP::SelectionObject Weight = XAMPP::SelectionObject::Other);

        ToolHandleSystematics(asg::AnaToolHandle<T>& AnaHandle, const std::string& SystTool,
                              XAMPP::SelectionObject Kine = XAMPP::SelectionObject::Other,
                              XAMPP::SelectionObject Weight = XAMPP::SelectionObject::Other);
        ToolHandleSystematics(asg::AnaToolHandle<T>& AnaHandle, XAMPP::SelectionObject Kine = XAMPP::SelectionObject::Other,
                              XAMPP::SelectionObject Weight = XAMPP::SelectionObject::Other);

        virtual ~ToolHandleSystematics();

    private:
        T* m_pointer;
        asg::AnaToolHandle<T> m_anaHandle;
        ToolHandle<T> m_handle;
        ToolHandle<ISystematics> m_systematics;
        XAMPP::SelectionObject m_affectKine;
        XAMPP::SelectionObject m_affectWeight;
    };

}  // namespace XAMPP
#include <XAMPPbase/ToolHandleSystematics.ixx>
#endif
