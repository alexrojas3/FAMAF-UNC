# Tareas del laboratorio 1 - MyBash.

## 📌 PARTE 1 : Modulo command.c (scommand & pipeline (TAD's))

### 👤 Sebastian - Creación y destrucción
- [x] `scommand_new` – Crear nueva estructura scommand
- [x] `scommand_destroy` – Liberar memoria de scommand
- [x] `pipeline_new` – Crear nueva estructura pipeline
- [x] `pipeline_destroy` – Liberar memoria de pipeline
- [x] `pipeline_set_wait` – Configurar espera en pipeline
**Notas / Observaciones:**  
_(Aquí pueden registrar problemas, decisiones de implementación o avances)_
- Se toma libreria GLib para facilitar manejo de listas.
- Se agrega stub en las demas funciones para poder correr test. 

### 👤 Kiefer - Operaciones de inserción/eliminación
- [x] `scommand_push_back` – Agregar argumento al final de scommand
- [x] `scommand_pop_front` – Eliminar primer argumento de scommand
- [x] `pipeline_push_back` – Agregar comando al final del pipeline
- [x] `pipeline_pop_front` – Eliminar primer comando del pipeline
**Notas / Observaciones:**  
_(Aquí pueden registrar problemas, decisiones de implementación o avances)_

### 👤 Sergio - Consultas simples
- [x] `scommand_is_empty` – Verificar si scommand está vacío
- [x] `scommand_length` – Obtener cantidad de argumentos
- [x] `scommand_front` – Obtener primer argumento
- [x] `pipeline_is_empty` – Verificar si pipeline está vacío
- [x] `pipeline_length` – Obtener cantidad de comandos
- [x] `pipeline_front` – Obtener primer comando
**Notas / Observaciones:**  
_(Aquí pueden registrar problemas, decisiones de implementación o avances)_

### 👤 Alexander - Redirecciones y conversión a string
- [x] `scommand_set_redir_in` – Establecer redirección de entrada
- [x] `scommand_set_redir_out` – Establecer redirección de salida
- [x] `scommand_get_redir_in` – Obtener redirección de entrada
- [x] `scommand_get_redir_out` – Obtener redirección de salida
- [x] `scommand_to_string` – Convertir scommand a string
- [x] `pipeline_get_wait` – Obtener estado de wait
- [x] `pipeline_to_string` – Convertir pipeline a string
**Notas / Observaciones:**  
_(Aquí pueden registrar problemas, decisiones de implementación o avances)_

<br>

## 📌 PARTE 2 : Modulos  mybash.c - parsing.c - strextra.c - execute.c - builtin.c

### 👤 Sebastian – Parsing + Strextra (parsing.c, strextra.c)
- [x] Implementar parse_pipeline()
- [x] Implementar parse_scommand()
- [x] Manejo de operadores <, >, |, &
- [x] Validación de entradas inválidas (devolver NULL)
- [x] Implementar función strmerge() en strextra.c
- [x] Probar con make test-parsing

*Dependencias:* genera los pipeline que usarán Execute y Main.  


### 👤 Kiefer – Execute (Redirecciones + Fore/Back) (execute.c)
- [x] Implementar ejecución de un solo comando simple con execvp
- [x] Manejo de redirecciones (<, >) con open(), dup2(), close()
- [x] Manejo de *foreground* y *background* (&)
- [x] Adaptar scommand a argv[] para execvp
- [x] Probar con make test y ejemplos simples (ls > out, sleep 3 &, etc.)

*Dependencias:* Parsing debe estar listo; Builtins se integran luego.  


### 👤 Sergio – Execute (Pipes + Zombies + Builtins) (execute.c)
- [x] Implementar ejecución de *pipelines* (cmd1 | cmd2 | ...)
- [x] Manejo de file descriptors (pipe(), dup2(), close())
- [x] Evitar procesos zombies (SIGCHLD + waitpid no bloqueante)
- [x] Integrar ejecución de *builtins dentro de pipelines*
- [x] Probar con make test y ejemplos con pipes (ls | wc -l, etc.)

*Dependencias:* requiere Parsing y Builtins.  


### 👤 Alexander – Builtins + Main + Documentación (builtin.c, mybash.c, README.md)
- [x] Implementar builtins:
  - cd (usar chdir())
  - help (mostrar ayuda del shell)
  - exit (terminar shell limpiamente)
- [x] Implementar ciclo REPL en mybash.c
  - Crear parser
  - Llamar a parse_pipeline() → execute()
  - Manejar CTRL-D
  - Liberar memoria en cada iteración
- [x] Redactar README.md con:
  - Instrucciones de compilación/ejecución
  - Descripción de archivos principales
  - Organización del equipo
  - Uso de asistentes de IA
- [x] Coordinar integración final y pruebas cruzadas
- [x] Probar con make test y make memtest

*Dependencias:* Parsing, Execute y Builtins deben estar listos para integración.
