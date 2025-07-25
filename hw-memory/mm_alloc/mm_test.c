#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Function pointers to hw3 functions */
void* (*mm_malloc)(size_t);
void* (*mm_realloc)(void*, size_t);
void (*mm_free)(void*);

static void* try_dlsym(void* handle, const char* symbol) {
  char* error;
  void* function = dlsym(handle, symbol);
  if ((error = dlerror())) {
    fprintf(stderr, "%s\n", error);
    exit(EXIT_FAILURE);
  }
  return function;
}

static void load_alloc_functions() {
  void* handle = dlopen("hw3lib.so", RTLD_NOW);
  if (!handle) {
    fprintf(stderr, "%s\n", dlerror());
    exit(EXIT_FAILURE);
  }

  mm_malloc = try_dlsym(handle, "mm_malloc");
  mm_realloc = try_dlsym(handle, "mm_realloc");
  mm_free = try_dlsym(handle, "mm_free");
}

void test_simple_alloc_free() {
  printf("Running simple malloc/free verification test...\n");
  
  // Test 1: Basic allocation and data writing
  printf("  Testing basic allocation and data writing...\n");
  int* int_ptr = (int*)mm_malloc(sizeof(int));
  assert(int_ptr != NULL);
  *int_ptr = 42;
  assert(*int_ptr == 42);
  mm_free(int_ptr);
  
  // Test 2: String allocation and manipulation
  printf("  Testing string allocation and manipulation...\n");
  char* str = (char*)mm_malloc(20);
  assert(str != NULL);
  strcpy(str, "Hello World!");
  assert(strcmp(str, "Hello World!") == 0);
  mm_free(str);
  
  // Test 3: Array allocation and memset
  printf("  Testing array allocation with memset...\n");
  unsigned char* array = (unsigned char*)mm_malloc(100);
  assert(array != NULL);
  
  // Fill with pattern using memset
  memset(array, 0xAB, 100);
  for (int i = 0; i < 100; i++) {
    assert(array[i] == 0xAB);
  }
  
  // Change pattern
  memset(array, 0x55, 50);
  for (int i = 0; i < 50; i++) {
    assert(array[i] == 0x55);
  }
  for (int i = 50; i < 100; i++) {
    assert(array[i] == 0xAB);
  }
  
  mm_free(array);
  
  // Test 4: Multiple allocations to verify free actually works
  printf("  Testing that free actually releases memory...\n");
  void* ptrs[10];
  
  // Allocate 10 blocks
  for (int i = 0; i < 10; i++) {
    ptrs[i] = mm_malloc(100);
    assert(ptrs[i] != NULL);
    // Write unique pattern to each block
    memset(ptrs[i], i + 1, 100);
  }
  
  // Verify patterns are intact
  for (int i = 0; i < 10; i++) {
    unsigned char* block = (unsigned char*)ptrs[i];
    for (int j = 0; j < 100; j++) {
      assert(block[j] == (unsigned char)(i + 1));
    }
  }
  
  // Free every other block
  for (int i = 0; i < 10; i += 2) {
    mm_free(ptrs[i]);
  }
  
  // Verify remaining blocks still have correct data
  for (int i = 1; i < 10; i += 2) {
    unsigned char* block = (unsigned char*)ptrs[i];
    for (int j = 0; j < 100; j++) {
      assert(block[j] == (unsigned char)(i + 1));
    }
  }
  
  // Try to allocate new blocks (should reuse freed space if free works)
  void* new_ptr1 = mm_malloc(50);
  void* new_ptr2 = mm_malloc(50);
  assert(new_ptr1 != NULL);
  assert(new_ptr2 != NULL);
  
  // Test new allocations work
  strcpy((char*)new_ptr1, "Reused1");
  strcpy((char*)new_ptr2, "Reused2");
  assert(strcmp((char*)new_ptr1, "Reused1") == 0);
  assert(strcmp((char*)new_ptr2, "Reused2") == 0);
  
  // Clean up remaining blocks
  for (int i = 1; i < 10; i += 2) {
    mm_free(ptrs[i]);
  }
  mm_free(new_ptr1);
  mm_free(new_ptr2);
  
  // Test 5: Large allocation with number patterns
  printf("  Testing large allocation with number patterns...\n");
  int* large_array = (int*)mm_malloc(sizeof(int) * 1000);
  assert(large_array != NULL);
  
  // Fill with sequence
  for (int i = 0; i < 1000; i++) {
    large_array[i] = i * i;
  }
  
  // Verify sequence
  for (int i = 0; i < 1000; i++) {
    assert(large_array[i] == i * i);
  }
  
  mm_free(large_array);
  
  printf("Simple malloc/free verification test passed!\n");
}

void test_basic_malloc_free() {
  printf("Running basic malloc/free tests...\n");
  
  // Test small allocation
  void* ptr1 = mm_malloc(16);
  assert(ptr1 != NULL);
  mm_free(ptr1);
  
  // Test medium allocation
  void* ptr2 = mm_malloc(1024);
  assert(ptr2 != NULL);
  mm_free(ptr2);
  
  // Test large allocation
  void* ptr3 = mm_malloc(65536);
  assert(ptr3 != NULL);
  mm_free(ptr3);
  
  printf("Basic malloc/free tests passed!\n");
}

void test_basic_realloc() {
  printf("Running basic realloc tests...\n");
  
  // Test expanding realloc
  void* ptr = mm_malloc(100);
  assert(ptr != NULL);
  strcpy((char*)ptr, "test data");
  
  ptr = mm_realloc(ptr, 200);
  assert(ptr != NULL);
  assert(strcmp((char*)ptr, "test data") == 0);
  
  // Test shrinking realloc
  ptr = mm_realloc(ptr, 50);
  assert(ptr != NULL);
  assert(strncmp((char*)ptr, "test data", 9) == 0);
  
  // Test same size realloc
  ptr = mm_realloc(ptr, 50);
  assert(ptr != NULL);
  
  mm_free(ptr);
  printf("Basic realloc tests passed!\n");
}

void test_edge_cases() {
  printf("Running edge case tests...\n");
  
  // Test zero size malloc
  void* ptr1 = mm_malloc(0);
  // Should either return NULL or a valid pointer that can be freed
  mm_free(ptr1);
  
  // Test NULL pointer free (should not crash)
  mm_free(NULL);
  
  // Test realloc with NULL pointer (should behave like malloc)
  void* ptr2 = mm_realloc(NULL, 100);
  assert(ptr2 != NULL);
  mm_free(ptr2);
  
  // Test realloc with zero size (should behave like free)
  void* ptr3 = mm_malloc(100);
  assert(ptr3 != NULL);
  ptr3 = mm_realloc(ptr3, 0);
  // ptr3 should now be either NULL or a pointer that can be freed
  mm_free(ptr3);
  
  printf("Edge case tests passed!\n");
}

void test_memory_integrity() {
  printf("Running memory integrity tests...\n");
  
  // Test data preservation
  int* data = (int*)mm_malloc(sizeof(int) * 10);
  assert(data != NULL);
  
  for (int i = 0; i < 10; i++) {
    data[i] = i * i;
  }
  
  for (int i = 0; i < 10; i++) {
    assert(data[i] == i * i);
  }
  
  mm_free(data);
  
  // Test realloc data preservation
  char* str = (char*)mm_malloc(20);
  assert(str != NULL);
  strcpy(str, "Hello World");
  
  str = (char*)mm_realloc(str, 100);
  assert(str != NULL);
  assert(strcmp(str, "Hello World") == 0);
  
  mm_free(str);
  
  // Test alignment
  void* ptr1 = mm_malloc(1);
  void* ptr2 = mm_malloc(1);
  assert(ptr1 != NULL && ptr2 != NULL);
  assert(((uintptr_t)ptr1 % sizeof(void*)) == 0);
  assert(((uintptr_t)ptr2 % sizeof(void*)) == 0);
  mm_free(ptr1);
  mm_free(ptr2);
  
  printf("Memory integrity tests passed!\n");
}

void test_memory_mapping() {
  printf("Running memory mapping verification tests...\n");
  
  // Test that all bytes in allocated memory are readable and writable
  size_t test_sizes[] = {1, 8, 16, 64, 256, 1024, 4096, 65536};
  size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
  
  for (size_t i = 0; i < num_sizes; i++) {
    size_t size = test_sizes[i];
    unsigned char* ptr = (unsigned char*)mm_malloc(size);
    assert(ptr != NULL);
    
    // Write a pattern to every byte
    for (size_t j = 0; j < size; j++) {
      ptr[j] = (unsigned char)(j % 256);
    }
    
    // Read back and verify every byte
    for (size_t j = 0; j < size; j++) {
      assert(ptr[j] == (unsigned char)(j % 256));
    }
    
    // Write a different pattern
    for (size_t j = 0; j < size; j++) {
      ptr[j] = (unsigned char)((255 - j) % 256);
    }
    
    // Verify the new pattern
    for (size_t j = 0; j < size; j++) {
      assert(ptr[j] == (unsigned char)((255 - j) % 256));
    }
    
    mm_free(ptr);
  }
  
  // Test that realloc preserves memory mapping
  unsigned char* realloc_ptr = (unsigned char*)mm_malloc(1000);
  assert(realloc_ptr != NULL);
  
  // Fill with pattern
  for (size_t i = 0; i < 1000; i++) {
    realloc_ptr[i] = (unsigned char)(i % 256);
  }
  
  // Expand and verify old data is still accessible
  realloc_ptr = (unsigned char*)mm_realloc(realloc_ptr, 2000);
  assert(realloc_ptr != NULL);
  
  for (size_t i = 0; i < 1000; i++) {
    assert(realloc_ptr[i] == (unsigned char)(i % 256));
  }
  
  // Write to new region
  for (size_t i = 1000; i < 2000; i++) {
    realloc_ptr[i] = (unsigned char)((i + 100) % 256);
  }
  
  // Verify all data is accessible
  for (size_t i = 0; i < 1000; i++) {
    assert(realloc_ptr[i] == (unsigned char)(i % 256));
  }
  for (size_t i = 1000; i < 2000; i++) {
    assert(realloc_ptr[i] == (unsigned char)((i + 100) % 256));
  }
  
  mm_free(realloc_ptr);
  
  // Test boundary access (first and last byte of allocation)
  size_t boundary_size = 10000;
  unsigned char* boundary_ptr = (unsigned char*)mm_malloc(boundary_size);
  assert(boundary_ptr != NULL);
  
  // Write to first byte
  boundary_ptr[0] = 0xAB;
  assert(boundary_ptr[0] == 0xAB);
  
  // Write to last byte
  boundary_ptr[boundary_size - 1] = 0xCD;
  assert(boundary_ptr[boundary_size - 1] == 0xCD);
  
  // Write to middle bytes
  for (size_t i = 1; i < boundary_size - 1; i++) {
    boundary_ptr[i] = (unsigned char)(i % 256);
  }
  
  // Verify all bytes
  assert(boundary_ptr[0] == 0xAB);
  assert(boundary_ptr[boundary_size - 1] == 0xCD);
  for (size_t i = 1; i < boundary_size - 1; i++) {
    assert(boundary_ptr[i] == (unsigned char)(i % 256));
  }
  
  mm_free(boundary_ptr);
  
  printf("Memory mapping verification tests passed!\n");
}

void test_stress() {
  printf("Running stress tests...\n");
  
  // Test multiple allocations
  void* ptrs[100];
  for (int i = 0; i < 100; i++) {
    ptrs[i] = mm_malloc((i + 1) * 10);
    assert(ptrs[i] != NULL);
  }
  
  for (int i = 0; i < 100; i++) {
    mm_free(ptrs[i]);
  }
  
  // Test fragmentation pattern
  void* frag_ptrs[50];
  for (int i = 0; i < 50; i++) {
    frag_ptrs[i] = mm_malloc(100);
    assert(frag_ptrs[i] != NULL);
  }
  
  // Free every other block
  for (int i = 0; i < 50; i += 2) {
    mm_free(frag_ptrs[i]);
  }
  
  // Try to allocate in the gaps
  for (int i = 0; i < 25; i++) {
    void* gap_ptr = mm_malloc(50);
    assert(gap_ptr != NULL);
    mm_free(gap_ptr);
  }
  
  // Free remaining blocks
  for (int i = 1; i < 50; i += 2) {
    mm_free(frag_ptrs[i]);
  }
  
  // Test mixed operations
  void* mixed_ptrs[20];
  for (int i = 0; i < 10; i++) {
    mixed_ptrs[i] = mm_malloc(100 + i * 50);
    assert(mixed_ptrs[i] != NULL);
  }
  
  for (int i = 0; i < 5; i++) {
    mixed_ptrs[i] = mm_realloc(mixed_ptrs[i], 200 + i * 100);
    assert(mixed_ptrs[i] != NULL);
  }
  
  for (int i = 0; i < 10; i++) {
    mm_free(mixed_ptrs[i]);
  }
  
  printf("Stress tests passed!\n");
}

void print_usage(const char* program_name) {
  printf("Usage: %s [test_name]\n", program_name);
  printf("Available tests:\n");
  printf("  simple_alloc_free  - Simple malloc/free verification\n");
  printf("  basic_malloc_free  - Basic malloc/free functionality\n");
  printf("  basic_realloc      - Basic realloc functionality\n");
  printf("  edge_cases         - Edge case testing\n");
  printf("  memory_integrity   - Memory integrity verification\n");
  printf("  memory_mapping     - Memory mapping verification\n");
  printf("  stress             - Stress testing\n");
  printf("  all                - Run all tests (default)\n");
}

int main(int argc, char* argv[]) {
  load_alloc_functions();
  
  const char* test_name = (argc > 1) ? argv[1] : "all";
  
  if (strcmp(test_name, "help") == 0 || strcmp(test_name, "-h") == 0 || strcmp(test_name, "--help") == 0) {
    print_usage(argv[0]);
    return 0;
  }
  
  printf("Running memory allocator tests...\n\n");
  
  if (strcmp(test_name, "simple_alloc_free") == 0) {
    test_simple_alloc_free();
  } else if (strcmp(test_name, "basic_malloc_free") == 0) {
    test_basic_malloc_free();
  } else if (strcmp(test_name, "basic_realloc") == 0) {
    test_basic_realloc();
  } else if (strcmp(test_name, "edge_cases") == 0) {
    test_edge_cases();
  } else if (strcmp(test_name, "memory_integrity") == 0) {
    test_memory_integrity();
  } else if (strcmp(test_name, "memory_mapping") == 0) {
    test_memory_mapping();
  } else if (strcmp(test_name, "stress") == 0) {
    test_stress();
  } else if (strcmp(test_name, "all") == 0) {
    test_simple_alloc_free();
    test_basic_malloc_free();
    test_basic_realloc();
    test_edge_cases();
    test_memory_integrity();
    test_memory_mapping();
    test_stress();
  } else {
    printf("Unknown test: %s\n\n", test_name);
    print_usage(argv[0]);
    return 1;
  }
  
  printf("\nTest(s) completed successfully!\n");
  return 0;
}
