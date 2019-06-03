#include <XAMPPbase/AnalysisUtils.h>

#include <XAMPPbase/AnalysisConfig.h>
#include <XAMPPbase/Cuts.h>
#include <XAMPPbase/EventStorage.h>
#include <XAMPPbase/HistoBase.h>
#include <XAMPPbase/HistoHelpers.h>
#include <iostream>
#include <vector>

#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
namespace XAMPP {
    //####################################################################################################
    //                                      HistoLib
    //####################################################################################################
    HistoLib::HistoLib(IHistoVariable* Base, LibType T) :
        m_Ref(Base),
        m_Type(T),
        m_Library(),
        m_Book(m_Library.end()),
        m_End(m_Library.end()),
        m_Nbins(-1) {}
    TH1* HistoLib::GetHisto() {
        if (m_Book == m_End || m_Book->first != m_Ref->Identifier()) {
            // Updae the curent Histo
            m_Book = m_Library.find(m_Ref->Identifier());
            if (m_Book == m_End) {
                // Create a copy from the template
                if (!m_Ref->GetTemplate()) {
                    Error("HistoLib::GetHisto()", "Invalid template");
                    return nullptr;
                }
                std::string CloneName = "CloneOf" + std::to_string(m_Ref->Identifier()) + "_" +
                                        std::string(m_Ref->GetTemplate()->GetName()) + "_" + m_Ref->name();
                TH1* Histo = dynamic_cast<TH1*>(m_Ref->GetTemplate()->Clone(CloneName.c_str()));
                if (!m_Ref->WriteHisto(Histo, m_Type)) {
                    if (Histo) delete Histo;
                    return nullptr;
                }
                m_Library.insert(std::pair<unsigned int, LibEntry>(m_Ref->Identifier(), LibEntry(Histo)));
                m_End = m_Library.end();
                SetNbins();
                return Histo;
            }
        }
        return m_Book->second.H;
    }
    void HistoLib::SetNbins() {
        if (m_Nbins > -1) return;
        if (!GetHisto()) return;
        m_Nbins = GetHisto()->GetNbinsX() + 2;
        if (dynamic_cast<TH2*>(GetHisto())) m_Nbins *= GetHisto()->GetNbinsY() + 2;
        if (dynamic_cast<TH3*>(GetHisto())) m_Nbins *= GetHisto()->GetNbinsZ() + 2;
    }
    bool HistoLib::FillBin(int Bin) {
        if (!GetHisto()) { return false; }
        if (Bin < 0 || Bin > nBins()) {
            Error("HistoLib::FillBin()", "Invalid bin %i", Bin);
            return false;
        }
        double Content = GetHisto()->GetBinContent(Bin) + (m_Type == LibType::Raw ? 1 : m_Ref->W());
        double Error = GetHisto()->GetBinError(Bin) + (m_Type == LibType::Raw ? 1 : m_Ref->W2());
        GetHisto()->SetBinContent(Bin, Content);
        GetHisto()->SetBinError(Bin, Error);
        m_Book->second.N += 1;
        return true;
    }

    bool HistoLib::Finalize() {
        for (auto& Entry : m_Library) {
            if (Entry.second.Finalized) continue;
            Finalize(Entry.second.H);
            Entry.second.H->SetEntries(Entry.second.N);
            Entry.second.Finalized = true;
        }
        return true;
    }
    int HistoLib::nBins() const { return m_Nbins; }
    HistoLib::~HistoLib() {}
    void HistoLib::Finalize(TH1* TH1) {
        double wX(1.), wY(1.), wZ(1.);
        int Nx = TH1->GetNbinsX();
        int Ny = TH1->GetDimension() > 1 ? TH1->GetNbinsY() : 0;
        int Nz = TH1->GetDimension() > 2 ? TH1->GetNbinsZ() : 0;
        for (int nX = Nx; nX >= 0; --nX) {
            wX = TH1->GetXaxis()->GetBinWidth(1) / TH1->GetXaxis()->GetBinWidth(nX);
            for (int nY = Ny; nY >= 0; --nY) {
                wY = TH1->GetYaxis()->GetBinWidth(1) / TH1->GetYaxis()->GetBinWidth(nY);
                for (int nZ = Nz; nZ >= 0; --nZ) {
                    wZ = TH1->GetZaxis()->GetBinWidth(1) / TH1->GetZaxis()->GetBinWidth(nZ);
                    int Bin = TH1->GetBin(nX, nY, nZ);
                    TH1->SetBinContent(Bin, TH1->GetBinContent(Bin) * wX * wY * wZ);
                    TH1->SetBinError(Bin, TMath::Sqrt(TH1->GetBinError(Bin)) * wX * wY * wZ);
                }
            }
        }
    }
    //##########################################################################################################################
    //                                          IHistoVariable
    //##########################################################################################################################
    IHistoVariable::IHistoVariable(HistoBase* Base, const CutFlow* Flow) :
        m_Base(Base),
        m_CutFlow(Flow),
        m_Name(),
        m_Path(),
        m_RawLib(this, HistoLib::LibType::Raw),
        m_WeightLib(this, HistoLib::LibType::Weighted) {}
    IHistoVariable::IHistoVariable(HistoBase* Base, const CutFlow* Flow, const std::string& Name) : IHistoVariable(Base, Flow) {
        SetName(Name);
    }
    void IHistoVariable::SetName(const std::string& Name) {
        if (!name().empty())
            Error("IHistoVariable::SetName()", "The variable has already a name %s ", name().c_str());
        else if (Name.empty())
            Error("IHistoVariable::SetPath()", "Empty name given");
        else
            m_Name = Name;
    }
    void IHistoVariable::SetPath(const std::string& Path) {
        if (!path().empty())
            Error("IHistoVariable::SetPath()", "%s has already a path %s ", name().c_str(), path().c_str());
        else if (Path.empty())
            Error("IHistoVariable::SetPath()", "Empty path given to %s", name().c_str());
        else
            m_Path = Path;
    }
    IHistoVariable::~IHistoVariable() {}

    bool IHistoVariable::FillBin(int Bin) {
        if (!m_RawLib.FillBin(Bin)) {
            Error("IHistoVariable::FillBin()",
                  "Could not fill histogram %s for systematic %s in analyisis "
                  "region %s",
                  name().c_str(), m_Base->name().c_str(), m_CutFlow->name().c_str());
            return false;
        }
        if (m_Base->isData()) { return true; }
        if (!m_WeightLib.FillBin(Bin)) { return false; }
        return true;
    }
    bool IHistoVariable::Finalize() { return m_RawLib.Finalize() && m_WeightLib.Finalize(); }
    double IHistoVariable::W() const { return m_Base->GetWeight(); }
    double IHistoVariable::W2() const { return m_Base->GetWeight2(); }
    std::string IHistoVariable::name() const { return m_Name; }
    std::string IHistoVariable::path() const { return m_Path; }
    std::string IHistoVariable::basepath() const { return "Histos_" + m_CutFlow->name() + "_" + m_Base->name() + "/"; }
    unsigned int IHistoVariable::Identifier() const { return m_Base->Identifier(); }
    bool IHistoVariable::WriteHisto(TH1* Histo, HistoLib::LibType T) {
        if (!Histo) {
            Error("IHistoVariable::WriteHisto()", "No histo has been given");
            return false;
        }
        if (name().empty()) {
            Error("IHistoVariable::WriteHisto()", "The variable has no name?!");
            return false;
        }
        std::string OutName = basepath() + path() + "DSID_" + std::to_string(Identifier()) + "_" + name() +
                              (T == HistoLib::LibType::Weighted ? "_weighted" : "");
        return m_Base->HistoInOutput(Histo, OutName);
    }
    void IHistoVariable::UpdateWeight() { m_Base->UpdateWeight(); }

    //##########################################################################################################################
    //                                          CutFlowHistoVariable
    //##########################################################################################################################
    CutFlowHistoVariable::CutFlowHistoVariable(HistoBase* Base, const CutFlow* Flow) :
        IHistoVariable(Base, Flow, "CutFlow"),
        m_Template(nullptr) {
        SetPath("InfoHistograms/");
    }
    CutFlowHistoVariable::~CutFlowHistoVariable() {
        if (m_Template) delete m_Template;
    }
    bool CutFlowHistoVariable::Fill() { return false; }
    TH1* CutFlowHistoVariable::GetTemplate() const { return m_Template; }
    bool CutFlowHistoVariable::FillBin(int Bin) {
        UpdateWeight();
        for (int i = 1; i <= Bin; ++i) {
            if (!IHistoVariable::FillBin(i)) { return false; }
        }
        return true;
    }
    bool CutFlowHistoVariable::InitializeCutFlow(const std::vector<std::string>& CutNames) {
        if (CutNames.empty()) {
            Error("CutFlowHistoVariable::InitializeCutFlow()", "No cuts were given");
            return false;
        }
        int nBins = CutNames.size() + 2;
        m_Template = new TH1D(name().c_str(), name().c_str(), nBins, 0, nBins);
        m_Template->SetDirectory(0);
        m_Template->LabelsDeflate("X");
        m_Template->GetXaxis()->SetBinLabel(1, "Initial");
        int Bin = 2;
        for (auto& Cut : CutNames) {
            m_Template->GetXaxis()->SetBinLabel(Bin, Cut.c_str());
            ++Bin;
        }
        m_Template->GetXaxis()->SetBinLabel(Bin, "Final");
        return true;
    }

}  // namespace XAMPP
