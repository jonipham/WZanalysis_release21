from ClusterSubmission.Utils import ResolvePath
from ClusterSubmission.ClusterEngine import ATLASPROJECT, ATLASVERSION, TESTAREA
import os, commands
import math
import sys


"""def GetKinematicCutFromConfFile(Path, CutName):
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
"""


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


"""def readXAMPPplottingInputConfig(config, sample=None):
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
    return files """

