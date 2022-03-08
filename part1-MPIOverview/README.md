# Message Passing
Parallel Programming using Process

---

### Concept of message passing parallelism    
* Distributed memory: data isolation between different workers, thus no danger of corrupting someone else's data  
* Same problem: the processes on different nodes attached to the interconnect   
* Explicit communication: by sending and receiving data   
* Synchronization: ensure the message is sent and received   

### Communication modes   
||Send completes|Analogy|Receive|
|:--:|:--:|:--:|:--:|
|Synchronous|until the message has started to be received|faxing a letter|Usually wait util message arrives|
|Asynchronous|as soon as the message has gone|Posting a letter|Unusual|

### Point-to-Point(P2P) Communications
* one sender and one receiver 
* simplest form of message passing      
* relies on matching send and receive     
* **Analogy with C++11**
  * Send: `std::promise`
  * Receive: `std::future`
  * Sender needs to input values into promise, otherwise when receiver tries to get values from `std::promise.get_future()`, it blocks.   

### Collective Communications 
* Broadcast: from one process to all others  
> Before: \[[0,1,2,3], [], [], []]
> After: \[[0,1,2,3], [0,1,2,3], [0,1,2,3], [0,1,2,3]] 
* Scatter: Information scattered to many processes 
> Before: \[[0,1,2,3], [], [], []]
> After: \[[0], [1], [2], [3]]
* Gather: Information gathered onto one process
> Before: \[[0], [1], [2], [3]]
> After: \[[0,1,2,3], [1], [2], [3]]
* Reduction: form a global sum, product, max, min, etc.
> Before: \[[0], [1], [2], [3]]
> After: \[[6], [1], [2], [3]] (Reduction sum)
> After: \[[3], [1], [2], [3]] (Reduction max)
> After: \[[0], [1], [2], [3]] (Reduction min)
* **AllReduce: reduction and broadcast**
> Before: \[[0], [1], [2], [3]]
> After: \[[6], [6], [6], [6]] (Reduction sum and broadcast)    


### Hardware   
* One process per processor-core    
  * Natural map to distributed memory.   
  * Messages go over the interconnect between nodes.   
* Multiple processes on each processor-core  
  * Share access to the network when communicating with other nodes
  * Messages between processes on the same nodes are fast by using the shared memory.  

### Computer Architecture
[**Flynn's taxonomy**](https://en.wikipedia.org/wiki/Flynn%27s_taxonomy) classified computer architecture according to the  number of concurrent instruction stream and data stream into four types: 
> S -> Single
> M -> Multiple
> I -> Instruction
> D -> Data
> P -> Program
> T -> Thread

||SISD|SIMD|MISD|MIMD|
|:--:|:--:|:--:|:--:|:--:|
|Concept|A sequential computer which exploits no parallelism|A single instruction is simultaneously applied to multiple different data streams|An uncommon architecture which is generally used for fault tolerance|Multiple autonomous processors simultaneously executing different instructions on different data|
|Example|uniprocessor machines|SIMT in GPU, AVX in CPU|Space Shuttle flight control computer|multi-core superscalar processors, and distributed systems|

* SPMD
  * A subcategory of MIMD. 
  * Tasks are split up and run simultaneously on multiple processors with different input in order to obtain results faster. 
  * each has a unique ID
  * processes can take different branches in the same code


### Exercise
An approximation to the value $\pi$ can be obtained from the following expression:   
$\frac{\pi}{4} = \int_{0}^{1}{\frac{dx}{1+x^{2}}} \approx \frac{1}{N}\sum_{i=1}^{N}{\frac{1}{1+(\frac{i-\frac{1}{2}}{N})^{2}}}$    
where the answer becomes more accurate with increasing N. Iterations over i are independent so the calculation can be parallelized.     

#### Solve
* If there is **single node**, MPI is not preferred compared to **OpenMP** due to its overload in initializing the MPI_COMM_WORLD and more complicated programming techniques. OpenMP with Reduction clause can solve this program well using multiple threads.    
* If there are **multiple nodes**, we can use MPI to enlarge the computing world and employ more computing resources, while OpenMP can still be used inside each node.   

**Usage**: For OpenMP version, refer to `pi_openmp.c`, for MPI(+OpenMP) version, refer to `pi_mpi.c`. 

**Interesting Founds:** For the continuous computation of pi calculation, OpenMP can not guarantee the performance, in fact, the serial version is faster than OpenMP version due to the data competition between different threads to the reduction variable in OpenMP clause.  

**Besides**, the `istart` and `istop` definitions in the video is not correct since it neglects some corner values when the N value is not divisible by the MPI world size. (e.g. N=450 -n=4, it will only calculate from 1-448 instead of 1-450.)   
 

### Usage of Slurm
* salloc - Obtain a job allocation
* sbatch - Submit a batch script for later execution
* srun - Obtain a job allocation and execute an application
  * --reservation: Operate only on jobs using the specified reservation
  * --time: Wall clock time limit
  * -N: Node count required for the job
  * -n: number of tasks to be launched
  * --partition: partition queue in which to run the job
  * --job-name: Job name

### MPI versions
- [Open MPI](https://www.open-mpi.org/)
- [MPICH2](https://www.mpich.org/)
- [MVAPICH2](https://mvapich.cse.ohio-state.edu/)
- [Intel MPI](https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library.html)

### Quiz
1. What is MPI
   > The Message-Passing Interface.
   > A library for distributed-memory parallel programming.
2. To run an MPI program requires
   > special libraries imported as header file "mpi.h".
3. After initializing an MPI program with "mpirun -n 4 ./mympiprogram", what does the call to MPI_Init do?
   > Enable the 4 independent programs subsequently communicate with each other.
   > MPI_Init will set a barrier before executing the remaining code. If no MPI_Init is called, program will execute independently.
4. If you call MPI receive and there is no incoming message, what happens? 
   > the Recv waits until a message arrives(potentially waiting forever) since MPI_Recv is synchronous.   
5. If you call MPI synchronous send (MPI_Ssend) and there is no receive posted
   > the send waits until a receive is posted (potentially waiting forever) since synchronous send is like faxing.
6. If you call MPI asynchronous send (MPI_Bsend) and there is no receive posted
   > the message is stored and delivered later on (if possible)
   > the program continues execution regardless of whether the message is arrived, since asynchronous send is like posting, the sender doesn't care the result.
7. The MPI receive has a parameter "count" - what does this mean?
   > the size available for storing the message (in terms, e.g. integers), so the total buffer size is count*size(data_type).
8. What happens if the incoming message is larger than "count"?
   > the receive fails with an error as the returned value, since it will cause buffer overflow and MPI doesn't want to corrupt the data.
9. What happens if the incoming message (of size "n") is smaller than "count"? 
   > the first "n" items are received and the remaining buffer is **not** zeroed.
10. How is the actual size of the incoming message reported?
    > it is stored in the Status parameter in MPI_Recv.
    > MPI_Status contains: **count**, cancelled, **MPI_SOURCE**, MPI_TAG, MPI_ERROR


### Reference
- [MPI Tutorial](https://mpitutorial.com/tutorials/mpi-broadcast-and-collective-communication/)
- [NCCL collective communication](https://docs.nvidia.com/deeplearning/nccl/user-guide/docs/usage/collectives.html)
- [Wikipedia SPMD](https://en.wikipedia.org/wiki/SPMD)
- [Flynn's taxonomy](https://en.wikipedia.org/wiki/Flynn%27s_taxonomy)
- [slurm workload manager summary page](https://slurm.schedmd.com/pdfs/summary.pdf)
- [MPI_Status structure](https://docs.microsoft.com/en-us/message-passing-interface/mpi-status-structure)