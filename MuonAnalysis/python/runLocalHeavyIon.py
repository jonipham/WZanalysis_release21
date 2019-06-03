import os
import math
import sys
import re

datatextdir = '/afs/cern.ch/work/t/tphamleh/private/MirtasCode/MuonTriggers18_addPython/source/MuonValidation/data/data18_hi/'
datasets1 = 'data18_hi.00365498.express_express.recon.AOD.x586.txt'
datasets2 = 'data18_hi.00365502.express_express.recon.AOD.x586.txt'
datadir2 = '/eos/atlas/atlastier0/tzero/prod/data18_hi/express_express/00365502/data18_hi.00365502.express_express.recon.AOD.x586'
datadir1 = '/eos/atlas/atlastier0/tzero/prod/data18_hi/express_express/00365498/data18_hi.00365498.express_express.recon.AOD.x586'

f = open(datatextdir + datasets2, 'r+')

for sample in f:
    sample=sample.replace("\n","")
    sampleout=sample.replace(".1","")
    command = "athena --filesInput=%s/%s MuonValidation/MuonValidationAlgJobOptions.py "%(datadir2, sample)
    command2 = "mv output_muons.root /afs/cern.ch/work/t/tphamleh/private/MirtasCode/data18_hi.Output/%s"%(sampleout)
   
    print ("executing:\n", command)
    os.system(command)
    print ("executing:\n", command2)
    os.system(command2)

f.close()

