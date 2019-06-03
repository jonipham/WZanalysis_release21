#ifndef XAMPPbase_AnalysisConfig_H
#define XAMPPbase_AnalysisConfig_H

#include <AsgTools/AsgTool.h>
#include <AsgTools/IAsgTool.h>

#include <XAMPPbase/Cuts.h>
#include <map>
#include <string>
#include <vector>

namespace CP {
    class SystematicSet;
}
namespace XAMPP {
    class HistoBase;
    class EventInfo;
    class ISystematics;
    class IHistoVariable;
    enum CutKind { MonitorCutFlow, EventDump };

    typedef std::vector<Cut*> CutRow;

    /**
     * @brief      Class for cut flow. This class stores the cuts for the event
     *             selection and can hold the cuts similar to a container. It
     *             supports indexing the cuts via integer hash values.
     */
    class CutFlow {
    public:
        /**
         * @brief      Default constructor for cut flow class
         */
        CutFlow();

        /**
         * @brief      Constructor for cut flow class allowing to name the
         *             cutflow. The hash of the cutflow (used for faster
         *             indexing) is based on the name of the cutflow.
         *
         * @param[in]  name  The name of the CutFlow (is also converted to a
         *                   hash value via std::hash)
         */
        CutFlow(const std::string& name);

        /**
         * @brief      Destroys the object.
         */
        ~CutFlow();

        /**
         * @brief      Adds a cut to the CutFlow
         *
         * @param      cut   The cut to add
         */
        void push_back(Cut* cut);

        /**
         * @brief      Replaces a cut of the CutFlow with another one
         *
         * @param      ToReplace  Cut to replace
         * @param      With       New cut which should be used
         */
        void replaceCut(Cut* ToReplace, Cut* With);

        /**
         * @brief      Converts a cut into an integer hash in order to speed up
         *             the code
         *
         * @return     Integer hash of the CutFlow
         */
        int hash() const;

        /**
         * @brief      Returns the name of the CutFlow
         *
         * @return     Name of CutFlow
         */
        const std::string& name() const;

        /**
         * @brief      Sets the name of the CutFlow
         *
         * @param[in]  N     Name of CutFlow
         */
        void setName(const std::string& N);

        /**
         * @brief      Gets the cuts as CutRow object (vector of Cut objects)
         *
         * @return     The cuts (as CutRow object - a std::vector of pointers to the Cut objects)
         */
        const CutRow& GetCuts() const;

        /**
         * @brief      Assignment operator. Assigns the cuts stored in another
         *             cutflow to this cutflow and sets cutflow name and hash to
         *             the values of the other cutflow.
         *
         * @param[in]  cutflow  The cutflow that should be assigned to this
         *                      cutflow by copying the name, cutflow hash and
         *                      inserting the cuts of the other cutflow to this
         *                      cutflow.
         *
         * @return     This existing object (this allows that this operator can be chained)
         */
        CutFlow& operator=(const CutFlow& cutflow);

    private:
        CutRow m_AnaCuts;
        std::string m_Name;
        int m_Hash;
    };

    /**
     * @brief      Interface for AnalysisConfig. The analysis config can store multiple user-defined cutflows.
     */
    class IAnalysisConfig : virtual public asg::IAsgTool {
        ASG_TOOL_INTERFACE(IAnalysisConfig)
    public:
        /**
         * @brief      Initialize the AnalysisConfig. If the analysis config is
         *             already initialized (m_init set is true), exit without
         *             doing anything. Otherwise retrieve ToolHandle for
         *             XAMPP::EventInfo and initialize standard + custom cuts.
         *
         * @return     Status code, needs to be checked with e.g. ATH_CHECK(...).
         */
        virtual StatusCode initialize() = 0;

        /**
         * @brief      Checks if at least one cut is passed. If at least one cut is passed
         *
         * @param[in]  K     { parameter_description }
         *
         * @return     { description_of_the_return_value }
         */
        virtual bool ApplyCuts(CutKind K) = 0;

        // adds an externally build cut flow to this instance
        virtual StatusCode AddToCutFlows(CutFlow& cf) = 0;

        /**
         * @brief      Gets the cut names.
         *
         * @param[in]  hash  The hash
         *
         * @return     The cut names.
         */
        virtual std::vector<std::string> GetCutNames(int hash = 0) const = 0;

        /**
         * @brief      Get the name of the output tree
         *
         * @return     { description_of_the_return_value }
         */
        virtual std::string TreeName() const = 0;

        /**
         * @brief      { function_description }
         *
         * @param      Base  The base
         *
         * @return     { description_of_the_return_value }
         */
        virtual bool RegisterHistoBase(HistoBase* Base) = 0;

        // create a cut
        virtual Cut* NewCut(const std::string& Name, Cut::CutType T, bool IsSkimming) = 0;

        /**
         * @brief      Destroys the object and deletes standard cuts and
         *             user-defined cutflows.
         */
        virtual ~IAnalysisConfig() {}
    };

    /**
     * @brief      Class for defining different CutFlows
     */
    class AnalysisConfig : public asg::AsgTool, virtual public IAnalysisConfig {
    public:
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(AnalysisConfig, XAMPP::IAnalysisConfig)

        /**
         * @brief      { function_description }
         *
         * @param[in]  Analysis  The analysis
         */
        AnalysisConfig(const std::string& Analysis);

        /**
         * @brief      Destroys the object.
         */
        virtual ~AnalysisConfig();

        /**
         * @brief      { function_description }
         *
         * @return     { description_of_the_return_value }
         */
        virtual StatusCode initialize();

        virtual StatusCode AddToCutFlows(CutFlow& cf);
        /**
         * @brief      { function_description }
         *
         * @param[in]  K     { parameter_description }
         *
         * @return     { description_of_the_return_value }
         */
        virtual bool ApplyCuts(CutKind K);

        /**
         * @brief      Gets the cut names.
         *
         * @param[in]  hash  The hash
         *
         * @return     The cut names.
         */
        virtual std::vector<std::string> GetCutNames(int hash = 0) const;

        /**
         * @brief      { function_description }
         *
         * @return     { description_of_the_return_value }
         */
        virtual std::string TreeName() const;

        /**
         * @brief      { function_description }
         *
         * @param      Base  The base
         *
         * @return     { description_of_the_return_value }
         */
        virtual bool RegisterHistoBase(HistoBase* Base);

        // create a cut
        virtual Cut* NewCut(const std::string& Name, Cut::CutType T, bool IsSkimming);

    protected:
        virtual bool isData() const;
        virtual StatusCode initializeCustomCuts();
        virtual StatusCode initializeStandardCuts();
        bool isActive(CutFlow& cf) const;
        Cut* NewSkimmingCut(const std::string& Name, Cut::CutType T);
        Cut* NewCutFlowCut(const std::string& Name, Cut::CutType T);

        EventInfo* m_XAMPPInfo;
        unsigned int NumActiveCutFlows() const;

    private:
        bool PassStandardCuts(unsigned int& N) const;
        IHistoVariable* FindCutFlowHisto(const CP::SystematicSet* Set, const CutFlow* Flow) const;

        // ASG properties
        std::string m_treeName;

        std::vector<std::string> m_ActiveCutflows;
        std::string m_ActiveCutflowsString;

    protected:
        // internal variables
        CutRow m_StandardCuts;

    private:
        std::vector<CutFlow> m_CutFlows;
        CutRow m_DefinedCuts;  // book-keeping of ALL defined cuts for cleaning
                               // up in the end

        typedef std::pair<const CP::SystematicSet*, const CutFlow*> SystSelectionPair;
        std::map<SystSelectionPair, IHistoVariable*> m_CutFlowHistos;
        bool m_init;
        asg::AnaToolHandle<XAMPP::IEventInfo> m_InfoHandle;
        ToolHandle<ISystematics> m_systematics;
    };

    class TruthAnalysisConfig : public AnalysisConfig {
    public:
        ASG_TOOL_CLASS(TruthAnalysisConfig, XAMPP::IAnalysisConfig)
        //
        TruthAnalysisConfig(const std::string& Analysis = "Custom");
        // Create a proper constructor for Athena
    protected:
        virtual StatusCode initializeStandardCuts();
        virtual StatusCode initializeCustomCuts();
    };
}  // namespace XAMPP
#endif
