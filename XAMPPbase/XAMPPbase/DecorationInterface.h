#ifndef XAMPP_DECORATIONINTERFACE__H
#define XAMPP_DECORATIONINTERFACE__H

#include <type_traits>
#include "AthContainers/AuxElement.h"

namespace XAMPP {

    // useful in all kinds of situations: standardised decoration access
    // do we also want to add exception handling (check for SG::ExcBadAuxVar)?
    // Downside: Exception handling comes at some performance cost...
    template <class decoType, class retType>
    inline bool getDeco(const SG::AuxElement::Accessor<decoType>& accessor, const SG::AuxElement& part, retType& val) {
        if (accessor.isAvailable(part)) {
            val = (retType)accessor(part);
            return true;
        }
        return false;
    }
    // in this version, look up a decorator by name. Slow! The user is responsible for specifying the correct deco type
    template <class decoType> inline bool getDeco(const std::string& name, const SG::AuxElement& part, decoType& val) {
        if (part.isAvailable<decoType>(name)) {
            val = part.auxdata<decoType>(name);
            return true;
        }
        return false;
    }
    // also define a standard setter - though this is not really gaining us anything... might want to add functionality in the future, so
    // the abstraction might still be of use
    template <class decoType, class inputType>
    inline void setDeco(const SG::AuxElement::Decorator<decoType>& deco, const SG::AuxElement& part, const inputType& val) {
        deco(part) = (decoType)val;
    }
    // also a slow version with arbitrary strings as name
    template <class decoType> inline void setDeco(const std::string& name, const SG::AuxElement& part, const decoType& val) {
        part.auxdecor<decoType>(name) = val;
    }

    // two-way decoration class, in case it is wanted / needed.
    // This should *not* be a private member of its owner if we want it to be useful...

    template <class decoType> class Decoration {
    public:
        Decoration(const std::string& name) : m_deco(name), m_accessor(name), m_decoName(name) {}
        ~Decoration(){};
        // read from object aux. return if access was success or failure.
        // In case of success, val is set to the deco. content
        bool get(const SG::AuxElement& aux, decoType& val) const { return getDeco(m_accessor, aux, val); }
        bool get(const SG::AuxElement* aux, decoType& val) const { return aux != nullptr && get(*aux, val); }
        // Exclusively for char decorations, we support an operator() access which will return a false
        // if either the decoration is set to false or not present.
        // Reason for supporting only char: No obvious default return for failure in other cases.
        template <typename T = decoType,
                  typename std::enable_if<std::is_same<T, char>::value, int>::type = 0>  // substitution fails for non-char parameters
        bool
        operator()(const SG::AuxElement& aux) {
            return (m_accessor.isAvailable(aux) && m_accessor(aux));
        }
        template <typename T = decoType,
                  typename std::enable_if<std::is_same<T, char>::value, int>::type = 0>  // substitution fails for non-char parameters
        bool
        operator()(const SG::AuxElement* aux) {
            return (m_accessor.isAvailable(*aux) && m_accessor(*aux));
        }
        // this is the backup implementation for non-char decorations. It will complain and always return a false.
        bool operator()(...) {
            std::cerr << " careful! You called XAMPP::Decoration::operator() for the decoration \"" << m_decoName << "\"." << std::endl
                      << " This should only be used for char decorations. Please use XAMPP::Decoration::get() instead." << std::endl
                      << " I will return a trivial false..." << std::endl;
            return false;
        }
        template <typename T = decoType, typename Q = decoType>
        // check for availability
        bool isAvailable(const SG::AuxElement& aux) const {
            return m_accessor.isAvailable(aux);
        }
        bool isAvailable(const SG::AuxElement* aux) const { return aux != nullptr && m_accessor.isAvailable(aux); }
        // set the decoration for object aux to val
        void set(const SG::AuxElement& aux, const decoType& val) const { setDeco(m_deco, aux, val); }
        void set(const SG::AuxElement* aux, const decoType& val) const {
            if (aux) set(*aux, val);
        }
        void setDecorationString(const std::string& name) {
            m_deco = SG::AuxElement::Decorator<decoType>(name);
            m_accessor = SG::AuxElement::Accessor<decoType>(name);
            m_decoName = name;
        }

        std::string getDecorationString() const { return m_decoName; }

    private:
        SG::AuxElement::Decorator<decoType> m_deco;
        SG::AuxElement::Accessor<decoType> m_accessor;
        std::string m_decoName;
    };

}  // namespace XAMPP

#endif  // XAMPP_DECORATIONINTERFACE__H
