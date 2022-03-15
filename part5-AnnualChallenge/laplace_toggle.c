#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

//size of plate
#define COLUMNS 1000
#define ROWS    1000

// largest permitted change in temp (This value takes about 3400 steps)
#define MAX_TEMP_ERROR 0.01

double grid_a[ROWS+2][COLUMNS+2];       // temperature grid
double grid_b[ROWS+2][COLUMNS+2];  // temperature grid from last iteration

// helper routines 
void initialize(void);
void track_progress(int iteration, double (*Temperature)[COLUMNS+2]);
void swap_pointer(double (**)[COLUMNS+2], double (**)[COLUMNS+2]);

int main(int argc, char **argv) {
    int i, j;                                           // grid indexes
    int max_iterations;                                 // number of iterations
    int iteration = 1;                                  // current iteration
    double dt = 100;                                    // largest chaneg in t
    struct timeval start_time, stop_time, elapsed_time; // timers
    double (*Temperature)[COLUMNS+2] = grid_a;
    double (*Temperature_last)[COLUMNS+2] = grid_b;

    printf("Maximum iterations [100-4000]?\n");
    scanf("%d", &max_iterations);

    gettimeofday(&start_time, NULL); // Unix timer
    initialize();                    // initialize Temp_last including boundary conditions

    // do util error is minimal of until max steps
    while ( dt > MAX_TEMP_ERROR && iteration <= max_iterations ) {
        dt = 0.0; // reset largest temperature change
        // main calculation: average my four neighbors
        for (i = 1; i <= ROWS; i++) {
            for (j = 1; j <= COLUMNS; j++) {
                Temperature[i][j] = 0.25 * (Temperature_last[i+1][j] + Temperature_last[i-1][j] +
                                            Temperature_last[i][j+1] + Temperature_last[i][j-1]); 
                dt = fmax(fabs(Temperature[i][j] - Temperature_last[i][j]), dt);  
            }
        }

        // periodically print test values
        if (iteration % 100 == 0) {
            track_progress(iteration, Temperature);
        }

        iteration++;
        swap_pointer(&Temperature, &Temperature_last);
    }

    gettimeofday(&stop_time, NULL); // Unix timer
    timersub(&stop_time, &start_time, &elapsed_time); // Unix timer substraction routine

    printf("\nMax error at iteration %d was %f\n", iteration-1, dt);
    printf("Total time was %f seconds\n", elapsed_time.tv_sec + (elapsed_time.tv_usec / 1000000.0));

    return 0;
}

// initialize plate and boundary conditions
// Temp_last is used tp start first iteration
void initialize(void) {
    int i, j;
    for (i = 0; i <= ROWS+1; i++) {
        for (j = 0; j <= COLUMNS+1; j++) {
            grid_b[i][j] = 0.0;
        }
    }

    // these boundary conditions never change thoughout run
    // set left side to 0 and right side to a linear increase
    for (i = 0; i <= ROWS+1; i++) {
        grid_a[i][0] = 0.0;
        grid_b[i][0] = 0.0;
        grid_a[i][COLUMNS+1] = (100.0/ROWS)*i;
        grid_b[i][COLUMNS+1] = (100.0/ROWS)*i;
    }
    
    // set top to 0 and bottom to linear increase
    for (j = 0; j <= COLUMNS+1; j++) {
        grid_a[0][j] = 0.0;
        grid_b[0][j] = 0.0;
        grid_a[ROWS+1][j] = (100.0/COLUMNS)*j;
        grid_b[ROWS+1][j] = (100.0/COLUMNS)*j;
    }
}

// print diagonal in bottom right corner where most action is
void track_progress(int iteration, double (*Temperature)[COLUMNS+2]) {
    int i;
    printf("--------- Iteration number: %d ---------\n", iteration);
    for (i = ROWS-5; i <= ROWS; i++) {
        printf("[%d,%d]: %5.2f ", i, i, Temperature[i][i]);
    }
    printf("\n");
}


void swap_pointer(double (**Temperature)[COLUMNS+2], double (**Temperature_last)[COLUMNS+2]) {
    double (*temp)[COLUMNS+2] = *Temperature;
    *Temperature = *Temperature_last;
    *Temperature_last = temp;
}
