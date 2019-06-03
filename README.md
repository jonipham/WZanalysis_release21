How to build the code: 
mkdir build run
git clone https://gitlab.cern.ch/tphamleh/muonanalysis_athanalysis21.2.67
cd source
asetup AthAnalysis,21.2.67,here
cd ../build && cmake ../source && make
cd ../run && source ../build/${AthAnalysis_PLATFORM}/setup.sh 

How to run the code:
locally: athena --filesInput=$"SOME_TEST_FILE_DATA" --evtMax=10 MuonAnalysis/MuonAnalysisAlgJobOptions.py
on the grid:
lsetup rucio
lsetup panda
python ../source/MuonAnalysis/python/SubmitToGrid.py -i InputDataSet 

If come back:
cd source && asetup [...]
cd ../run && source ../build/${AthAnalysis_PLATFORM}/setup.sh 

Note: InputDataSet can be a dataset or a text file that has a list of datasets
--nevents to specify the number of maximum events
--debug to run on debug mode
-j and add whatever you want to the outputdataset name
--exSite to exclude the sites that may cause troubles (edit the list excludeSite in the SubmitToGrid.py)
Specity number of files per job in FilesPerJob in SumitToGrid.py
--jobOption default is MuonAnalysis/MuonAnalysisAlgJobOptions.py



############################
what changed in athena:

On branch my21.2
Your branch is up-to-date with 'origin/21.2'.
Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git checkout -- <file>..." to discard changes in working directory)

	modified:   Event/xAOD/xAODPrimitives/Root/getIsolationAccessor.cxx
	modified:   Event/xAOD/xAODPrimitives/xAODPrimitives/IsolationType.h
	modified:   PhysicsAnalysis/AnalysisCommon/IsolationSelection/Root/IsolationSelectionTool.cxx
	modified:   PhysicsAnalysis/SUSYPhys/SUSYTools/Root/SUSYObjDef_xAOD.cxx
	modified:   PhysicsAnalysis/SUSYPhys/SUSYTools/Root/SUSYToolsInit.cxx
	modified:   PhysicsAnalysis/SUSYPhys/SUSYTools/Root/Taus.cxx
	modified:   PhysicsAnalysis/SUSYPhys/SUSYTools/share/applyST.py

Untracked files:
  (use "git add <file>..." to include in what will be committed)

	PhysicsAnalysis/HeavyIonPhys/HIEventUtils/HIEventUtils/HICentralityTool.h
	PhysicsAnalysis/HeavyIonPhys/HIEventUtils/HIEventUtils/IHICentralityTool.h
	PhysicsAnalysis/HeavyIonPhys/HIEventUtils/Root/HICentralityTool.cxx
	PhysicsAnalysis/HeavyIonPhys/HIEventUtils/data/


