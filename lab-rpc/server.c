/**
 * Server binary.
 */

#include "kv_store.h"
#include <glib.h>
#include <memory.h>
#include <netinet/in.h>
#include <rpc/pmap_clnt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#ifndef SIG_PF
#define SIG_PF void (*)(int)
#endif

GHashTable* ht;

extern void kvstore_1(struct svc_req*, SVCXPRT*);

/* Set up and run RPC server. */
int main(int argc, char** argv) {
  register SVCXPRT* transp;

  pmap_unset(KVSTORE, KVSTORE_V1);

  transp = svcudp_create(RPC_ANYSOCK);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create udp service.");
    exit(1);
  }
  if (!svc_register(transp, KVSTORE, KVSTORE_V1, kvstore_1, IPPROTO_UDP)) {
    fprintf(stderr, "%s", "unable to register (KVSTORE, KVSTORE_V1, udp).");
    exit(1);
  }

  transp = svctcp_create(RPC_ANYSOCK, 0, 0);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create tcp service.");
    exit(1);
  }
  if (!svc_register(transp, KVSTORE, KVSTORE_V1, kvstore_1, IPPROTO_TCP)) {
    fprintf(stderr, "%s", "unable to register (KVSTORE, KVSTORE_V1, tcp).");
    exit(1);
  }

  ht = g_hash_table_new(g_bytes_hash, g_bytes_equal);
  if (ht == NULL) {
    fprintf(stderr, "Failed to create hash table\n");
    exit(1);
  }

  svc_run();
  fprintf(stderr, "%s", "svc_run returned");
  exit(1);
  /* NOTREACHED */
}

/* Example server-side RPC stub. */
int* example_1_svc(int* argp, struct svc_req* rqstp) {
  static int result;

  result = *argp + 1;

  return &result;
}

char** echo_1_svc(char** argp, struct svc_req* rqstp) {
  static char** result;

  result = argp;

  return result;
}

void* put_1_svc(put_args* argp, struct svc_req* rqstp) {
  static void* result;

  if (!argp || !argp->k.buf_val || !argp->k.buf_len || !argp->v.buf_val) {
    return NULL;
  }

  GBytes* key = g_bytes_new(argp->k.buf_val, argp->k.buf_len);
  GBytes* value = g_bytes_new(argp->v.buf_val, argp->v.buf_len);
  g_hash_table_insert(ht, key, value);

  return &result;
}

buf* get_1_svc(buf* argp, struct svc_req* rqstp) {
  static buf result;

  if (result.buf_val) {
    free(result.buf_val);
  }

  GBytes* key = g_bytes_new(argp->buf_val, argp->buf_len);
  GBytes* value = g_hash_table_lookup(ht, key);

  g_bytes_unref(key);

  if (value == NULL) {
    result.buf_len = 0;
    return &result;
  }

  long unsigned int len;
  const char* data = g_bytes_get_data(value, &len);

  result.buf_len = len;
  result.buf_val = strdup(data);

  return &result;
}
