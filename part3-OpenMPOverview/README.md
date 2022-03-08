# OpenMP Overview

> OpenMP is not magic!

## Shared Variables
Parallel Programming using Threads

----------------------------------------------------------------

Most commonly, many applications has single thread in each process, btu a single process can contain multiple threads. Each thread is like a child process contained within parent process.     
* Threads can see all data in parent process. 
* Threads can run on different cores. 
* Threads have potential for parallel speedup.

**Analogy:**
* Huge whiteboard: shared memory
* Different people: threads
* Do not write on the same place: not interfering with each other
* Have a place inaccessible to the others: **private data**

Each thread has its own PC(Program Counter, which has the address of the next instruction to be executed from memory.) and private data, as well as shared data with all other threads.

**Synchronisation:**
* Crucial for shared variables approach.   
* Most commonly use global barrier synchronisation (Coarse-grained), also can use **lock** (Fine-grained) and even **CAS** (Compare and Swap, atomic instruction guaranteed by hardware)
* Writing parallel codes relatively straightforward, access shared data **as and when** its needed.
* Getting correct code can be difficult.   

**Example:**
* Computing $asum = a_0+a_1+...+a_7$
  * Shared:
    * main array: a[8]
    * result: asum
  * private:
    * loop counter: i
    * loop limits: istart, istop
    * local sum: mysum
  * synchronisation: 
    * thread0: asum += mysum
    * barrier
    * thread1: asum += mysum

### Threads in HPC
* Threads existed before parallel computers
  * designed for concurrency
  * many more threads running than physical cores
    * scheduled / de-scheduled when needed
    * schedule policy: FIFO, LRU, LFU, CLOCK
* For parallel computing
  * typically run a single thread per core (**For affinity and avoid resources overload**)
  * want them all to run all the time (**Avoid Context-Switch overload**)
* Os optimisations
  * place threads on selected cores (`taskset` in Linux, `KMP_AFFINITY` in OpenMP, `numactl` in NUMA architecture)
  * stop them from migrating (**Mitigate Cache-miss and context switch**)

