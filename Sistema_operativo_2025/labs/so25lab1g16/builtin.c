#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "command.h"

#include "tests/syscall_mock.h"

// Prototipos de funciones internas
static void cd_internal(scommand cmd);
static void help_internal(scommand cmd);
static void exit_internal(scommand cmd);

// Estructura para tabla de despacho
typedef struct {
  const char *name;
  void (*handler)(scommand);
} builtin_command;

// Tabla de comandos internos 
static const builtin_command cmd_table[] = {
    {"cd", cd_internal},
    {"help", help_internal},
    {"exit", exit_internal},
};

static const unsigned int num_commands =
    sizeof(cmd_table) / sizeof(cmd_table[0]);

// Implementaciones
static void cd_internal(scommand cmd) {
  assert(cmd != NULL);

  // Remover el nombre del comando
  scommand_pop_front(cmd);

  char *path = NULL;
  if (!scommand_is_empty(cmd)) {
    path = scommand_front(cmd);
  } else {
    path = getenv("HOME");
  }

  if (path != NULL && chdir(path) != 0) {
    perror("cd");
  }
}

static void help_internal(scommand cmd) {
  (void)cmd; // no lo usamos pero lo dejamos para que opere de manera correcta la funcion

  printf("MyBash\n\n"

         "Autores :\n"
         "Arndt, Jürgen Kiefer.\n"
         "Huaman Rojas, Alexander J.\n"
         "Sierra Sierra, Sebastian.\n"
         "Venguiarrutti, Sergio.\n\n"

         "Comandos :\n"
         "cd [DIR] : Cambiar el directorio actual.\n"
         "help : Ver listas de comandos.\n"
         "exit : Cierra mybash.\n\n");
}

static void exit_internal(scommand cmd) {
  (void)cmd;
  exit(EXIT_SUCCESS);
}

bool builtin_is_internal(scommand cmd) {
  assert(cmd != NULL);
  char *name = scommand_front(cmd);

  for (unsigned int i = 0; i < num_commands; i++) {
    if (strcmp(name, cmd_table[i].name) == 0) {
      return true;
    }
  }
  return false;
}

bool builtin_alone(pipeline p) {
  assert(p != NULL);

  bool res = pipeline_length(p) == 1 && 
                builtin_is_internal(pipeline_front(p));

  return res;
}

void builtin_run(scommand cmd) {
  assert(builtin_is_internal(cmd));

  char *cmd_name = scommand_front(cmd);

  // buscamos el comando y lo ejecutamos 
  for (unsigned int i = 0; i < num_commands; i++) {
    if (strcmp(cmd_name, cmd_table[i].name) == 0) {
      cmd_table[i].handler(cmd);
      return;
    }
  }
}