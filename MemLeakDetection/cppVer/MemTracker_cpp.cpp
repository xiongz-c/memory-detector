#include <iostream>
#include <stdio.h>
#include <unordered_map>
#include <cstring>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <execinfo.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/file.h>
#include <execinfo.h>
#include "MemTracker_cpp.h"

FILE *stkInfo;
FILE *logInfo;
FILE *listInfo;
FILE *traceInfo;
char stk_bot[20];
char stk_top[20];
long long int bot_addr;
long long int top_addr;
static int cnt_init = 0;
static int cnt_print = 0;
Mem_node dummy_head;
Mem_node* tail_node = &dummy_head;
static int cnt_read = 0;
static int cnt_print_mem = 0;
void _exit_hook(int __status, void *__arg);


void print_trace (void*);
void count_list();
void get_addr_range();
void sigint_handler(int sig){
    if(sig == SIGINT){
		show_list(1);
		read_stkInfo();
        exit(120);
	}
}

void init_dummyHead(){
    dummy_head.next = NULL;
    dummy_head.prev = NULL;
    dummy_head.addr = NULL;
    dummy_head._size = 0;
    dummy_head.file_name = (char*)"dummy_name";
    dummy_head.function_name =(char*)"dummy_name";
}

void file_initial(){
    logInfo = fopen("./Data/Mem_Info", "w+");
    listInfo = fopen("./Data/Node_Info","w+");
    traceInfo = fopen("./Data/Trace_Info","w+");
    fclose(logInfo);
    fclose(listInfo);
}

void __initial(){
    if(cnt_init == 0){
        signal(SIGINT, sigint_handler);
        init_dummyHead();
        file_initial();
        get_addr_range();
        void* tmpargs;
        on_exit(_exit_hook,tmpargs);
        cnt_init++;
    }
}

void get_addr_range(){
    stkInfo = fopen("/proc/self/maps", "r");
    char buff[1024];
    while (!feof(stkInfo)) {
        fgets(buff, 1024, stkInfo);
        if (strstr(buff, "stack")) {
            strncpy(stk_bot, buff, 12);
            strncpy(stk_top, buff + 13, 12);
            fprintf(stderr,"%s\n",stk_bot);
            fprintf(stderr,"%s\n",stk_top);
            fprintf(stderr,"=======\n");
            bot_addr = to_longlong(stk_bot);
            top_addr = to_longlong(stk_top);
        }
    }
}



void read_stkInfo() {
    void *ptr = (void *)bot_addr;
    hexdump(ptr,top_addr-bot_addr);
}

long long to_longlong(char *mystring) {
    long long res = 0;
    for (int i = 0; i <= 11; i++) {
        long long tmp = 0;
        res *= 16;
        if (mystring[i] >= 'a' && mystring[i] <= 'f') {
            tmp += 10 + mystring[i] - 'a';
        } else {
            tmp += mystring[i] - '0';
        }
        res += tmp;
    }
    return res;
}

void hexdump(void *pSrc,int len) {
    unsigned char *line;
    int i;
    int thisline;
    int offset;
    line = (unsigned char *) pSrc;
    offset = 0;
    logInfo = fopen("./Data/Mem_Info", "w+");
    setbuf(logInfo,NULL);
    fprintf(logInfo,"CurrentInfo%d\n",++cnt_print_mem);
    fflush(logInfo);
    if(flock(fileno(logInfo), LOCK_EX)==0){
        while (offset < len) {
            thisline = len - offset;
            if (thisline > 16) {
                thisline = 16;
            }
            for (i = thisline-1; i >=0 ; i--) {
                fprintf(logInfo,"%02x", line[i]);
                fflush(logInfo);
            }
            fprintf(logInfo,"\n");
            fflush(logInfo);
            offset += thisline;
            line += thisline;
        }
    }
    flock(fileno(logInfo), LOCK_UN);
    fclose(logInfo);
}

void allocate_prework(){
    Mem_node* cur_node = (Mem_node*)malloc(sizeof(Mem_node));
    tail_node->next = cur_node;
    cur_node->prev = tail_node;
    cur_node->next = NULL;
    tail_node = cur_node;

}

void allocate_afterwork(size_t _size, char* file_name, char* function_name,void* addr){
    tail_node->_size = _size;
    tail_node->file_name = (char*)(malloc(strlen(file_name)+1));
    strcpy(tail_node->file_name,file_name);
    tail_node->file_name[strlen(file_name)] = '\0';
    tail_node->function_name = (char*)(malloc(strlen(function_name)+1));
    strcpy(tail_node->function_name,function_name);
    tail_node->function_name[strlen(function_name)] = '\0';
    tail_node->addr = addr;
    
}

void * operator new(size_t size){
    __initial();
    void *ptr=malloc(size);
    Dl_info info;
    if (dladdr(__builtin_return_address(0), &info)) {
        if(!strstr((char*)info.dli_fname,"libstdc++")){
            allocate_prework();
            allocate_afterwork(size,(char*)info.dli_fname,(char*)__builtin_FUNCTION(),ptr);
            #ifdef ONLINE
                show_list(0);
                read_stkInfo();
                print_trace(ptr);
            #endif
        }
    }
    return ptr;
}

void *operator new[](size_t size){
    __initial();
    void *ptr=malloc(size);
    Dl_info info;
    if (dladdr(__builtin_return_address(0), &info)){
        if(!strstr((char*)info.dli_fname,"libstdc++")){
            print_trace(ptr);
            allocate_prework();
            allocate_afterwork(size,(char*)info.dli_fname,(char*)__builtin_FUNCTION(),ptr);
            #ifdef ONLINE
                show_list(0);
                read_stkInfo();
            #endif
        }
    }
    return ptr;
}


void operator delete(void *ptr){   
    __initial();
    delete_node(ptr);
    free(ptr);
}

void operator delete[](void *ptr){
    __initial();
    delete_node(ptr);
    free(ptr);
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
                free(cur_node);
                #ifdef ONLINE
                    show_list(0);
                    read_stkInfo();
                    print_trace(ptr);
                #endif
                return 1;
            }
        }
        cur_node = cur_node->next;
    }
    return 0;
}


void show_list(int status){ 
    listInfo = fopen("./Data/Node_Info","w+");
    setbuf(listInfo,NULL);
    if(status == 0){
        if(flock(fileno(listInfo), LOCK_EX)==0){
            Mem_node* cur_node = &dummy_head;
            fprintf(listInfo,"CurrentInfo%d\n",++cnt_print);
            fflush(listInfo);
            while(cur_node!=NULL){  
            if(cur_node->file_name != "dummy_name" && !strstr(cur_node->file_name,"libstdc++")){
                fprintf(listInfo,"%p,%ldbytes,%s\n",cur_node->addr,
                        cur_node->_size,cur_node->file_name);
                 fflush(listInfo);
            }   
                cur_node = cur_node->next;
            }
            fprintf(listInfo,"===\n");
            fflush(listInfo);
        }
        flock(fileno(listInfo), LOCK_UN);
    }else{
        Mem_node* cur_node = &dummy_head;
            fprintf(listInfo,"CurrentInfo%d\n",++cnt_print);
            fflush(listInfo);
            while(cur_node!=NULL){  
            if(cur_node->file_name != "dummy_name" && !strstr(cur_node->file_name,"libstdc++")){
                fprintf(listInfo,"%p,%ldbytes,%s\n",cur_node->addr,
                        cur_node->_size,cur_node->file_name);
                        fflush(listInfo);
            }   
                cur_node = cur_node->next;
            }
            fprintf(listInfo,"===\n");
            fflush(listInfo);
    }
    fclose(listInfo);
}

void count_list(){
    int res = 0;
    Mem_node* cur_node = &dummy_head;
    while(cur_node!=NULL){  
        cur_node = cur_node->next;
        res++;
    }
    fprintf(stderr,"%d\n",res);
}


void _exit_hook(int __status, void *__arg){
    show_list(0);
    read_stkInfo();
}

void print_trace (void* ptr){
  void *array[10];
  char **strings;
  int size, i;
  fprintf (traceInfo,"%p\n", ptr);
  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);
  if (strings != NULL)
  {

    fprintf (traceInfo,"Obtained %d stack frames.\n", size);
    for (i = 0; i < size; i++)
      fprintf (traceInfo,"%s\n", strings[i]);
  }
  free (strings);
}