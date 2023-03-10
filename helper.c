#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "helper.h"

extern clock_t begin;
extern double time_spent;
extern int count;
extern FILE *fd;
double get_time();
void c_ask(int id);
void c_receive(int id, int amount);
void p_work( int amount);
void p_sleep(int amount);
void c_complete(int id, int amount);
void p_end();


double get_time(){
    clock_t end = clock();
 
    return (double)(end - begin) / CLOCKS_PER_SEC;

}
void c_ask(int id){
    //0.000 ID= 1 Ask 
    fprintf(fd,"%.3f    ID= %d             Ask \n",get_time(), id);
}

void c_receive(int id, int amount){
    //0.000 ID= 3 Q= 0 Receive 4 
    fprintf(fd,"%.3f    ID= %d  Q= %d       Receive  %d \n",get_time(), id,count,amount);
}

void p_work( int amount){
    //0.000 ID= 0 Q= 1 Work 4
    fprintf(fd,"%.3f    ID= 0  Q= %d       Work  %d\n",get_time(),count,amount);
}

void p_sleep(int amount){
    // 0.000 ID= 0 Sleep 9 // Parent sleeps, n=9
    fprintf(fd,"%.3f    ID= 0             Sleep  %d \n",get_time(),amount);

}

void c_complete(int id, int amount){
    // 0.002 ID= 2 Complete 1 // Thread 2 completes task, n=1
    fprintf(fd,"%.3f    ID= %d             Complete  %d \n",get_time(),id,amount);

}

void p_end(){
    // 0.100 ID= 0 End // End of input for producer
    fprintf(fd,"%.3f    ID= 0             End \n",get_time());
}