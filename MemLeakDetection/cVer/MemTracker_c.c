#include<stdio.h>
#include<stdlib.h>
#include<dlfcn.h>
#include<string.h>
#include<execinfo.h>
#include"MemTracker_c.h"
typedef struct Mem_node Mem_node;

void *(*realMalloc)(size_t size) = NULL;
void *(*realCalloc)(size_t nmemb, size_t size) = NULL;
void *(*realRealloc)(void *ptr, size_t size) = NULL;
void (*realFree)(void* ptr) = NULL; //
void _exit_hook(int __status, void *__arg);
Mem_node dummy_head;
Mem_node* tail_node = &dummy_head;
FILE * stkInfo;
static int cnt_init = 0;
static int cnt_print = 0;
void init_dummyHead(){
    dummy_head.next = NULL;
    dummy_head.prev = NULL;
    dummy_head.addr = NULL;
    dummy_head._size = 0;
    dummy_head.file_name = "dummy_name";
    dummy_head.function_name = "dummy_name";
}

void allocate_prework(){
    Mem_node* cur_node = (Mem_node*)realMalloc(sizeof(Mem_node));
    tail_node->next = cur_node;
    cur_node->prev = tail_node;
    cur_node->next = NULL;
    tail_node = cur_node;
}

void allocate_afterwork(size_t _size, char* file_name, char* function_name,void* addr){
    tail_node->_size = _size;
    tail_node->file_name = (char*)(realMalloc(strlen(file_name)+1));
    strcpy(tail_node->file_name,file_name);
    tail_node->file_name[strlen(file_name)] = '\0';
    tail_node->function_name = (char*)(realMalloc(strlen(function_name)+1));
    strcpy(tail_node->function_name,function_name);
    tail_node->function_name[strlen(function_name)] = '\0';
    tail_node->addr = addr;
}

int delete_node(void* ptr){
    Mem_node* cur_node = &dummy_head;
    while(cur_node!=NULL){      
        if(cur_node->addr){
            if(cur_node->addr == ptr){        
                if(cur_node->prev)cur_node->prev->next = cur_node->next;
                if(cur_node->next)cur_node->next->prev = cur_node->prev;
                if(cur_node->next == NULL){
                    tail_node = tail_node->prev;
                }
                realFree(cur_node);
                return 1;
            }
        }
        cur_node = cur_node->next;
    }
    return 0;
}

void show_list(int status){
    Mem_node* cur_node = &dummy_head;
    fprintf(stderr,"CurrentInfo%d\n",++cnt_print);
    while(cur_node!=NULL){  
       if(cur_node->file_name != "dummy_name")fprintf(stderr,"%p size %ld bytes %s\n",cur_node->addr,
       cur_node->_size,cur_node->file_name);
        cur_node = cur_node->next;
    }
    fprintf(stderr,"=========================================\n");
}

void *malloc(size_t size){
    __initial();
    static int callcounter;
    callcounter++;
    char *error;
    realMalloc = dlsym(RTLD_NEXT,"malloc");
    void *ptr = realMalloc(size); 
    if(1 == callcounter){
        allocate_prework();
        Dl_info info;
        if (dladdr(__builtin_return_address(0), &info)) {
            allocate_afterwork(size,(char*)info.dli_fname,(char*)__builtin_FUNCTION(),ptr);
        }
        #ifdef ONLINE
            show_list(0);
        #endif 
    } 
    callcounter = 0;
    return ptr;
}

void *calloc(size_t nmemb, size_t size){
    __initial();
    static int callcounter;
    callcounter++;
    realCalloc = dlsym(RTLD_NEXT,"calloc");
    void *ptr = realCalloc(nmemb,size);
    if(1 == callcounter){
        allocate_prework();
        Dl_info info;
        if (dladdr(__builtin_return_address(0), &info)) {
            allocate_afterwork(size*nmemb,(char*)info.dli_fname,(char*)__builtin_FUNCTION(),ptr);
        }
        #ifdef ONLINE
            show_list(0);
        #endif 
    } 
    callcounter = 0;
    return ptr;
}

void *realloc(void *ptr, size_t size){
    if(!ptr){
        ptr = malloc(size);
    }else{
        __initial();
        free(ptr);
        ptr = malloc(size);
    }
    return ptr;
}

void free(void *ptr){
    realFree = dlsym(RTLD_NEXT,"free");
    delete_node(ptr);
    #ifdef ONLINE
            show_list(0);
    #endif 
    realFree(ptr);
    return;
}

void __initial(){
    if(cnt_init == 0){
        init_dummyHead();
        void* tmpargs;
        on_exit(_exit_hook,tmpargs);
        cnt_init++;
    }
}

void _exit_hook(int __status, void *__arg){
    show_list(0);
}