# Work sharing directives
## Experiment
```c
#pragma omp parallel default(none) private(i, istart, istop, mypi) shared(pi,N)
{
    mypi = 0.0;
    for (i = istart; i < istop; i++) {
        mypi = mypi + 1.0/(1.0 + pow((double)(i - 0.5)/((double) N), 2.0));
    }
    
    #pragma omp critical // important
    {
        pi = pi + mypi;
    }
}
```
If not adding the critical region, there will be data races between threads when reading and writing a shared variable `pi`. In the critical region, threads are serial.

`#pragma omp parallel` is spawning threads, `#pragma omp for` is splitting for loop into many pieces, and `#pragma omp parallel for` combines both together.

## Lecture 
**Parallel do/for loops:**
* A parallel do/for loop divides up the iterations of the loop between threads. 
* There is an **implicit** synchronization point at the end of the loop: all thread must finish their iterations before any thread can proceed. (**Except `nowait` identifier**)
* Loop has to have determinable trip count (Since OpenMP directive will be translated in compile time.)
  * `for (var = a;  var logical-op b; incr-exp)`
    * where `logical-op` is one of `<, <=, >, >=`
    * `incr-exp` is `var = var +/- incr` or semantic equivalents of `var++/--` 
    * **Cannot** modify `var` within the loop body.
  * Jumps out of the loop are not permitted.
* How can you tell if a loop is parallel or not?    
  * Useful test: if the loop gives **the same answers if it is run in reverse order**, then it is almost certainly parallel
  * In other words, there should be **no data dependency** between threads.
* Clauses:
  * PRIVATE(var): var will be private in each thread, without initialization, i.e. random value. (loop index is PRIVATE by default)
  * FIRSTPRIVATE(var): var will be initialized with the value before parallelisation.
  * LASTPRIVATE(var): var will bring the last assigned value in parallelisation into the serial part.
  * REDUCTION(reduction-op:val): threads will execute reduction-op (e.g. +,*,max,min) on val.
* schedule(kind[, chunksize]): 
  * STATIC: divide iteration space equally and assign with order
  ```
  schedule(static):      
  ************
              ************       
                          ************
                                      ************
  schedule(static, 4):    
  ****            ****            ****
      ****            ****             ****
          ****            ****              ****
              ****            ****              ****
  ```
  * DYNAMIC: divide iteration space equally and assign to threads on a **first-come-fist-served** basis.
  ```
  schedule(dynamic):     
  *   ** **  * * *  *      *  *    **   *  *  * 
    *       *     *    * *     * *   *    *  
   *       *    *     * *   *   *     *  *       
     *  *     *    * *    *  *    *    *    ** 
  schedule(dynamic, 4):
  ****                    ****
      ****        ****             ****
          ****                 ****
              ****    ****              ****
  ```
  * GUIDED: similar to DYNAMIC, but the chunks start off large and get smaller exponentially. The size of next chunk is proportional to the number of remaining iterations divided by the number of threads. (i.e. remaining//threads). The chunksize specifies the minimum size of the chunks.
  ```
  schedule(guided):
  *******                     ***           *
         *******                        ***  *    *
                *******          ***            *
                       *******      ***        *
  schedule(guided, 3):
        ******            ***
  ******                     ***
              ******                ***
                    ******       ***
  ```
  * AUTO: lets the runtime have full freedom to choose its own assignment of iterations to threads. **If the parallel loop is executed many times, the runtime can evolve a good schedule which has good load balance and low overheads.**
  * RUNTIME: defer the choice of schedule to runtime, when it is determined by the value of the environment variable `OMP_SCHEDULE`. e.g. `export OMP_SCHEDULE="guided,4"` **Noted: illegal to specify a chunksize in the code with RUNTIME schedule**.
  * chunksize: if not specified, the iteration space is divided into approximately equal chunks, and one chunk is assigned to each thread in order (**block schedule**); if specified, the iteration space is divided into chunks, each of chunksize iterations, and the chunks are assigned cyclically to each thread in order (**block cyclic schedule**)


**Which to use?**
* **STATIC**: best for **load balanced loops** - least overhead.
* **STATIC, n**: good for loops with mild or smooth load imbalance, but can induce overheads. (**multiple iterations at each time can mitigate load imbalance statistically.**)
* **DYNAMIC**: useful if iterations have **widely varying loads**, but ruins data locality.
* **GUIDED**: often less expensive than DYNAMIC, but beware of loops where the first iterations are the most expensive! (Since it will cause huge load imbalance for the thread taking first few iterations.)
* **AUTO**: may be useful if the loop is executed many times over.
  
**Nested rectangular loops**:    
* We can use `collapse(num_loops)` to parallelise multiple loops. 
* Will form a single loop of the multiplication of length at each loop and parallelise that.
* Useful is the outermost loop length N

> **Some Tips on Using Nested Parallelism** from [Sun Studio](https://docs.oracle.com/cd/E19205-01/819-5270/aewbc/index.html)
> * Nesting parallel regions provides an immediate way to allow more threads to participate in the computation.
>   * For example, suppose you have a program that contains two levels of parallelism and the degree of parallelism at each level is 2. Also, suppose your system has four cpus and you want use all four CPUs to speed up the execution of this program. Just parallelizing any one level will use only two CPUs. You want to parallelize both levels.
> * Nesting parallel regions can easily create too many threads and oversubscribe the system. Set **`SUNW_MP_MAX_POOL_THREADS`** and **`SUNW_MP_MAX_NESTED_LEVELS`** appropriately to limit the number of threads in use and prevent runaway oversubscription.
> * Creating nested parallel regions adds overhead. **If there is enough parallelism at the outer level and the load is balanced, generally it will be more efficient to use all the threads at the outer level of the computation than to create nested parallel regions at the inner levels.**
>   * For example, suppose you have a program that contains two levels of parallelism. The degree of parallelism at the outer level is 4 and the load is balanced. You have a system with four CPUs and want to use all four CPUs to speed up the execution of this program. Then, in general, using all 4 threads for the outer level could yield better performance than using 2 threads for the outer parallel region, and using the other 2 threads as slave threads for the inner parallel regions.

**SINGLE directive**:
* `SINGLE` indicates that a block of code is to be executed by a single thread only.
* The first thread reaching the `SINGLE` directive will execute the block.
* There is **an implicit synchronization point** at the end of the block: all other threads will wait until the block has been executed.
* As the `for` clause, `SINGLE` also can take `PRIVATE` and `FIRSTPRIVATE`.
* Directive must contain a structured block, **can not branch into or out of it**.

**MASTER directive**:
* `MASTER` indicates that only thread with index 0 will execute the block.
* There is **NO synchronisation point** at the end of the block: other threads will skip the block and continue executing. (different from `SINGLE`)

**SECTION directive**:
* Allow separate blocks of code to be executed in parallel (e.g. several independent subroutines)
* There is a synchronisation point at the end of the blocks: all threads must finish their blocks before any thread can proceed.
* Not scalable: the source code determines the amount of parallelism available.
* Rarely used, except with nested parallelism.
* Also can take `PRIVATE` and `FIRSTPRIVATE` and `LASTPRIVATE`
* Each section must contain a structured block: can not branch into or out of a section.
```c
// section without parallel with be assigned to single thread 
#pragma omp sections
{
    #pragma omp section
    { 
        printf ("id = %d, \n", omp_get_thread_num());
    }

    #pragma omp section
    { 
        printf ("id = %d, \n", omp_get_thread_num());
    }
}
// Output:  
//      id = 0,
//      id = 0,
// ----------------------------------------------------------------
// with parallel directive, sections inside will be assigned to different threads.
#pragma omp parallel sections num_threads(2)
{
    #pragma omp section
    { 
        printf ("id = %d, \n", omp_get_thread_num());
    }

    #pragma omp section
    { 
        printf ("id = %d, \n", omp_get_thread_num());
    }
}
// Output:  
//      id = 0,
//      id = 1,
```

**WORKSHARE directive**: (Only available in FORTRAN)
* Kind of **IMPLICIT**, can be replaced by explicit for/do clause. (Not encouraged to use.)
* The workshare construct divides the execution of the enclosed structured block into separate units of work, and causes the threads of the team to share the work such that **each unit is executed only once by one thread**, in the context of its implicit task.
* No Schedule clause: distribution of work units to threads is entirely up to the compiler!  
* There is a synchronisation point at the end of workshare: all threads must finish their work before any thread can proceed.
* No function calls except array intrinsics and those declared **ELEMENTAL**
* Examples: 
```fortran
!$OMP PARALLEL WORKSHARE REDUCTION(+:t)
A = B + C 
WHERE (D .ne. 0) E = 1/D
t = t + SUM(F) 
FORALL (i=1:n, X(i)=0) X(i)= 1
!$OMP END PARALLEL WORKSHARE

---------------------------------------------

! In the following example, 
! the barrier at the end of the first workshare region 
! is eliminated with a nowait clause. 
! Threads doing CC = DD immediately begin work on EE = FF 
! when they are done with CC = DD.


SUBROUTINE WSHARE2(AA, BB, CC, DD, EE, FF, N)
INTEGER N
REAL AA(N,N), BB(N,N), CC(N,N)
REAL DD(N,N), EE(N,N), FF(N,N)

!$OMP PARALLEL 
!$OMP WORKSHARE
AA = BB
CC = DD
!$OMP END WORKSHARE NOWAIT
!$OMP WORKSHARE
EE = FF
!$OMP END WORKSHARE
!$OMP END PARALLEL
END SUBROUTINE WSHARE2
```


### Reference
- [OpenMP Clauses](https://docs.microsoft.com/en-us/cpp/parallel/openmp/reference/openmp-clauses?view=msvc-170)
- [How are firstprivate and lastprivate different than private clauses in OpenMP?](https://stackoverflow.com/questions/15304760/how-are-firstprivate-and-lastprivate-different-than-private-clauses-in-openmp)
- [OpenMP parallel for loops: scheduling](https://ppc.cs.aalto.fi/ch3/schedule/)
- [OpenMP: For & Scheduling](http://jakascorner.com/blog/2016/06/omp-for-scheduling.html)
- [Sun Studio : Chapter 4 Nested Parallelism](https://docs.oracle.com/cd/E19205-01/819-5270/aewbc/index.html)
- [OpenMP WORKSHARE construct](https://www.openmp.org/spec-html/5.0/openmpsu39.html)
- [openmp examples 4.5.0](https://www.openmp.org/wp-content/uploads/openmp-examples-4.5.0.pdf)