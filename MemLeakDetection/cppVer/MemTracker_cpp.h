#ifndef MEMTRACER_CPP
#define MEMTRACER_CPP
#include<string>
#include<cstddef>


class Mem_node{
public:
    size_t _size;
    char* function_name;
    char* file_name;
    void* addr;
    Mem_node* prev;
    Mem_node* next;
};
void *operator new(size_t size);
void *operator new[](size_t size);
void operator delete(void* ptr);
void operator delete[](void* ptr);
int delete_node(void* ptr);
void *myMalloc(size_t size);
void myFree(void *p);
void hexdump(void *pSrc,int len);
void read_stkInfo();
long long to_longlong(char *mystring);
void __initial();
void allocate_prework();
void allocate_afterwork(size_t _size, char* file_name, char* function_name,void* addr);
void init_dummyHead();
void show_list(int status);

#endif