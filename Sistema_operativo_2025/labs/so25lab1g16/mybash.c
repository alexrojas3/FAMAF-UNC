#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"
#include "builtin.h"

#define PATH_MAX 4086

static void show_prompt(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL){
        printf("**************************************************************\n");
        printf ("Estas en: %s \nmybash> ", cwd);
    } else {
        perror("Error getcwd");
        printf("mypash> ");
    }
    fflush (stdout);
}

int main(int argc, char *argv[]) {
    pipeline pipe;
    Parser input;
    bool quit = false;

    input = parser_new(stdin);
    while (!quit) {
        // Limpiar procesos zombie antes de mostrar el prompt
        execute_cleanup_zombies();
        
        show_prompt();
        pipe = parse_pipeline(input);

        if (pipe != NULL){
            execute_pipeline(pipe);
            pipeline_destroy(pipe);
            // Asegurar que toda la salida se muestre antes del próximo prompt
            fflush(stdout);
            fflush(stderr);
        }

        /* Hay que salir luego de ejecutar? */
        quit = parser_at_eof(input);
        
    }
    parser_destroy(input); input = NULL;
    return EXIT_SUCCESS;
}
