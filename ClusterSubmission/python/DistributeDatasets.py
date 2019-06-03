#! /usr/bin/env python
from ClusterSubmission.ListDisk import *
from ClusterSubmission.RequestToGroupDisk import initiateReplication, ReadListFromFile
from ClusterSubmission.Utils import CheckRemainingProxyTime, CheckRucioSetup, CreateDirectory
import random

if __name__ == '__main__':

    CheckRucioSetup()
    CheckRemainingProxyTime()
    parser = argparse.ArgumentParser(
        description='This script lists datasets located at a RSE location. Futher patterns to find or exclude can be specified.',
        prog='ListGroupDisk',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-l', '--list', help='specify a list containing the datasets to be requested', required=True)
    parser.add_argument("--rucio", help="With this option you can set the rucio_account", default=RUCIO_ACCOUNT)
    parser.add_argument("--lifetime", help="Defines a lifetime after which the rules are automatically deleted", type=int, default=-1)
    parser.add_argument("--askapproval", help="Asks for approval of the request", default=False, action="store_true")
    parser.add_argument("--comment", help="Comment", default="")

    RunOptions = parser.parse_args()
    SCRATCH_DISKS = [R for R in getRSEs() if R.find("SCRATCHDISK") != -1]
    List = ReadListFromFile(RunOptions.list)
    for L in List:
        initiateReplication(
            ListOfDataSets=[L],
            Rucio=RunOptions.rucio,
            RSE=random.choice(SCRATCH_DISKS),
            lifeTime=14 * 24 * 3600,  # 14 days replication time
            approve=RunOptions.askapproval,
            comment=RunOptions.comment)
