/* Ejecuta comandos simples y pipelines.
 * No toca ningún comando interno.
 */

#ifndef EXECUTE_H
#define EXECUTE_H

#include "command.h"


void execute_pipeline(pipeline apipe);
/*
 * Ejecuta un pipeline, identificando comandos internos, forkeando, y
 *   redirigiendo la entrada y salida. puede modificar `apipe' en el proceso
 *   de ejecución.
 *   apipe: pipeline a ejecutar
 * Requires: apipe!=NULL
 */

void execute_cleanup_zombies(void);
/*
 * Limpia procesos zombie background sin bloquear
 * Debe llamarse periódicamente desde el main loop
 */

void execute_show_background_status(void);
/*
 * Muestra el estado de los procesos background activos
 */

#endif /* EXECUTE_H */
