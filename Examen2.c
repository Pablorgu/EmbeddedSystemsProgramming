#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>
#include <signal.h>


#define PERIODO_TMP_SEC 1
#define PERIODO_TMP_NSEC 0 
#define PRIORIDAD_A 27

#define PERIODO_B_SEC 4
#define PERIODO_B_NSEC 0
#define PRIORIDAD_B 25

#define PERIODO_C_SEC 4
#define PERIODO_C_NSEC 0
#define PRIORIDAD_C 22

#define PRECIO_ENERGIA 10
#define TMP_CONFORT 22

#define N 20
#define M 25

struct Data{
    pthread_mutex_t mutex;
    float valtemp;
    int aire_cale;
    float gasto;
};

void espera_activa(time_t seg) {
    volatile time_t t = time(0) + seg;
    while(time(0) < t);
}

void addtime(struct timespec* tm, const struct timespec* val)
{
	tm->tv_sec += val->tv_sec;
	tm->tv_nsec += val->tv_nsec;
	if (tm->tv_nsec >= 1000000000L) {
		tm->tv_sec += (tm->tv_nsec / 1000000000L);
		tm->tv_nsec = (tm->tv_nsec % 1000000000L);
	}
}

void *tarea_a(void *arg)
{
    const struct timespec periodo = {PERIODO_TMP_SEC, PERIODO_TMP_NSEC};
    struct timespec next;
    struct Data* data = arg;
    float temprand;

    clock_gettime(CLOCK_MONOTONIC, &next);
    while(1) {
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

        pthread_mutex_lock(&data->mutex);

        temprand = rand() % (M + 1 - N) + N;
        printf("TEMP RANDOM %f\n", temprand);

        data->valtemp = (TMP_CONFORT-temprand) * 2.5;

        pthread_mutex_unlock(&data->mutex);

        addtime(&next, &periodo);
    }
    return NULL;
}

void *tarea_b(void *arg) {
    const struct timespec periodo = {PERIODO_B_SEC, PERIODO_B_NSEC};
    struct timespec next;
    struct Data* data = arg;

    clock_gettime(CLOCK_MONOTONIC, &next);
    while(1) {
        clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME, &next, NULL);

        pthread_mutex_lock(&data->mutex);
        
        if(data->valtemp < 0.0) {
            data->aire_cale = 0;
        }else{
            data->aire_cale = 1;
        }
        data->gasto = fabs(data->valtemp) * PRECIO_ENERGIA;

        pthread_mutex_unlock(&data->mutex);
        addtime(&next, &periodo);
    }
    return NULL;
}

void *tarea_c(void *arg) {
    const struct timespec periodo = {PERIODO_C_SEC, PERIODO_C_NSEC};
    struct timespec next;
    struct Data* data = arg;

    clock_gettime(CLOCK_MONOTONIC, &next);
    while(1) {
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        pthread_mutex_lock(&data->mutex);
        printf("CONTROL - PARAMETRO ENERGETICO: %f \n", data->valtemp);
        printf("CONTROL - ESTADO AIRE: %s \n", data->aire_cale == 0 ? "aire":"calefaccion");
        printf("CONTROL - GASTO ULTIMA ACCION: %f \n", data->gasto);

        pthread_mutex_unlock(&data->mutex);

        addtime(&next, &periodo);
    }
    return NULL;
}

int main(int argc, const char *argv[]) {
    struct Data data;
    pthread_attr_t attr;
    struct sched_param param;
    pthread_t t1, t2, t3;

    mlockall(MCL_CURRENT | MCL_FUTURE);

    data.valtemp = 0;
    data.gasto = 0;
    srand(time(NULL));

    pthread_mutex_init(&data.mutex, NULL);
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    param.sched_priority = PRIORIDAD_A;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t1, &attr, tarea_a, &data);
    param.sched_priority = PRIORIDAD_B;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t2, &attr, tarea_b, &data);
    param.sched_priority = PRIORIDAD_C;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t3, &attr, tarea_c, &data);

    pthread_join(t1,NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    pthread_attr_destroy(&attr);

    pthread_mutex_destroy(&data.mutex);
    return 0;
}
