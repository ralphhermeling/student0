/*
 * Implementation of the word_count interface using Pintos lists and pthreads.
 *
 * You may modify this file, and are expected to modify it.
 */

/*
 * Copyright © 2021 University of California, Berkeley
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "list.h"
#include <pthread.h>
#include <stdlib.h>
#ifndef PINTOS_LIST
#error "PINTOS_LIST must be #define'd when compiling word_count_lp.c"
#endif

#ifndef PTHREADS
#error "PTHREADS must be #define'd when compiling word_count_lp.c"
#endif

#include "word_count.h"

void init_words(word_count_list_t* wclist) { /* TODO */
  list_init(&wclist->lst);
  pthread_mutex_init(&wclist->lock, NULL);
}

size_t len_words(word_count_list_t* wclist) {
  pthread_mutex_lock(&wclist->lock);
  size_t len = list_size(&wclist->lst);
  pthread_mutex_unlock(&wclist->lock);
  return len;
}

word_count_t* find_word(word_count_list_t* wclist, char* word) {
  pthread_mutex_lock(&wclist->lock);

  struct list_elem* e;
  for (e = list_begin(&wclist->lst); e != list_end(&wclist->lst); e = list_next(e)) {
    word_count_t* wc = list_entry(e, struct word_count, elem);

    if (strcmp(wc->word, word) == 0) {
      pthread_mutex_unlock(&wclist->lock);
      return wc;
    }
  }

  pthread_mutex_unlock(&wclist->lock);
  return NULL;
}

word_count_t* add_word(word_count_list_t* wclist, char* word) {
  pthread_mutex_lock(&wclist->lock);

  struct list_elem* e;

  word_count_t* wc = NULL;

  for (e = list_begin(&wclist->lst); e != list_end(&wclist->lst); e = list_next(e)) {
    word_count_t* wce = list_entry(e, struct word_count, elem);

    if (strcmp(wce->word, word) == 0) {
      wc = wce;
      break;
    }
  }

  if(wc != NULL){
    wc->count++;
    free(word);
    pthread_mutex_unlock(&wclist->lock);
    return wc;
  }

  wc = malloc(sizeof(word_count_t));
  if(wc == NULL){
    pthread_mutex_unlock(&wclist->lock);
    return NULL;
  }

  wc->word = word;
  wc->count = 1;
  list_push_back(&wclist->lst, &wc->elem);

  pthread_mutex_unlock(&wclist->lock);

  return wc;
}

void fprint_words(word_count_list_t* wclist, FILE* outfile) {
  pthread_mutex_lock(&wclist->lock);

  struct list_elem *e = NULL;

  for (e = list_begin(&wclist->lst); e != list_end(&wclist->lst); e = list_next(e)) {
    word_count_t* wce = list_entry(e, struct word_count, elem);
    fprintf(outfile, "%i\t%s\n", wce->count, wce->word);
  }

  pthread_mutex_unlock(&wclist->lock);
}

static bool less_list(const struct list_elem* ewc1, const struct list_elem* ewc2, void* aux) {
  word_count_t *wc1 = list_entry(ewc1, word_count_t, elem);
  word_count_t *wc2 = list_entry(ewc2, word_count_t, elem);

  return wc1->count < wc2->count;
}

void wordcount_sort(word_count_list_t* wclist,
                    bool less(const word_count_t*, const word_count_t*)) {
  pthread_mutex_lock(&wclist->lock);
  list_sort(&wclist->lst, less_list, less);
  pthread_mutex_unlock(&wclist->lock);
}
