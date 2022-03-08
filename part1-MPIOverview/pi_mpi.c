// Pi calculation using the approximation formula.
// $\frac{\pi}{4} = \int_{0}^{1}{\frac{dx}{1+x^{2}}} \approx \frac{1}{N}\sum_{i=1}^{N}{\frac{1}{1+(\frac{i-\frac{1}{2}}{N})^{2}}}$ 

// compile command: mpicc pi_mpi.c -lm (-fopenmp if you want to use OpenMP) 
// run command: mpirun -np $NUM_PROCESS ./a.out $N_VALUE(default 100000)

#include <stdio.h>  // printf
#include <stdlib.h> // atoi
#include <math.h>   // M_PI, pow, need to compile with -lm
#include <mpi.h>    // MPI
#include <assert.h> // assert

#if defined(_OPENMP)
#include <omp.h>    // OpenMP
#endif

#define min(a,b) ((a) < (b) ? (a) : (b))

int main (int argc, char* argv[])
{
    int rank, size, error;
    double pi = 0.0;
    double partial_pi = 0.0;
    double exact_pi = M_PI;
    double start = 0.0, end = 0.0;
    MPI_Comm comm = MPI_COMM_WORLD;

    error = MPI_Init(NULL, NULL);
    assert(error == MPI_SUCCESS);
    
    //Get process ID
    MPI_Comm_rank (comm, &rank);
    
    //Get processes Number
    MPI_Comm_size (comm, &size);
    
    int N = 100000; // set default N value
    if (argc == 2) { // read N from command line
        N = atoi(argv[1]);
    } 

    if (rank == 0) { // master process
        #if defined(_OPENMP)
        printf("MPI+OpenMP version of Pi calculation.\n");
        #else
        printf("MPI version of Pi calculation.\n");
        #endif
        printf("N value is %d\n", N);
    }

    //Synchronize all processes and get the start time
    MPI_Barrier(comm);

    if (rank == 0) { // master process
        start = MPI_Wtime();
    }

    int part = N/size + 1;
    int istart = part * rank + 1;
    int istop = min(N, istart + part - 1);
    
    //Each process calculates a part of the sum
    #if defined(_OPENMP)
    #pragma omp parallel for reduction(+:partial_pi)
    #endif
    for (int i = istart; i <= istop; i++) {
        partial_pi += 1.0/(1.0 + pow((double)(i - 0.5)/((double) N), 2.0));
    }
    
    //Sum up all results
    MPI_Reduce(&partial_pi, &pi, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    /* Using MPI_Reduce to sum up is better than using MPI_Send and MPI_Recv
     * the send and recv version is as below:
    if (rank == 0) { // main process
        pi = partial_pi;
        for (int i = 1; i < size; i++) {
            double temp;
            MPI_Recv(&temp, 1, MPI_DOUBLE, i, 0, comm, MPI_STATUS_IGNORE);
            printf("rank 0 receiving from rank %d, partial_pi = %f\n", i, temp);
            pi += temp;
        }     
    } else { // replica process
        MPI_Send(&partial_pi, 1, MPI_DOUBLE, 0, 0, comm);
    }
    
     */
    //Synchronize all processes and get the end time
    MPI_Barrier(comm);
    if (rank == 0) { // master process
        end = MPI_Wtime();
    }

    printf("On rank %d, calcalute from %d to %d, partial_pi is %lf\n", rank, istart, istop, partial_pi*4.0/((double)N));
    
     //Caculate and print PI
    if (rank==0) {
        pi = 4.0/((double) N) * pi;
        printf("Calculated pi: %lf, Exact pi: %lf, Error: %lf\n", pi, exact_pi, fabs(pi - exact_pi));
        printf("Using time: %lf\n", end - start);
    }
    
    error = MPI_Finalize();
    assert(error == MPI_SUCCESS);
    
    return 0;
}