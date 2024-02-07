#ifndef bins_problem_h
#define bins_problem_h

#include "thread_pool.h"

/*
 
 ▪ The input will be:-
 1.The number of measurements: data_count
 2.An array of data count floats: data
 3.The minimum value for the bin containing the smallest values: min_meas
 4.The maximum value for the bin containing the largest values: max_meas
 5.The number of bins: bin_count
 
 ▪ The output will be an array containing the number of elements of data that lie in each bin.
 1.bin_maxes: An array of bin count floats
 2.bin_counts: An array of bin count ints
 
 */

//typedef struct {
//    long double *bin_maxes;
//    long double *bin_counts;
//} bins;

typedef struct {
    long double *data;
    long double min_meas;
    long double max_meas;
    long double bin_width;
    int data_count;
    int bin_count;
} in_pack;

typedef struct {
    long double *bin_maxes;
    int *bin_counts;
} out_pack; // bins

typedef struct {
    in_pack *args;
    out_pack *output;
//    int in_thread_width;
//    int in_ptr;
//    pthread_mutex_t mtx;
//    int *outptr;
    int out_thread_width;
    int bin_idx;
} in_out_pack;

typedef struct {
    thread_pool *pool;
    in_out_pack *data;
} pool_and_data;

void distribute_into_bins(void*);

pool_and_data* solve(long double*,long double,long double,int,int);

#endif

