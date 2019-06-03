#! /usr/bin/env python
from ClusterSubmission.ListDisk import *
from ClusterSubmission.Utils import ReadListFromFile, CheckRemainingProxyTime, CheckRucioSetup, CreateDirectory


def initiateReplication(ListOfDataSets, Rucio, RSE, lifeTime=-1, approve=False, comment=""):
    Arguments = []
    if lifeTime > -1: Arguments.append(" --lifetime %d " % (lifeTime))
    if approve: Arguments.append(" --ask-approval ")
    if len(comment) > 0: Arguments.append(" --comment \"%s\" " % (comment))

    os.environ['RUCIO_ACCOUNT'] = Rucio

    for Item in ListOfDataSets:
        if Item.startswith("#") or len(Item) == 0:
            continue
        print "Request new rule for %s to %s" % (Item, RSE)
        Cmd = "rucio add-rule --account %s %s %s 1 %s" % (Rucio, " ".join(Arguments), Item, RSE)
        os.system(Cmd)


if __name__ == '__main__':

    CheckRucioSetup()
    CheckRemainingProxyTime()

    parser = argparse.ArgumentParser(
        description='This script lists datasets located at a RSE location. Futher patterns to find or exclude can be specified.',
        prog='ListGroupDisk',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-r', '-R', '--RSE', '--rse', help='specify RSE storage element (default: %s)' % RSE, default=RSE)
    parser.add_argument('-l', '--list', help='specify a list containing the datasets to be requested', required=True)
    parser.add_argument("--rucio", help="With this option you can set the rucio_account", default=RUCIO_ACCOUNT)
    parser.add_argument("--lifetime", help="Defines a lifetime after which the rules are automatically deleted", type=int, default=-1)
    parser.add_argument("--askapproval", help="Asks for approval of the request", default=False, action="store_true")
    parser.add_argument("--comment", help="Comment", default="")

    RunOptions = parser.parse_args()
    List = ReadListFromFile(RunOptions.list)

    ### Start replication of the datasets
    initiateReplication(
        ListOfDataSets=List,
        Rucio=RunOptions.rucio,
        RSE=RunOptions.RSE,
        lifeTime=RunOptions.lifetime,
        approve=RunOptions.askapproval,
        comment=RunOptions.comment)
