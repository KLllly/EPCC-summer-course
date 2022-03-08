# OpenMP and MPI

* Clustered architectures have distributed memory systems,where each node consist of a traditional memory multiprocessor.
* Single address space within each node, but separate nodes have separate address space.

## Development / maintenance
* In most cases, development and maintenance will be harder than for an MPI code, and much harder than for an OpenMP code.
* If MPI code already exists, addition of OpenMP may not be too much overhead.
* In some cases, it may be possible to use a simpler MPI implementation because the need for scalability is reduced. 
  * e.g. 1-D domain decomposition instead of 2-D

## Portability
* Both OpenMP and MPI are themselves highly portable (but not perfect). 
* Combined MPI/OpenMP is less so
  * main issue is thread safety of MPI 
  * if maximum thread safety is assumed, portability will be reduced
* Desirable to make sure code functions correctly (maybe with conditional compilation) as stand-alone MPI code (and as stand-alone OpenMP code)


## Performance
Four possible performance reasons for mixed OpenMP/MPI codes:    
* Replicated data
  * Replicated data strategy
    * all processes have a copy of a major data structure
    * classical domain decomposition code have replication in halos
    * MPI buffers can consume significant amounts of memory
  * A pure MPI code needs one copy per process/core
  * A mixed code would only require one copy per node
    * data structure can be shared by multiple threads within a process
    * MPI buffers for intra-node messages no longer required
  * Halo regions are a type of replicated data
    * Although the amount of halo data does decrease as the local domain size decreases, it eventually starts to occupy a significant amount fraction of the storage
* Poorly scaling MPI codes
  * If the MPI version of the code scales poorly, then a mixed MPI/OpenMP version may scale better.
  * May be true in cases where OpenMP scales better than MPI due to:
    * Algorithmic reasons. e.g. Load balancing easier to achieve in OpenMP.
    * Simplicity reasons. e.g. 1-D domain decomposition.
* Load balancing
  * Load balancing between MPI processes can be hard
    * need to transfer both computational tasks and data from overloaded to underloaded processes
    * transferring small tasks may not be beneficial
    * having a global view of loads may not scale well
    * may need to restrict to transferring loads only between neighbors
  * Load balancing between threads is much easier
    * only need to transfer tasks, not data
    * overheads are lower, so fine grained balancing is possible 
    * easier to have a global view
  * For applications with load balance problems, **keeping the number of MPI processes small can be an advantage**. (Maybe statistically)
* Mix-mode implementation effect:
  * reduce within a node via OpenMP reduction clause
  * then reduce across nodes with `MPI_Reduce`
  * send one large message per node instead of several small ones
  *  reduces latency effects, and contention for network injection

## Styles of mixed-mode programming
### Master-only
* Definition: all MPI communication takes place **in the sequential part** of the OpenMP program (no MPI in parallel regions)

* Advantages
  * simple to write and maintain 
  * clear separation between outer (MPI) and inner (OpenMP) levels of parallelism
  * no concerns about synchronising threads before/after sending messages
* Disadvantages
  * threads other than the master are idle during MPI calls
  * all communicated data passes through the cache where the master thread is executing.
  * inter-process and inter-thread communication do not overlap.
  * only way to synchronise threads before and after message transfers is by parallel regions which have a relatively high overhead.
  * packing/unpacking of derived data types is sequential.

```c
#pragma omp parallel
{
    work...
}

ierror = MPI_Send(...);

#pragma omp parallel
{
    work...
}
```

### Funneled
* Definition
  * all MPI communication **takes place through the same (master) thread**, can be inside parallel regions
* Advantages
  * relatively simple to write and maintain 
  * cheaper ways to synchronise threads before and after message transfers
  * possible for other threads to compute while master is in an MPI call
* Disadvantages
  * less clear separation between outer (MPI) and inner (OpenMP) levels of parallelism
  * all communicated data still passes through the cache where the master thread is executing.
  * inter-process and inter-thread communication still do not overlap.

```c
#pragma omp parallel
{
    work...
    #pragma omp barrier
    {
        #pragma omp master
        {
            ierror = MPI_Send(...);
        }
    }
    #pragma omp barrier
    work...
}
```

### Serialized
* Definition 
  * only one thread makes MPI calls at any one time (Don't know which thread will call)
  * distinguish sending/receiving threads via MPI tags or communicators
  * be very careful about race conditions on send/recv buffers etc.
* Advantages
  * easier for other threads to compute while one is in an MPI call
  * can arrange for threads to communicate only their “own” data (i.e. the data they read and write). 
* Disadvantages
  * getting harder to write/maintain
  * more, smaller messages are sent, incurring additional latency overheads
  * **need to use tags or communicators to distinguish between messages from or to different threads in the same MPI process**.

```c
#pragma omp parallel
{
    work...
    #pragma omp critical
    {
        ierror = MPI_Send(...);
    }
    work...
}
```

### Multiple
* Definition
  * MPI communication simultaneously in more than one thread
  * some MPI implementations don’t support this
  * and those which do mostly don’t perform well
* Advantages
  * Messages from different threads can (in theory) overlap 
  * many MPI implementations serialise them internally.
  * Natural for threads to communicate only their “own” data
  * Fewer concerns about synchronising threads (responsibility passed to the MPI library) 
* Disdavantages
  * Hard to write/maintain
  * Not all MPI implementations support this – loss of portability
  * Most MPI implementations don’t perform well like this
  * Thread safety implemented crudely using global locks.

```c
#pragma omp parallel
{
    work...
    ierror = MPI_Send(...);
    work...
}
```

### MPI execution environment
#### MPI_Init_thread
* works in a similar way to MPI_Init by initializing MPI on the main thread.
* Two integer arguments:
  * Required ([in] Level of desired thread support)
  * Provided ([out] Level of provided thread support)
* Levels from [MPICH](https://www.mpich.org/static/docs/v3.1/www3/MPI_Init_thread.html):
  * MPI_THREAD_SINGLE
    * Only one thread will execute.
  * MPI_THREAD_FUNNELED
    * The process may be multi-threaded, but only the main thread will make MPI calls (all MPI calls are funneled to the main thread).
  * MPI_THREAD_SERIALIZED
    * The process may be multi-threaded, and multiple threads may make MPI calls, but only one at a time: MPI calls are not made concurrently from two distinct threads (all MPI calls are serialized).
  * MPI_THREAD_MULTIPLE
    * Multiple threads may call MPI, with no restrictions.

#### MPI_Query_thread()
* returns the current level of thread support
* `int MPI_Query_thread(int *provided)`
* Need to compare the output manually:
```c
int provided, requested;
...
MPI_Query_thread(&provided);
if (provided < requested) {
    printf("Not a high enough level of thread support!\n");
    MPI_Abort(MPI_COMM_WORLD, 1);
    ...
}
```

## Reference
- [MPI_Init_thread in MPICH](https://www.mpich.org/static/docs/v3.1/www3/MPI_Init_thread.html)
- 