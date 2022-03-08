// Pi calculation using the approximation formula.
// $\frac{\pi}{4} = \int_{0}^{1}{\frac{dx}{1+x^{2}}} \approx \frac{1}{N}\sum_{i=1}^{N}{\frac{1}{1+(\frac{i-\frac{1}{2}}{N})^{2}}}$ 

// compile command: gcc omp.c -lm -fopenmp
// run command: ./a.out $N_VALUE(default 100000)


#include <stdio.h>  // printf
#include <stdlib.h> // atoi
#include <math.h>   // M_PI, pow, need to compile with -lm
#include <omp.h>    // OpenMP
#include <assert.h> // assert
#include <time.h>   // time 

typedef struct timespec timespec;

timespec diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

int main (int argc, char* argv[])
{
    int rank, size, error;
    double pi = 0.0;
    double exact_pi = M_PI;
    timespec start, end, res;
    
    int N = 100000; // set default N value
    if (argc == 2) { // read N from command line
        N = atoi(argv[1]);
    } 
    printf("OpenMP version of Pi calculation.\n");
    printf("N value is %d\n", N);
    
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    #pragma omp parallel for reduction(+:pi)
    for (int i = 1; i <= N; i++) {
        pi += 1.0/(1.0 + pow((double)(i - 0.5)/((double) N), 2.0));
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
    
    pi = 4.0/((double) N) * pi;
    printf("Calculated pi: %lf, Exact pi: %lf, Error: %lf\n", pi, exact_pi, fabs(pi - exact_pi));
    res = diff(start, end);
    printf("Using time: %lf\n", (double)(end.tv_sec-start.tv_sec)+(double)(end.tv_nsec-start.tv_nsec)/1e9);

    return 0;
}