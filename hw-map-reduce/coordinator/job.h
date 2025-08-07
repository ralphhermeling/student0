/**
 * Logic for job and task management.
 *
 * You are not required to modify this file.
 */

#ifndef JOB_H__
#define JOB_H__

#include <sys/types.h>
#include <stdbool.h>

typedef int job_id;
typedef int task_id;
typedef char* path;

/* You may add definitions here */
struct job_config_t {
  struct {
    u_int files_len;
    path* files_val;
  } files;
  path output_dir;
  char* app;
  int n_reduce;
  struct {
    u_int args_len;
    char* args_val;
  } args;
};
typedef struct job_config_t job_config_t;

typedef enum { COMPLETED, FAILED, IN_PROGRESS } job_status_t;

typedef struct {
  job_id id;
  job_config_t config;
  int n_reduce_completed;
  int n_map_completed;
  job_status_t status;
} job_t;

typedef enum { MAP, REDUCE } task_type_t;

typedef struct {
  job_id job_id;
  task_id task_id;
  task_type_t type;
  char* app;
  path file;
  path output_dir;
  struct {
    u_int args_len;
    char* args_val;
  } args;
} task_t;

extern void init_job_and_task_management();
extern job_id submit_job(job_config_t* job_config);
extern job_t* lookup_job(job_id id);
extern void finish_task(job_id job_id, task_id task_id);
extern task_t* get_task();

#endif
