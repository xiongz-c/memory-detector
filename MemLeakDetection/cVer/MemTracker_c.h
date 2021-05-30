#ifndef MEMTRACER_C
#define MEMTRACER_C
#include <stddef.h>
struct Mem_node{
    struct Mem_node* prev;
    struct Mem_node* next;
    size_t _size;
    char* function_name;
    char* file_name;
    void* addr;
};

void init_dummyHead();
void allocate_prework();
void allocate_afterwork(size_t _size, char* file_name, char* function_name,void* addr);
int delete_node(void* ptr);
void __initial();
void show_list(int);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);

#endif