#! /usr/bin/env python
from ClusterSubmission.ListDisk import *
from ClusterSubmission.Utils import prettyPrint, WriteList
import errno, subprocess


def GetDataSetFiles(dsname, RSE, protocols):
    prettyPrint("Get the files of the dataset", dsname)
    DSReplicas = commands.getoutput("rucio list-file-replicas --protocols %s --rse %s %s " % (protocols, RSE, dsname))
    DS = []
    for line in DSReplicas.split("\n"):
        Entry = None
        LineInfo = line.split()
        for i, column in enumerate(LineInfo):
            if RSE in column:
                try:
                    Entry = LineInfo[i + 1]
                    break
                except:
                    print "There was some strange noise here ", column
                    pass
        if Entry:
            print Entry
            DS.append(Entry)
    return DS


def GetScopes():
    print "Reading in the scopes:"
    Scopes = commands.getoutput("rucio list-scopes")
    ScopeList = []
    for Entry in Scopes.split():
        if ("user." not in Entry and "group" not in Entry):
            ScopeList.append(Entry + ":")
    ScopeList.append("user." + RUCIO_ACC)
    print "Done found %d scopes" % (len(ScopeList))
    return ScopeList


def createFileList(dsname, options):
    prettyPrint('Creating file list for', dsname)
    DS = GetDataSetFiles(dsname, options.RSE, options.protocols)
    if len(DS) == 0:
        print "No datasets found"
        return
    if dsname.find(":") > -1:
        dsname = dsname[dsname.find(":") + 1:len(dsname)]
    OutDir = options.OutDir
    if os.path.exists(OutDir) == False:
        print "mkdir -p " + OutDir
        os.system("mkdir -p " + OutDir)
    filelistname = OutDir + "/" + dsname.rstrip('/') + ".txt"
    if os.path.exists(filelistname) == True:
        print "Remove the old FileList"
        os.system("rm " + filelistname)
    WriteList(DS, filelistname)


if __name__ == '__main__':

    OutDir = os.getcwd()

    parser = argparse.ArgumentParser(
        description='This script lists datasets located at a RSE location. Futher patterns to find or exclude can be specified.',
        prog='ListGroupDisk',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-s', '-S', '--FLsuffix', help='File list suffix', default='')
    parser.add_argument(
        '-d', '-l', '-D', '-L', '--dataset', help='Dataset to collect data for, or filename containing list of datasets', default='')
    parser.add_argument('-o', '-O', '--OutDir', help='specify output directory to put file list(s) into', default=OutDir)
    parser.add_argument('-r', '-R', '--RSE', help='specify RSE storage element which should be read', default=RSE)
    parser.add_argument(
        '-p', '-P', '--protocols', help="Specify the protocols you want to use for the file list creation. Default: 'dcap'", default="root")
    RunOptions = parser.parse_args()

    # Do we have one dataset, or a file with a list of them?
    if os.path.exists(RunOptions.dataset):
        with open(RunOptions.dataset) as dsfile:
            for line in dsfile:
                # Ignore comment lines and empty lines
                if line.startswith('#'): continue
                realline = line.strip()
                if realline.find("_tid") > -1: realline = realline[0:realline.find("_tid")]
                if not realline: continue  # Ignore whitespace
                createFileList(realline, RunOptions)
    else:
        createFileList(RunOptions.dataset, RunOptions)
