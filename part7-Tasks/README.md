# OpenMP Tasks

**Definition**:   
* Task construct defines a section of code.
* Inside a parallel region, a thread encountering a task construct will package up the task for execution.
* Some thread in the parallel region will execute the task at some point in the future.
```c
#pragma omp task [clauses]
structured-block
```

**Data Sharing**:    
* The default for tasks is usually `FIRSTPRIVATE`, because the task may not be executed until later (and variables may have gone out of scope).
* Variables that are shared in all constructs starting from the innermost enclosing parallel construct are shared.
```c
#pragma omp parallel shared(A) private(B)
{
    ...
    #pragma omp task
    {
        int C;
        compute(A, B, C); // A is shared, B is firstprivate, C is private
    }
}
```
* Getting data attribute scoping right can be quite tricky, using **`default(none)`** is a good idea.

**Task complete time**:
* At thread barriers, whether explicit or implicit
  * at the end of current parallel region, there is an implicit barrier to all tasks
* At `taskwait` directive
  * Wait until all tasks defined in the current task have completed.
  * `#pragma omp taskwait`
  * Note: applies only to tasks generated in the current task, not to "descendants".

```c
// Example of taskwait: Binary Tree Traversal

void postorder(node* p) {
    if (p->left) {
        #pragma omp task
        {
            postorder(p->left);
        }
    }
    if (p->right) {
        #pragma omp task 
        {
            postorder(p->right);
        }
    }
    #pragma omp taskwait // Parent task suspended until children tasks complete
    process(p->data);
}
```

**Task switching**:
* Certain constructs have task scheduling points at defined locations within them
* When a thread encounters a task scheduling point, it is allowed to suspend the current task and execute another (called task switching) 
* It can then return to the original task and resume.
* Example of **`taskyield`** from [SC15](https://www.openmp.org/wp-content/uploads/sc15-openmp-CT-MK-tasking.pdf)
```c
omp_lock_t *lock;
for (int i = 0; i < n; i++) {
    #pragma omp task
    {
        something_useful();
        while (!omp_test_lock(lock)) {
            #pragma omp taskyield
            // to make full use of computing resources 
            // and avoid deadlock situations.
        }
        something_critical();
        omp_unset_lock(lock);
    }
}
```


## Reference
- [OpenMP Tasking | SC15](https://www.openmp.org/wp-content/uploads/sc15-openmp-CT-MK-tasking.pdf)