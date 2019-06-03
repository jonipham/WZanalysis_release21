#!/bin/bash
if [  "${SLURM_JOB_USER}" == ""  ];then
    ID=${SGE_TASK_ID}
else
    ID=$((SLURM_ARRAY_TASK_ID+IdOffSet))
fi
if [ -z "${ID}" ]; then
    ID=1
fi
Cmd=""
if [ -f "${ListOfCmds}" ];then
    echo "Cmd=`sed -n \"${ID}{p;q;}\" ${ListOfCmds}`"
    Cmd=`sed -n "${ID}{p;q;}" ${ListOfCmds}`
else
    echo "ERROR: No list of commands"
    scontrol requeuehold ${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}
fi
if [ -z "${Cmd}" ]; then
    echo "ERROR: No list of commands"
    scontrol requeuehold ${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}
fi
# some initial output
echo "###############################################################################################"
echo "                     Enviroment variables"
echo "###############################################################################################"
export
echo "###############################################################################################"
echo " "

if [ -z "${ATLAS_LOCAL_ROOT_BASE}" ];then
    echo "###############################################################################################"
    echo "                    Setting up the enviroment"
    echo "###############################################################################################"
    echo "Setting Up the ATLAS Enviroment:"
    export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
    echo "source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh" 
    source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh
    echo "Setup athena:"
    echo "cd ${OriginalArea}"
    cd ${OriginalArea}
    echo "asetup ${OriginalProject},${OriginalPatch},here"
    asetup ${OriginalProject},${OriginalPatch},here
    #Check whether we're in release 21
    if [ -f ${OriginalArea}/../build/${BINARY_TAG}/setup.sh ]; then
        echo "source ${OriginalArea}/../build/${BINARY_TAG}/setup.sh"
        source ${OriginalArea}/../build/${BINARY_TAG}/setup.sh
        WORKDIR=${OriginalArea}/../build/${BINARY_TAG}/bin/
    elif [ -f ${OriginalArea}/../build/${WorkDir_PLATFORM}/setup.sh ];then
        echo "source ${OriginalArea}/../build/${WorkDir_PLATFORM}/setup.sh"
        source ${OriginalArea}/../build/${WorkDir_PLATFORM}/setup.sh
        source ${OriginalArea}/../build/${WorkDir_PLATFORM}/setup.sh
        WORKDIR=${OriginalArea}/../build/${WorkDir_PLATFORM}/bin/
     elif [ -f ${OriginalArea}/../build/${AthAnalysis_PLATFORM}/setup.sh ];then
            echo "source ${OriginalArea}/../build/${AthAnalysis_PLATFORM}/setup.sh"
            source ${OriginalArea}/../build/${AthAnalysis_PLATFORM}/setup.sh        
            WORKDIR=${OriginalArea}/../build/${AthAnalysis_PLATFORM}/bin/
    elif [ -f ${OriginalArea}/../build/${LCG_PLATFORM}/setup.sh ];then
            echo "source ${OriginalArea}/../build/${LCG_PLATFORM}/setup.sh"
            source ${OriginalArea}/../build/${LCG_PLATFORM}/setup.sh
            WORKDIR=${OriginalArea}/../build/${LCG_PLATFORM}/bin/            
    elif [ -z "${CMTBIN}" ];then
        echo "Something strange happens?!?!?!"
        echo " ${OriginalArea}/../build/${BINARY_TAG}/setup.sh"
        echo " ${OriginalArea}/../build/${WorkDir_PLATFORM}/setup.sh"            
        scontrol requeuehold ${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}        
        exit 100
    fi    
fi
echo "cd ${TMPDIR}"
cd ${TMPDIR}
echo "${Cmd}"
${Cmd}
if [ $? -eq 0 ]; then
    echo "###############################################################################################"
    echo "                        Command execution terminated successfully"
    echo "###############################################################################################"
else
    echo "###############################################################################################"
    echo "                    WriteDefaultHistos job has experienced an error"
    echo "###############################################################################################"
    scontrol requeuehold ${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}            
    exit 100
fi
