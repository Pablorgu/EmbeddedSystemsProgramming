/*-------------------------------------------------------------------------*/
/*Tareas periódicas con relojes de tiempo real, prioridades y planificación*/
/*-------------------------------------------------------------------------*/
#include <stdlib.h> //Librería estándar de C
#include <stddef.h> //Librería que incluye definiciones como NULL
#include <assert.h> //Librería que incluye la macro assert
#include <string.h> //Librería que incluye funciones para manipular cadenas de caracteres
#include <stdio.h> //Librería que incluye funciones de entrada y salida estándar

#include <unistd.h> //Librería que incluye funciones para el sistema operativo Unix
#include <time.h> //Librería que incluye funciones para manipular el tiempo
#include <errno.h> //Librería que incluye la variable errno
#include <sched.h> //Librería que incluye funciones para manipular la planificación de tareas
#include <pthread.h> //Librería que incluye funciones para manipular hilos
#include <sys/mman.h> //Librería que incluye funciones para manipular la memoria

#define PERIODO_A_SEC 90 //Periodo de la tarea A en segundos
#define PERIODO_A_NSEC 200000000 //Periodo de la tarea A en nanosegundos
#define PRIORIDAD_A 24 //Prioridad de la tarea A
#define ITER_A 40 //Número de iteraciones de la tarea A
#define INC_A 100 //Incremento de la tarea A

#define PERIODO_B_SEC 100 //Periodo de la tarea B en segundos
#define PERIODO_B_NSEC 300000000 //Periodo de la tarea B en nanosegundos
#define PRIORIDAD_B 26 //Prioridad de la tarea B
#define ITER_B 40 //Número de iteraciones de la tarea B
#define INC_B 1 //Incremento de la tarea B

struct Data{ //Estructura de datos para pasar a las tareas
    pthread_mutex_t mutex; //Mutex para sincronizar la salida
    int cnt; //Contador de la tarea
};
/*-----------------*/
/*
* Nota: los "printf" están con el objetivo de depuración, para que el
* alumno pueda analizar el comportamiento de las tareas.
*/
/*-----------------*/
/*--Soporte--------*/
/*-----------------*/
// #define CHKN(syscall) //Macro para verificar errores en llamadas al sistema
//     do{
//         int err = syscall; //Llamada al sistema
//         if (err != 0){ //Si hay error
//             fprintf(stderr, "%s:%d: SysCall Error: %s\n", __FILE__, __LINE__, strerror(errno)); //Mensaje de error. strerror convierte el código de error en una cadena. errno es una variable global que contiene el código de error. __FILE__ y __LINE__ son macros que contienen el nombre del archivo y el número de línea respectivamente. stderr es el flujo de error estándar. %s es un marcador de posición para una cadena y %d para un entero.
//             exit(EXIT_FAILURE); //Salir con error
//         }
//     } while (0) //Hacer mientras 0. Se usa para que la macro se comporte como una función.
// /*-----------------*/
// #define CHKE(syscall) //Macro para verificar errores en llamadas a funciones de la biblioteca estándar
//     do{
//         int err = syscall; //Llamada a la función
//         if (err != 0){ //Si hay error
//             fprintf(stderr, "%s:%d: Error: %s\n", __FILE__, __LINE__, strerror(err)); //Mensaje de error. strerror convierte el código de error en una cadena. __FILE__ y __LINE__ son macros que contienen el nombre del archivo y el número de línea respectivamente. stderr es el flujo de error estándar. %s es un marcador de posición para una cadena y %d para un entero. err es el código de error.
//             exit(EXIT_FAILURE); //Salir con error
//         }
//     } while (0) //Hacer mientras 0. Se usa para que la macro se comporte como una función.
/*-----------------*/
void espera_activa(time_t seg){ //Función para esperar seg segundos
    volatile time_t t = time(0) + seg;
    while (time(0) < t){ /*Esperar hasta que el tiempo actual sea mayor que t*/}
}
/*-----------------*/
const char *get_time(char *buf){ //Función para obtener la hora actual
    time_t t = time(0); //Obtener el tiempo actual
    char *f = ctime_r(&t, buf); //Convertir el tiempo en una cadena /* buf de longitud minima 26 */
    f[strlen(f)-1] = '\0'; //Eliminar el salto de línea
    return f; //Retornar la cadena
}
/*-----------------*/
void addtime(struct timespec *tm, const struct timespec *val){ //Función para sumar tiempos
    tm->tv_sec += val->tv_sec; //Sumar los segundos
    tm->tv_nsec += val->tv_nsec; //Sumar los nanosegundos
    if (tm->tv_nsec >= 1000000000L){ //Si los nanosegundos son mayores o iguales a 1000000000, es decir, un segundo
        tm->tv_sec += (tm->tv_nsec / 1000000000L); //Sumar los segundos
        tm->tv_nsec = tm->tv_nsec % 1000000000L; //Obtener el residuo de los nanosegundos
    }
}
/*-----------------*/
int clock_nanosleep_intr(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain){ //Función para dormir con interrupciones
    int err; //
    while((err = clock_nanosleep(clock_id, flags, request, remain)) == EINTR){ //Mientras la función devuelva una interrupción
        /*Repetir si ha sido interrumpido*/
    }
    return err; //Retornar el resultado
}
/*-----------------*/
/*--Tareas---------*/
/*-----------------*/
void *tarea_a(void *arg){
    const struct timespec periodo = {PERIODO_A_SEC, PERIODO_A_NSEC}; //Periodo de la tarea A
    struct timespec next; //Siguiente tiempo de ejecución
    char buf[30]; //Buffer para la hora
    struct Data *data = arg; //Datos de la tarea
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int i; //Contador
    int policy; //Política de planificación
    //CHKE(pthread_getschedparam(pthread_self(), &policy, &param)); //Obtener la política de planificación y los parámetros
    pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--" //Obtener la política de planificación

    //CHKN(clock_gettime(CLOCK_MONOTONIC, &next)); //Obtener el tiempo actual
    clock_gettime(CLOCK_MONOTONIC, &next); //Obtener el tiempo actual
    while(1){ //Ciclo infinito
        clock_nanosleep_intr(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL); //Dormir hasta next
        addtime(&next, &periodo); //Sumar el periodo a next
        printf("Tarea A [%s]\n", get_time(buf)); //Mensaje de depuración
        for(i=0; i<ITER_A; i++){
            pthread_mutex_lock(&data->mutex); //Bloquear el mutex
            data->cnt += INC_A; //Incrementar el contador
            printf("Tarea A [%s:%d]: %d\n", pol, param.sched_priority, data->cnt); //Mensaje de depuración
            pthread_mutex_unlock(&data->mutex); //Desbloquear el mutex
            espera_activa(1); //Esperar un segundo
        }
    }
    return NULL; //Retornar NULL
}
/*-----------------*/
void *tarea_b(void *arg){
    const struct timespec periodo = {PERIODO_B_SEC, PERIODO_B_NSEC}; //Periodo de la tarea B
    struct timespec next; //Siguiente tiempo de ejecución
    char buf[30]; //Buffer para la hora
    struct Data *data = arg; //Datos de la tarea
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int i; //Contador
    int policy; //Política de planificación
    //CHKE(pthread_getschedparam(pthread_self(), &policy, &param)); //Obtener la política de planificación y los parámetros
    pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--" //Obtener la política de planificación

    //CHKN(clock_gettime(CLOCK_MONOTONIC, &next)); //Obtener el tiempo actual
    clock_gettime(CLOCK_MONOTONIC, &next); //Obtener el tiempo actual
    while(1){ //Ciclo infinito
        clock_nanosleep_intr(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL); //Dormir hasta next
        addtime(&next, &periodo); //Sumar el periodo a next
        printf("Tarea B [%s]\n", get_time(buf)); //Mensaje de depuración
        for(i=0; i<ITER_B; i++){
            pthread_mutex_lock(&data->mutex); //Bloquear el mutex
            data->cnt += INC_B; //Incrementar el contador
            printf("Tarea B [%s:%d]: %d\n", pol, param.sched_priority, data->cnt); //Mensaje de depuración
            pthread_mutex_unlock(&data->mutex); //Desbloquear el mutex
            espera_activa(1); //Esperar un segundo
        }
    }
    return NULL; //Retornar NULL
}
    /*-------------------------------*/
    /*--Programa Principal*/
    /*-------------------------------*/
void usage(const char *nm){
    fprintf(stderr, "Usage: %s [-h] [-ff] [-rr] [-p1] [-p2]\n", nm); //Mensaje de uso
    exit(EXIT_FAILURE); //Salir con error
}

void get_args(int argc, const char *argv[], int *policy, int *prio1, int *prio2){
    int i; //Contador
    if(argc<2){ //Si no hay argumentos
        usage(argv[0]); //Mostrar el mensaje de uso
    }else{
        for(i=1; i<argc; i++){
            if (strcmp(argv[i], "-h") == 0){ //Si el argumento es -h
                usage(argv[0]); //Mostrar el mensaje de uso
            }else if (strcmp(argv[i], "-ff") == 0){ //Si el argumento es -ff
                *policy = SCHED_FIFO; //Establecer la política de planificación en FIFO
            }else if (strcmp(argv[i], "-rr") == 0){ //Si el argumento es -rr
                *policy = SCHED_RR; //Establecer la política de planificación en RR
            }else if (strcmp(argv[i], "-p1") == 0){ //Si el argumento es -p1
                *prio1 = PRIORIDAD_A; //Establecer la prioridad de la tarea A
                *prio2 = PRIORIDAD_B; //Establecer la prioridad de la tarea B
            }else if (strcmp(argv[i], "-p2") == 0){ //Si el argumento es -p2
                *prio1 = PRIORIDAD_B; //Establecer la prioridad de la tarea B
                *prio2 = PRIORIDAD_A; //Establecer la prioridad de la tarea A
            }else{
                usage(argv[0]); //Mostrar el mensaje de uso
            }
        }
    }
}

int main(int argc, const char *argv[]){
    struct Data shared_data; //Datos compartidos
    pthread_attr_t attr; //Atributos del hilo
    struct sched_param param; //Parámetros de planificación
    const char *pol; //Política de planificación
    int pcy, policy = SCHED_FIFO; //Política de planificación. pcy es una variable auxiliar
    pthread_t t1, t2; //Hilos
    /*
    * La tarea principal debe tener la mayor prioridad, para poder
    * crear todas las tareas necesarias.
    */
    get_args(argc, argv, &policy, &prio1, &prio2); //Obtener los argumentos
    prio0 = (prio1 > prio2 ? prio1 : prio2) + 1; //Establecer la prioridad de la tarea principal

    mlockall(MCL_CURRENT | MCL_FUTURE); //Bloquear la memoria
    pthread_getschedparam(pthread_self(), &pcy, &param); //Obtener la política de planificación y los parámetros
    param.sched_priority = prio0; //Establecer la prioridad
    pthread_setschedparam(pthread_self(), policy, &param); //Establecer la política de planificación y los parámetros
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; //Obtener la política de planificación

    shared_data.cnt = 0; //Inicializar el contador
    pthread_mutex_init(&shared_data.mutex, NULL); //Inicializar el mutex

    pthread_attr_init(&attr); //Inicializar los atributos del hilo
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); //Establecer la herencia de la planificación
    pthread_attr_setschedpolicy(&attr, policy); //Establecer la política de planificación
    param.sched_priority = prio1; //Establecer la prioridad
    pthread_attr_setschedparam(&attr, &param); //Establecer los parámetros de planificación
    pthread_create(&t1, &attr, tarea_a, &shared_data); //Crear la tarea A
    param.sched_priority = prio2; //Establecer la prioridad
    pthread_attr_setschedparam(&attr, &param); //Establecer los parámetros de planificación
    pthread_create(&t2, &attr, tarea_b, &shared_data); //Crear la tarea B
    pthread_attr_destroy(&attr); //Destruir los atributos del hilo

    printf("Tarea Principal [%s:%d]\n", pol, prio0); //Mensaje de depuración

    pthread_join(t1, NULL); //Esperar a la tarea A
    pthread_join(t2, NULL); //Esperar a la tarea B
    pthread_mutex_destroy(&shared_data.mutex); //Destruir el mutex
    return 0; //Retornar 0