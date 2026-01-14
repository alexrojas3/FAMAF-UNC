#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/syscall.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {

    // Validar que se paso exactamente un parametro
    if (argc != 2) {
        printf("ERROR: debe ingresar exactamente un parametro\n");
        exit(0);
    }

    int n = atoi(argv[1]);

    // Validar que n sea mayor a 0
    if (n <= 0) {
        printf("ERROR: el valor ingresado debe ser mayor que 0\n");
        exit(0);
    }

    // Definir IDs de semáforos con nombres descriptivos (evitar conflicto con tateti)
    int ping = (getpid()+64)*2; // usando el hecho de que hay 64 procesos maximo
    int pong = ping+1;          // chequear que los pid empiecen en 1, no 0

    // Creamos semaforo ping con valor 1 para que realice un PING
    if (sem_open(ping, 1) == 0) {
        printf("ERROR: no se pudo crear el semáforo ping\n");
        exit(0);
    }
    
    // Creamos semaforo pong con valor 0 para esperar que se imprima PING y luego imprimir PONG
    if (sem_open(pong, 0) == 0) {
        printf("ERROR: no se pudo crear el semáforo pong\n");
        sem_close(ping);
        exit(0);
    }

    int pid = fork();

    if (pid < 0) {
        printf("ERROR: al crear el proceso.\n");
        sem_close(ping);
        sem_close(pong);
        exit(0);
    } 
    if (pid == 0) {
        // Proceso hijo - imprime "ping"
        for (int i = 1; i <= n; i++) {
            // Bloqueo semáforo ping para que solo imprima PING una sola vez
            sem_down(ping);
            printf("ping\n");
            // Despertamos semáforo pong 
            sem_up(pong);
        }
        exit(1);

    } else if (pid > 0) {
        // Proceso padre - imprime "pong"
        for (int i = 1; i <= n; i++) {
            // Bloqueamos semáforo pong para que solo imprima PONG una sola vez
            sem_down(pong);
            printf("\t pong\n");
            // Despertamos semáforo ping
            sem_up(ping);
        }
        // Esperar que termine el proceso hijo
        wait(0);
    }

    // Cerrar todos los semáforos
    sem_close(ping);
    sem_close(pong);
}
