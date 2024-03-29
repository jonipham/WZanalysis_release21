#include "GaudiKernel/DeclareFactoryEntries.h"

// Basic tools
#include <XAMPPbase/AnalysisConfig.h>
#include <XAMPPbase/EventInfo.h>
#include <XAMPPbase/MetaDataTree.h>
#include <XAMPPbase/ReconstructedParticles.h>
#include <XAMPPbase/SUSYSystematics.h>
#include <XAMPPbase/SUSYTriggerTool.h>

// Particle selectors
#include <XAMPPbase/SUSYElectronSelector.h>
#include <XAMPPbase/SUSYJetSelector.h>
#include <XAMPPbase/SUSYMuonSelector.h>
#include <XAMPPbase/SUSYPhotonSelector.h>
#include <XAMPPbase/SUSYTauSelector.h>
#include <XAMPPbase/SUSYTruthSelector.h>

#include <XAMPPbase/SUSYMetSelector.h>

// AnalysisHelpers
#include <XAMPPbase/SUSYAnalysisHelper.h>
#include <XAMPPbase/SUSYTruthAnalysisHelper.h>

#include "../XAMPPalgorithm.h"

// using namespace XAMPP;
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, AnalysisConfig)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, TruthAnalysisConfig)

DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, EventInfo)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, ReconstructedParticles)

DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, SUSYTriggerTool)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, SUSYSystematics)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, MetaDataTree)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, SUSYElectronSelector)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, SUSYMuonSelector)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, SUSYJetSelector)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, SUSYTauSelector)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, SUSYPhotonSelector)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, SUSYTruthSelector)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, SUSYMetSelector)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, SUSYAnalysisHelper)
DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, SUSYTruthAnalysisHelper)

DECLARE_NAMESPACE_ALGORITHM_FACTORY(XAMPP, XAMPPalgorithm)

DECLARE_FACTORY_ENTRIES(XAMPPbase) {
    DECLARE_NAMESPACE_TOOL(XAMPP, AnalysisConfig)
    DECLARE_NAMESPACE_TOOL(XAMPP, TruthAnalysisConfig)
    DECLARE_NAMESPACE_TOOL(XAMPP, EventInfo)
    DECLARE_NAMESPACE_TOOL(XAMPP, ReconstructedParticles)

    DECLARE_NAMESPACE_TOOL(XAMPP, SUSYTriggerTool)
    DECLARE_NAMESPACE_TOOL(XAMPP, SUSYSystematics)
    DECLARE_NAMESPACE_TOOL(XAMPP, MetaDataTree)
    DECLARE_NAMESPACE_TOOL(XAMPP, SUSYElectronSelector)
    DECLARE_NAMESPACE_TOOL(XAMPP, SUSYMuonSelector)
    DECLARE_NAMESPACE_TOOL(XAMPP, SUSYJetSelector)
    DECLARE_NAMESPACE_TOOL(XAMPP, SUSYTauSelector)
    DECLARE_NAMESPACE_TOOL(XAMPP, SUSYPhotonSelector)
    DECLARE_NAMESPACE_TOOL(XAMPP, SUSYTruthSelector)
    DECLARE_NAMESPACE_TOOL(XAMPP, SUSYMetSelector)
    DECLARE_NAMESPACE_TOOL(XAMPP, SUSYAnalysisHelper)
    DECLARE_NAMESPACE_TOOL(XAMPP, SUSYTruthAnalysisHelper)
    DECLARE_NAMESPACE_ALGORITHM(XAMPP, XAMPPalgorithm)
}
