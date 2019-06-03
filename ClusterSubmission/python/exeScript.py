#! /usr/bin/env python
from ClusterSubmission.Utils import ResolvePath
import argparse, os

######
## Helper script to obtain encapsulated
## environments for LocalCluserEnginge
##########
if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='exScript',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="Helper script used to provide encapsulated environments for local submission",
    )
    parser.add_argument('--Cmd', help='Location where the command list is stored', required=True)
    parser.add_argument('--envVars', help='List of variables to execute', required=True, nargs="+", default=[])
    options = parser.parse_args()
    ### Arguments are parsed <var_name> <value> --> odd numbers indicate one of the components is missing
    if len(options.envVars) % 2 == 1:
        print "ERROR: Please give to every variable a value to assign"
        exit(1)
    ### Export the environment variables
    for i in range(0, len(options.envVars), 2):
        os.environ[options.envVars[i]] = options.envVars[i + 1]
    ### Find the location of the script to execute
    cmd_to_exec = ResolvePath(options.Cmd)
    if not cmd_to_exec:
        print "ERROR: %d does not exist" % (options.Cmd)
        exit(1)
    ### Make sure that we can execute it
    os.system("chmod 0755 %s" % (cmd_to_exec))
    ### Submit it
    exit(os.system(cmd_to_exec))
