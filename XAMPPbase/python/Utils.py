from ClusterSubmission.Utils import ResolvePath, ClearFromDuplicates
from ClusterSubmission.ClusterEngine import ATLASPROJECT, ATLASVERSION, TESTAREA
import os, commands
import math
import sys


def GetKinematicCutFromConfFile(Path, CutName):
    CutValue = -1
    with open(ResolvePath(Path)) as InFile:
        for line in InFile:
            if line.find(CutName) > -1:
                return float(line.replace(CutName + ":", "").strip())
    print "WARNING: Could not find the property %s in ST config file %s" % (CutName, Path)
    return CutValue


def GetPropertyFromConfFile(Path, CutName):
    with open(ResolvePath(Path)) as InFile:
        for line in InFile:
            line = line.strip()
            if line.find("#") > -1: line = line[:line.find("#")]
            if line.find(CutName) > -1:
                return line.replace(CutName + ":", "").strip()
    print "WARNING: Could not find the property %s in ST config file %s" % (CutName, Path)
    return ""


### This method augments to the trigger names the run numbers when
### they were not prescaled based on the information provided here
##     -- https://twiki.cern.ch/twiki/bin/viewauth/Atlas/LowestUnprescaled
##     -- https://atlas-tagservices.cern.ch/tagservices/RunBrowser/runBrowserReport/rBR_Period_Report.php
##     -- https://gitlab.cern.ch/atlas/athena/blob/21.2/PhysicsAnalysis/SUSYPhys/SUSYTools/Root/Trigger.cxx#L30
def getLowestUnPrescaled():
    return {
        ### 2015
        (266904, 284484): [
            "HLT_e24_lhmedium_L1EM20VH",
            "HLT_e60_lhmedium",
            "HLT_e120_lhloose",
            "HLT_mu20_iloose_L1MU15",
            "HLT_mu40",
            "HLT_mu60_0eta105_msonly",
            "HLT_xe70_mht",
        ],
        ###2016 period A
        (296939, 300287): [
            "HLT_mu24_iloose",
            "HLT_mu24_iloose_L1MU15",
            "HLT_mu24_ivarloose",
            "HLT_mu24_ivarloose_L1MU15",
            "HLT_mu40",
            "HLT_mu50",
        ],

        ### 2016 period A - D3
        (296939, 302872): [
            "HLT_e24_lhtight_nod0_ivarloose",
            "HLT_e60_lhmedium_nod0",
            "HLT_e60_medium",
            "HLT_e140_lhloose_nod0",
            "HLT_e300_etcut",
            "HLT_xe90_mht_L1XE50",
        ],
        ### 2016 period B to D3
        (300345, 302872): [
            "HLT_mu24_ivarmedium",
            "HLT_mu24_imedium",
            "HLT_mu50",
        ],
        ### 2016 period D4 to E
        (302919, 303892): ["HLT_mu24_ivarmedium", "HLT_mu24_imedium", "HLT_mu26_ivarmedium", "HLT_mu26_imedium", "HLT_mu50"],
        ### //2016 period F - G2
        (303943, 305379): ["HLT_mu26_ivarmedium", "HLT_mu26_imedium", "HLT_mu50"],
        ### //2016 period G3 to -
        (305380, 311481): [
            "HLT_mu26_ivarmedium",
            "HLT_mu50",
        ],

        ### 2016 period D4 -
        (302919, 311481):
        ["HLT_e26_lhtight_nod0_ivarloose", "HLT_e60_lhmedium_nod0", "HLT_e60_medium", "HLT_e140_lhloose_nod0", "HLT_e300_etcut"],
        ### 2016 D4-F1
        (302919, 303892): ["HLT_xe100_mht_L1XE50"],
        ### 2016 F2 -
        (303943, 311481): ["HLT_xe110_mht_L1XE50"],
        ### 2017 B1- D5
        (325713, 331975): ["HLT_xe110_pufit_L1XE55"],
        ### 2017 D6 -
        (332303, 341649): ["HLT_xe110_pufit_L1XE50"],

        ### 2017
        (324320, 341649): [
            "HLT_e26_lhtight_nod0_ivarloose", "HLT_e60_lhmedium_nod0", "HLT_e140_lhloose_nod0", "HLT_e300_etcut", "HLT_mu26_ivarmedium",
            "HLT_mu50", "HLT_mu60_0eta105_msonly"
        ],

        ##2018 A to run 349168
        (348197, 349168): ["HLT_e26_lhtight_nod0_ivarloose", "HLT_e60_lhmedium_nod0", "HLT_e140_lhloose_nod0", "HLT_e300_etcut"],
        ### 2018  run 349169 to -
        (349169, 364485):
        ["HLT_e26_lhtight_nod0_ivarloose", "HLT_e26_lhtight_nod0", "HLT_e60_lhmedium_nod0", "HLT_e140_lhloose_nod0", "HLT_e300_etcut"],

        ### 2018 B-C5
        (348885, 350013): ["HLT_xe110_pufit_xe70_L1XE50"],
        ## 2018 C5 -
        (350067, 364485): ["HLT_xe110_pufit_xe65_L1XE50"],
        ### 2018
        (348197, 364485): ["HLT_mu26_ivarmedium", "HLT_mu50", "HLT_mu60_0eta105_msonly"],
    }


def getUnPreScaledTrigger():
    triggers = []
    for trigs in getLowestUnPrescaled().itervalues():
        triggers += trigs
    return ClearFromDuplicates(constrainToPeriods(triggers))


def constrainToPeriods(trigger_list):
    unprescaled_periods = getLowestUnPrescaled()
    period_list = []
    for trig in trigger_list:
        new_trig = str(trig)
        for period, in_period in unprescaled_periods.iteritems():
            if trig in in_period: new_trig += ";%d-%d" % (period[0], period[1])

        period_list += [new_trig]

    return period_list


def IsArgumentDefault(Arg, Name, Parser):
    if Parser == None: return False
    return Arg == Parser.get_default(Name)


def StringToBool(Str):
    if Str.lower() == "true": return True
    return False


def BoolToString(Boolean):
    if Boolean == True: return "true"
    return "false"


def IsListIn(List1=[], List2=[]):
    for L in List1:
        EntryIn = False
        if len(List2) == 0:
            print "Reference list not found"
            return False
        for C in List2:
            if C.find(L.strip()) > -1:
                EntryIn = True
                break
        if EntryIn == False:
            print C + " not found in reference list"
            return False
    return True


def IsTextFile(FileName):
    FileFormats = [".txt", ".list", ".conf"]
    for end in FileFormats:
        if FileName.endswith(end): return True
    return False


def readXAMPPplottingInputConfig(config, sample=None):
    files = []
    with open(ResolvePath(config)) as cfile:
        for line in cfile:
            # Ignore comment lines and empty lines
            line = line.strip()
            if line.startswith('#'): continue
            if line.startswith('SampleName '):
                sample = line.replace('SampleName ', '').replace('\n', '')
            if line.startswith('Input '):
                files.extend(line.replace('Input ', '').replace('\n', '').split())
            if line.startswith("Import "):
                files.extend(readXAMPPplottingInputConfig(line.replace('Import', '').replace('\n', '').strip(), sample))
    return files
