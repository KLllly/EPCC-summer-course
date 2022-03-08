#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "traffic.h"

#include <mpi.h>

#if defined(_OPENMP)
#include "omp.h"
#endif

#define NCELL 100000
#define min(a, b) ((a) < (b) ? (a) : (b))

int main(int argc, char **argv)
{

    int rank, size, error;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Request send_req, recv_req;
    MPI_Status send_status, recv_status;

    error = MPI_Init(NULL, NULL);
    assert(error == MPI_SUCCESS);

    // Get process ID
    MPI_Comm_rank(comm, &rank);

    // Get processes Number
    MPI_Comm_size(comm, &size);

    #if defined(_OPENMP)
    int n_threads = omp_get_num_procs() / size;
    #endif

    int *oldroad, *newroad;

    int i, iter, nmove, ncars;
    int maxiter, printfreq;
    int nmove_all = 0;

    float density;

    double tstart, tstop;

    int PART_NCELL = (NCELL + size - 1) / size;
    int istart = PART_NCELL * rank;
    int istop = min(PART_NCELL * (rank + 1), NCELL);
    int irange = istop - istart;
    int last_rank = (rank + size - 1) % size;
    int next_rank = (rank + 1) % size;
    printf("Rank %d from %d to %d, range %d\n", rank, istart, istop, irange);
    if (rank == 0)
    {
        oldroad = (int *)malloc((NCELL + 2) * sizeof(int));
        newroad = (int *)malloc((NCELL + 2) * sizeof(int));
    }
    else
    {
        oldroad = (int *)malloc((irange + 2) * sizeof(int));
        newroad = (int *)malloc((irange + 2) * sizeof(int));
    }

    maxiter = 200000000 / NCELL;
    // maxiter = 10;
    printfreq = maxiter / 10;

    // Set target density of cars

    density = 0.52;

    if (rank == 0)
    {
        printf("Length of road is %d\n", NCELL);
        printf("Number of iterations is %d \n", maxiter);
        printf("Target density of cars is %f \n", density);

        // Initialise road accordingly using random number generator
        printf("Initialising road ...\n");
        ncars = initroad(&oldroad[1], NCELL, density, SEED);
        printf("...done\n");
        printf("Actual density of cars is %f\n\n", (float)ncars / (float)NCELL);
        for (int i = 1; i < size; i++)
        {
            printf("Sending %d data to rank %d\n",
                   min(PART_NCELL, NCELL - i * PART_NCELL), i);
            MPI_Send(&oldroad[1] + PART_NCELL * i,
                     min(PART_NCELL, NCELL - i * PART_NCELL), MPI_INT, i, 0, comm);
        }
    }
    else
    {
        MPI_Recv(&oldroad[1], irange, MPI_INT, 0, 0, comm, &recv_status);
        // assert(recv_status.MPI_ERROR == MPI_SUCCESS);
        assert(recv_status.MPI_SOURCE == 0);
        printf("Rank[%d] received initialized data from rank[0]\n", rank);
    }

    MPI_Barrier(comm);
    if (rank == 0)
    {
        tstart = gettime();
    }
    for (iter = 1; iter <= maxiter; iter++)
    {
        /*
          void updatebcs(int *road, int n) {
            road[0] = road[n];
            road[n + 1] = road[1];
          }
        */
        if (rank % 2 == 0)
        {
            MPI_Send(&oldroad[irange], 1, MPI_INT, next_rank, 0, comm);
            MPI_Send(&oldroad[1], 1, MPI_INT, last_rank, 0, comm);
            MPI_Recv(&oldroad[0], 1, MPI_INT, last_rank, 0, comm,
                     &recv_status);
            MPI_Recv(&oldroad[irange + 1], 1, MPI_INT, next_rank, 0, comm,
                     &recv_status);
        }
        else
        {
            MPI_Recv(&oldroad[0], 1, MPI_INT, last_rank, 0, comm,
                     &recv_status);
            MPI_Recv(&oldroad[irange + 1], 1, MPI_INT, next_rank, 0, comm,
                     &recv_status);
            MPI_Send(&oldroad[irange], 1, MPI_INT, next_rank, 0, comm);
            MPI_Send(&oldroad[1], 1, MPI_INT, last_rank, 0, comm);
        }
        // Apply CA rules to all cells
        nmove = 0;

#if defined(_OPENMP)
#pragma omp parallel for num_threads(n_threads) reduction(+:nmove)
#endif
#if 1
        // Simplicity version using bitwise operations
        for (int i = 1; i <= irange; i++)
        {
            newroad[i] =
                (oldroad[i] & oldroad[i + 1]) | (oldroad[i - 1] & (!oldroad[i]));
            nmove += oldroad[i] & (!newroad[i]);
        }
#else
        for (i = 1; i <= irange; i++)
        {
            if (oldroad[i] == 1)
            {
                if (oldroad[i + 1] == 1)
                {
                    newroad[i] = 1;
                }
                else
                {
                    newroad[i] = 0;
                    nmove++;
                }
            }
            else
            {
                if (oldroad[i - 1] == 1)
                {
                    newroad[i] = 1;
                }
                else
                {
                    newroad[i] = 0;
                }
            }
        }

#endif
#if defined(_OPENMP)
#pragma omp parallel for num_threads(n_threads)
#endif
        for (int i = 1; i <= irange; i++)
        {
            oldroad[i] = newroad[i];
        }

        MPI_Reduce(&nmove, &nmove_all, 1, MPI_INT, MPI_SUM, 0, comm);
        if (rank == 0)
        {
            if (iter % printfreq == 0)
            {
                // printf("nmove: %d\n", nmove_all);
                printf("At iteration %d average velocity is %f \n", iter,
                       (float)nmove_all / (float)ncars);
            }
        }
    }
    if (rank == 0)
    {
        tstop = gettime();
    }

    free(oldroad);
    free(newroad);

    if (rank == 0)
    {
        printf("\nFinished\n");
        printf("\nTime taken was  %f seconds\n", tstop - tstart);
        printf("Update rate was %f MCOPs\n\n",
               1.e-6 * ((double)NCELL) * ((double)maxiter) / (tstop - tstart));
    }

    error = MPI_Finalize();
    assert(error == MPI_SUCCESS);
    return 0;
}
