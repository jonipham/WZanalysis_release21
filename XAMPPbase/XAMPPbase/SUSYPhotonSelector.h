#ifndef XAMPPbase_SUSYPhotonSelector_H
#define XAMPPbase_SUSYPhotonSelector_H

#include <XAMPPbase/IPhotonSelector.h>
#include <XAMPPbase/SUSYParticleSelector.h>

class IAsgPhotonEfficiencyCorrectionTool;
namespace XAMPP {
    class PhotonWeight;
    typedef ToolHandle<IAsgPhotonEfficiencyCorrectionTool> PhotonEffToolHandle;
    typedef std::shared_ptr<PhotonWeight> PhotonWeight_Ptr;
    typedef std::map<const CP::SystematicSet*, PhotonWeight_Ptr> PhotonWeightMap;

    class PhotonWeightHandler;
    typedef std::shared_ptr<PhotonWeightHandler> PhotonWeightHandler_Ptr;

    class SUSYPhotonSelector : public SUSYParticleSelector, virtual public IPhotonSelector {
    public:
        // Create a proper constructor for Athena
        ASG_TOOL_CLASS(SUSYPhotonSelector, XAMPP::IPhotonSelector)

        SUSYPhotonSelector(const std::string& myname);
        virtual ~SUSYPhotonSelector();

        virtual StatusCode initialize();

        virtual StatusCode LoadContainers();
        virtual StatusCode InitialFill(const CP::SystematicSet& systset);
        virtual StatusCode FillPhotons(const CP::SystematicSet& systset);

        virtual PhoLink GetLink(const xAOD::Photon& ph) const;
        virtual PhoLink GetOrigLink(const xAOD::Photon& ph) const;

        virtual xAOD::PhotonContainer* GetPhotons() const;
        virtual xAOD::PhotonContainer* GetPrePhotons() const;
        virtual const xAOD::PhotonContainer* GetPhotonContainer() const;
        virtual xAOD::PhotonContainer* GetSignalPhotons() const;
        virtual xAOD::PhotonContainer* GetBaselinePhotons() const;
        virtual xAOD::PhotonContainer* GetSignalNoORPhotons() const;

        virtual xAOD::PhotonContainer* GetCustomPhotons(const std::string& kind) const;

        virtual StatusCode SaveScaleFactor();
        virtual std::shared_ptr<PhotonDecorations> GetPhotonDecorations() const;

    protected:
        virtual StatusCode CallSUSYTools();
        virtual bool PassPreSelection(const xAOD::IParticle& P) const;
        virtual bool PassSignal(const xAOD::IParticle& P) const;
        virtual bool PassBaseline(const xAOD::IParticle& P) const;
        virtual void setupDecorations(std::shared_ptr<PhotonDecorations> input = nullptr);

    private:
        StatusCode initializeScaleFactors(const std::string& sf_type, PhotonEffToolHandle& sf_tool, PhotonWeightMap& map,
                                          unsigned int content);

        const xAOD::PhotonContainer* m_xAODPhotons;
        xAOD::PhotonContainer* m_Photons;
        xAOD::ShallowAuxContainer* m_PhotonsAux;

    protected:
        xAOD::PhotonContainer* m_PrePhotons;       // before OR
        xAOD::PhotonContainer* m_BaselinePhotons;  // after OR
        xAOD::PhotonContainer* m_SignalPhotons;
        xAOD::PhotonContainer* m_SignalQualPhotons;
        std::shared_ptr<PhotonDecorations> m_photonDecorations;

    private:
        bool m_SeparateSF;
        std::vector<PhotonWeightHandler_Ptr> m_SF;
        bool m_doRecoSF;
        bool m_doIsoSF;

        bool m_requireIso_PreSelection;
        bool m_requireIso_Baseline;
        bool m_requireIso_Signal;
        bool m_writeBaselineSF;
    };

    class PhotonWeightDecorator : public IPartilceWeightDecorator {
    public:
        PhotonWeightDecorator();
        virtual ~PhotonWeightDecorator();
        // Method to be called to store the SF per each Photon
        virtual StatusCode saveSF(const xAOD::Photon& Photon, bool isSignal);

    protected:
        virtual StatusCode calculateSF(const xAOD::Photon& Photon, double& SF) = 0;
    };
    class PhotonWeight : public PhotonWeightDecorator {
    public:
        PhotonWeight(PhotonEffToolHandle& SFTool);
        virtual ~PhotonWeight();

    protected:
        virtual StatusCode calculateSF(const xAOD::Photon& Photon, double& SF);

    private:
        PhotonEffToolHandle m_SFTool;
    };

    class PhotonWeightHandler : public PhotonWeightDecorator {
    public:
        PhotonWeightHandler(const CP::SystematicSet* syst_set);
        const CP::SystematicSet* systematic() const;

        virtual StatusCode saveSF(const xAOD::Photon& Photon, bool isSignal);
        virtual StatusCode applySF();

        bool append(const PhotonWeightMap& map, const CP::SystematicSet* nominal);

    protected:
        virtual StatusCode calculateSF(const xAOD::Photon& Photon, double& SF);

    private:
        const CP::SystematicSet* m_Syst;
        std::vector<PhotonWeight_Ptr> m_Weights;
        bool m_init;
    };
}  // namespace XAMPP
#endif
