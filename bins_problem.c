#include <stdlib.h>
#include <stdio.h>

#include "bins_problem.h"

void distribute_into_bins(void *args){
    in_out_pack *pack=(in_out_pack*)args;
    in_pack *input=pack->args;
    out_pack *output=pack->output;
    // initialize bin_maxes
    int i=pack->bin_idx;
    if(i==input->bin_count-1) output->bin_maxes[i]=input->max_meas;
    else output->bin_maxes[i]=input->min_meas+input->bin_width*(i+1);
    // increment bin_counts based on the value range
    for(int j=0;j<input->data_count;j++){
        if(i==0&&input->data[j]>=input->min_meas&&input->data[j]<output->bin_maxes[0])
            output->bin_counts[0]++;
        else if(input->data[j]>=input->min_meas+input->bin_width*i&&input->data[j]<output->bin_maxes[i])
            output->bin_counts[i]++;
    }
}

pool_and_data* solve(long double *data,long double min_meas,long double max_meas,int data_count,int bin_count){
    
    // pack input values into a data structure
    in_pack *args=(in_pack*)malloc(sizeof(in_pack));
    if(!args) return NULL;
    args->data=data;
    args->min_meas=min_meas;
    args->max_meas=max_meas;
    args->data_count=data_count;
    args->bin_count=bin_count;
    long double bin_width=(max_meas-min_meas)/bin_count;
    args->bin_width=bin_width;
    
    // pack output result into a data structure
    out_pack *out=(out_pack*)malloc(sizeof(out_pack));
    if(!out){
        free(args);
        return NULL;
    }
    out->bin_maxes=(long double*)malloc(sizeof(long double)*bin_count);
    if(!out->bin_maxes){
        free(args);
        free(out);
        return NULL;
    }
    out->bin_counts=(int*)malloc(sizeof(int)*bin_count);
    if(!out->bin_counts){
        free(args);
        free(out->bin_maxes);
        free(out);
        return NULL;
    }
    
    in_out_pack *pack=(in_out_pack*)malloc(sizeof(in_out_pack)*bin_count);
    if(!pack){
        free(args);
        free(out->bin_maxes);
        free(out->bin_counts);
        free(out);
        return NULL;
    }
    
    // start dividing the task between threads
    thread_pool *pool=init_thread_pool();
    for(int i=0;i<bin_count;i++){
        pack[i].args=args;
        pack[i].output=out;
        pack[i].bin_idx=i;
        submit_task_to_pool(pool,distribute_into_bins,&pack[i]);
    }
    
    pool_and_data *res=(pool_and_data*)malloc(sizeof(pool_and_data));
    res->pool=pool;
    res->data=pack;
    return res;
}
