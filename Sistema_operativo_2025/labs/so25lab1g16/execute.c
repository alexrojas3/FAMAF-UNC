#include "execute.h"
#include "builtin.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       /*fork(), dup2(), sleep()*/
#include <sys/wait.h>     /*wait()*/
#include <sys/types.h>    /*pid_t*/
#include <fcntl.h>        /*open()*/
#include <errno.h>        /*errno*/

#include "tests/syscall_mock.h"

// Lista para manejar PIDs de procesos background
#define MAX_BACKGROUND_PROCS 100
static pid_t background_pids[MAX_BACKGROUND_PROCS];
static int background_count = 0;

// Agregar PID a la lista de procesos background
static void add_background_pid(pid_t pid) {
    if (background_count < MAX_BACKGROUND_PROCS) {
        background_pids[background_count] = pid;
        background_count++;
    } else {
        fprintf(stderr, "Warning: máximo número de procesos background alcanzado\n");
    }
}

// Funcion para limpiar procesos zombies background
static void cleanup_zombies(void) {
    pid_t pid;
    int status;
    
    // Limpiar todos los procesos hijo que hayan terminado sin bloquear
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Remover el PID de la lista de background si es que esta
        for (int i = 0; i < background_count; i++) {
            if (background_pids[i] == pid) {
                // Mover todos los elementos hacia la izquierda
                for (int j = i; j < background_count - 1; j++) {
                    background_pids[j] = background_pids[j + 1];
                }
                background_count--;
                break;
            }
        }
    }
}

static void execute_scommand(scommand scmd) {
    assert(scmd != NULL);
    assert(!scommand_is_empty(scmd));  // Asegurar que el comando no este vacío

    if (builtin_is_internal(scmd)) {
        builtin_run(scmd);
        return;
    }

    // Preparacion de argumentos para la ejecucion del comando
    size_t argc = scommand_length(scmd);
    char **argv = calloc(argc + 1, sizeof(char *));
    if (argv == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    
    for (size_t i = 0; i < argc; ++i) {
        char *arg = scommand_front(scmd);
        argv[i] = strdup(arg);
        scommand_pop_front(scmd);
    }
    argv[argc] = NULL;
    
    char *in = scommand_get_redir_in(scmd);
    char *out = scommand_get_redir_out(scmd);
    int fd;

    if (in != NULL) {
        fd = open(in, O_RDONLY, 0);
        if (fd < 0) {
            perror("Error al abrir archivo de entrada");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (out != NULL){
        fd = open(out, O_WRONLY | O_CREAT| O_TRUNC, 0644);
        if(fd < 0) {
            perror("Error al abrir archivo de salida");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    execvp(argv[0], argv);
    perror("Error al ejecutar comando");
    
    for (size_t i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
    argv = NULL;
    exit(EXIT_FAILURE);
}

void execute_pipeline(pipeline apipe) {
    assert(apipe != NULL);

    int n_pipe = pipeline_length(apipe);
    if (n_pipe == 0){
        fprintf(stderr, "Error: pipeline vacío\n");
        return;
    }

    scommand first_cmd = NULL;
    first_cmd = pipeline_front(apipe);
    
    // Validar que el primer comando no sea NULL
    if (first_cmd == NULL) {
        fprintf(stderr, "Error: comando inválido en pipeline\n");
        return;
    }

    if (builtin_is_internal(first_cmd)) {
        builtin_run(first_cmd);
        return;
    }

    // Si es un comando simple (no pipeline), manejarlo de forma especial
    if (n_pipe == 1) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Error en fork");
            return;
        } else if (pid == 0) {
            // Proceso hijo - ejecutar el comando
            execute_scommand(first_cmd);
            exit(EXIT_FAILURE); // No debería llegar aquí
        } else {
            // Proceso padre - esperar al hijo
            bool must_wait = pipeline_get_wait(apipe);
            if (must_wait) {
                int status;
                if (waitpid(pid, &status, 0) == -1) {
                    perror("waitpid");
                }
            } else {
                // Proceso background - guardar PID para limpieza posterior
                add_background_pid(pid);
            }
        }
        return;
    }

    // Pipeline multiple -  manejar con pipes
    int (*pipes)[2] = malloc(sizeof(int[2]) * (n_pipe - 1));
    if (pipes == NULL) {
        perror("Error malloc pipes");
        return;
    }

    /*
    *pipes[0][0] → extremo de lectura del 1er pipe
    *pipes[0][1] → extremo de escritura del 1er pipe
    *pipes[1][0] → extremo de lectura del 2do pipe
    *pipes[1][1] → extremo de escritura del 2do pipe
    */

    for (int i = 0; i < n_pipe -1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("Error pipe");
            free(pipes);
            return;
        }
    }
    
    // Array dinamico para guardar todos los PIDs del pipeline
    pid_t *pids = malloc(sizeof(pid_t) * n_pipe);
    if (pids == NULL) {
        perror("Error malloc pids");
        free(pipes);
        return;
    }
    int pid_count = 0;

    for (int i = 0; i < n_pipe; i++){
        scommand cmd = pipeline_front(apipe);
        
        // Validar comando antes de hacer fork
        if (cmd == NULL || scommand_is_empty(cmd)) {
            fprintf(stderr, "Error: comando vacío en pipeline\n");
            // Limpiar pipes ya creados
            for (int j = 0; j < n_pipe-1; j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            free(pids);
            free(pipes);
            return;
        }

        pid_t pid = fork();
        if(pid < 0) {
            perror("fork failed");
            // Limpiar recursos en caso de error
            for (int j = 0; j < n_pipe-1; j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            free(pids);
            free(pipes);
            return;
        } else if(pid == 0) {
            if(i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            if(i < n_pipe-1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            for (int j = 0; j < n_pipe-1; j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            execute_scommand(cmd);
            exit(EXIT_FAILURE);
        } else {
            // Proceso padre - guardar PID
            pids[pid_count] = pid;
            pid_count++;
        }
        pipeline_pop_front(apipe);
    } 
    for (int j = 0; j < n_pipe-1; j++){
        close(pipes[j][0]);
        close(pipes[j][1]);
    }
    bool must_wait = pipeline_get_wait(apipe);

    if (must_wait) {
        // Procesos foreground - esperar todos los procesos del pipeline
        for (int j = 0; j < pid_count; j++) {
            int status;
            pid_t waited_pid = waitpid(pids[j], &status, 0);
            if (waited_pid == -1) {
                perror("waitpid");
            }
        }
    } else {
        // Procesos background - guardar todos los PIDs para limpieza posterior
        for (int j = 0; j < pid_count; j++) {
            add_background_pid(pids[j]);
        }
    }
    
    free(pids);
    free(pipes);
}

// Funcion publica para limpiar zombies
void execute_cleanup_zombies(void) {
    cleanup_zombies();
}

// Funcion para mostrar procesos background activos 
void execute_show_background_status(void) {
    if (background_count == 0) {
        printf("No hay procesos background activos\n");
    } else {
        printf("Procesos background activos (%d):\n", background_count);
        for (int i = 0; i < background_count; i++) {
            printf("  PID: %d\n", background_pids[i]);
        }
    }
}
