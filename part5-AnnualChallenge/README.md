# Environement
CPU(s):                          64
On-line CPU(s) list:             0-63
Thread(s) per core:              1
Core(s) per socket:              32
Socket(s):                       2
NUMA node(s):                    2
Vendor ID:                       GenuineIntel
CPU family:                      6
Model:                           106
Model name:                      Intel(R) Xeon(R) Platinum 8358 CPU @ 2.60GHz
Stepping:                        6
Frequency boost:                 enabled

# Result collection

Compile: `mpicc serial.c -lm -march=icelake-server -O3 -fopenmp`
Serial: 9.069154
Toggle: 7.444072

## 10K
Should converge on the 3578th iteration.
* Serial: 1241.080101
* MPI-8 : 173.817748 +OpenMP: 152.902791
  * computation: 0.046360  communication: 0.000089
* MPI-16: 100.195788 +OpenMP: 77.925535
  * computation: 0.026209  communication: 0.000104
* MPI-32: 66.373975  +OpenMP: 58.763544
  * computation: 0.017150  communication: 0.000124
* MPI-48: 57.670676  +OpenMP: 56.740736
  * computation: 0.014831  communication: 0.000161