// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)
#include "ppos_data.h"

#ifndef __DISK_MGR__
#define __DISK_MGR__

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

// estrutura que representa um disco no sistema operacional
typedef struct
{
  // completar com os campos necessarios
  int current_position;
  int last_position;
} disk_t ;

typedef struct disk_duty
{
  struct disk_duty* next;
  struct disk_duty* prev;
  int block;
  unsigned char* buffer;
  int operation;
  task_t* owner;

} disk_duty_t;

typedef struct disk_duty_queue
{
  disk_duty_t* head;
  disk_duty_t* tail;

} disk_duty_queue_t;


// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer);

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer);

#endif
