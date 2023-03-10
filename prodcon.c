#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <semaphore.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "tands.h"
#include "helper.h"
/* Resource:
    get basic idea from: https://www.youtube.com/watch?v=l6zkaJFjUbM
    using broadcast: https://www.youtube.com/watch?v=RtTlIvnBw10
                     https://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_broadcast.html
    using conditional variable: Book: Modern Operating system 4th adition by Andrew S. Tanenbaum and Herbert Bos on page 138
    using clock: https://www.techiedelight.com/find-execution-time-c-program/
    buffer or index get idea from: https://www.cs.fsu.edu/~baker/opsys/notes/prodcons.html

    Ideas: 
    Using conditional varaible to implement and solve this problem.
    Consumer using conditional variable : condc
    Producer using conditional variable: condp

    Using mutex_take_condition (lock) to take control of the queue(buffer)
    Using mutex_access_file (lock) to take control of the written file.
    
    When queue is empty, the consumer will wait until producer send a condc broadcast so consumer could check again
    When queue is full, the producer will wait until consumer send a condp singal 
    
    When producer finish reading the file or EOF from keyboard, the parent_finish falg will be 1
          and transaction_total will not be changed( total_word), and the parent will broadcast evey waiting child 
          to wake up and check the buffer, when there is no work in the queue and parent is finish,
          the child will end.
*/

pthread_mutex_t mutex_take_condition = PTHREAD_MUTEX_INITIALIZER; // take task (conditional variable)
pthread_mutex_t mutex_do_trans = PTHREAD_MUTEX_INITIALIZER; // do task ( mutex exclusion)
pthread_mutex_t mutex_access_file = PTHREAD_MUTEX_INITIALIZER; 

// use conditional variable( implememt empty and full)
pthread_cond_t condc, condp; 

FILE *fd;
int MAX_BUF_SIZE;
int *buffer;
char *instruction[100];
int count_producer = 0;
int count_consumer = 0;
int transaction_total=0;
int parent_finish = 0;
int count = 0;
clock_t begin;
double time_spent = 0.000;
int finish_consumer = 0;
int consumer_need_stop = 0;

// for summary
int ask;
int receive; 
int complete = 0;


void* customer_routine(void* args_c);
double get_time();

void* customer_routine(void* args_c){ // each customer
    int *id = (int*) args_c;
    int *id_complete = malloc(sizeof(int));
    *id_complete = 0;
    for(;;){
        //ready to receive task( if has task)
        pthread_mutex_lock(&mutex_access_file);
        c_ask(*id);
        ask++;
        pthread_mutex_unlock(&mutex_access_file);

        //check the length of the queue
        pthread_mutex_lock(&mutex_take_condition);
        // mutex1
        while(count ==0 ){                                    // wait for the producer to send a signal
            if( count ==0 && parent_finish==1){
                pthread_mutex_unlock(&mutex_take_condition);
                consumer_need_stop = 1;
                break;
                //exit() EOF+ queue empty
            }
            pthread_cond_wait(&condc,&mutex_take_condition); // unlocked mutex_access_buffer
        }

        if(consumer_need_stop==1){
            break;
        }
        int get_task_amount = buffer[count_consumer];

        count_consumer++;           // increase consumer index
        count_consumer = count_consumer%MAX_BUF_SIZE;
        count--;                    // decrease # of tasks
        receive++;
        
        if( count== MAX_BUF_SIZE-1){ 
            pthread_cond_signal(&condp);
        }

        // print to file thread takes task from the queue
        pthread_mutex_lock(&mutex_access_file);
        c_receive(*id, get_task_amount);
        pthread_mutex_unlock(&mutex_access_file);

        pthread_mutex_unlock(&mutex_take_condition); // unlock

        //pthread_mutex_lock(&mutex_do_trans); // mutex : do jobs
        Trans(get_task_amount);
        complete++;
        *(id_complete) +=1;

        // print to file
        pthread_mutex_lock(&mutex_access_file);
        c_complete(*id, get_task_amount);
        pthread_mutex_unlock(&mutex_access_file);

        //pthread_mutex_unlock(&mutex_do_trans); 

        if(( parent_finish==1)&&( complete== transaction_total)){
            break;
        }
    }
    finish_consumer++;
    return (void*)id_complete;

}

int main(int argc, char *argv[]){
    time_spent = 0.000;
    begin = clock();
    int sleep = 0;
        
    // read the command
    int consumer_num = atoi(argv[1]);
    MAX_BUF_SIZE = consumer_num*2;

    // initialize the buffer ( 2* consumer amount)
    buffer = malloc(MAX_BUF_SIZE*sizeof(int)); 

    if(argc==3){
        char *file_name = (char*) malloc(5*sizeof(char));
        sprintf(file_name, "prodcon.%s.log",argv[2]);
        fd = fopen(file_name,"w");
    }else{
        fd = fopen("prodcon.log","w");
    }

    pthread_t customers[consumer_num];
    int consumer_id[consumer_num];

    for ( int id = 0; id< consumer_num;id++){
        consumer_id[id] = id+1; // consumer0: id = 1
        if ( pthread_create(&customers[id],NULL,customer_routine, &(consumer_id[id]))!=0){
            perror("Failed to create customer thread\n");
        }
    }

    // start reading input
    int quit = 0;
    while(quit==0){ 
        int i;
        char buf[4];
        i = scanf("%s",buf);
        if ( i==EOF){
            quit = 1;
            pthread_cond_broadcast(&condc); 
            break;
        }
        //parent( producer) receive work with amount 
        int amount;
        amount = atoi(&buf[1]);  

        if( buf[0] == 'S'){
            pthread_mutex_lock(&mutex_access_file);
            p_sleep(amount);
            pthread_mutex_unlock(&mutex_access_file);

            sleep++;
            Sleep(amount); // producer no can't access buffer
        }else{
            pthread_mutex_lock(&mutex_take_condition); // lock
            
            while(count == MAX_BUF_SIZE){
                pthread_cond_wait(&condp,&mutex_take_condition); // producer wait
            }
            
            count++;  // task in queue
            
            buffer[count_producer] = amount; // add the task into the buffer            

            pthread_cond_broadcast(&condc);  // braodcast to all the wait consumer
            //pthread_mutex_unlock(&mutex_take_condition); // unlock

            pthread_mutex_lock(&mutex_access_file);
            p_work(amount);
            pthread_mutex_unlock(&mutex_access_file);
            
            // write file here before
            pthread_mutex_unlock(&mutex_take_condition); // unlock

            // increase producer index
            count_producer++; 
            count_producer = count_producer% MAX_BUF_SIZE;
            transaction_total++;     
        }

    }
    // finish reading input
    parent_finish = 1;
    pthread_mutex_lock(&mutex_access_file); // write End to file
    p_end();
    pthread_mutex_unlock(&mutex_access_file); 
    
    // while(complete!=transaction_total){
    //     pthread_cond_signal(&condc); 
    // }
    
    int* consumer_complete[consumer_num]; // collect complete for each thread
    for ( int j = 0; j< consumer_num;j++){
        pthread_join(customers[j],(void**)&consumer_complete[j]);
        // consumer[0]: id=1, consumer_complete[0] 
    }
    float end = get_time();
    float trans_per_sec = complete/end;

    // pthread_mutex_lock(&mutex_access_file);
    fprintf(fd,"Summary:\n");
    fprintf(fd,"    Work          %d\n",transaction_total);
    fprintf(fd,"    Ask           %d\n",ask);
    fprintf(fd,"    Complete      %d\n",complete);
    fprintf(fd,"    Receive       %d\n",receive);
    fprintf(fd,"    Sleep         %d\n",sleep);
    for ( int i = 0; i< consumer_num;i++){
        fprintf(fd,"    Thread  %d        %d\n",i+1,*(consumer_complete[i]));
        free(consumer_complete[i]);
    }
    fprintf(fd,"Transactions per second: %.3f\n",trans_per_sec);
    // pthread_mutex_unlock(&mutex_access_file); 
    
    fclose(fd);
    free(buffer);

    return 0;


}