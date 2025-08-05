/**
 * Logic for job and task management.
 *
 * You are not required to modify this file.
 */

#include "job.h"
#include <sys/types.h>
#include <stdlib.h>

/** Supports monotonically increasing job IDs */
static u_int next_job_id = 0;

/** Initialize HashMap and MinHeap */
void init_job_and_task_management(){

};

job_id submit_job(job_config_t job_config){
  return next_job_id;
};

job_t* lookup_job(job_id id){
  return NULL;
};
