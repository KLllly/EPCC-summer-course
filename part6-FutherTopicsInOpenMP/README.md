# Future topics in OpenMP

**Nested parallelism**:
* enabled with `OMP_NESTED` environment variable or the `omp_set_nested(1)` routine.  
* If a PARALLEL directive is encountered within another PARALLEL directive, a new term of threads will be created.
* The new team will **contain only one thread** unless nested parallelism is enabled.


**NUM_THREADS clause**:
* One way to control the number of threads used at each level is with the `num_threads(integer)` clause:
  ```c
  #pragma omp parallel for num_threads(4)
  for (int i = 0; i < N; i++) {
      #pragma omp parallel for num_threads(total_threads/4)
      for (int j = 0; j < M; j++) {
          // do computations
      }
  }
  ```
* The value set in the clause supersedes the value in the environment variable `OMP_NUM_THREADS` (or that set by `omp_set_num_threads(integer)`)    


**Orphaned directives**:
* Directives are active in the dynamic scope of a parallel region, not just its **lexical scope**. e.g.
  ```c
  void foo() {
      // do computations
  }
  #pragma omp parallel
  {
      foo()
  }
  ```
* Useful as it allows a **modular programming style**   
* But also can be confusing if the call tree is complicated.
* Extra rules about data scope attributes:
  * Variables **in the argument list** inherit their data scope attribute from the calling routine.
  * **Global variables** in C++ and COMMON blocks or module variables in Fortran are shared, unless declared `THREADPRIVATE`.
  * **static local variables** in C/C++ and **SAVE variables** in Fortran are shared.  
  * **All other local variables** are private.   
* Binding rules:
  * **`DO/FOR, SECTIONS, SINGLE, MASTER and BARRIER`** directives always bind to the nearest enclosing PARALLEL directive.
* Thread private global variables:
  * It can be convenient for each thread to have its own copy of variables with global scope.
  * Outside parallel regions and in **`MASTER`** directives, accesses to these variables refer to the master thread’s copy.
  * `#pragma omp threadprivate (list)`
  * This directive must be at file or namespace scope, **after all declarations of variables in list and before any references to variables in list** or **static variables**.
  * Difference between `PRIVATE`: Reference to [StackOverflow](https://stackoverflow.com/questions/18022133/difference-between-openmp-threadprivate-and-private)
  
||PRIVATE|THREADPRIVATE|
|:--:|:--:|:--:|
|list variable|Any|Declared but not referenced or static|
|Local to|Region|Thread|
|Placed|Stack|Heap or TLS|
|Lifetime|Data scoping clause|Persist across region, recycle along with thread (maybe at the end of program)|
|Storage-associated|Each thread has a private copy.|**Master thread use the original**, other threads has a private copy.|
  * Here is an example of `THREADPRIVATE`, which illustrates the lifetime of value and the storage association between master thread and program:
  ```c
  static int k;
  #pragma omp parallel for num_threads(4) threadprivate(k)
  for (int i = 0; i < 16; i++) {
      printf("This is thread %2d with k %2d entered i %2d\n", omp_get_thread_num(), k, i);
  }
  printf("Original k: %d\n", k);
  /* output: 
    This is thread  1 with k  0 entered i  4
    This is thread  1 with k  4 entered i  5
    This is thread  1 with k  5 entered i  6
    This is thread  1 with k  6 entered i  7
    This is thread  2 with k  0 entered i  8
    This is thread  2 with k  8 entered i  9
    This is thread  2 with k  9 entered i 10
    This is thread  2 with k 10 entered i 11
    This is thread  0 with k  0 entered i  0
    This is thread  0 with k  0 entered i  1
    This is thread  0 with k  1 entered i  2
    This is thread  0 with k  2 entered i  3
    This is thread  3 with k  0 entered i 12
    This is thread  3 with k 12 entered i 13
    This is thread  3 with k 13 entered i 14
    This is thread  3 with k 14 entered i 15
    Original k: 3 # exact the same as thread 0
  */
  ```

**COPYIN clause**:
* Allows the values of the master thread’s `THREADPRIVATE` data to be copied to all other threads at the start of a parallel region.

**Timing routines**:
* Return current wall clock time (relative to arbitrary origin) with: `double omp_get_wtime(void)`
* Return clock precision with: `double omp_get_wtick(void)`
* Timers are local to a thread, so must make both calls on the same thread to get the duration.
* **NO guarantee about resolution!**




## Reference
- [Difference between OpenMP threadprivate and private | StackOverflow](https://stackoverflow.com/questions/18022133/difference-between-openmp-threadprivate-and-private)