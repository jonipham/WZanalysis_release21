#ifndef XAMPPbase_HistoBase_H
#define XAMPPbase_HistoBase_H
#include <TH1.h>
#include <XAMPPbase/HistoHelpers.h>

#include "TDirectory.h"
#include "TFile.h"

#include <map>
#include <memory>
#include <string>
#include <vector>
namespace CP {
    class SystematicSet;
}

#ifndef XAOD_STANDALONE
class ITHistSvc;
#endif

namespace XAMPP {
    class ISystematics;
    class IAnalysisConfig;
    class IEventInfo;
    class EventInfo;
    class HistoBase {
    public:
        HistoBase(const CP::SystematicSet* set);

        virtual ~HistoBase();
        StatusCode InitializeHistos();
        bool FinalizeHistos();
        StatusCode FillHistos();

        void SetEventInfoHandler(const XAMPP::EventInfo* Info);
        void SetEventInfoHandler(const ToolHandle<XAMPP::IEventInfo>& EvInfo);
        void SetSystematicsTool(const ToolHandle<XAMPP::ISystematics>& Syst);
        void SetAnalysisConfig(const ToolHandle<XAMPP::IAnalysisConfig>& config);

        void SetDoHistos(bool doH) {
            if (!m_init) m_doHistos = doH;
        }
        void WriteCutFlow(bool B) {
            if (!m_init) m_WriteCutFlow = B;
        }

        // This function returns for mc the mcChannelNumber
        // and for data the runNumber
        unsigned int Identifier() const;
        bool isData() const;
        // Get the computed Weights... Nominal only!!
        double GetWeight() const;
        double GetWeight2() const;
        std::string name() const;
        const CP::SystematicSet* SystSet() const;

        bool HistoInOutput(TH1* Histo, const std::string& FullPath);

        bool RegisterCutFlow(const CutFlow& Flow);
        CutFlowHistoVariable* CreateCutFlowHisto(const CutFlow& Flow);

        void UpdateWeight();

    private:
        const CP::SystematicSet* m_set;

    protected:
        const XAMPP::EventInfo* m_XAMPPInfo;

    private:
        const XAMPP::ISystematics* m_systematics;
        ToolHandle<XAMPP::IAnalysisConfig> m_config;
        ServiceHandle<ITHistSvc> m_histSvc;

        bool m_init;
        bool m_doHistos;
        bool m_WriteCutFlow;

        void AppendWeight(const std::string& Name);
        double m_CurrentWeight;
        double m_CurrentWeight2;
        std::vector<XAMPP::Storage<double>*> m_EventWeights;
        std::vector<IHistoVariable_Ptr> m_Variables;
        std::vector<IHistoVariable_Ptr> m_CutFlowHistos;
        std::vector<const CutFlow*> m_CutFlows;
        unsigned int m_EvNumber;
    };

}  // namespace XAMPP
#endif
