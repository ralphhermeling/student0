/*
 * mm_alloc.c
 */

#include "mm_alloc.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct meta_data {
  size_t size;
  bool free;
  struct meta_data* next;
  struct meta_data* prev;
  char memory_block[];
} mm_md;

static mm_md* head = NULL;

void* mm_malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }

  /* Allocating first block */
  if (head == NULL) {
    mm_md* new_block = (mm_md*)sbrk(sizeof(mm_md) + size);
    if (new_block == (void*)-1) {
      return NULL;
    }

    new_block->size = size;
    new_block->free = false;
    new_block->prev = NULL;
    new_block->next = NULL;

    memset(new_block->memory_block, 0, size);
    head = new_block;

    return new_block->memory_block;
  }

  /* Find block using first fit strategy */
  mm_md* curr = head;
  mm_md* prev = NULL;
  mm_md* match = NULL;
  while (curr != NULL) {
    if (curr->free && curr->size >= size) {
      match = curr;
      break;
    }
    prev = curr;
    curr = curr->next;
  }

  /* Unable to find block */
  if (match == NULL) {
    mm_md* new_block = (mm_md*)sbrk(sizeof(mm_md) + size);
    if (new_block == (void*)-1) {
      return NULL;
    }

    new_block->size = size;
    new_block->free = false;
    new_block->next = NULL;
    memset(new_block->memory_block, 0, size);

    /* Curr should be populated with address of last visited block */
    mm_md* tail = prev;
    if (tail != NULL) {
      tail->next = new_block;
      new_block->prev = tail;
    }

    return new_block->memory_block;
  }

  assert(match != NULL);
  assert(match->size >= size);

  /* Block is large enough to accomodate both the newly allocated block 
   * and another block in addition.
   */
  if (match->size > sizeof(mm_md) + size) {
    /* Reinterpret memory block such that at end of newly allocated block is another block */
    mm_md* new_block = (mm_md*)(match->memory_block + size);
    new_block->size = match->size - size - sizeof(mm_md);
    new_block->free = true;

    /* New block comes after match */
    new_block->prev = match;
    new_block->next = match->next;

    /* If match has a block next adjust prev */
    if (match->next != NULL) {
      match->next->prev = new_block;
    }

    match->free = false;
    match->size = size;
    memset(match->memory_block, 0, size);
    match->next = new_block;

    return match->memory_block;
  } else {
    memset(match->memory_block, 0, match->size);
    match->free = false;

    return match->memory_block;
  }

  return NULL;
}

void* mm_realloc(void* ptr, size_t size) {
  if(ptr == NULL && size == 0){
    return NULL;
  }

  if(ptr == NULL && size > 0){
    return mm_malloc(size);
  }

  if(ptr != NULL && size == 0){
    mm_free(ptr);
    return NULL;
  }

  assert(ptr != NULL && size > 0);
  void* new_mem_block = mm_malloc(size);
  if(new_mem_block == NULL){
    return NULL;
  }

  mm_md* old_block = (mm_md*)((char*)ptr - offsetof(mm_md, memory_block));
  memcpy(new_mem_block, ptr, old_block->size);
  mm_free(ptr);
  return new_mem_block;
}

void mm_free(void* ptr) {
  if (ptr == NULL) {
    return;
  }

  mm_md* block = (mm_md*)((char*)ptr - offsetof(mm_md, memory_block));
  /* Knowning heap consists of mm_md blocks block should exist */
  assert(block != NULL);

  block->free = true;

  /* Coalesce consecutive blocks in both directions */
  while (block->next != NULL && block->next->free) {
    /* Coalesce */
    mm_md* next = block->next;
    size_t added_space = sizeof(mm_md) + next->size;
    block->size += added_space;
    block->next = next->next;
    if (next->next) {
      next->next->prev = block;
    }
  }

  while (block->prev != NULL && block->prev->free) {
    mm_md* prev = block->prev;
    size_t added_space = sizeof(mm_md) + prev->size;
    prev->size += added_space;

    prev->next = block->next;
    if(block->next){
      block->next->prev = prev;
    }
  }

  return;
}
