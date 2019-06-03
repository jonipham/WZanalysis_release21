#! /usr/bin/env python
from ClusterSubmission.Utils import ReadListFromFile
from ClusterSubmission.ClusterEngine import setup_engine, setupBatchSubmitArgParser
import sys, os, argparse, commands, time

if __name__ == '__main__':

    parser = setupBatchSubmitArgParser()
    parser.add_argument("--fileLists", help="Specify the file lists to submit", default=[], nargs="+")
    parser.add_argument("--remainingSplit", help="Specify a remaining split of the files", default=1)
    parser.add_argument("--nFilesPerJob", help="Specify number of files per merge job", default=10)
    parser.add_argument("--HoldJob", help="Specify a list of jobs to hold on", default=[])
    RunOptions = parser.parse_args()
    submit_engine = setup_engine(RunOptions)
    merging = [
        submit_engine.create_merge_interface(
            out_name=L[L.rfind("/") + 1:L.rfind(".")],
            files_to_merge=ReadListFromFile(L),
            files_per_job=RunOptions.nFilesPerJob,
            hold_jobs=RunOptions.HoldJob,
            final_split=RunOptions.remainingSplit) for L in RunOptions.fileLists
    ]
    for merge in merging:
        merge.submit_job()
    clean_hold = [submit_engine.subjob_name("merge-%s" % (merge.outFileName())) for merge in merging]

    submit_engine.submit_clean_all(clean_hold)
