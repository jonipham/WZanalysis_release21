#!/bin/bash
if [  "$BATCH_SYSTEM" == "HTCondor"  ];then
	echo "TASK_ID $TASK_ID needs to be incremented by 1"
	TASK_ID="$((TASK_ID + 1))"
	echo $TASK_ID
elif [  "${SLURM_JOB_USER}" == ""  ];then
	TASK_ID=${SGE_TASK_ID}
else
	echo "${SLURM_ARRAY_TASK_ID} + ${IdOffSet}"
	TASK_ID=$((SLURM_ARRAY_TASK_ID+IdOffSet))
fi
# some initial output
echo "###############################################################################################"
echo "					 Environment variables"
echo "###############################################################################################"
export
echo "###############################################################################################"
echo " "
if [ -z "${ATLAS_LOCAL_ROOT_BASE}" ];then    
    echo "###############################################################################################"
    echo "                    Setting up the environment"
    echo "###############################################################################################"
    # check if TMPDIR exists or define it as TMP
    [[ -d "${TMPDIR}" ]] || export TMPDIR=${TMP}
    echo "cd ${TMPDIR}"
    cd ${TMPDIR}
    echo "export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase"
    export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase    
    echo "Setting up the ATLAS environment:"
    echo "source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh" 
    source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh
    echo "Setting up ROOT:"
    echo "source ${ATLAS_LOCAL_ROOT_BASE}/packageSetups/atlasLocalROOTSetup.sh ${ROOTVER}-${ROOTCORECONFIG} --skipConfirm" 
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
        source  ${OriginalArea}/../build/x86_64*/setup.sh
        if [ $? -ne 0 ];then
            echo "Something strange happens?!?!?!"
            export
            echo " ${OriginalArea}/../build/${BINARY_TAG}/setup.sh"
            echo " ${OriginalArea}/../build/${WorkDir_PLATFORM}/setup.sh"            
            scontrol requeuehold ${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}        
            exit 100
        fi
    fi
fi

InFile=""
if [ -f "${InCfg}" ];then
    echo "InFile=`sed -n \"${TASK_ID}{p;q;}\" ${InCfg}`"
    InFile=`sed -n "${TASK_ID}{p;q;}" ${InCfg}`
fi
OutFile=""
if [ -f "${OutCfg}" ];then
    echo "OutFile=`sed -n \"${TASK_ID}{p;q;}\" ${OutCfg}`"
    OutFile=`sed -n "${TASK_ID}{p;q;}" ${OutCfg}`
fi


To_Process=""
while read -r M; do 
    
    To_Process="${To_Process},${M}"        
done < "${InFile}"
# Process job
cd ${TMPDIR}
echo "execute jobOptions..."
echo "athena ${Execute} --filesInput  \"${To_Process}\" -  --parseFilesForPRW ${Options}"
athena ${Execute} --filesInput  "${To_Process}" - ${Options}  --parseFilesForPRW 


if [ $? -eq 0 ]; then
	echo "###############################################################################################"
	echo "						Analysis job terminated successfully"
	echo "###############################################################################################"
	ls -lh
	echo "${TMPDIR}/AnalysisOutput.root ${OutFile}"
	mv ${TMPDIR}/AnalysisOutput.root ${OutFile}
	
else
	echo "###############################################################################################"
	echo "					Analysis job has experienced an error"
	echo "###############################################################################################"
	if [  "${SLURM_JOB_USER}" != ""  ];then
		scontrol requeuehold ${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}
    fi
	exit 100
fi
