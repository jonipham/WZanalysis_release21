# ClusterSubmission

Package containing infrastructure for submitting jobs to all kind of clusters.

Introduction
--- 

This package is meant to hold a common infrastructure for submitting jobs to all kind of clusters. Thereby, _cluster_ can refer to a computing cluster at a Tier-2 site but also a local multi-core machine to which one wants to submit jobs running in multiple threads.

Supported batch systems
---

The main functionality of this package is encoded inside `python/ClusterEngine.py`. Currently supported batch systems are:

1) Slurm
2) Local machine
3) condor (work in progress)

Help
---

Feel free to contribute and support your favourite type of batch systems. Contact:
* jojungge@cern.ch
* nkoehler@cern.ch
*   pgadow@cern.ch
