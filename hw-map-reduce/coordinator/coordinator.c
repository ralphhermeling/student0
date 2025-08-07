/**
 * The MapReduce coordinator.
 */

#include "coordinator.h"
#include "job.h"
#include <assert.h>
#include <cstring>
#include <string.h>

#ifndef SIG_PF
#define SIG_PF void (*)(int)
#endif

/* Global coordinator state. */
coordinator* state;

extern void coordinator_1(struct svc_req*, SVCXPRT*);

/* Set up and run RPC server. */
int main(int argc, char** argv) {
  register SVCXPRT* transp;

  pmap_unset(COORDINATOR, COORDINATOR_V1);

  transp = svcudp_create(RPC_ANYSOCK);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create udp service.");
    exit(1);
  }
  if (!svc_register(transp, COORDINATOR, COORDINATOR_V1, coordinator_1, IPPROTO_UDP)) {
    fprintf(stderr, "%s", "unable to register (COORDINATOR, COORDINATOR_V1, udp).");
    exit(1);
  }

  transp = svctcp_create(RPC_ANYSOCK, 0, 0);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create tcp service.");
    exit(1);
  }
  if (!svc_register(transp, COORDINATOR, COORDINATOR_V1, coordinator_1, IPPROTO_TCP)) {
    fprintf(stderr, "%s", "unable to register (COORDINATOR, COORDINATOR_V1, tcp).");
    exit(1);
  }

  coordinator_init(&state);

  svc_run();
  fprintf(stderr, "%s", "svc_run returned");
  exit(1);
  /* NOTREACHED */
}

/* EXAMPLE RPC implementation. */
int* example_1_svc(int* argp, struct svc_req* rqstp) {
  static int result;

  result = *argp + 1;

  return &result;
}

/* SUBMIT_JOB RPC implementation. */
int* submit_job_1_svc(submit_job_request* argp, struct svc_req* rqstp) {
  static int result;

  printf("Received submit job request\n");

  /* TODO */

  /* Do not modify the following code. */
  /* BEGIN */
  struct stat st;
  if (stat(argp->output_dir, &st) == -1) {
    mkdirp(argp->output_dir);
  }

  job_config_t job_config;
  job_config.files.files_val = argp->files.files_val;
  job_config.files.files_len = argp->files.files_len;
  job_config.n_reduce = argp->n_reduce;
  job_config.app = argp->app;
  job_config.output_dir = argp->output_dir;
  job_config.args.args_len = argp->args.args_len;
  job_config.args.args_val = argp->args.args_val;

  result = submit_job(&job_config);

  return &result;
  /* END */
}

/* POLL_JOB RPC implementation. */
poll_job_reply* poll_job_1_svc(int* argp, struct svc_req* rqstp) {
  static poll_job_reply result;

  printf("Received poll job request\n");
  job_t* job = lookup_job(*argp);
  if (job == NULL) {
    printf("Job not found\n");
    result.invalid_job_id = true;
    result.done = false;
    result.failed = false;
    return &result;
  }

  result.invalid_job_id = false;
  if (job->status == COMPLETED) {
    result.done = true;
    result.failed = false;
  } else if (job->status == FAILED) {
    result.done = false;
    result.failed = true;
  } else {
    result.done = false;
    result.failed = false;
  }

  return &result;
}

/* GET_TASK RPC implementation. */
get_task_reply* get_task_1_svc(void* argp, struct svc_req* rqstp) {
  static get_task_reply result;

  printf("Received get task request\n");

  /* Free previous allocation if any */
  if (result.args.args_len > 0 && result.args.args_val != NULL) {
    free(result.args.args_val);
    result.args.args_val = NULL;
    result.args.args_len = 0;
  }

  result.file = "";
  result.output_dir = "";
  result.app = "";
  result.wait = true;

  task_t* t = get_task();
  if (t == NULL) {
    printf("No task found\n");
    result.wait = true;
    return &result;
  }
  job_t* j = lookup_job(t->job_id);
  assert(j != NULL);

  /* Task specific config */
  result.task = t->task_id;
  result.file = t->file;
  result.wait = false;
  result.output_dir = t->output_dir;

  /* Variable-length byte array - need to allocate */
  result.args.args_len = t->args.args_len;
  if (t->args.args_len > 0) {
    result.args.args_val = malloc(t->args.args_len);
    if (result.args.args_val == NULL) {
      printf("Unable to allocate args_val for task\n");
      result.file = "";
      result.output_dir = "";
      result.app = "";
      result.wait = true;
      return &result;
    }
    memcpy(result.args.args_val, t->args.args_val, t->args.args_len);
  } else {
    result.args.args_val = NULL;
  }

  /* Inherited from job */
  result.app = t->app;
  result.job_id = t->job_id;
  result.n_reduce = j->config.n_reduce;
  result.n_map = j->config.files.files_len;
  result.reduce = (t->type == REDUCE);

  printf("Found task and returning\n");

  return &result;
}

/* FINISH_TASK RPC implementation. */
void* finish_task_1_svc(finish_task_request* argp, struct svc_req* rqstp) {
  static char* result;

  printf("Received finish task request\n");

  /* TODO */

  return (void*)&result;
}

/* Initialize coordinator state. */
void coordinator_init(coordinator** coord_ptr) {
  *coord_ptr = malloc(sizeof(coordinator));

  coordinator* coord = *coord_ptr;

  init_job_and_task_management();
  /* TODO */
}
