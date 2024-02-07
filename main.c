#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bins_problem.h"

int main(int argc, const char * argv[]) {
    long double data[]={
        1.3, 2.9, 0.4, 0.3, 1.3, 4.4,
        1.7, 0.4, 3.2, 0.3, 4.9, 2.4,
        3.1, 4.4, 3.9, 0.4, 4.2, 4.5,
        4.9, 0.9
    };
    pool_and_data *res=solve(data, 0, 5, 20, 5);
    sleep(1);
    for(int i=0;i<5;i++)
        printf("%d ",res->data->output->bin_counts[i]);
    printf("%c", '\n');
end:
    terminate_threads(res->pool);
    destroy_thread_pool(res->pool);
    free(res->data->args);
    free(res->data);
    free(res);
    return 0;
}

