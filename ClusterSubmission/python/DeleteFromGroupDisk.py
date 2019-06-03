#! /usr/bin/env python
from ClusterSubmission.ListDisk import *
from ClusterSubmission.Utils import ReadListFromFile

if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='This script lists datasets located at a RSE location. Futher patterns to find or exclude can be specified.',
        prog='ListGroupDisk',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-r', '-R', '--RSE', "--rse", help='specify RSE storage element', default=RSE)
    parser.add_argument('-d', '-l', '-D', '-L', '--list', help='specify a list containing the datasets to be deleted')
    parser.add_argument(
        "--AllRequests", help="Allows to delete rules from disk not requested by oneself", default=False, action="store_true")
    parser.add_argument("--rucio", help="With this option you can set the rucio_account", default=RUCIO_ACCOUNT)

    RunOptions = parser.parse_args()
    List = ReadListFromFile(RunOptions.list)
    MyRequests = []
    if not RunOptions.AllRequests: MyRequests = ListUserRequests(RunOptions.RSE, RunOptions.rucio)
    else: MyRequests = ListDisk(RunOptions.RSE)

    Delete = []
    for Item in List:
        if len(Item) <= 1: continue
        for DS in MyRequests:
            if Item == DS or (len(Item) < len(DS) and DS[:len(Item)] == Item and DS[len(Item)] in ["_", "."]):
                print "Found rucio rule " + DS + " matching " + Item
                Delete.append(DS)

    os.environ["RUCIO_ACCOUNT"] = RunOptions.rucio
    for Item in Delete:
        print "Delete rule for " + Item + " at storage element " + RunOptions.RSE + " subscribed by " + RunOptions.rucio
        ID, Owner = GetDataSetInfo(Item, RunOptions.RSE, RunOptions.rucio)
        if not ID: continue
        os.system("rucio delete-rule %s --account %s" % (ID, RunOptions.rucio))
