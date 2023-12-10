#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "signal.h"
#include "disk.h"
#include "ppos_disk.h"
#include "ppos-core-globals.h"
#include "ppos.h"

void disk_task_body();
void disk_signal_handler();
void insert_duty(disk_duty_t* duty);
void print_duty_queue();
disk_duty_t* escalonate(int option);
disk_duty_t* create_duty(task_t* owner, int block, void* buffer, int op);
void print_queue(task_t* queue);

disk_t main_disk;
task_t disk_manager_task;
mutex_t mutex;
task_t* disk_wait_queue;
disk_duty_queue_t disk_duty_queue;

int disk_mgr_init (int *numBlocks, int *blockSize) 
{
    int result = disk_cmd (DISK_CMD_INIT, 0, 0);

    if(result < 0)
        return -1;
    
    int disk_size = disk_cmd (DISK_CMD_DISKSIZE, 0, 0) ;

    if(disk_size < 0)
        return -1;

    *numBlocks = disk_size;

    int disk_block_size = disk_cmd (DISK_CMD_BLOCKSIZE, 0, 0) ;

    if(disk_block_size < 0)
        return -1;

    *blockSize = disk_block_size;

    signal(SIGUSR1, disk_signal_handler);

    task_create(&disk_manager_task, disk_task_body, NULL);
    task_suspend(&disk_manager_task, NULL);

    mutex_create(&mutex);

    return 0;
}

int disk_block_read (int block, void *buffer)
{
    //printf("Solicitou disco\n");
    mutex_lock(&mutex);

    disk_duty_t* duty = create_duty(taskExec, block, buffer, 0);
    insert_duty(duty);

    mutex_unlock(&mutex);

    task_resume(&disk_manager_task);
    
    task_suspend(taskExec, &disk_wait_queue);
    task_yield();

    //printf("Resumed task and copying buffer\n");

    memcpy(buffer,duty->buffer, 64);

    free(duty);

    return 0;
}

int disk_block_write (int block, void *buffer)
{
    mutex_lock(&mutex);

    disk_duty_t* duty = create_duty(taskExec, block, buffer, 1);
    insert_duty(duty);

    mutex_unlock(&mutex);

    task_resume(&disk_manager_task);
    task_suspend(taskExec, &disk_wait_queue);
    task_yield();

    free(duty);

    return 0;
}

void disk_task_body()
{
    while(true)
    {
        //printf("Gerente de disco\n");
        if(disk_manager_task.awake_by_disk == 1)
        {
            //printf("Gerente de disco recebeu um sinal do disco\n");
            task_resume(disk_wait_queue);
            disk_manager_task.awake_by_disk = 0;
        }

        if(disk_duty_queue.head != NULL)
        {
            mutex_lock(&mutex);
            disk_duty_t* duty = escalonate(0);
            mutex_unlock(&mutex);
            
            if(duty->operation == 0)
            {
                int result = disk_cmd(DISK_CMD_READ, duty->block, duty->buffer);
                //printf("Reading\n");
            }
            else
            {
                int result = disk_cmd(DISK_CMD_WRITE, duty->block, duty->buffer);
                //printf("Writing\n");
            } 
        }

        task_suspend(&disk_manager_task, NULL);
        task_yield();
    }
}

void disk_signal_handler()
{
    disk_manager_task.awake_by_disk = 1;
    task_resume(&disk_manager_task);
}

void insert_duty(disk_duty_t* duty)
{
    if(disk_duty_queue.head == NULL)
    {
        disk_duty_queue.head = duty;
        disk_duty_queue.tail = duty;

        duty->prev = NULL;
        duty->next = NULL;

        return;
    }

    duty->next = NULL;
    duty->prev = disk_duty_queue.tail;
    disk_duty_queue.tail->next = duty;
    disk_duty_queue.tail = duty;

    return;
}

disk_duty_t* escalonate(int option)
{
    if(option == 0)
    {
        //print_duty_queue();
        disk_duty_t* duty = disk_duty_queue.head;
        //printf("Removendo da lista...\n");
        if(disk_duty_queue.head->next == NULL)
        {
            disk_duty_queue.head = NULL;

            //print_duty_queue();

            return duty;
        }

        disk_duty_queue.head->next->prev = NULL;
        disk_duty_queue.head = disk_duty_queue.head->next;

        //print_duty_queue();

        return duty;
    }
}

disk_duty_t* create_duty(task_t* owner, int block, void* buffer, int op)
{
    disk_duty_t* duty = (disk_duty_t*) malloc(sizeof(disk_duty_t));
    duty->operation = op;
    duty->block = block;
    duty->owner = owner;

    duty->buffer = malloc(64);

    if(op == 0)
        return duty;

    memcpy(duty->buffer, buffer, 64);

    return duty;
}

void print_duty_queue()
{   
    disk_duty_t* start = disk_duty_queue.head;
    disk_duty_t* aux = start;
    disk_duty_t* next = NULL;

    if(start == NULL)
        return;

    ("Duty block na lista -> %d\n", aux->block);

    aux = aux->next;

    while(aux != NULL)
    {
        next = aux->next;
        printf("Duty block na lista -> %d\n", aux->block);
        aux = next;
    }
}

void print_queue(task_t* queue)
{   
    task_t* start = disk_wait_queue;
    task_t* aux = start;
    task_t* next = NULL;

    if(start == NULL)
        return;

    printf("Tarefa na lista -> %d\n", aux->id);

    aux = aux->next;

    while(aux != start)
    {
        next = aux->next;
        printf("Tarefa na lista -> %d\n", aux->id);
        aux = next;
    }
}
