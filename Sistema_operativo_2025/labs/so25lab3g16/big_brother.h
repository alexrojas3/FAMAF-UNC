#ifndef _BIG_BROTHER_H
#define _BIG_BROTHER_H

#define BB_LOG_FILE "/bb/fs.log"
#define BB_DIRNAME "/bb"
#define LOG_FILE_BASENAME "fs"
#define LOG_FILE_EXTENSION "log"
#define BAD_CLUSTER 0

#include "fat_file.h"

int bb_is_log_file_dentry(fat_dir_entry dir_entry);

int bb_is_log_filepath(char *filepath);

int bb_is_log_dirpath(char *filepath);

// Ejemplos de funciones que tal vez les sean utiles

/*
devuelve el nro de cluster si es que lo encuentra entre los primeros 10k
(capaz modificar eso despues)
devuelve 0 (BAD_CLUSTER) si no encuentra el cluster malo, uso 0 porque total
empieza la busqueda salteandose los primeros clusters que estan reservados
*/
u32 search_bb_orphan_dir_cluster(fat_volume vol);
// int bb_create_new_log_files(fat_volume vol);
// int bb_init_log_dir(fat_volume vol, u32 start_cluster);

#endif