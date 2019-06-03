#!/bin/bash

##############################
# Setup                      #
##############################

#prepare AthAnalysis or build if not already done so
if [ -f /xampp/build/${AthAnalysis_PLATFORM}/setup.sh ]; then
    if [[ -z "${TestArea}" ]]; then
        export TestArea=/xampp/XAMPPbase
    fi
    source /xampp/build/${AthAnalysis_PLATFORM}/setup.sh
else
    asetup AthAnalysis,latest,here
    if [ -f ${TestArea}/build/${AthAnalysis_PLATFORM}/setup.sh ]; then
        source ${TestArea}/build/${AthAnalysis_PLATFORM}/setup.sh
    else
        mkdir -p ${TestArea}/build && cd ${TestArea}/build
        cmake ..
        cmake --build .
        cd .. && source build/${AthAnalysis_PLATFORM}/setup.sh
    fi
fi

# definition of folder for storing test results
TESTDIR=test_job/
TESTFILE="root://eoshome.cern.ch//eos/user/x/xampp/ci/base//DAOD_SUSY1.14703199._000112.pool.root.1"
LOCALCOPY=DxAOD.root
TESTRESULT=processedNtuple.root

##############################
# Process test sample        #
##############################

# create directory for results
mkdir -p ${TESTDIR}
cd ${TESTDIR}

# get kerberos token
if [ -z ${SERVICE_PASS} ]; then
  echo "You did not set the environment variable SERVICE_PASS.\n\
Please define in the gitlab project settings/CI the secret variables SERVICE_PASS and CERN_USER."
else
  echo "${SERVICE_PASS}" | kinit ${CERN_USER}@CERN.CH
fi

# copy file with xrdcp to local space
if [ ! -f ${LOCALCOPY} ]; then
    echo "File not found! Copying it from EOS"
    echo xrdcp ${TESTFILE} ${LOCALCOPY}
    xrdcp ${TESTFILE} ${LOCALCOPY}
fi

# clean up old job result
if [ -f ${TESTRESULT} ]; then
    rm ${TESTRESULT} 
fi

# run job
python ${TestArea}/XAMPPbase/python/runAthena.py --filesInput ${LOCALCOPY} --outFile ${TESTRESULT} --jobOptions XAMPPbase/runXAMPPbase.py


###################################################
# Raise error if execution failed                 #
###################################################
if [ $? -ne 0 ]; then
  printf '%s\n' "Execution of runAthena.py failed" >&2  # write error message to stderr
  exit 1
fi


##############################
# Evalulate cut flows        #
##############################
python ${TestArea}/XAMPPbase/python/printCutFlow.py -i ${TESTRESULT} -a MyCutFlow | tee cutflow.txt
