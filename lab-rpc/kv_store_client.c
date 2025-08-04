/**
 * Client binary.
 */

#include "kv_store_client.h"
#include "kv_store.h"
#include <stdio.h>
#include <string.h>

#define HOST "localhost"

CLIENT* clnt_connect(char* host) {
  CLIENT* clnt = clnt_create(host, KVSTORE, KVSTORE_V1, "udp");
  if (clnt == NULL) {
    clnt_pcreateerror(host);
    exit(1);
  }
  return clnt;
}

int example(int input) {
  CLIENT *clnt = clnt_connect(HOST);

  int ret;
  int *result;

  result = example_1(&input, clnt);
  if (result == (int *)NULL) {
    clnt_perror(clnt, "call failed");
    exit(1);
  }
  ret = *result;
  xdr_free((xdrproc_t)xdr_int, (char *)result);

  clnt_destroy(clnt);
  
  return ret;
}

char* echo(char* input) {
  printf("Test");
  CLIENT *clnt = clnt_connect(HOST);

  char* ret;
  char** result;
  result = echo_1(&input, clnt);
  if(result == NULL || *result == NULL){
    clnt_perror(clnt, "call failed");
    exit(1);
  }
  ret = strdup(*result);
  xdr_free((xdrproc_t)xdr_int, (char**) result);

  clnt_destroy(clnt);
  
  return ret;
}

void put(buf key, buf value) {
  CLIENT *clnt = clnt_connect(HOST);

  put_args pargs;

  pargs.k = key;
  pargs.v = value;

  put_1(&pargs, clnt);

  clnt_destroy(clnt);
}

buf* get(buf key) {
  CLIENT *clnt = clnt_connect(HOST);

  buf* ret;
  buf* result;

  result = get_1(&key, clnt);

  ret = malloc(sizeof(buf));
  
  if (result == NULL || result->buf_val == NULL) {
    ret->buf_len = 0;
    ret->buf_val = NULL;
  } else {
    ret->buf_len = result->buf_len;
    ret->buf_val = strdup(result->buf_val);
  }

  xdr_free((xdrproc_t)xdr_int, (buf*) result);

  clnt_destroy(clnt);
  
  return ret;
}
