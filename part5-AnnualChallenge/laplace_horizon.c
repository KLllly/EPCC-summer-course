#if defined(_OPENMP)
#include "omp.h"
#endif

#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define rank_zero_only(statement) \
    do                            \
    {                             \
        if (rank == 0)            \
            statement;           \
    } while (0);


#define TIMING 1   // Timing macro
#define NONBLOCK 1 // Non-block communication
#define HYPER 2    // enable hyperthreading

// size of plate
#define COLUMNS 10000
#define ROWS 10000

// largest permitted change in temp (This value takes about 3400 steps)
#define MAX_TEMP_ERROR 0.01

// helper routines
void initialize(double (*Temperature_last)[COLUMNS + 2],
                int range_row, int start_row, int last_rank);

void track_progress(double (*Temperature)[COLUMNS + 2], int iteration, int range_row, int start_row);

int main(int argc, char **argv)
{

    int i, j;                                           // grid indexes
    int max_iterations;                                 // number of iterations
    int iteration = 1;                                  // current iteration
    double dt = 100;                                    // largest chaneg in t
    struct timeval start_time, stop_time, elapsed_time; // timers


    int rank, size, error, last_rank;
    double start, end, duration, temp;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Request send_head_req, recv_head_req, send_tail_req, recv_tail_req;
    MPI_Status send_status, recv_status;
    int send_head_tag, send_tail_tag, recv_tail_tag, recv_head_tag;
    send_head_tag = recv_tail_tag = 1;
    send_tail_tag = recv_head_tag = 2;

    error = MPI_Init(NULL, NULL);
    assert(error == MPI_SUCCESS);

    // Get process ID
    MPI_Comm_rank(comm, &rank);

    // Get processes Number
    MPI_Comm_size(comm, &size);

    rank_zero_only(printf("Maximum iterations [100-4000]?\n"));
    rank_zero_only(scanf("%d", &max_iterations));
    MPI_Bcast(&max_iterations, 1, MPI_INT, 0, comm);

    #if defined(_OPENMP)
    int total_cores = omp_get_num_procs()*HYPER;
    int local_cores = total_cores / size;
    rank_zero_only(printf("Openmp total_cores: %d local_cores: %d\n", total_cores, local_cores));
    #endif

    int PART_ROW = (ROWS + size - 1) / size;
    int start_row = rank * PART_ROW;
    int stop_row = min((rank + 1) * PART_ROW, ROWS);
    int range_row = stop_row - start_row;
    last_rank = (rank == (size - 1));

    double(*Temperature)[COLUMNS + 2] = (double(*)[COLUMNS + 2]) malloc(sizeof(double) * (range_row + 2) * (COLUMNS + 2));      // temperature grid
    double(*Temperature_last)[COLUMNS + 2] = (double(*)[COLUMNS + 2]) malloc(sizeof(double) * (range_row + 2) * (COLUMNS + 2)); // temperature grid from last iteration

    gettimeofday(&start_time, NULL); // Unix timer

    #if TIMING
    start = MPI_Wtime();
    #endif
    initialize(Temperature_last,
               range_row, start_row, last_rank); // initialize Temp_last including boundary conditions
    #if TIMING
    end = MPI_Wtime();
    duration = end - start;
    MPI_Allreduce(&duration, &temp, 1, MPI_DOUBLE, MPI_SUM, comm);
    rank_zero_only(printf("initialization: %lf ", temp/size));
    #endif

    printf("This is rank %d, world_size: %d max_iter: %d\n", rank, size, max_iterations);
    // do util error is minimal of until max steps
    while (dt > MAX_TEMP_ERROR && iteration <= max_iterations)
    {
        #if TIMING
        start = MPI_Wtime();
        #endif

        // main calculation: average my four neighbors
        #if defined(_OPENMP)
        #pragma omp parallel for num_threads(local_cores) schedule(runtime)
        #endif
        for (i = 1; i <= range_row; i++)
        {
            for (j = 1; j <= COLUMNS; j++)
            {
                Temperature[i][j] =
                    0.25 * (Temperature_last[i + 1][j] + Temperature_last[i - 1][j] +
                            Temperature_last[i][j + 1] + Temperature_last[i][j - 1]);
            }
        }

        dt = 0.0; // reset largest temperature change

        // copy grid to old grid for next iteration and find latest dt
        #if defined(_OPENMP)
        double *dtt = (double *)calloc(local_cores, sizeof(double));
        #pragma omp parallel for num_threads(local_cores) schedule(runtime)
        for (i = 1; i <= range_row; i++)
        {
            for (j = 1; j <= COLUMNS; j++)
            {
                dtt[omp_get_thread_num()] = fmax(fabs(Temperature[i][j] - Temperature_last[i][j]), dtt[omp_get_thread_num()]);
                Temperature_last[i][j] = Temperature[i][j];
            }
        }
        dt = 0.0;
        for (int i = 0; i < local_cores; i++) {
            dt = fmax(dt, dtt[i]);
        }
        #else   
        for (i = 1; i <= range_row; i++)
        {
            for (j = 1; j <= COLUMNS; j++)
            {
                dt = fmax(fabs(Temperature[i][j] - Temperature_last[i][j]), dt);
                Temperature_last[i][j] = Temperature[i][j];
            }
        }
        #endif

        #if TIMING
        end = MPI_Wtime();
        duration = end - start;
        MPI_Allreduce(&duration, &temp, 1, MPI_DOUBLE, MPI_SUM, comm);
        rank_zero_only(printf("computation: %lf ", temp/size));
        #endif

        temp = dt;
        error = MPI_Allreduce(&temp, &dt, 1, MPI_DOUBLE, MPI_MAX, comm);
        // assert(error == MPI_SUCCESS);

        // rank_zero_only(printf("iter: %d dt: %lf\n", iteration, dt));

        // periodically print test values
        if (last_rank && iteration % 100 == 0)
        {
            track_progress(Temperature, iteration, range_row, start_row);
        }

        start = MPI_Wtime();
        #if NONBLOCK
        if (!last_rank) {
            // send tail to next rank
            error = MPI_Isend(&(Temperature[range_row][1]), COLUMNS, MPI_DOUBLE, rank + 1, send_tail_tag, comm, &send_tail_req);
            // assert(error == MPI_SUCCESS);
            // receive tail from next rank
            error = MPI_Irecv(&(Temperature_last[range_row+1][1]), COLUMNS, MPI_DOUBLE, rank + 1, recv_tail_tag, comm, &recv_tail_req);
            // assert(error == MPI_SUCCESS);
        }
        if (rank > 0) {
            // receive head from last rank
            error = MPI_Irecv(&(Temperature_last[0][1]), COLUMNS, MPI_DOUBLE, rank - 1, recv_head_tag, comm, &recv_head_req);
            // assert(error == MPI_SUCCESS);
            // send head to last rank
            error = MPI_Isend(&(Temperature_last[1][1]), COLUMNS, MPI_DOUBLE, rank - 1, send_head_tag, comm, &send_head_req);
            // assert(error == MPI_SUCCESS);
        }
        if (!last_rank) {
            // wait for send and receive to complete
            error = MPI_Wait(&send_tail_req, &send_status);
            // assert(error == MPI_SUCCESS);
            error = MPI_Wait(&recv_tail_req, &recv_status);
            // assert(error == MPI_SUCCESS);
        }
        if (rank > 0) {
            // wait for send and receive to complete
            error = MPI_Wait(&send_head_req, &send_status);
            // assert(error == MPI_SUCCESS);
            error = MPI_Wait(&recv_head_req, &recv_status);
            // assert(error == MPI_SUCCESS);
        }
        #else
        if (rank % 2 == 0)
        {
            if (!last_rank)
            {
                error = MPI_Send(&(Temperature[range_row][1]), COLUMNS, MPI_DOUBLE, rank + 1, send_tail_tag, comm);
                assert(error == MPI_SUCCESS);
                error = MPI_Recv(&(Temperature_last[range_row + 1][1]), COLUMNS, MPI_DOUBLE, rank + 1, recv_tail_tag, comm, &recv_status);
                assert(recv_status.MPI_SOURCE == rank + 1);
                assert(error == MPI_SUCCESS);
            }
            if (rank > 0)
            {
                error = MPI_Recv(&(Temperature_last[0][1]), COLUMNS, MPI_DOUBLE, rank - 1, recv_head_tag, comm, &recv_status);
                assert(error == MPI_SUCCESS);
                assert(recv_status.MPI_SOURCE == rank - 1);
                error = MPI_Send(&(Temperature_last[1][1]), COLUMNS, MPI_DOUBLE, rank - 1, send_head_tag, comm);
                assert(error == MPI_SUCCESS);
            }
        }
        else
        {
            if (rank > 0)
            {
                error = MPI_Recv(&(Temperature_last[0][1]), COLUMNS, MPI_DOUBLE, rank - 1, recv_head_tag, comm, &recv_status);
                assert(recv_status.MPI_SOURCE == rank - 1);
                assert(error == MPI_SUCCESS);
                error = MPI_Send(&(Temperature_last[1][1]), COLUMNS, MPI_DOUBLE, rank - 1, send_head_tag, comm);
                assert(error == MPI_SUCCESS);
            }
            if (!last_rank)
            {
                error = MPI_Send(&(Temperature[range_row][1]), COLUMNS, MPI_DOUBLE, rank + 1, send_tail_tag, comm);
                assert(error == MPI_SUCCESS);
                error = MPI_Recv(&(Temperature_last[range_row + 1][1]), COLUMNS, MPI_DOUBLE, rank + 1, recv_tail_tag, comm, &recv_status);
                assert(recv_status.MPI_SOURCE == rank + 1);
                assert(error == MPI_SUCCESS);
            }
        }
        #endif
        #if TIMING
        end = MPI_Wtime();
        duration = end - start;
        MPI_Allreduce(&duration, &temp, 1, MPI_DOUBLE, MPI_SUM, comm);
        rank_zero_only(printf(" communication: %lf\n", temp/size));
        #endif
        iteration++;
    }

    gettimeofday(&stop_time, NULL); // Unix timer
    timersub(&stop_time, &start_time,
             &elapsed_time); // Unix timer substraction routine

    rank_zero_only(printf("\nMax error at iteration %d was %f\n", iteration - 1, dt));
    rank_zero_only(printf("Total time was %f seconds\n",
           elapsed_time.tv_sec + (elapsed_time.tv_usec / 1000000.0)));

    MPI_Finalize();
    free(Temperature);
    free(Temperature_last);

    return 0;
}

// initialize plate and boundary conditions
// Temp_last is used tp start first iteration
void initialize(double (*Temperature_last)[COLUMNS + 2],
                int range_row, int start_row, int last_rank)
{
    int i, j;
    for (i = 0; i <= range_row + 1; i++)
    {
        for (j = 0; j <= COLUMNS + 1; j++)
        {
            Temperature_last[i][j] = 0.0;
        }
    }

    // these boundary conditions never change thoughout run
    // set left side to 0 and right side to a linear increase

    for (i = 0; i <= range_row + 1; i++)
    {
        Temperature_last[i][COLUMNS + 1] = (100.0 / ROWS) * (i + start_row);
    }

    // set top to 0 and bottom to linear increase (only need to set in last rank)
    if (last_rank)
    {
        for (j = 0; j <= COLUMNS + 1; j++)
        {
            Temperature_last[range_row + 1][j] = (100.0 / COLUMNS) * j;
        }
    }
}

// print diagonal in bottom right corner where most action is
void track_progress(double (*Temperature)[COLUMNS + 2], int iteration, int range_row, int start_row)
{
    int i;
    printf("--------- Iteration number: %d ---------\n", iteration);
    for (i = range_row - 5; i <= range_row; i++)
    {
        printf("[%d,%d]: %5.2f ", i + start_row, i + start_row, Temperature[i][COLUMNS+i-range_row]);
    }
    printf("\n");
}
