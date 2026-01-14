#include "big_brother.h"
#include "fat_table.h"
#include "fat_util.h"
#include "fat_volume.h"
#include <stdio.h>
#include <string.h>

int bb_is_log_file_dentry(fat_dir_entry dir_entry) {
    return strncmp(LOG_FILE_BASENAME, (char *)(dir_entry->base_name), 2) == 0 &&
           strncmp(LOG_FILE_EXTENSION, (char *)(dir_entry->extension), 3) == 0;
}

int bb_is_log_filepath(char *filepath) {
    return strncmp(BB_LOG_FILE, filepath, 8) == 0;
}

int bb_is_log_dirpath(char *filepath) {
    return strncmp(BB_DIRNAME, filepath, 15) == 0;
}

/*
para verificar que ese bad cluster contiene a fs.log
crea un dir temporal que apunta al cluster donde estaria
fs.log, lee las dir entries buscando a fs.log
devuelve true si lo esta, falso caso contrario
*/
bool bb_verify_orphan_directory(fat_volume vol, u32 cluster) {
    bool orphan_res = false; // valor base por si no encuentra
    fat_file tmp_dir =
        fat_file_init_orphan_dir(".bb_orphan", vol->table, cluster);
    GList *children_list = fat_file_read_children(tmp_dir);

    // recorremos la lista creada de fat_files
    // idea sacada de fat_fuse_load_directory_children()
    for (GList *l = children_list; l != NULL; l = l->next) {
        fat_dir_entry children_dir_entry = (((fat_file)l->data)->dentry);

        if (bb_is_log_file_dentry(children_dir_entry) == 1) {
            // osea en stdin aparece .bb_orphan/fs.log pero es porq apuntamos
            // al cluster donde si esta fs.log desde otro dir temporal
            printf("fs.log encontrado, esta en (ignorando parent dir): %s\n",
                   ((fat_file)l->data)->filepath);
            orphan_res = true;
        }
    }

    return orphan_res;
}

u32 search_bb_orphan_dir_cluster(fat_volume vol) {
    u32 orphan_dir_cluster =
        BAD_CLUSTER; // porq nunca va a ser 0 ya q arranca de 2

    // off_t start_offset = vol->table->data_start_offset;
    // start_offset = start_offset/fat_table_bytes_per_cluster(vol->table);

    // lo que esta arriba es el cluster 2 empieza en el byte = data_start_offset
    // no que data_start_offset/bytes_per_cluster = cluster inicial de datos
    // antes de data_start_offset tiene guardada otra info como estructuras
    // internas del file system
    u32 start_cluster = vol->root_dir_start_cluster;

    for (u32 curr_cluster = start_cluster; curr_cluster < 10000;
         curr_cluster++) {
        if (fat_table_cluster_is_bad_sector(
                fat_table_get_next_cluster(vol->table, curr_cluster))) {
            // verificamos que tenga a fs.log
            fat_table_print(vol->table, curr_cluster - 5,
                            curr_cluster + 10); // debug
            bool verify_orphan = bb_verify_orphan_directory(vol, curr_cluster);
            printf("verify_orphan: %s\n",
                   verify_orphan ? "true" : "false"); // debug

            if (verify_orphan) { // para frenar antes si lo encuentra
                orphan_dir_cluster = curr_cluster;
                printf("orphan dir cluster: %u\n", orphan_dir_cluster); // debug
                fat_table_print(vol->table, orphan_dir_cluster - 5,
                                orphan_dir_cluster + 10); // debug
                return orphan_dir_cluster;
            } else {
                printf("es bad sector pero no contiene a fs.log \n"); // debug
            }
        }
    }
    return orphan_dir_cluster;
}
