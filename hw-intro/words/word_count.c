/*

Copyright © 2019 University of California, Berkeley

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

word_count provides lists of words and associated count

Functional methods take the head of a list as first arg.
Mutators take a reference to a list as first arg.
*/

#include "word_count.h"
#include <stdlib.h>
#include <string.h>

/* Basic utilities */

char* new_string(char* str) {
  char* new_str = (char*)malloc(strlen(str) + 1);
  if (new_str == NULL) {
    return NULL;
  }
  return strcpy(new_str, str);
}

int init_words(WordCount** wclist) {
  /* Initialize word count.
     Returns 0 if no errors are encountered
     in the body of this function; 1 otherwise.
  */
  WordCount* wc = malloc(sizeof(WordCount));
  if (wc == NULL) {
    return 0;
  }
  *wclist = wc;
  return 1;
}

ssize_t len_words(WordCount* wchead) {
  /* Return -1 if any errors are
     encountered in the body of
     this function.
  */
  size_t len = 0;
  if (wchead == NULL) {
    return len;
  }
  WordCount* curr = wchead;

  while (curr) {
    if (curr->word == NULL) {
      return -1;
    }
    len++;
    curr = curr->next;
  }

  return len;
}

WordCount* find_word(WordCount* wchead, char* word) {
  /* Return count for word, if it exists */
  if (word == NULL) {
    return NULL;
  }

  WordCount* wc = NULL;
  WordCount* curr = wchead;

  while (curr) {
    if (curr->word != NULL && !strcmp(word, curr->word)) {
      wc = curr;
      break;
    }
    curr = curr->next;
  }

  return wc;
}

int add_word(WordCount** wclist, char* word) {
  /* If word is present in word_counts list, increment the count.
     Otherwise insert with count 1.
     Returns 0 if no errors are encountered in the body of this function; 1 otherwise.
  */
  if (wclist == NULL || word == NULL) {
    return 1;
  }

  char* word_copy = strdup(word);
  if (word_copy == NULL) {
    return 1;
  }

  if ((*wclist)->word == NULL) {
    (*wclist)->count = 1;
    (*wclist)->word = word_copy;
    return 0;
  }

  WordCount* wc = find_word(*wclist, word);
  if (wc == NULL) {
    WordCount* newWordCount = malloc(sizeof(WordCount));
    if (newWordCount == NULL) {
      return 1;
    }

    newWordCount->count = 1;
    newWordCount->word = word_copy;
    newWordCount->next = *wclist;
    *wclist = newWordCount;
  } else {
    wc->count += 1;
  }

  return 0;
}

void fprint_words(WordCount* wchead, FILE* ofile) {
  /* print word counts to a file */
  WordCount* wc;
  for (wc = wchead; wc; wc = wc->next) {
    fprintf(ofile, "%i\t%s\n", wc->count, wc->word);
  }
}
