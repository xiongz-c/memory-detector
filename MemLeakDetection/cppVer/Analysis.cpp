#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <string>
#include<errno.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <execinfo.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/file.h>
#include <list>
#include <execinfo.h>
#include <unordered_map>
using namespace std;
FILE *Mem_Info;
FILE *Node_Info;
FILE *Trace_Info;
FILE *report;

list<string> pend_mem_list;
list<string> info_mem_list;
unordered_map<string,vector<string> >records;

void open_file(char* argv){
    Mem_Info = fopen("../Data/Mem_Info","r");
    Node_Info = fopen("../Data/Node_Info","r");
    Trace_Info = fopen("../Data/Trace_Info","r");
    char arr[30] = "../Report/";
    strcat(arr,argv);
    report = fopen(arr,"w+");
}

void read_node(){
    char buff[255];
    while (!feof(Node_Info)) {
        fgets(buff, 255, Node_Info);
        if (buff[0]=='0' && buff[1]=='x') {
            string tmp;
            string tmp_info;
            for(int i = 0;i < 12;i++){
                tmp.push_back(buff[i+2]);
            }
            for(int i = 15;buff[i]!=',';i++){
                tmp_info.push_back(buff[i]);
            }
            pend_mem_list.push_back(tmp);
            info_mem_list.push_back(tmp_info);
        }
    }
    return;
}

void read_mem(){
    char buff[255];
    while (!feof(Mem_Info)) {
        fgets(buff, 255, Mem_Info);
        auto j = info_mem_list.begin();
        for(auto i = pend_mem_list.begin() ; i !=pend_mem_list.end();){
            string tmp = *i;
            if(strstr(buff,tmp.c_str())){
                pend_mem_list.erase(i++);
                info_mem_list.erase(j++);
            }else{
                i++;
                j++;
            }
        }
    }
    return;
}

void read_trace(){
    char buff[255];
    int flag = 0;
    while (!feof(Trace_Info)) {
        if(!flag)fgets(buff, 255, Trace_Info);
        flag = 0;
        for(auto i = pend_mem_list.begin() ; i !=pend_mem_list.end();i++){
            string tmp = *i;
            string res;
            if(strstr(buff,tmp.c_str())){
                while (!feof(Trace_Info)){
                    res += buff;
                    fgets(buff, 255, Trace_Info);
                    if(buff[0]=='0' && buff[1]=='x'){
                        flag = 1;
                        break;
                    }     
                }
            }
            if(records.find(tmp) == records.end()){ 
                vector<string> ele;
                ele.push_back(res);
                records.insert({tmp,ele});
            }else{
                records[tmp].push_back(res);
            }
        }
    }
    return;
}

void generate_records(char* argv){
    char arr[30] = "../Report/";
    strcat(arr,argv);
    report = fopen(arr,"w+");
    setbuf(report,NULL);
    printf("There may be %d memory leaks.\n\n",(int)pend_mem_list.size());
    fprintf(report,"There may be %d memory leaks.\n\n",(int)pend_mem_list.size());
    auto j = info_mem_list.begin();
    for(auto i = pend_mem_list.begin() ; i !=pend_mem_list.end();i++){
        printf("The memory in addres :0x%s %s\n",(*i).c_str(),(*j).c_str());
        fprintf(report,"The memory in addres :0x%s %s\n",(*i).c_str(),(*j).c_str());
        fflush(report);
        printf("The stack Info: \n");
        fprintf(report,"The stack Info: \n");
        fflush(report);
        printf("=======================================================\n");
        fprintf(report,"=======================================================\n");
        fflush(report);
        int last = records[*i].size()-1;
        for(int k = 0 ; k <= last ; k++){
            if(strlen(records[*i][k].c_str())>5){
            printf("%s",records[*i][k].c_str());
            fprintf(report,"%s",records[*i][k].c_str());
            break;
            }
        }
        fflush(report);
        printf("=======================================================\n");
        fprintf(report,"=======================================================\n");
        fflush(report);
    } 
    
}


int main(int argc, char *argv[]){
    while(true){
        sleep(5);
        open_file(argv[1]);
        if(flock(fileno(Node_Info), LOCK_EX)==0){
            if(flock(fileno(Mem_Info), LOCK_EX)==0){
                if(flock(fileno(Trace_Info), LOCK_EX)==0){
                    read_node();
                    read_mem();
                    if(pend_mem_list.size() == 0){
                        cout << "No memory leak has been detected." << endl;
                    }else{
                        read_trace();
                        generate_records(argv[1]);
                        pend_mem_list.clear();
                        info_mem_list.clear();
                        records.clear();
                    }
                }
                flock(fileno(Trace_Info), LOCK_UN);
                fclose(Trace_Info);
            }
            flock(fileno(Mem_Info), LOCK_UN);
            fclose(Mem_Info);
        }
        flock(fileno(Node_Info), LOCK_UN);
        fclose(Node_Info);  
    }
}
