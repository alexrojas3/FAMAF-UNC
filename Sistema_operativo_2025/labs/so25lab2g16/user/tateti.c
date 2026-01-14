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

    // Definir IDs de semáforos con nombres descriptivos
    int dir1 = 0;  // Semáforo para sincronizar "Ta" (inicia con 1)
    int dir2 = 1;  // Semáforo para sincronizar "Te" (inicia con 0)  
    int dir3 = 2;  // Semáforo para sincronizar "Ti" (inicia con 0)
    
    if (sem_open(dir1, 1) == 0) {
        printf("ERROR: no se pudo crear el semaforo Ta\n");
        exit(0);
    }

    if (sem_open(dir2, 0) == 0) {
        printf("ERROR: no se pudo crear el semaforo Te\n");
        sem_close(dir1);
        exit(0);
    }

    if (sem_open(dir3, 0) == 0) {
        printf("ERROR: no se pudo crear el semaforo Ti\n");
        sem_close(dir1);
        sem_close(dir2);
        exit(0);
    }

    int pid = fork();

    if (pid < 0) {
        printf("ERROR: al crear el proceso.\n");
        sem_close(dir1);
        sem_close(dir2);
        sem_close(dir3);
        exit(0);
    } else if (pid == 0) {
        // Proceso hijo 1 - imprime "Ta"
        for (int i = 1; i <= n; i++) {
            sem_down(dir1);  // Espera su turno
            printf("Ta\n");
            sem_up(dir2);    // Habilita a "Te"
        }
        exit(1);

    } else {
        int pid2 = fork();

        if (pid2 < 0) {
            printf("ERROR: al crear el proceso 2.\n");
            sem_close(dir1);
            sem_close(dir2);
            sem_close(dir3);
            exit(0);
        } else if (pid2 == 0) {
            // Proceso hijo 2 - imprime "Te"
            for (int i = 1; i <= n; i++) {
                sem_down(dir2);  // Espera que "Ta" termine
                printf("\t Te\n");

                sem_up(dir3);    // Habilita a "Ti"
            }
            exit(1);
    
        } else {
            // Proceso padre - imprime "Ti"
            for (int i = 1; i <= n; i++) {
                sem_down(dir3);  // Espera que "Te" termine
                printf("\t \t Ti\n");
                sem_up(dir1);    // Habilita a "Ta" para la siguiente iteración
            }
            
            // Esperar que terminen los procesos hijos
            wait(0);
            wait(0);
        }
    }

    // Cerrar todos los semaforos
    sem_close(dir1);
    sem_close(dir2);
    sem_close(dir3);

    exit(1);
}
