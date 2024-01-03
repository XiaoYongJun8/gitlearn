#ifndef __SHM_UTIL_H
#define __SHM_UTIL_H

#include <stdio.h>   
#include <sys/types.h>   
#include <sys/ipc.h>   
#include <sys/shm.h> 

typedef struct {
    float current;          //电流均值
    float voltage;          //电压均值
    float gasspeed;         //气体流速均值
    float wfsspeed;         //送丝速度
    float temperature;      //温度
    float humidity;         //湿度
    float windspeed;        //风速
}shm_sensor_t;


#define SHM_SENSOR_KEY  0x1111   
#define SEM_SENSOR_KEY  0x2222

int get_shm(key_t key, int size);
int destroy_shm(int shmid);

int get_sem(key_t key);
int sem_p(int semid);
int sem_v(int semid);
int destroy_sem(int semid);


#endif


