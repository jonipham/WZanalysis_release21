// XAMPPbase includes
#include "XAMPPalgorithm.h"
#include <EventInfo/EventStreamInfo.h>
#include <GaudiKernel/ServiceHandle.h>
#include <XAMPPbase/AnalysisUtils.h>
#include <XAMPPbase/IAnalysisHelper.h>
#include <XAMPPbase/ISystematics.h>

__attribute__((constructor)) static void initializer(void) {
    printf(
        "\n"
        "\x1b[32m");  // ANSI_COLOR_GREEN
    printf("XAMPPbase");
    printf(" v");
    printf("1.0.0");
    printf(" -- xAOD analysis code from MPP Munich/ CERN / Brandeis University\n");
    printf("                    ");
    printf("Developed by Philipp Gadow (pgadow@cern.ch),\n");
    printf("                    ");
    printf("             Max Goblirsch-Kolb (goblirsc@cern.ch)\n");
    printf("                    ");
    printf("             Johannes Junggeburth (jojungge@cern.ch) and\n");
    printf("                    ");
    printf("             Nicolas Koehler (nkoehler@cern.ch)\n");
    printf("                    ");
    printf("Copyright (c) 2016-2019, GNU General Public License\n");
    printf("                    ");
    printf("https://cern.ch\n\n");
    printf(
        "\x1b[0m"
        "\n");  // color reset
}

namespace XAMPP {
    XAMPPalgorithm::XAMPPalgorithm(const std::string& name, ISvcLocator* pSvcLocator) :
        AthAnalysisAlgorithm(name, pSvcLocator),
        m_systematics("SystematicsTool"),
        m_helper("AnalysisHelper"),
        m_RunCutFlow(true),
        m_init(false),
        m_tsw(),
        m_Events(0),
        m_CurrentEvent(0),
        m_printInterval(1.e3),
        m_TotSyst(0),
        m_updateTotEvents(false),
        m_TotFiles(0),
        m_CurrentFile(0) {
        declareProperty("AnalysisHelper", m_helper);
        declareProperty("SystematicsTool", m_systematics);
        declareProperty("RunCutFlow", m_RunCutFlow);
        declareProperty("nevents", m_Events);
        declareProperty("nfiles", m_TotFiles);
        declareProperty("printInterval", m_printInterval);
    }

    XAMPPalgorithm::~XAMPPalgorithm() {}

    StatusCode XAMPPalgorithm::initialize() {
        if (m_init) { return StatusCode::SUCCESS; }
        ATH_MSG_INFO("Initializing " << name() << "...");
        ATH_MSG_DEBUG("Initialize the analysis helper class");
        ATH_CHECK(m_helper.retrieve());
        ATH_CHECK(m_systematics.retrieve());
        ATH_MSG_DEBUG("The analysis helper must be initialized by the loop, it then will initialize the systematics tool.");
        ATH_CHECK(m_helper->initialize());

        if (m_systematics->GetKinematicSystematics().empty()) {
            ATH_MSG_FATAL("Have not found any iteration to run on, exiting...!");
            return StatusCode::FAILURE;
        }
        m_TotSyst = m_systematics->GetKinematicSystematics().size();
        m_init = true;
        m_CurrentEvent = 0;
        m_updateTotEvents = (m_Events == 0);
        m_tsw.Start();
        return StatusCode::SUCCESS;
    }

    StatusCode XAMPPalgorithm::finalize() {
        ATH_MSG_INFO("Finalizing " << name() << "...");
        m_tsw.Stop();
        CHECK(m_helper->finalize());
        return StatusCode::SUCCESS;
    }
    StatusCode XAMPPalgorithm::execute() {
        ATH_MSG_DEBUG("Executing " << name() << "...");
        if (!m_init) {
            ATH_MSG_ERROR("Algorithm not initialized");
            return StatusCode::FAILURE;
        }
        ++m_CurrentEvent;
        CHECK(ExecuteEvent());
        if (m_RunCutFlow) CHECK(CheckCutflow());
        if (m_printInterval > 0 && m_CurrentEvent % m_printInterval == 0) {
            double t2 = m_tsw.RealTime();
            long long int totEvents = m_Events;
            if (m_updateTotEvents) totEvents += (m_TotFiles - m_CurrentFile) * m_Events / m_CurrentFile;

            std::cout << "Entry " << m_CurrentEvent << " / " << totEvents << " (" << std::setprecision(2)
                      << (float)m_CurrentEvent / (float)totEvents * 100. << "%) in file " << m_CurrentFile << " / " << m_TotFiles
                      << std::setprecision(6) << " @ " << TimeHMS(t2);
            std::cout << ". Physics Event Rate: " << std::setprecision(3) << m_CurrentEvent / t2 << " Hz, Computing Event Rate: ";
            std::cout << std::setprecision(3) << m_CurrentEvent * m_TotSyst / t2
                      << " Hz, E.T.A.: " << TimeHMS(t2 * ((float)totEvents / (float)m_CurrentEvent - 1.));
            std::cout << " (updating screen each " << m_printInterval << " events)";
            std::cout << std::endl;
            m_tsw.Continue();
        }
        return StatusCode::SUCCESS;
    }

    StatusCode XAMPPalgorithm::ExecuteEvent() {
        ATH_MSG_DEBUG("Call beginEvent...");
        ATH_CHECK(m_helper->LoadContainers());
        ATH_MSG_DEBUG("ExecuteEvent()....");
        if (!m_helper->AcceptEvent()) {
            ATH_MSG_DEBUG("The event is discarded by the AnalysisHelper");
            return StatusCode::SUCCESS;
        }

        ATH_MSG_DEBUG("Check event cleaning...");
        if (!m_helper->EventCleaning()) {
            ATH_MSG_DEBUG("Event Failed the cleaning");
            return StatusCode::SUCCESS;
        }
        ATH_MSG_DEBUG("Check trigger...");
        if (!m_helper->CheckTrigger()) {
            ATH_MSG_DEBUG("Trigger failed");
            return StatusCode::SUCCESS;
        }
        for (const auto& current_syst : m_systematics->GetKinematicSystematics()) {
            ATH_MSG_DEBUG("Running kinematic systematic: " << current_syst->name() << ".");
            ATH_CHECK(m_systematics->resetSystematics());
            ATH_CHECK(m_systematics->setSystematic(current_syst));
            ATH_MSG_DEBUG("FillInitialObjects: ");
            ATH_CHECK(m_helper->FillInitialObjects(current_syst));
            ATH_MSG_DEBUG("RemoveOverlap: ");
            ATH_CHECK(m_helper->RemoveOverlap());
            ATH_MSG_DEBUG("FillObjects: ");
            ATH_CHECK(m_helper->FillObjects(current_syst));
            ATH_MSG_DEBUG("CleanObjects?");
            if (!m_helper->CleanObjects(current_syst)) {
                ATH_MSG_DEBUG("Found bad objects in the current systematic" << current_syst->name());
                continue;
            }
            ATH_MSG_DEBUG("Call FillEvent");
            ATH_CHECK(m_helper->FillEvent(current_syst));
        }
        return StatusCode::SUCCESS;
    }
    StatusCode XAMPPalgorithm::CheckCutflow() {
        if (!m_RunCutFlow) return StatusCode::SUCCESS;
        for (const auto& current_syst : m_systematics->GetKinematicSystematics()) { ATH_CHECK(m_helper->CheckCutFlow(current_syst)); }
        return StatusCode::SUCCESS;
    }
    std::string XAMPPalgorithm::TimeHMS(float t) const {
        std::stringstream ostr;
        ostr << std::setw(2) << std::setfill('0') << (int)((t / 60. / 60.)) % 24 << ":" << std::setw(2) << std::setfill('0')
             << ((int)(t / 60.)) % 60 << ":" << std::setw(2) << std::setfill('0') << ((int)t) % 60;
        return ostr.str();
    }
    StatusCode XAMPPalgorithm::beginInputFile() {
        const EventStreamInfo* esi = nullptr;
        ATH_CHECK(inputMetaStore()->retrieve(esi));
        if (m_updateTotEvents) m_Events += esi->getNumberOfEvents();
        ++m_CurrentFile;
        return StatusCode::SUCCESS;
    }

}  // namespace XAMPP
