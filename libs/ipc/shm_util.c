#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "shm_util.h"

/*********************************************************************
* @fn	      sem_p
* @brief      信号量p操作，获取资源
* @param      semid[in]: 信号量id
* @return     -1：失败, 0: 成功
* @history:
**********************************************************************/
int sem_p(int semid)
{
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = -1;
    sops.sem_flg = SEM_UNDO;
    if(semop(semid, &sops, 1) == -1) {
        return -1;
    }
    return 0;
}

/*********************************************************************
* @fn	      sem_v
* @brief      信号量v操作，释放资源
* @param      semid[in]: 信号量id
* @return     -1：失败, 0: 成功
* @history:
**********************************************************************/
int sem_v(int semid)
{
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = 1;
    sops.sem_flg = SEM_UNDO;
    if(semop(semid, &sops, 1) == -1) {
        return -1;
    }
    return 0;
}


/*********************************************************************
* @fn	      get_sem
* @brief      创建或者获取信号量
* @param      key[in]: 共享内存key
* @param      size[in]: 共享内存大小
* @return     -1: 失败，其他: semid
* @history:
**********************************************************************/
int get_sem(key_t key)
{
    int semid;

    semid = semget(key, 1, 0666 | IPC_CREAT); // 创建1个信号量，保护传感器数据共享内存
    if(semid == -1) {
        return -1;
    }

    if(semctl(semid, 0, SETVAL, 1) == -1) {
        return -1;
    }

    return semid;
}


/*********************************************************************
* @fn	      destroy_sem
* @brief      删除信号量
* @param      semid[in]: 信号量id
* @return     -1：失败, 0: 成功
* @history:
**********************************************************************/
int destroy_sem(int semid)
{
    if(semctl(semid, 0, IPC_RMID, 0) == -1) {
        return -1;
    }

    return 0;
}


/*********************************************************************
* @fn	      get_shm
* @brief      创建或者获取共享内存
* @param      key[in]: 共享内存key
* @param      size[in]: 共享内存大小
* @return     -1: 失败，其他: shmid
* @history:
**********************************************************************/
int get_shm(key_t key, int size)
{
    return shmget(key, size, 0666 | IPC_CREAT);
}

/*********************************************************************
* @fn	      destroy_shm
* @brief      释放共享内存
* @param      shmid[in]: 共享内存id
* @return     -1: 失败，0: 成功
* @history:
**********************************************************************/
int destroy_shm(int shmid)
{
    return shmctl(shmid, IPC_RMID, 0);
}
