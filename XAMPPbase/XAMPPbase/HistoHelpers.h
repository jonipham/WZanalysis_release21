#ifndef XAMPPbase_HistoHelpers_H
#define XAMPPbase_HistoHelpers_H

#include <XAMPPbase/Defs.h>
#include <xAODBase/IParticleContainer.h>

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <TH1.h>

namespace XAMPP {
    template <class T> class Storage;
    class ParticleStorage;
    class HistoBase;
    class CutFlow;
    class IHistoVariable;
    typedef std::shared_ptr<IHistoVariable> IHistoVariable_Ptr;

    class HistoLib {
    public:
        enum LibType { Raw, Weighted };
        HistoLib(IHistoVariable* Base, LibType T);
        bool Finalize();
        bool FillBin(int Bin);
        ~HistoLib();

    private:
        TH1* GetHisto();
        void SetNbins();
        int nBins() const;
        void Finalize(TH1* TH1);
        IHistoVariable* m_Ref;
        HistoLib::LibType m_Type;
        struct LibEntry {
            LibEntry(TH1* Hist) {
                H = Hist;
                N = 0;
                Finalized = false;
            }
            TH1* H;
            unsigned int N;
            bool Finalized;
        };
        std::map<unsigned int, LibEntry> m_Library;
        std::map<unsigned int, LibEntry>::iterator m_Book;
        std::map<unsigned int, LibEntry>::iterator m_End;
        int m_Nbins;
    };
    class IHistoVariable {
    public:
        virtual bool Fill() = 0;
        virtual TH1* GetTemplate() const = 0;
        virtual bool FillBin(int Bin);
        bool WriteHisto(TH1* Histo, HistoLib::LibType T);
        bool Finalize();

        void SetName(const std::string& Name);
        void SetPath(const std::string& Path);

        std::string name() const;
        std::string basepath() const;
        std::string path() const;
        unsigned int Identifier() const;
        double W() const;
        double W2() const;

        virtual ~IHistoVariable();

    protected:
        IHistoVariable(HistoBase* Base, const CutFlow* Flow, const std::string& Name);
        IHistoVariable(HistoBase* Base, const CutFlow* Flow);
        void UpdateWeight();

    private:
        HistoBase* m_Base;
        const CutFlow* m_CutFlow;
        std::string m_Name;
        std::string m_Path;
        HistoLib m_RawLib;
        HistoLib m_WeightLib;
    };

    class CutFlowHistoVariable : public IHistoVariable {
    public:
        virtual bool Fill();
        virtual TH1* GetTemplate() const;
        virtual bool FillBin(int Bin);

        CutFlowHistoVariable(HistoBase* Base, const CutFlow* Flow);
        virtual ~CutFlowHistoVariable();
        bool InitializeCutFlow(const std::vector<std::string>& CutNames);

    private:
        TH1* m_Template;
    };
    template <class T> class EventHistoVariable : public IHistoVariable {
    public:
        EventHistoVariable(XAMPP::Storage<T>* Store, HistoBase* Base, const CutFlow* Flow, const std::string& Name);
        virtual bool Fill();
        virtual TH1* GetTemplate() const;

    private:
        XAMPP::Storage<T>* m_Store;
        TH1* m_Template;
    };
}  // namespace XAMPP
#include <XAMPPbase/HistoHelpers.ixx>
#endif
