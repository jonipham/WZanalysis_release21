#! /usr/bin/env python
import ROOT
import sys
import argparse
import pprint
import math
from ClusterSubmission.Utils import ReadListFromFile, ResolvePath, IsROOTFile
from XAMPPbase.Utils import IsTextFile, readXAMPPplottingInputConfig


# this function returns a dictionnary with MetaData information of the form:
# { mcChannelNumber : [initialEntries, initialWeightedEntries, xSecTimesEff] }
# for Data: mcChannelNumber = initialWeightedEntries = xSecTimesEff = -1
class MetaDataElement(object):
    def __init__(self, isData=False, DSID=-1, xSec=-1, runNumber=-1, procID=0):
        self.__isData = isData
        self.__DSID = DSID
        self.__xSec = xSec
        self.__runNumber = runNumber
        self.__procID = procID
        self.__TotalEvents = 0
        self.__ProcessedEvents = 0.
        self.__SumW = 0.

    def isData(self):
        return self.__isData

    def Add(self, TotalEvents=0, ProcessedEvents=0, SumW=0):
        self.__TotalEvents += TotalEvents
        self.__ProcessedEvents += ProcessedEvents
        self.__SumW += SumW

    def DSID(self):
        return self.__DSID

    def procID(self):
        return self.__procID

    def runNumber(self):
        return self.__runNumber

    def SumW(self):
        return self.__SumW

    def TotalEvents(self):
        return self.__TotalEvents

    def ProcessedEvents(self):
        return self.__ProcessedEvents

    def xSection(self):
        return self.__xSec


class MetaDataDict(object):
    def __init__(self):
        self.__Dict = []

    def findMCSample(self, DSID, procID=0):
        for S in self.__Dict:
            if not S.isData() and S.DSID() == DSID and S.procID() == procID:
                return S
        return None

    def findRun(self, run):
        for S in self.__Dict:
            if S.isData() and S.runNumber() == run: return S
        return None

    def append(self, Element):
        Fetched_Element = self.findRun(
            run=Element.runNumber()) if Element.isData() else self.findMCSample(DSID=Element.DSID(), procID=Element.procID())
        if Fetched_Element:
            Fetched_Element.Add(TotalEvents=Element.TotalEvents(), ProcessedEvents=Element.ProcessedEvents(), SumW=Element.SumW())

        else:
            self.__Dict += [Element]

    def getDict(self):
        return self.__Dict

    def hasData(self):
        return len([X for X in self.getDict() if X.isData()]) > 0

    def hasMC(self):
        return len([X for X in self.getDict() if not X.isData()]) > 0


def getMetaData(MDtree):
    print 'Extracting MetaData information...'
    MetaData = MetaDataDict()
    Nentries = MDtree.GetEntries()
    for ientry in range(0, Nentries):
        MDtree.GetEntry(ientry)
        ### processIDs greater than 1000 correspond to LHE variations. They are not of special interest for the
        ### cutflow comparison
        if not MDtree.isData and MDtree.ProcessID > 1000: continue
        MetaEntry = MetaDataElement(isData=MDtree.isData,
                                    runNumber=MDtree.runNumber,
                                    DSID=-1 if MDtree.isData else MDtree.mcChannelNumber,
                                    xSec=-1 if MDtree.isData else MDtree.xSection * MDtree.kFactor * MDtree.FilterEfficiency,
                                    procID=0 if MDtree.isData else MDtree.ProcessID)
        MetaEntry.Add(TotalEvents=MDtree.TotalEvents, ProcessedEvents=MDtree.ProcessedEvents, SumW=0 if MDtree.isData else MDtree.TotalSumW)
        MetaData.append(MetaEntry)

    return MetaData


def readMetaData(ROOTFiles):
    TotalMetaData = MetaDataDict()
    for TheFile in ROOTFiles:
        MetaData = getMetaData(TheFile.Get("MetaDataTree"))
        for M in MetaData.getDict():
            TotalMetaData.append(M)

    return TotalMetaData


# this function returns the CutFlow histogram 'HistoName' from the file 'File'
def getCutFlow(File, HistoName):
    if not File.Get(HistoName): return None
    else: CutFlowHisto = File.Get(HistoName)
    CutFlowHisto.SetDirectory(0)
    CutFlowHisto.AddDirectory(ROOT.kFALSE)
    return CutFlowHisto


def getCutFlowsFromFile(ROOTFiles, HistoName):
    CFHisto = None
    for File in ROOTFiles:
        CutFlow = getCutFlow(File, HistoName)
        if not CFHisto: CFHisto = CutFlow
        elif CutFlow: CFHisto.Add(CutFlow)
    return CFHisto


# this functions prints out the CutFlowHisto
def printCutFlow(CutFlowHisto, xSecWeight=1., doRaw=False, systematic='', lumi=1.):
    Nbins = CutFlowHisto.GetNbinsX()
    labellengths = []
    for ibin in range(1, Nbins + 1):
        labellengths.append(len(CutFlowHisto.GetXaxis().GetBinLabel(ibin)) + 2)
    prettyPrint("CutFlowHisto", CutFlowHisto.GetName(), "", width1=max(labellengths))
    prettyPrint("Systematic variation", systematic, "", width1=max(labellengths))
    separatorString = ""
    for i in range(max(labellengths) + max(len(CutFlowHisto.GetName()), len(systematic))):
        separatorString += "#"
    print separatorString
    prettyPrint("Cut", "Yields", "", width1=max(labellengths))
    print separatorString
    width2 = 0
    for ibin in range(1, Nbins + 1):
        thecontent = CutFlowHisto.GetBinContent(ibin)
        theerror = CutFlowHisto.GetBinError(ibin)
        thelabel = CutFlowHisto.GetXaxis().GetBinLabel(ibin)
        if thecontent == 0:
            break
        if doRaw:
            content = '%.0f ' % (thecontent * xSecWeight)
            error = ' %.2f' % (theerror * xSecWeight)
        else:
            content = '%.8f ' % (thecontent * xSecWeight * lumi)
            error = ' %.8f' % (theerror * xSecWeight * lumi)
        if ibin == 1:
            width2 = len(content)
        prettyPrint(thelabel, content, error, width1=max(labellengths), width2=width2, separator2=(u'\u00B1').encode('utf-8'))
    print separatorString


# this function prints uniformly-formatted lines of the type 'preamble : data1 data2' (if separator1=':')
def prettyPrint(preamble, data1, data2, width1=24, width2=10, separator1="", separator2=""):
    preamble = preamble.ljust(width1)
    data1 = data1.ljust(width2)
    print(preamble + separator1 + data1 + separator2 + data2)


def OpenFiles(MyList):
    ROOTFiles = []
    for Entry in MyList:
        if IsROOTFile(Entry): ROOTFiles.append(ROOT.TFile.Open(Entry))
        elif IsTextFile(Entry):
            #### Adapt for the possibility that someone passes a XAMPPplotting config
            if Entry.endswith(".conf"):
                ROOTFiles += [ROOT.TFile.Open(File) for File in readXAMPPplottingInputConfig(Entry)]
            else:
                ROOTFiles += [ROOT.TFile.Open(Line) for Line in ReadListFromFile(Entry)]
    return ROOTFiles


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='printCutFlow', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-i', '--inputFile', help='choose input ROOT file', default=['AnalysisOutput.root'], nargs="+", type=str)
    parser.add_argument('-a', '--analysis', help='choose CutFlow to be printed', default='MyCutFlow', required=True)
    parser.add_argument('--weighted', help='use for weighted cutflow comparisons', action='store_true', default=False)
    parser.add_argument('--weightedNoXSec', help='use for weighted cutflow comparisons', action='store_true', default=False)
    parser.add_argument('-l',
                        '--lumi',
                        help='specify a luminosity (in pb^-1) for which the weighted CutFlow shall be printed',
                        default=1.,
                        type=float)
    parser.add_argument('-s',
                        '--systematic',
                        '--syst',
                        help='specify a systematic uncertainty for which the CutFlow shall be printed',
                        default='Nominal')
    parser.add_argument("--sumUpData", help="Sums up all cutflows together for data.", action="store_true", default=False)
    RunOptions = parser.parse_args()

    # Set batch mode and atlas style
    ROOT.gROOT.SetBatch(True)

    #### Open all the ROOTFiles
    TheFiles = OpenFiles(RunOptions.inputFile)
    MetaData = readMetaData(TheFiles)

    if MetaData.hasData():
        print 'Having found CutFlow for Data, printing...'
        SummedHisto = None
        for M in MetaData.getDict():
            if not M.isData(): continue
            HistoName = "Histos_%s_Nominal/InfoHistograms/DSID_%s_CutFlow" % (RunOptions.analysis, M.runNumber())
            CutFlowHisto = getCutFlowsFromFile(TheFiles, HistoName)
            if not RunOptions.sumUpData:
                if not CutFlowHisto:
                    print "ERROR: Could not find cutflow histo %s" % (HistoName)
                    sys.exit(1)
                else:
                    printCutFlow(CutFlowHisto, doRaw=True)

            elif not SummedHisto:
                SummedHisto = CutFlowHisto
            elif CutFlowHisto:
                SummedHisto.Add(CutFlowHisto)

        if RunOptions.sumUpData:
            printCutFlow(SummedHisto, doRaw=True)

    if MetaData.hasMC():
        print 'Having found CutFlow for MC, printing...'
        for M in MetaData.getDict():
            if M.isData(): continue

            if RunOptions.weighted:
                HistoName = "Histos_%s_%s/InfoHistograms/DSID_%s_CutFlow_weighted" % (RunOptions.analysis, RunOptions.systematic, M.DSID())
                CutFlowHisto = getCutFlowsFromFile(TheFiles, HistoName)
                xSecWeight = 0.
                DSIDs = [X for X in MetaData.getDict() if X.DSID() == M.DSID()] if M.procID() == 0 else [M]
                ## Remove the element itself if we've more thanone proccess
                if len(DSIDs) > 1:
                    DSIDs = [X for X in DSIDs if X.procID() != 0]
                ### the xsection is given in pb
                xSecWeight = 1.e3 * (sum([X.xSection() for X in DSIDs]) if not RunOptions.weightedNoXSec else 1.) / M.SumW()

                if not RunOptions.weightedNoXSec:
                    print 'Printing weighted events (with xSection weight %s)' % str(xSecWeight)
                else:
                    print 'Printing weighted events (without xSection weight)'
                printCutFlow(CutFlowHisto, xSecWeight=xSecWeight, systematic=RunOptions.systematic, lumi=RunOptions.lumi)
            else:
                HistoName = "Histos_%s_%s/InfoHistograms/DSID_%s_CutFlow" % (RunOptions.analysis, RunOptions.systematic, M.DSID())
                CutFlowHisto = getCutFlowsFromFile(TheFiles, HistoName)
                print 'Printing raw events'
                printCutFlow(CutFlowHisto, doRaw=True, systematic=RunOptions.systematic)
