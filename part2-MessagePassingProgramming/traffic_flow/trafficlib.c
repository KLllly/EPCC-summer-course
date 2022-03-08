#include "uni.h"

int initroad(int *road, int n, float density, int seed) {
  int i, ncar;
  float rng;

  // seed random number generator
  rinit(seed);
  ncar = 0;
  for (i = 0; i < n; i++) {
    rng = uni();
    if (rng < density) {
      road[i] = 1;
    } else {
      road[i] = 0;
    }
    ncar += road[i];
  }
  return ncar;
}

#include <stdlib.h>
#include <sys/time.h>

double gettime() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec + tp.tv_usec / (double)1.0e6;
}