#! /usr/bin/env python
import math, os, sys, commands, time, threading, string, random


class CmdLineThread(threading.Thread):
    def __init__(self, cmd, args=""):
        threading.Thread.__init__(self)
        self.__cmd = cmd
        self.__args = args
        self.__statusCode = -1

    def run(self):
        self.start_info()
        self.__statusCode = os.system("%s %s" % (self.__cmd, self.__args))

    def start_info(self):
        print "INFO: Execute command %s %s" % (self.__cmd, self.__args)

    def isSuccess():
        if self.__statusCode == -1:
            print 'WARNING: Thread not executed'
        return (self.__statusCode == 0)


def WriteList(Files, OutLocation):
    """Write list of files to output location. If output location does not exist, create directory."""
    if OutLocation.find("/") != -1:
        CreateDirectory(OutLocation[:OutLocation.rfind("/")], CleanUpOld=False)
    with open(OutLocation, "w") as Out:
        if Out is None:
            print 'ERROR: Could not create the File List: ' + OutLocation
            return False
        for F in Files:
            Out.write(F + "\n")
        Out.close()
    return True


def id_generator(size=45, chars=string.ascii_letters + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))


def CheckRemainingProxyTime():
    """Check if the GRID proxy lifetime if larger than 0. Otherwise generate new GRID proxy."""
    RemainingTime = 0
    try:
        RemainingTime = int(commands.getoutput("voms-proxy-info --timeleft").strip().split('\n')[-1])
    except ValueError:
        pass
    if not RemainingTime > 0:
        print "No valid VOMS-PROXY, creating one..."
        os.system("voms-proxy-init --voms atlas")
        CheckRemainingProxyTime()
    return RemainingTime


def CheckRucioSetup():
    try:
        import rucio
    except ImportError:
        print 'No RUCIO setup is found please SETUP rucio using "lsetup rucio"'
        sys.exit(1)

    if not os.getenv("RUCIO_ACCOUNT"):
        print "No RUCIO ACCOUNT is available.. please define a rucio Account"
        exit(1)
    print "INFO: Rucio is setup properly."


def CheckPandaSetup():
    PANDASYS = os.getenv("PANDA_SYS")
    if PANDASYS == None:
        print('Please setup Panda using "lsetup panda"')
        exit(1)
    CheckRucioSetup()
    CheckRemainingProxyTime()


def IsROOTFile(FileName):
    if FileName.endswith(".root"): return True
    if FileName.split(".")[-1].isdigit():
        return FileName.split(".")[-2] == "root"
    return False


def CreateDirectory(Path, CleanUpOld=True):
    """Create new directory if possible. Option to delete existing directory of same name."""
    if len(Path) == 0 or Path == os.getenv("HOME"):
        prettyPrint("Could not create", Path)
        return False
    if os.path.exists(Path) and CleanUpOld:
        print "INFO: Found old copy of the folder " + Path
        print "INFO: Will delete it."
        os.system("rm -rf " + Path)
    if not os.path.exists(Path):
        print "INFO: Create directory " + Path
    os.system("mkdir -p " + Path)
    return os.path.exists(Path)


def ReadListFromFile(File):
    List = []
    In_Path = ResolvePath(File)
    if In_Path and len(In_Path) > 0:
        with open(In_Path) as myfile:
            for line in myfile:
                if line.startswith('#'):
                    continue
                realline = line.strip()
                if not realline:
                    continue  # Ignore whitespace
                List.append(realline)
    else:
        print "WARNING: Could not find list file %s" % (File)
    return List


def prettyPrint(preamble, data, width=30, separator=":"):
    """Prints uniformly-formatted lines of the type "preamble : data"."""
    preamble = preamble.ljust(width)
    print '%s%s %s' % (preamble, separator, data)


def TimeToSeconds(Time):
    """Convert time in format DD:MM:SS to seconds."""
    S = 0
    for i, E in enumerate(Time.split(":")):
        try:
            S += int(E) * math.pow(60, 2 - i)
        except Exception:
            if i == 0:
                try:
                    S += (24 * int(E.split("-")[0]) + int(E.split("-")[1])) * math.pow(60, 2 - i)
                except Exception:
                    pass
    return S


def ResolvePath(In):
    import ROOT
    try:
        PathResolver = ROOT.PathResolver()
    except Exception:
        try:
            from PathResolver import PathResolver
        except:
            print "I've no idea what to do next"
            exit(1)
    if os.path.exists(In):
        return In
    # Remove the 'data/' in the file name
    if "data/" in In:
        In = In.replace("data/", "")
    ResIn = PathResolver.FindCalibFile(In)
    if not ResIn:
        ResIn = PathResolver.FindCalibDirectory(In)
    if len(ResIn) > 0 and os.path.exists(ResIn):
        return ResIn

    PkgDir = PathResolver.FindCalibDirectory(In.split("/")[0])
    if PkgDir and os.path.exists("%s/%s" % (PkgDir, In)):
        return "%s/%s" % (PkgDir, In)
    try:
        Ele = os.listdir(PkgDir)[0]
        PkgDir = os.path.realpath(PkgDir + "/" + Ele + "/../../")
        if os.path.exists("%s/%s" % (PkgDir, In.split("/", 1)[-1])):
            return "%s/%s" % (PkgDir, In.split("/", 1)[-1])
    except Exception:
        pass
    print "ERROR: No such file or directory " + In
    return None


def CheckConfigPaths(Configs=[], filetype="conf"):
    Files = []
    for C in Configs:
        TempConf = ResolvePath(C)
        if TempConf is None:
            continue
        if os.path.isdir(TempConf) is True:
            print "INFO: The Config " + C + " is a directory read-out all config files"
            Files += [
                "%s/%s" % (TempConf, Cfg) for Cfg in os.listdir(TempConf)
                if Cfg.endswith(filetype) and not os.path.isdir("%s/%s" % (TempConf, Cfg))
            ]
        elif os.path.isfile(TempConf) is True and C.endswith(filetype):
            Files.append(TempConf)
    return Files


def ExecuteCommands(ListOfCmds, MaxCurrent=16, MaxExec=-1):
    Threads = []
    for Cmd in ListOfCmds:
        Threads.append(CmdLineThread(Cmd))
    ExecuteThreads(Threads, MaxCurrent, MaxExec)


def getRunningThreads(Threads):
    Running = 0
    for Th in Threads:
        if Th.isAlive():
            Running += 1
    return Running


def ExecuteThreads(Threads, MaxCurrent=16, MaxExec=-1, verbose=True):
    Num_Executed = 0
    Num_Threads = len(Threads)
    N_Prompt = min([100, int(Num_Threads / 100)])
    if N_Prompt == 0:
        N_Prompt = int(Num_Threads / 10) if int(Num_Threads / 10) > 0 else 100
    while Num_Executed != Num_Threads:
        if Num_Executed == MaxExec:
            break
        while getRunningThreads(Threads) < MaxCurrent and Num_Threads != Num_Executed:
            Threads[Num_Executed].start()
            Num_Executed += 1
            if Num_Executed == MaxExec:
                break
            if verbose and Num_Executed % N_Prompt == 0:
                print "INFO: Executed %d out of %d threads" % (Num_Executed, Num_Threads)
        WaitCounter = 0
        while getRunningThreads(Threads) >= MaxCurrent:
            if verbose:
                WaitCounter += 1
            if WaitCounter == 5000:
                print "INFO: At the moment %d threads are active. Executed %d out of %d Threads" % (getRunningThreads(Threads),
                                                                                                    Num_Executed, len(Threads))
                WaitCounter = 0
            time.sleep(0.005)
    WaitCounter = 0
    while getRunningThreads(Threads) > 0:
        if WaitCounter == 5000:
            print "INFO: Wait until the last %d threads are going to finish " % (getRunningThreads(Threads))
            WaitCounter = 0
        time.sleep(0.005)
        if verbose:
            WaitCounter += 1


def RecursiveLS(Dir, data_types=[]):
    n_data_types = len(data_types)
    if not os.path.isdir(Dir):
        print "WARNING: Not a directory %s" % (Dir)
        return []
    LS = []
    for item in os.listdir(Dir):
        full_path = "%s/%s" % (Dir, item)
        full_path = full_path.replace("//", "/")
        if os.path.isdir(full_path): LS += RecursiveLS(full_path, data_types)
        if n_data_types == 0 or len([d for d in data_types if item.endswith(".%s" % (d))]) > 0:
            LS += [full_path]
    return LS


def ClearFromDuplicates(In=[]):
    TmpIn = []
    for I in In:
        if I not in TmpIn: TmpIn.append(I)
    return TmpIn


def FillWhiteSpaces(N, Space=" "):
    str = ""
    for i in range(0, N):
        str += Space
    return str
