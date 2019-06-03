#ifndef XAMPPBASE_ParticleSelector_IXX
#define XAMPPBASE_ParticleSelector_IXX
#include <XAMPPbase/ISystematics.h>
#include <XAMPPbase/ParticleSelector.h>
#include <XAMPPbase/ToolHandleSystematics.h>

#include <PATInterfaces/SystematicSet.h>
#include <xAODBase/IParticleHelpers.h>

namespace XAMPP {
    template <typename Container> StatusCode ParticleSelector::ViewElementsContainer(const std::string& Key, Container*& Cont) {
        if (!isInitialized()) {
            ATH_MSG_ERROR("The tool is not initalized");
            return StatusCode::FAILURE;
        }
        if (Key.empty()) {
            ATH_MSG_ERROR("Empty keys are not allowed");
            return StatusCode::FAILURE;
        }
        ATH_MSG_DEBUG("Create new SG::VIEW_ELEMENTS container for " << Key);
        Cont = new Container(SG::VIEW_ELEMENTS);
        return evtStore()->record(Cont, Key + StoreName());
    }
    template <typename Container>
    StatusCode ParticleSelector::LoadViewElementsContainer(const std::string& Key, Container*& Cont, bool LoadNominal) const {
        if (!isInitialized()) {
            ATH_MSG_ERROR("The tool is not initalized");
            return StatusCode::FAILURE;
        }
        if (LoadNominal) {
            ATH_MSG_DEBUG("Load nominal SG::VIEW_ELEMENTS container " << Key);
            return LoadContainer(Key + name(), Cont);
        }
        return LoadContainer(Key + StoreName(), Cont);
    }
    template <typename Container> StatusCode ParticleSelector::LoadContainer(const std::string& Key, const Container*& Cont) const {
        if (!isInitialized()) {
            ATH_MSG_ERROR("The tool is not initalized");
            return StatusCode::FAILURE;
        }
        if (!evtStore()->contains<Container>(Key)) {
            ATH_MSG_ERROR("Could not find a container named " << Key);
            return StatusCode::FAILURE;
        }
        if (!evtStore()->retrieve(Cont, Key).isSuccess()) {
            ATH_MSG_ERROR("Could not retrieve " << Key << ", exiting..");
            return StatusCode::FAILURE;
        }

        return StatusCode::SUCCESS;
    }
    template <typename Container> StatusCode ParticleSelector::LoadContainer(const std::string& Key, Container*& Cont) const {
        if (!isInitialized() || !checkForValidSystematics()) {
            ATH_MSG_ERROR("The tool is not initalized");
            return StatusCode::FAILURE;
        }
        if (!evtStore()->contains<Container>(Key)) {
            ATH_MSG_ERROR("Could not find a container named " << Key);
            return StatusCode::FAILURE;
        }
        if (!evtStore()->retrieve(Cont, Key).isSuccess()) {
            ATH_MSG_ERROR("Could not retrieve " << Key << ", exiting..");
            return StatusCode::FAILURE;
        }

        return StatusCode::SUCCESS;
    }
    template <typename Container>
    ParticleSelector::LinkStatus ParticleSelector::CreateContainerLinks(const std::string& Key, Container*& Cont, bool linkOriginal) {
        xAOD::ShallowAuxContainer* AuxContainer = nullptr;
        return CreateContainerLinks(Key, Cont, AuxContainer, linkOriginal);
    }

    template <typename Container>
    ParticleSelector::LinkStatus ParticleSelector::CreateContainerLinks(const std::string& Key, Container*& Cont,
                                                                        xAOD::ShallowAuxContainer*& AuxContainer, bool linkOriginal) {
        if (!checkForValidSystematics()) return ParticleSelector::LinkStatus::Failed;
        std::string storeName = StoreName() + "_" + Key;
        std::string nominalStore = name() + "_" + Key;
        // All calibrated containers are stored in the event store
        // Check if the container is already stored
        if (evtStore()->contains<Container>(storeName)) {
            ATH_MSG_DEBUG("Container " << storeName << " is already in the StoreGate. Load this container instead");
            if (!LoadContainer(storeName, Cont).isSuccess() || !LoadContainer(storeName + "Aux.", AuxContainer).isSuccess())
                return ParticleSelector::LinkStatus::Failed;  // Failed to retrieve Container
            return ParticleSelector::LinkStatus::Loaded;      // The container is
                                                              // loaded from the
                                                              // TStore
        }
        // The systematic does not affect the kinematic selection and the
        // nominal container is already stored-> Why redo the callibration?
        else if (!SystematicAffects(m_ActSys) && evtStore()->contains<Container>(nominalStore)) {
            ATH_MSG_DEBUG("Current systemtatic " << SystName(false) << " does not affect container " << Key
                                                 << ". Load the nominal Container");
            if (!LoadContainer(nominalStore, Cont).isSuccess() || !LoadContainer(nominalStore + "Aux.", AuxContainer).isSuccess())
                return ParticleSelector::LinkStatus::Failed;  // Failed to
                                                              // retrieve
                                                              // Container
            return ParticleSelector::LinkStatus::Loaded;
        }
        // Create a new shallow Copy container incl. links
        const Container* constCont = nullptr;
        ATH_MSG_DEBUG("Create new ShallowCopy Container " << Key << " for systematic " << SystName(false));
        if (!LoadContainer(Key, constCont).isSuccess()) return ParticleSelector::LinkStatus::Failed;
        std::pair<Container*, xAOD::ShallowAuxContainer*> shallowcopy = xAOD::shallowCopyContainer(*constCont);
        Cont = shallowcopy.first;
        AuxContainer = shallowcopy.second;
        if (linkOriginal && !xAOD::setOriginalObjectLink(*constCont, *Cont)) {
            ATH_MSG_ERROR("Failed to set original object links on " << Key);
            return ParticleSelector::LinkStatus::Failed;
        }
        if (!evtStore()->record(Cont, storeName).isSuccess() || !evtStore()->record(AuxContainer, storeName + "Aux.").isSuccess()) {
            ATH_MSG_ERROR("Failed to parse the containers to the Store Gate");
            return ParticleSelector::LinkStatus::Failed;
        }
        return ParticleSelector::LinkStatus::Created;
    }
    template <typename T> StatusCode ParticleSelector::DeclareAsWeightSyst(ToolHandle<T>& handle, XAMPP::SelectionObject O) {
        ATH_CHECK(handle.retrieve());
        ToolHandleSystematics<T>* syst_handle =
            new ToolHandleSystematics<T>(handle, XAMPP::SelectionObject::Other, O != XAMPP::SelectionObject::Other ? O : ObjectType());
        ATH_CHECK(syst_handle->initialize());
        return StatusCode::SUCCESS;
    }
    template <typename T> StatusCode ParticleSelector::DeclareAsKineSyst(ToolHandle<T>& handle, XAMPP::SelectionObject O) {
        ATH_CHECK(handle.retrieve());
        ToolHandleSystematics<T>* syst_handle = new ToolHandleSystematics<T>(handle, O != XAMPP::SelectionObject::Other ? O : ObjectType());
        ATH_CHECK(syst_handle->initialize());
        return StatusCode::SUCCESS;
    }
    template <typename T> StatusCode ParticleSelector::DeclareAsWeightSyst(asg::AnaToolHandle<T>& handle, XAMPP::SelectionObject O) {
        ATH_CHECK(handle.retrieve());
        ToolHandleSystematics<T>* syst_handle =
            new ToolHandleSystematics<T>(handle, XAMPP::SelectionObject::Other, O != XAMPP::SelectionObject::Other ? O : ObjectType());
        ATH_CHECK(syst_handle->initialize());
        return StatusCode::SUCCESS;
    }
    template <typename T> StatusCode ParticleSelector::DeclareAsKineSyst(asg::AnaToolHandle<T>& handle, XAMPP::SelectionObject O) {
        ATH_CHECK(handle.retrieve());
        ToolHandleSystematics<T>* syst_handle = new ToolHandleSystematics<T>(handle, O != XAMPP::SelectionObject::Other ? O : ObjectType());
        ATH_CHECK(syst_handle->initialize());
        return StatusCode::SUCCESS;
    }
}  // namespace XAMPP
#endif
