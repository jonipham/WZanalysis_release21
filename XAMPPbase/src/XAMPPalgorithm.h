#ifndef XAMPPBASE_XAMPPALGORITHM_H
#define XAMPPBASE_XAMPPALGORITHM_H

#include <AthAnalysisBaseComps/AthAnalysisAlgorithm.h>
#include <AthenaBaseComps/AthAlgorithm.h>
#include <GaudiKernel/ToolHandle.h>
#include <TStopwatch.h>
#include <string>

namespace XAMPP {
    class IAnalysisHelper;
    class ISystematics;
    class XAMPPalgorithm : public AthAnalysisAlgorithm {
    public:
        XAMPPalgorithm(const std::string& name, ISvcLocator* pSvcLocator);
        virtual ~XAMPPalgorithm();

        virtual StatusCode initialize();
        virtual StatusCode execute();
        virtual StatusCode finalize();
        virtual StatusCode beginInputFile();

    private:
        StatusCode CheckCutflow();
        StatusCode ExecuteEvent();

        ToolHandle<XAMPP::ISystematics> m_systematics;
        ToolHandle<XAMPP::IAnalysisHelper> m_helper;

        bool m_RunCutFlow;
        bool m_init;

        TStopwatch m_tsw;
        long long int m_Events;
        long long int m_CurrentEvent;
        long long int m_printInterval;
        unsigned int m_TotSyst;

        bool m_updateTotEvents;  // Update the number of total events at the beginning of each file
        unsigned int m_TotFiles;
        unsigned int m_CurrentFile;
        std::string TimeHMS(float t) const;
    };

}  // namespace XAMPP
#endif  //> !XAMPPBASE_XAMPPALGORITHM_H
