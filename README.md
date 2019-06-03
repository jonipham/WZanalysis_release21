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


