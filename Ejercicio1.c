/*---------------------------------*/
/* Tareas con prioridades y planificador*/
/*---------------------------------*/
#include <stdlib.h> // Para usar exit
#include <stddef.h> // Para usar NULL
#include <assert.h> // Para usar assert
#include <string.h> // Para usar strcmp y strcpy
#include <stdio.h> // Para usar printf

#include <unistd.h> // Para usar fork
#include <time.h> // Para usar time
#include <errno.h> // Para usar errno
#include <sched.h> // Para usar sched_fifo, sched_rr, sched_param
#include <pthread.h> // Para usar pthread_create, pthread_join, pthread_exit
#include <sys/mman.h> // Para usar mmap, munmap. Da error porque solo funciona en sistemas operativos de tiempo real

#define PRIORIDAD_A 24 // Prioridad de la tarea A. Debe ser mayor que la prioridad de la tarea B
#define ITER_A 40 // Número de iteraciones de la tarea A
#define INC_A 100 // Incremento de la tarea A

#define PRIORIDAD_B 26 // Prioridad de la tarea B. Debe ser menor que la prioridad de la tarea A
#define ITER_B 40 // Número de iteraciones de la tarea B
#define INC_B 1 // Incremento de la tarea B

struct Data{ // Estructura de datos para pasar a las tareas
    pthread_mutex_t mutex; // Mutex para sincronizar la salida
    int cnt; // Contador de la tarea
};
/*---------------------------------*/
/*
* Nota: los "printf" están con el objetivo de depuración, para que el
* alumno pueda analizar el comportamiento de las tareas.
*/
/*---------------------------------*/
/*--Soporte------------------------*/
/*---------------------------------*/
// #define CHKN(syscall) // Macro para verificar errores en llamadas al sistema
//     do{
//         int err = syscall; // Llamada al sistema
//         if (err != 0){ //Si hay error
//             fprintf(stderr, "%s:%d: SysCall Error: %s\n", __FILE__, __LINE__, strerror(errno)); // Mensaje de error. strerror convierte el código de error en una cadena. errno es una variable global que contiene el código de error. __FILE__ y __LINE__ son macros que contienen el nombre del archivo y el número de línea respectivamente. stderr es el flujo de error estándar. %s es un marcador de posición para una cadena y %d para un entero.
//             exit(EXIT_FAILURE); // Salir con error
//         }
//     } while (0) // Hacer mientras 0. Se usa para que la macro se comporte como una función.
/*---------------------------------*/
// #define CHKE(syscall) // Macro para verificar errores en llamadas a funciones de la biblioteca estándar
//     do{
//         int err = syscall; // Llamada a la función
//         if (err != 0){ // Si hay error
//             fprintf(stderr, "%s:%d: Error: %s\n", __FILE__, __LINE__, strerror(err)); // Mensaje de error. strerror convierte el código de error en una cadena. __FILE__ y __LINE__ son macros que contienen el nombre del archivo y el número de línea respectivamente. stderr es el flujo de error estándar. %s es un marcador de posición para una cadena y %d para un entero. err es el código de error.
//             exit(EXIT_FAILURE); // Salir con error
//         }
//     } while (0) // Hacer mientras 0. Se usa para que la macro se comporte como una función.
/*---------------------------------*/
void espera_activa(time_t seg){ // Función para esperar seg segundos
    volatile time_t t = time(0) + seg; // Obtener el tiempo actual y sumarle seg
    while (time(0) < t){ /*Esperar hasta que el tiempo actual sea mayor que t*/}
}
/*---------------------------------*/
/*--Tareas-------------------------*/
/*---------------------------------*/
void *tarea_a(void *arg){ //
    struct Data *data = arg; // Obtener la estructura de datos
    struct sched_param param; // Estructura para la política de planificación. sched_param es una estructura que contiene un entero que representa la prioridad de la tarea.
    const char *pol; // Política de planificación. Es un puntero a una cadena de caracteres.
    int i; // Contador. Es un entero.
    int policy; // Política de planificación. Es un entero.
    //CHKE(pthread_getschedparam(pthread_self(), &policy, &param)); // Obtener la política de planificación y los parámetros de la tarea
    pthread_getschedparam(pthread_self(), &policy, &param); // Obtener la política de planificación y los parámetros de la tarea
    pol = (policy == SCHED_FIFO) ? "FF": (policy == SCHED_RR) ? "RR": "--"; // Asignar la política de planificación

    for(i=0; i<ITER_A; i++){ //Para i desde 0 hasta ITER_A (40)
        //CHKE(pthread_mutex_lock(&data->mutex)); //Bloquear el mutex
        pthread_mutex_lock(&data->mutex); //Bloquear el mutex
        data->cnt += INC_A; //Incrementar el contador
        printf("Tarea A [%s:%d]: %d\n", pol, param.sched_priority, data->cnt); //Imprimir la prioridad y el contador
        //CHKE(pthread_mutex_unlock(&data->mutex)); //Desbloquear el mutex
        pthread_mutex_unlock(&data->mutex); //Desbloquear el mutex
        espera_activa(1); //Esperar 1 segundo
    }
    return NULL; //Retornar NULL
}

void *tarea_b(void *arg){ //Función para la tarea B
    struct Data *data = arg; //Obtener la estructura de datos
    struct sched_param param; //Estructura para la política de planificación
    const char *pol; //Política de planificación
    int i; //Contador
    int policy; //Política de planificación
    //CHKE(pthread_getschedparam(pthread_self(), &policy, &param)); //Obtener la política de planificación y los parámetros de la tarea
    pthread_getschedparam(pthread_self(), &policy, &param); //Obtener la política de planificación y los parámetros de la tarea
    pol = (policy == SCHED_FIFO) ? "FF": (policy == SCHED_RR) ? "RR": "--"; //Asignar la política de planificación

    for(i=0; i<ITER_B; i++){ //Para i desde 0 hasta ITER_B (40)
        //CHKE(pthread_mutex_lock(&data->mutex)); //Bloquear el mutex
        pthread_mutex_lock(&data->mutex); //Bloquear el mutex
        data->cnt += INC_B; //Incrementar el contador
        printf("Tarea B [%s:%d]: %d\n", pol, param.sched_priority, data->cnt); //Imprimir la prioridad y el contador
        //CHKE(pthread_mutex_unlock(&data->mutex)); //Desbloquear el mutex
        pthread_mutex_unlock(&data->mutex); //Desbloquear el mutex
        espera_activa(1); //Esperar 1 segundo
    }
    return NULL; //Retornar NULL
}
/*---------------------------------*/
/*--Principal----------------------*/
/*---------------------------------*/
void usage(const char *nm){ //
    fprintf(stderr, "usage: %s [-h] [-ff] [-rr] [-p1] [-p2]\n", nm); //Imprimir mensaje de uso en el flujo de error estándar. %s es un marcador de posición para una cadena. nm significa nombre.
    exit(EXIT_FAILURE); //Salir con error
}

void get_args(int argc, const char *argv[], int *policy, int *prio1, int *prio2){
    int i; //Contador
    if (argc < 2){ //Si el número de argumentos es menor que 2
        usage(argv[0]); //Imprimir mensaje de uso
    }else{
        for(i=1; i<argc; i++){
            if(strcmp(argv[i], "-h") == 0){ //Si el argumento es "-h". -h es para mostrar la ayuda
                usage(argv[0]); //Imprimir mensaje de uso
            }else if(strcmp(argv[i], "-ff") == 0){ //Si el argumento es "-ff". -ff es para la política de planificación FIFO
                *policy = SCHED_FIFO; //Asignar la política de planificación FIFO
            }else if(strcmp(argv[i], "-rr") == 0){ //Si el argumento es "-rr". -rr es para la política de planificación RR
                *policy = SCHED_RR; //Asignar la política de planificación RR
            }else if(strcmp(argv[i], "-p1") == 0){ //Si el argumento es "-p1". -p1 es para la prioridad de la tarea A
                *prio1 = PRIORIDAD_A; //Asignar la prioridad de la tarea A
                *prio2 = PRIORIDAD_B; //Asignar la prioridad de la tarea B
            }else if(strcmp(argv[i], "-p2") == 0){ //Si el argumento es "-p2". -p2 es para la prioridad de la tarea B
                *prio1 = PRIORIDAD_B; //Asignar la prioridad de la tarea B
                *prio2 = PRIORIDAD_A; //Asignar la prioridad de la tarea A
            }else{
                usage(argv[0]); //Imprimir mensaje de uso
            }
        }
    }

int main(int argc, const char *argv[]){
    struct Data shared_data; //Datos compartidos
    pthread_attr_t attr; //Atributos de la tarea
    struct sched_param param; //Parámetros de la política de planificación
    const char *pol; //Política de planificación
    int pcy, policy = SCHED_FIFO; //Política de planificación
    int prio0=1, prio1=1, prio2=1; //Prioridades de las tareas
    pthread_t t1, t2; //Hilos
    /*
    * La tarea principal debe tener la mayor prioridad, para poder
    * crear todas las tareas necesarias.
    */
    get_args(argc, argv, &policy, &prio1, &prio2); //Obtener los argumentos
    prio0 = (prio1 > prio2 ? prio1: prio2) + 1; //Asignar la mayor prioridad más 1 a la tarea principal

   //CHKN(mlockall(MCL_CURRENT | MCL_FUTURE)); //Bloquear todas las páginas de memoria en la memoria física
    mlockall(MCL_CURRENT | MCL_FUTURE); //Bloquear todas las páginas de memoria en la memoria física
    //CHKE(pthread_getschedparam(pthread_self(), &pcy, &param)); //Obtener la política de planificación y los parámetros de la tarea principal
    pthread_getschedparam(pthread_self(), &pcy, &param); //Obtener la política de planificación y los parámetros de la tarea principal
    param.sched_priority = prio0; //Asignar la prioridad de la tarea principal
    //CHKE(pthread_setschedparam(pthread_self(), policy, &param)); //Establecer la política de planificación y los parámetros de la tarea principal
    pthread_setschedparam(pthread_self(), policy, &param); //Establecer la política de planificación y los parámetros de la tarea principal
    pol = (policy == SCHED_FIFO) ? "FF": (policy == SCHED_RR) ? "RR": "--"; //Asignar la política de planificación

    shared_data.cnt = 0; //Inicializar el contador
    //CHKE(pthread_mutex_init(&shared_data.mutex, NULL)); //Inicializar el mutex
    pthread_mutex_init(&shared_data.mutex, NULL); //Inicializar el mutex

    //CHKE(pthread_attr_init(&attr)); //Inicializar los atributos de la tarea
    pthread_attr_init(&attr); //Inicializar los atributos de la tarea
    //CHKE(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)); //Establecer la herencia de la política de planificación
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); //Establecer la herencia de la política de planificación
    //CHKE(pthread_attr_setschedpolicy(&attr, policy)); //Establecer la política de planificación
    pthread_attr_setschedpolicy(&attr, policy); //Establecer la política de planificación
    param.sched_priority = prio1; //Asignar la prioridad de la tarea A
    //CHKE(pthread_attr_setschedparam(&attr, &param)); //Establecer los parámetros de la política de planificación
    pthread_attr_setschedparam(&attr, &param); //Establecer los parámetros de la política de planificación
    //CHKE(pthread_create(&t1, &attr, tarea_a, &shared_data)); //Crear la tarea A
    pthread_create(&t1, &attr, tarea_a, &shared_data); //Crear la tarea A
    param.sched_priority = prio2; //Asignar la prioridad de la tarea B
    //CHKE(pthread_attr_setschedparam(&attr, &param)); //Establecer los parámetros de la política de planificación
    pthread_attr_setschedparam(&attr, &param); //Establecer los parámetros de la política de planificación
    //CHKE(pthread_create(&t2, &attr, tarea_b, &shared_data)); //Crear la tarea B
    pthread_create(&t2, &attr, tarea_b, &shared_data); //Crear la tarea B
    //CHKE(pthread_attr_destroy(&attr)); //Destruir los atributos de la tarea
    pthread_attr_destroy(&attr); //Destruir los atributos de la tarea

    printf("Tarea principal [%s:%d]\n", pol, prio0); //Imprimir la prioridad y el contador
    printf("Tarea A configurada con prioridad %d\n", prio1);
    printf("Tarea B configurada con prioridad %d\n", prio2);

    //CHKE(pthread_join(t1, NULL)); //Esperar a que la tarea A termine
    pthread_join(t1, NULL); //Esperar a que la tarea A termine
    //CHKE(pthread_join(t2, NULL)); //Esperar a que la tarea B termine
    pthread_join(t2, NULL); //Esperar a que la tarea B termine
    //CHKE(pthread_mutex_destroy(&shared_data.mutex)); //Destruir el mutex
    pthread_mutex_destroy(&shared_data.mutex); //Destruir el mutex
    return 0; //Retornar 0

}

    