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
semaphore_t s;
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

    main_disk.current_position = 0;
    main_disk.last_position = 0;

    signal(SIGUSR1, disk_signal_handler);

    task_create(&disk_manager_task, disk_task_body, NULL);
    task_suspend(&disk_manager_task, NULL);

    sem_create(&s, 1);

    return 0;
}

int disk_block_read (int block, void *buffer)
{
    //printf("Solicitou disco\n");
    sem_down(&s);

    disk_duty_t* duty = create_duty(taskExec, block, buffer, 0);
    insert_duty(duty);

    sem_up(&s);

    task_resume(&disk_manager_task);
    
    task_suspend(taskExec, &disk_wait_queue);
    task_yield();

    //printf("Resumed task and copying buffer\n");

    main_disk.current_position = duty->block;

    memcpy(buffer,duty->buffer, 64);

    free(duty);

    return 0;
}

int disk_block_write (int block, void *buffer)
{
    sem_down(&s);

    disk_duty_t* duty = create_duty(taskExec, block, buffer, 1);
    insert_duty(duty);

    sem_up(&s);

    task_resume(&disk_manager_task);
    task_suspend(taskExec, &disk_wait_queue);
    task_yield();

    main_disk.current_position = duty->block;

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
            disk_manager_task.awake_by_disk = 0;
            task_resume(disk_wait_queue);
        }

        if(disk_duty_queue.head != NULL)
        {
            sem_down(&s);
            disk_duty_t* duty = escalonate(0);
            sem_up(&s);
            
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
    else if(option == 1)
    {
        //printf("SSTF\n");
        int minor_distance = 9999;

        disk_duty_t* next_duty = NULL;

        disk_duty_t* start = disk_duty_queue.head;
        disk_duty_t* aux = start;
        disk_duty_t* next = NULL;

        if(start == NULL)
            return NULL;

        //printf("Getting next\n");

        if(aux->next == NULL)
        {
            disk_duty_queue.head = NULL;

            return aux;
        }

        aux = aux->next;

        while(aux != NULL)
        {
            next = aux->next;
            int distance = abs(main_disk.current_position - aux->block);
            
            if(distance < minor_distance)
            {
                next_duty = aux;
                minor_distance = distance;
            }

            aux = next;
        }

        disk_duty_queue.head->next->prev = NULL;
        disk_duty_queue.head = disk_duty_queue.head->next;

        task_t* duty_owner = next_duty->owner;

        disk_wait_queue->prev = duty_owner;
        duty_owner->next = disk_wait_queue;
        disk_wait_queue = duty_owner;
        disk_wait_queue->prev = NULL;

        return next_duty;
    }
    else if(option == 2)
    {
        //printf("CSCAN\n");

        int minor_distance = 9999;

        disk_duty_t* next_duty = NULL;

        disk_duty_t* start = disk_duty_queue.head;
        disk_duty_t* aux = start;
        disk_duty_t* next = NULL;

        if(start == NULL)
            return NULL;

        //printf("Getting next\n");

        if(aux->next == NULL)
        {
            disk_duty_queue.head = NULL;

            return aux;
        }

        aux = aux->next;

        while(aux != NULL)
        {
            next = aux->next;
            int distance = abs(main_disk.current_position - aux->block);
            
            if(distance < minor_distance && aux->block > main_disk.current_position)
            {
                next_duty = aux;
                minor_distance = distance;
            }

            aux = next;
        }

        if(next_duty == NULL)
        {
            int minor = 9999;

            disk_duty_t* start = disk_duty_queue.head;
            disk_duty_t* aux = start;
            disk_duty_t* next = NULL;

            if(start == NULL)
                return NULL;

            if(aux->next == NULL)
            {
                disk_duty_queue.head = NULL;

                return aux;
            }

            aux = aux->next;

            while(aux != NULL)
            {
                next = aux->next;
                
                if(aux->block < minor)
                {
                    next_duty = aux;
                    minor = aux->block;
                }

                aux = next;
            }
        }

        disk_duty_queue.head->next->prev = NULL;
        disk_duty_queue.head = disk_duty_queue.head->next;

        task_t* duty_owner = next_duty->owner;

        disk_wait_queue->prev = duty_owner;
        duty_owner->next = disk_wait_queue;
        disk_wait_queue = duty_owner;
        disk_wait_queue->prev = NULL;

        return next_duty;

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
