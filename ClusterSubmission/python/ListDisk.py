#! /usr/bin/env python
from ClusterSubmission.Utils import CheckRemainingProxyTime, CheckRucioSetup, CreateDirectory
import os, commands, sys, argparse, time

RUCIO_ACCOUNT = os.getenv("RUCIO_ACCOUNT")
RSE = os.getenv("RUCIO_RSE") if os.getenv("RUCIO_RSE") else ""


def ListDisk(RSE="MPPMU_PERF-MUONS"):
    print "Read content of " + RSE
    OnDisk = commands.getoutput("rucio list-datasets-rse " + RSE)
    MyDataSets = []
    for Candidates in OnDisk.split("\n"):
        MyDataSets.append(Candidates)
    return MyDataSets


def ListDiskWithSize(RSE="MPPMU_PERF-MUONS"):
    print "Read content of %s and also save the size of each dataset" % (RSE)
    OnDisk = commands.getoutput("rucio list-datasets-rse %s --long" % (RSE)).split("\n")
    MyDS = []
    for Candidates in OnDisk:
        try:
            DS = Candidates.split("|")[1].strip()
            Size = Candidates.split("|")[3].strip()
            Stored = float(Size[:Size.find("/")]) / 1024 / 1024 / 1024
            TotalSize = float(Size[Size.find("/") + 1:]) / 1024 / 1024 / 1024
        except:
            continue
        #print DS, Stored, TotalSize
        MyDS.append((DS, TotalSize))
    return sorted(MyDS, key=lambda size: size[1], reverse=True)


def GetDataSetInfo(DS, RSE="MPPMU_PERF-MUONS", Subscriber=None):
    Cmd = "rucio list-rules %s " % (DS)
    Rules = commands.getoutput(Cmd).split("\n")
    for i in range(2, len(Rules)):
        Rule = Rules[i]
        try:
            ID = Rule.split()[0].strip()
            Owner = Rule.split()[1].strip()
            RuleRSE = Rule.split()[4].strip()
            if RuleRSE == RSE and (Subscriber == None or Subscriber == Owner): return ID, Owner
        except:
            continue
    return None, None


def GetUserRules(user=RUCIO_ACCOUNT):
    OnDisk = commands.getoutput("rucio list-rules --account %s" % user)
    MyRules = []
    for Rule in OnDisk.split("\n"):
        try:
            ID = Rule.split()[0].strip()
            DataSet = Rule.split()[2].strip()
            Rule_RSE = Rule.split()[4].strip()
        except:
            continue
        MyRules.append((ID, DataSet, Rule_RSE))
    return MyRules


def ListUserRequests(RSE="MPPMU_PERF-MUONS", user=RUCIO_ACCOUNT):
    print "List requests of user %s at %s" % (user, RSE)
    AllRules = GetUserRules(user)
    MyDataSets = []
    for Candidates in AllRules:
        ID = Candidates[0]
        DataSet = Candidates[1]
        Rule_RSE = Candidates[2]
        if not RSE == Rule_RSE: continue
        MyDataSets.append(DataSet)
    return MyDataSets


def GetDataSetReplicas(DS):
    RSE = commands.getoutput("rucio list-rses").split("\n")
    Replicas = []
    for line in commands.getoutput("rucio list-dataset-replicas " + DS).split("\n"):
        line = line.strip()
        if line.startswith("|"): line = line[1:]
        else: continue
        Candidate = line.split("|")[0].strip()
        if Candidate in RSE: Replicas.append(Candidate)
    return Replicas


def getRSEs():
    Cmd = "rucio list-rses"
    return commands.getoutput(Cmd).split()


if __name__ == '__main__':

    CheckRucioSetup()
    CheckRemainingProxyTime()

    OutDir = os.getcwd()

    parser = argparse.ArgumentParser(
        description='This script lists datasets located at a RSE location. Futher patterns to find or exclude can be specified.',
        prog='ListDisk',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-p', '-P', '--pattern', help='specify a pattern which is part of dataset name', nargs='+', default=[])
    parser.add_argument('-e', '-E', '--exclude', help='specify a pattern which must not be part of dataset name', nargs='+', default=[])
    parser.add_argument('-o', '-O', '--OutDir', help='specify output directory', default=OutDir)
    parser.add_argument('-r', '-R', '--RSE', '--rse', help='specify a RSE', default=RSE)
    parser.add_argument('--MyRequests', help='list datasets which you requested yourself', action='store_true', default=False)
    parser.add_argument('--rucio', help='Which rucio account shall be used for MyRequests', default=RUCIO_ACCOUNT)

    RunOptions = parser.parse_args()

    Today = time.strftime("%Y-%m-%d")
    Patterns = RunOptions.pattern
    OutDir = RunOptions.OutDir
    RSE = RunOptions.RSE
    if ',' in RSE: RSE = RSE.split(',')[0]  # in case people have more than one RSE in their environment variable for grid submits

    Prefix = ''
    if RunOptions.MyRequests:
        Prefix = 'MyRequestTo_'
        DS = ListUserRequests(RSE, RunOptions.rucio)
    else:
        DS = ListDisk(RSE)

    #    MetaFile = open("Content_%s.txt"%(RSE), 'w')
    #    for DataSet, Size in ListDiskWithSize(RSE):
    #        Owner, ID = GetDataSetInfo(DataSet,RSE)
    #        line = "%s  |   %s   | %s  | %.2f GB"%(ID, Owner,DataSet, Size)
    #        MetaFile.write("%s\n"%(line))
    #        print line
    #    MetaFile.close()
    #    exit(0)

    if len(DS) == 0:
        print "INFO: Disk is empty."
        exit(0)
    CreateDirectory(OutDir, False)

    ###########
    #   Define the file list name
    ###########
    FileList = "%s%s_%s" % (Prefix, RSE, Today)
    if len(Patterns) > 0: FileList += "_%s" % ('_'.join(Patterns))
    if len(RunOptions.exclude) > 0: FileList += "_exl_%s" % ('_'.join(RunOptions.exclude))
    FileList += '.txt'
    Write = []
    for d in sorted(DS):
        allPatternsFound = True
        for Pattern in Patterns:
            if not Pattern in d:
                allPatternsFound = False
                break
        for Pattern in RunOptions.exclude:
            if Pattern in d:
                allPatternsFound = False
                break
        if allPatternsFound:
            IsInWrite = False
            if d.split(".")[-1].isdigit(): d = d[:d.rfind(".")]
            if d.find("_tid") != -1: d = d[0:d.rfind("_tid")]
            for w in Write:
                if d in w:
                    IsInWrite = True
                    break
            if IsInWrite == True: continue
            print "INFO: Write dataset %s" % (d)
            Write.append(d)
    if len(Write) == 0:
        print "No datasets containing given pattern(s) found!"
        exit(0)
    List = open(OutDir + "/" + FileList, "w")
    for w in Write:
        List.write(w + "\n")
    print 'Datasets written to file %s' % List.name
