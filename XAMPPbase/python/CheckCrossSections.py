from XAMPPbase.CreateMergedNTUP_PILEUP import GetPRW_datasetID
from XAMPPbase.Utils import CreateDirectory, ReadListFromFile, IsListIn, ClearFromDuplicates, WriteList, IsTextFile, ResolvePath
import os, argparse, ROOT


def getRunOptions():
    """Get run options from command line."""
    parser = argparse.ArgumentParser(
        prog='CheckCrossSections',
        description=
        'This script takes directory containing samples and checks if their cross-section information is avaliable. If this is not the case it accesses AMI to get the information and write it to a file. ',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--ListDir', '-o', '-O', help='Directory containing file-lists', type=str, required=True)
    parser.add_argument("--missingFile",
                        help="Text file where to dump the missing cross-sections",
                        type=str,
                        default="MISSING_xSections.txt")
    return parser.parse_args()


def main():
    """Check for files specified in file lists in directory the cross-section information."""

    # get arguments from command line + find list directory
    RunOptions = getRunOptions()
    Sample_Dir = ResolvePath(RunOptions.ListDir)
    if not Sample_Dir:
        print("ERROR: Please give a valid  directory")
        exit(1)

    # set up cross-section database
    MyxSecDB = ROOT.SUSY.CrossSectionDB()

    # get DSIDs from file lists
    DSIDs = []
    for File in os.listdir(Sample_Dir):
        if os.path.isdir("%s/%s" % (Sample_Dir, File)): continue
        print "INFO: Look for samples in list %s" % (File)
        DSIDs += sorted(
            ClearFromDuplicates([GetPRW_datasetID(DS) for DS in ReadListFromFile("%s/%s" % (Sample_Dir, File)) if DS.find("data") == -1]))
    DSIDs = ClearFromDuplicates(DSIDs)

    # get cross-section information for files from cross-section database and if that fails from AMI
    from XAMPPplotting.AMIDataBase import getAMIDataBase
    getAMIDataBase().getMCDataSets(channels=DSIDs)
    Missing_xSections = []
    for ds in DSIDs:
        if len(MyxSecDB.name(ds)) == 0:
            ami_channel = getAMIDataBase().getMCchannel(dsid=ds)
            if ami_channel: Missing_xSections += ["%d (%s): %f" % (ds, ami_channel.name(), ami_channel.xSection())]
            else: Missing_xSections += ["%d (UNKOWN): -1"]
    if len(Missing_xSections):
        WriteList(Missing_xSections, RunOptions.missingFile)


if __name__ == "__main__":
    main()
