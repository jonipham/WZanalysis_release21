import argparse
import os

#include("XAMPPbase/CPToolSetup.py")
m_fileFlags = None


class FileFlags(object):
    def __init__(self):

        self.__isData = False
        self.__isAF2 = False
        self.__isDAOD = False
        self.__isTruth3 = False

        self.__mc_runNumber = -1
        self.__mcChannel = -1
        from AthenaCommon.AppMgr import ServiceMgr
        if len(ServiceMgr.EventSelector.InputCollections) == 0:
            print "WARNING: No infiles were configured thus far"
            return
        from PyUtils import AthFile
        af = AthFile.fopen(ServiceMgr.EventSelector.InputCollections[0])

        self.__isData = "data" in af.fileinfos['tag_info']['project_name']
        self.__isAF2 = not self.isData() and 'tag_info' in af.fileinfos and len(
            [key for key in af.fileinfos['tag_info'].iterkeys() if 'AtlfastII' in key or 'Fast' in key]) > 0
        self.__mc_runNumber = af.fileinfos["run_number"][0] if len(af.fileinfos["run_number"]) > 0 else -1
        self.__mcChannel = af.fileinfos["mc_channel_number"][0] if not self.isData() and len(af.fileinfos["mc_channel_number"]) > 0 else -1

        self.__isDAOD = "DAOD" in af.fileinfos['stream_names'][0]
        self.__isTruth3 = "TRUTH3" in af.fileinfos['stream_names'][0]

    def isData(self):
        return self.__isData

    def isAF2(self):
        return self.__isAF2

    def mcRunNumber(self):
        return self.__mc_runNumber

    def mcChannelNumber(self):
        return self.__mcChannel

    def isDAOD(self):
        return self.__isDAOD

    def isTRUTH3(self):
        return self.__isTruth3


def getFlags():
    global m_fileFlags
    if m_fileFlags == None: m_fileFlags = FileFlags()
    return m_fileFlags


def AssembleIO():
    #--------------------------------------------------------------
    # Reduce the event loop spam a bit
    #--------------------------------------------------------------
    from AthenaCommon.Logging import logging
    recoLog = logging.getLogger('MuonAnalysis I/O')
    recoLog.info('****************** STARTING the job *****************')

    if os.path.exists("%s/athfile-cache.ascii.gz" % (os.getcwd())):
        recoLog.info("Old athfile-cache found. Will delete it otherwise athena just freaks out. This little boy.")
        os.system("rm %s/athfile-cache.ascii.gz" % (os.getcwd()))
    from GaudiSvc.GaudiSvcConf import THistSvc
    from AthenaCommon.JobProperties import jobproperties
    import AthenaPoolCnvSvc.ReadAthenaPool
    from AthenaCommon.AthenaCommonFlags import athenaCommonFlags as acf
    from AthenaServices.AthenaServicesConf import AthenaEventLoopMgr
    from AthenaCommon.AppMgr import ServiceMgr
    from ClusterSubmission.Utils import ReadListFromFile, ResolvePath, IsROOTFile
    from MuonAnalysis.Utils import IsTextFile
    ServiceMgr += AthenaEventLoopMgr(EventPrintoutInterval=1000000)

    ServiceMgr += THistSvc()
    OutFileName = "AnalysisOutput.root" if not "outFile" in globals() else outFile
    ServiceMgr.THistSvc.Output += ["MuonAnalysis DATAFILE='{}' OPT='RECREATE'".format(OutFileName)]
    recoLog.info("Will save the job's output to " + OutFileName)
    ROOTFiles = []

    if "inputFile" in globals():
        recoLog.info("Use the following %s as input" % (inputFile))
        ROOTFiles = []
        ResolvedInFile = ResolvePath(inputFile)

        if inputFile.startswith('root://'):
            ROOTFiles.append(inputFile)

        elif ResolvedInFile and os.path.isfile(ResolvedInFile):
            if IsTextFile(ResolvedInFile):
                ROOTFiles = ReadListFromFile(ResolvedInFile)
            else:
                ROOTFiles.append(ResolvedInFile)

        elif ResolvedInFile and os.path.isdir(ResolvedInFile):
            for DirEnt in os.listdir(ResolvedInFile):
                if IsROOTFile(DirEnt):
                    if DirEnt.find(ResolvedInFile) != -1:
                        ROOTFiles.append(DirEnt)
                    else:
                        ROOTFiles.append("%s/%s" % (ResolvedInFile, DirEnt))
        else:
            raise RuntimeError("Invalid input " + inputFile)
        if len(ROOTFiles) == 0:
            raise RuntimeError("No ROOT files could be loaded as input")
        ServiceMgr.EventSelector.InputCollections = ROOTFiles
        acf.FilesInput = ROOTFiles

    if "nevents" in globals():
        recoLog.info("Only run on %i events" % (int(nevents)))
        theApp.EvtMax = int(nevents)
    if "nskip" in globals():
        recoLog.info("Skip the first %i events" % (int(nskip)))
        ServiceMgr.EventSelector.SkipEvents = int(nskip)
    """if isData(): recoLog.info("We're running over data today")
    elif isAF2():
        recoLog.info("Please fasten your seatbelt the journey will be on Atlas fast ")
    else:
        recoLog.info("Fullsimulation. Make sure that you wear your augmented reality glasses")"""
