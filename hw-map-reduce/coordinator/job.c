/**
 * Logic for job and task management.
 *
 * You are not required to modify this file.
 */

#include "job.h"
#include "glib.h"
#include <sys/types.h>
#include <stdlib.h>

/** Supports monotonically increasing job IDs */
static u_int next_job_id = 0;
static GHashTable* ht;
static GQueue* q;

/** Smallest Job ID has the highest priority */
static int compare_ints(gconstpointer a, gconstpointer b) {
  return GPOINTER_TO_INT(a) - GPOINTER_TO_INT(b);
}

/** Initialize HashMap and MinHeap */
void init_job_and_task_management() {
  ht = g_hash_table_new(g_direct_hash, g_direct_equal);
  q = g_queue_new();
};

static int schedule_map_tasks(job_t* job) {
  /** Initialize tasks */
  int n_map = job->config.files.files_len;
  task_t** tasks = malloc(n_map * sizeof(task_t));
  if (!tasks) {
    return -1;
  }

  for (int i = 0; i < n_map; i++) {
    tasks[i] = malloc(sizeof(task_t));
    if (!tasks[i]) {
      for (size_t j = 0; j < i; j++) {
        free(tasks[j]);
      }
      free(tasks);
      return -1;
    }
    tasks[i]->job_id = job->id;
    tasks[i]->app = job->config.app;
    tasks[i]->type = MAP;
    tasks[i]->file = job->config.files.files_val[i];
    tasks[i]->output_dir = job->config.output_dir;
    tasks[i]->task_id = i;
  }

  /** Add tasks to queue */
  for (int i = 0; i < n_map; i++) {
    g_queue_push_tail(q, tasks[i]);
  }
};

static void schedule_reduce_tasks(job_t* job) { return; };

job_id submit_job(job_config_t* job_config) {
  job_t* job = malloc(sizeof(job_t));
  if (job == NULL) {
    return -1;
  }
  job->id = next_job_id++;
  job->n_reduce_completed = 0;
  job->n_map_completed = 0;

  /** Hard copy job config */
  job->config.output_dir = strdup(job_config->output_dir);
  job->config.app = strdup(job_config->app);
  job->config.n_reduce = job_config->n_reduce;
  job->config.files.files_len = job_config->files.files_len;
  job->config.files.files_val = malloc(job_config->files.files_len * sizeof(char*));
  for (int i = 0; i < job_config->files.files_len; i++) {
    job->config.files.files_val[i] = strdup(job_config->files.files_val[i]);
  }
  job->config.args.args_len = job_config->args.args_len;
  job->config.args.args_val = malloc(job_config->args.args_len);
  memcpy(job->config.args.args_val, job_config->args.args_val, job_config->args.args_len);

  job->status = IN_PROGRESS;
  g_hash_table_insert(ht, GPOINTER_TO_INT(job->id), job);
  if (schedule_map_tasks(job) == -1) {
    return -1;
  }
  return job->id;
};

job_t* lookup_job(job_id id) {
  job_t* job = g_hash_table_lookup(ht, GINT_TO_POINTER(id));
  return job;
};
