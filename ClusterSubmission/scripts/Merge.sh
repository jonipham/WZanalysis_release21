#!/bin/bash
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
    echo "cd ${TMPDIR}"
    cd ${TMPDIR}
    export ATLAS_LOCAL_ROOT_BASE=/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase
    echo "Setting Up the ATLAS Enviroment:"
    echo "source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh" 
    source ${ATLAS_LOCAL_ROOT_BASE}/user/atlasLocalSetup.sh
    echo "Setting Up ROOT:"
    echo "source ${ATLAS_LOCAL_ROOT_BASE}/packageSetups/atlasLocalROOTSetup.sh --skipConfirm" 
    source ${ATLAS_LOCAL_ROOT_BASE}/packageSetups/atlasLocalROOTSetup.sh --skipConfirm
fi

if [  "${SLURM_JOB_USER}" == ""  ];then
    ID=${SGE_TASK_ID}
else
    ID=$((SLURM_ARRAY_TASK_ID+IdOffSet))
fi
if [ -z "${ID}" ]; then
    ID=1
fi

MergeList=""
if [ -f "${JobConfigList}" ];then
    echo "MergeList=`sed -n \"${ID}{p;q;}\" ${JobConfigList}`"
    MergeList=`sed -n "${ID}{p;q;}" ${JobConfigList}`
else 
    echo "ERROR: List ${JobConfigList} does not exist"
    scontrol requeuehold ${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}
fi

#SLURM_JOB_ID
if [ ! -f "${MergeList}" ];then
    echo "########################################################################"
    echo " ${MergeList} does not exist"
    echo "########################################################################"
    scontrol requeuehold ${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}

    exit 100
fi
To_Merge=""
while read -r M; do 
    FindSlash=${M##*/}
    if [ ! -d "${M/$FindSlash/}" ]; then
        echo "The Dataset ${M} is stored on dCache.  Assume that the file is healthy"            
    elif [ ! -f "${M}" ];then
        echo "File ${M} does not exist. Merging Failed"
        echo "###############################################################################################"
        echo "                    Merging failed"
        echo "###############################################################################################"
        scontrol requeuehold ${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}            
        exit 100        
    fi
    To_Merge="${To_Merge} ${M}"        
done < "${MergeList}"
echo "cd ${TMPDIR}"
cd ${TMPDIR}
if [ -f "${OutFileList}" ]; then
    echo "OutFile=`sed -n \"${ID}{p;q;}\" ${OutFileList}`"
    OutFile=`sed -n "${ID}{p;q;}" ${OutFileList}`
fi
if [ -f "${OutFile}" ];then
    echo "Remove the old ROOT file"
    rm -f ${OutFile}
fi
echo "hadd ${OutFile} ${To_Merge}"
hadd  ${OutFile} ${To_Merge}
if [ $? -eq 0 ]; then
    ls -lh        
    echo "###############################################################################################"
    echo "                        Merging terminated successfully"
    echo "###############################################################################################"
else
    echo "###############################################################################################"
    echo "                    Merging failed"
    echo "###############################################################################################"
    scontrol requeuehold ${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}            
    exit 100
fi


