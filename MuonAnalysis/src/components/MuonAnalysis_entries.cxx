
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

#include "../MuonAnalysisAlg.h"

//DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, EventInfo)
//DECLARE_NAMESPACE_TOOL_FACTORY(XAMPP, MuonAnalysisAlg)
DECLARE_NAMESPACE_ALGORITHM_FACTORY(XAMPP, MuonAnalysisAlg)
//DECLARE_ALGORITHM_FACTORY( MuonAnalysisAlg )


DECLARE_FACTORY_ENTRIES( MuonAnalysis )
{
  //DECLARE_ALGORITHM( MuonAnalysisAlg );
  DECLARE_NAMESPACE_ALGORITHM(XAMPP, MuonAnalysisAlg)
}
