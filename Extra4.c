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
#include <signal.h> //nueva

#define PERIODO_A_SEC 90 
#define PERIODO_A_NSEC 200000000 
#define PRIORIDAD_A 24
#define ITER_A 40
#define INC_A 100
#define PERIODO_B_SEC 100
#define PERIODO_B_NSEC 300000000
#define PRIORIDAD_B 26
#define ITER_B 40
#define INC_B 1

struct Data {
    pthread_mutex_t mutex; 
    int cnt; 
};

#define CHKN(syscall) \
    do { \
        int err = syscall; \
        if (err != 0) { \
            fprintf(stderr, "%s: %d: SysCall Error: %s\n", \
                    _FILE, __LINE_, strerror(errno)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define CHKE(syscall) \ 
    do { \
        int err = syscall; \
        if (err != 0) { \
            fprintf(stderr, "%s: %d: SysCall Error: %s\n", \
                    _FILE, __LINE_, strerror(err)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

void espera_activa(time_t seg) { //espera x segundos dados
    volatile time_t t = time(0) + seg;
    while (time(0) < t) { /* esperar activamente */ }
}

const char *get_time(char *buf) { // devuelve el tiempo en cadena de caracteres
    time_t t = time(0);
    char f = ctime_r(&t, buf); / buf de longitud mínima 26 */
    f[strlen(f) - 1] = '\0';
    return f;
}

int sigwait_intr(const sigset_t *sigset, int *signum) { // espera a una señal dada y la asigna a signum
    int err = sigwait(sigset, signum);
    return err;
}

void *tarea_a(void *arg) { // igual a la tarea b
    const struct timespec periodo = {PERIODO_A_SEC, PERIODO_A_NSEC};
    timer_t timerid; //temporizador
    struct sigevent sgev; //notificacion o evento que relaciono con el temporizador (cuando el temporizador espera esa señal es la que se lanza)
    struct itimerspec its; //estructura que permite configurar el temporizador(quiero que te ejecutes cuando pasen x segundos con un periodo de x segundos)
    sigset_t sigset; // mascara de la señal
    char buf[30]; // muestra la fecha en cadena de caracteres
    struct Data *data = arg; // Data que queremos modificar
    struct sched_param param; // establece prioridad
    const char *pol; // política de planificacion
    int i, policy, signum; 

    CHKE(pthread_getschedparam(pthread_self(), &policy, &param));
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--";
    sgev.sigev_notify = SIGEV_SIGNAL; //tipo de notificacion
    sgev.sigev_signo = SIGUSR1; // numero de la señal
    sgev.sigev_value.sival_ptr = &timerid; // temporizador al que asocio la señal
    CHKN(timer_create(CLOCK_MONOTONIC, &sgev, &timerid)); // creo temporizador asociando un evento CLOCK_MONOTONIC (NO PARA NUNCA)
    its.it_interval = periodo; // its es para configurar el temporizador y el periodo es para periodizar la señal
    its.it_value.tv_sec = 0; // creo el temporizador
    its.it_value.tv_nsec = 1; // configuro el temporizador
    CHKN(timer_settime(timerid, 0, &its, NULL)); // activo el temporizador, segundo argumento (0 o absoluto), 4to argumento siempre NULL
    CHKN(sigemptyset(&sigset)); // Máscara de señales vacia inicialmente
    CHKN(sigaddset(&sigset, SIGUSR1)); // añado la señal SIGUSR1

    while (1) {
        CHKE(sigwait_intr(&sigset, &signum)); // sigset tiene que ser de tipo SIGUSR1
        printf("Tarea A [%s]\n", get_time(buf)); // imprime la fecha
        for (i = 0; i < ITER_A; ++i) {
            CHKE(pthread_mutex_lock(&data->mutex)); // bloqueo mutex
            data->cnt += INC_A; // incremento en el inc_A
            printf("Tarea A [%s: %d]: %d\n", pol, param.sched_priority, data->cnt); // IMPRIME INFO
            CHKE(pthread_mutex_unlock(&data->mutex)); // DESBLOQUEO EL MUTEX
            espera_activa(1); // espera el tiempo necesario
        }
    }
    CHKN(timer_delete(timerid)); // LIBERO RECURSOS
    return NULL;
}

void *tarea_b(void *arg) { // IGUAL QUE LA A PERO CON DATOS DE LA B
    const struct timespec periodo = {PERIODO_B_SEC, PERIODO_B_NSEC};
    timer_t timerid;
    struct sigevent sgev;
    struct itimerspec its;
    sigset_t sigset;
    char buf[30];
    struct Data *data = arg;
    struct sched_param param;
    const char *pol;
    int i, policy, signum;

    CHKE(pthread_getschedparam(pthread_self(), &policy, &param));
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--";
    sgev.sigev_notify = SIGEV_SIGNAL; //SUPERIMPORTANTE EVENTO DE TIPO SEÑAL
    sgev.sigev_signo = SIGUSR2;
    sgev.sigev_value.sival_ptr = &timerid;
    CHKN(timer_create(CLOCK_MONOTONIC, &sgev, &timerid));
    its.it_interval = periodo; // configuracion del temporizador
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;
    CHKN(timer_settime(timerid, 0, &its, NULL));
    CHKN(sigemptyset(&sigset));
    CHKN(sigaddset(&sigset, SIGUSR2));

    while (1) {
        CHKE(sigwait_intr(&sigset, &signum));
        printf("Tarea B [%s]\n", get_time(buf));
        for (i = 0; i < ITER_B; ++i) {
            CHKE(pthread_mutex_lock(&data->mutex));
            data->cnt += INC_B;
            printf("Tarea B [%s: %d]: %d\n", pol, param.sched_priority, data->cnt);
            CHKE(pthread_mutex_unlock(&data->mutex));
            espera_activa(1);
        }
    }
    CHKN(timer_delete(timerid));
    return NULL;
}

void usage(const char *nm) {
    fprintf(stderr, "usage: %s [-h] [-ff] [-rr] [-p1] [-p2]\n", nm);
    exit(EXIT_FAILURE);
}

void get_args(int argc, const char *argv[], int *policy, int *prio1, int *prio2) {
    int i;
    if (argc < 2) {
        usage(argv[0]);
    } else {
        for (i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "-h") == 0) {
                usage(argv[0]);
            } else if (strcmp(argv[i], "-ff") == 0) {
                *policy = SCHED_FIFO;
            } else if (strcmp(argv[i], "-rr") == 0) {
                *policy = SCHED_RR;
            } else if (strcmp(argv[i], "-p1") == 0) {
                *prio1 = PRIORIDAD_A;
                *prio2 = PRIORIDAD_B;
            } else if (strcmp(argv[i], "-p2") == 0) {
                *prio1 = PRIORIDAD_B;
                *prio2 = PRIORIDAD_A;
            } else {
                usage(argv[0]);
            }
        }
    }
}

int main(int argc, const char *argv[]) {
    sigset_t sigset; //Declara la señal
    struct Data shared_data;
    pthread_attr_t attr;
    struct sched_param param;
    const char *pol;
    int pcy, policy = SCHED_FIFO;
    int prio0 = 1, prio1 = 1, prio2 = 1;
    pthread_t t1, t2;

    get_args(argc, argv, &policy, &prio1, &prio2);
    prio0 = (prio1 > prio2 ? prio1 : prio2) + 1; (SIEMPRE SUELE IR IGUAL)
    CHKN(mlockall(MCL_CURRENT | MCL_FUTURE));
    CHKE(pthread_getschedparam(pthread_self(), &pcy, &param));
    param.sched_priority = prio0; // asigno prioridad
    CHKE(pthread_setschedparam(pthread_self(), policy, &param));
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--"; 
 //NEW SIGNALS
    CHKN(sigemptyset(&sigset)); // Vacia el conjunto de señales para evitar conflicto
    CHKN(sigaddset(&sigset, SIGUSR1)); //Agrega la señal "SIGUSR1" al conjunto de señales
    CHKN(sigaddset(&sigset, SIGUSR2)); //Agrega la señal "SIGUSR2" al conjunto de señales
    CHKE(pthread_sigmask(SIG_BLOCK, &sigset, NULL)); // Bloqueo todas las señales y decirle al padre las va a usar y todos sus hijos (SIEMPRE SE PONE)
//FIN
    shared_data.cnt = 0;
    CHKE(pthread_mutex_init(&shared_data.mutex, NULL));
    CHKE(pthread_attr_init(&attr));
    CHKE(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));
    CHKE(pthread_attr_setschedpolicy(&attr, policy));


    // CREO AMBOS HILOS CON SUS RESPECTIVAS PRIORIDADES
    param.sched_priority = prio1;
    CHKE(pthread_attr_setschedparam(&attr, &param));
    CHKE(pthread_create(&t1, &attr, tarea_a, &shared_data));

    param.sched_priority = prio2;
    CHKE(pthread_attr_setschedparam(&attr, &param));
    CHKE(pthread_create(&t2, &attr, tarea_b, &shared_data));

    CHKE(pthread_attr_destroy(&attr));

    printf("Tarea principal [%s: %d]\n", pol, prio0);
    CHKE(pthread_join(t1, NULL)); //ESPERA A LOS HIJOS
    CHKE(pthread_join(t2, NULL));
    CHKE(pthread_mutex_destroy(&shared_data.mutex)); //LIBERA MUTEX
    return 0;
}

// sudo gcc E4.c -o E4 -lrt -pthread