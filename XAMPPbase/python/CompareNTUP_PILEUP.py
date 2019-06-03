import os, argparse, ROOT
from ClusterSubmission.Utils import WriteList, IsROOTFile, ResolvePath
from XAMPPbase.CreateMergedNTUP_PILEUP import readPRWchannels
from pprint import pprint

if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        prog='CompareNTUP_PILEUP',
        description=
        'This script searches for NTUP_PILEUP derivations in rucio (or takes a given list) and sorts the datasets by their AMI-tags. Then it donwloads and merges them accordingly.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--oldPRWDir', help='Path to the previous files', default=ResolvePath("XAMPPbase/PRWFiles"))
    parser.add_argument('--newPRWDir', help='Path to the new file', required=True)
    parser.add_argument('--uniteFiles',
                        help="Put everything which was in the old file also in the new one",
                        default=False,
                        action="store_true")
    RunOptions = parser.parse_args()

    files_in_old = [f for f in os.listdir(RunOptions.oldPRWDir) if IsROOTFile(f)]
    files_in_new = [f for f in os.listdir(RunOptions.newPRWDir) if IsROOTFile(f)]

    MyxSecDB = ROOT.SUSY.CrossSectionDB()
    for new in files_in_new:
        if not new in files_in_old:
            print "WARNING: Strange the file %s is new. Is it a new campaign?"
            continue

        chan_in_old = readPRWchannels("%s/%s" % (RunOptions.oldPRWDir, new))
        chan_in_new = readPRWchannels("%s/%s" % (RunOptions.newPRWDir, new))
        messages = []
        AnythingNew = False
        ### Compare the prw channels of both files
        for c in chan_in_new:
            if not c in chan_in_old:
                messages += ["INFO: Channel %d (%s) has been added through the last iteration to %s" % (c, MyxSecDB.name(c), new)]
                AnythingNew = True
            else:
                chan_in_old.remove(c)

        ### The old file somehow contains additional channels. We need to double check
        if len(chan_in_old) > 0:
            messages += [
                "WARNING: The following channels were merged into the old file %s/%s but are no longer present in %s/%s" %
                (RunOptions.oldPRWDir, new, RunOptions.newPRWDir, new)
            ]

            for c in chan_in_old:
                messages += ["     -=-=- %d (%s)" % (c, MyxSecDB.name(c))]
        if not AnythingNew:
            messages += ["INFO: Nothing new has been added to %s/%s w.r.t %s/%s" % (RunOptions.oldPRWDir, new, RunOptions.newPRWDir, new)]

        WriteList(messages, "PRWcheck_%s.log" % (new[:new.rfind(".")]))
        if len(chan_in_old) > 0 and AnythingNew and RunOptions.uniteFiles:
            Cmd = "SlimPRWFile --inFile %s/%s --inFile %s/%s --outFile %s/%s --InIsSlimmed" % (
                RunOptions.newPRWDir, new, RunOptions.oldPRWDir, new, RunOptions.newPRWDir, new)
            os.system(Cmd)
