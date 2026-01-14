/*
 * fat_fuse_ops.c
 *
 * FAT32 filesystem operations for FUSE (Filesystem in Userspace)
 */

#include "fat_fuse_ops.h"

#include "big_brother.h"
#include "fat_file.h"
#include "fat_filename_util.h"
#include "fat_fs_tree.h"
#include "fat_util.h"
#include "fat_volume.h"
#include <alloca.h>
#include <errno.h>
#include <gmodule.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define LOG_MESSAGE_SIZE 100
#define DATE_MESSAGE_SIZE 30

static void now_to_str(char *buf) {
    time_t now = time(NULL);
    struct tm *timeinfo;
    timeinfo = localtime(&now);

    strftime(buf, DATE_MESSAGE_SIZE, "%d-%m-%Y %H:%M", timeinfo);
}

static void fat_fuse_log_activity(char *operation_type, fat_file file) {

    char buf[LOG_MESSAGE_SIZE] = "";
    now_to_str(buf);

    const char *login = NULL;
    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_name) {
        login = pw->pw_name;
    } else {
        login = "vigilante";
    }

    strcat(buf, "\t");
    // strcat(buf, getlogin());
    strcat(buf, login);
    strcat(buf, "\t");
    strcat(buf, file->filepath);
    strcat(buf, "\t");
    strcat(buf, operation_type);
    strcat(buf, "\n");
    // cargo el vol
    fat_volume vol = get_fat_volume();
    // uso el mismo estilo de nodo, file y parent como esta en write y read.
    fat_tree_node vigilante_node =
        fat_tree_node_search(vol->file_tree, "/bb/fs.log");
    fat_file vigilate_file = fat_tree_get_file(vigilante_node);
    fat_file parent = fat_tree_get_parent(vigilante_node);
    // ahora proceso a escribir en el log
    // @sizeBuf guarda el tamaño del buf para pasarlo como parametro para el
    // pwrite.
    // @offset lo usamos como esta en el fat_fuse_write, ademas que es parametro
    // para el pwrite.
    size_t sizeBuf = strlen(buf);
    off_t offset = vigilate_file->dentry->file_size;
    ssize_t vigilanteAnota =
        fat_file_pwrite(vigilate_file, buf, sizeBuf, offset, parent);
    if (vigilanteAnota != sizeBuf) {
        printf("Error al escribir en el log.\n");
    }
}

/* Loads directory children from the volume into in-memory tree.
 * Calls function fat_file_read_children which returns
 * a list of files inside a GList. The children were read from the directory
 * entries in the cluster of the directory. This function iterates over the
 * list of children and adds them to the file tree.
 * This operation should be performed only once per directory, the first time
 * readdir is called.
 */
static void fat_fuse_load_directory_children(fat_volume vol,
                                             fat_tree_node dir_node) {
    fat_file dir = fat_tree_get_file(dir_node);
    GList *children_list = fat_file_read_children(dir);
    // Add children to tree. TODO handle duplicates
    for (GList *l = children_list; l != NULL; l = l->next) {
        vol->file_tree =
            fat_tree_insert(vol->file_tree, dir_node, (fat_file)l->data);
    }
}

static fat_volume vigilante(fat_volume vol, fat_tree_node root_dir_node) {
    u32 bad_cluster = search_bb_orphan_dir_cluster(vol);
    if (bad_cluster == BAD_CLUSTER) { // usamos un resultado base por si no
                                      // encuentra el cluster malo
        printf("El vigilante no existe, vamos a crearlo...\n");

        // obtenemos cluster libre y lo marcamos como bad sector
        u32 free_cluster = fat_table_get_next_free_cluster(vol->table);
        int res_set = fat_table_set_next_cluster(vol->table, free_cluster,
                                                 FAT_CLUSTER_BAD_SECTOR);
        if (res_set == -1) {
            printf("no se seteo\n");
        }

        // creamos dir bb en el cluster marcado y lo insertamos como children
        // del root
        fat_file bb_dir =
            fat_file_init_orphan_dir("/bb", vol->table, free_cluster);
        vol->file_tree = fat_tree_insert(vol->file_tree, root_dir_node, bb_dir);

        // escribimos en la FAT table los direntry del children recien creado
        fat_file root = fat_tree_get_file(root_dir_node);
        fat_file_dentry_add_child(root, bb_dir);

        // usamos false porque estamos creando un archivo y no un directorio.
        // idea sacada del mknod comparando con el mkdir.
        // creamos archivo fs.log en /bb/
        fat_file vigilante =
            fat_file_init(vol->table, false, strdup("/bb/fs.log"));
        // obtenemos nodo del dir /bb para poder insertar el children vigilante
        fat_tree_node bb_log_node = fat_tree_node_search(vol->file_tree, "/bb");
        // insertamos el children en el arbol como children del dir /bb
        vol->file_tree =
            fat_tree_insert(vol->file_tree, bb_log_node, vigilante);

        // escribimos en la FAT table los direntry del children recien creado
        fat_file_dentry_add_child(bb_dir, vigilante);
    } else {
        printf("El vigilante ya existe. Cargamos los archivos en el arbol\n");
        printf("bad cluster: %u\n", bad_cluster);
        // cargamos los archivos en el arbol para que el sistema pueda acceder a
        // ellos (que estan en disco) leyendo los hijos del dir ya existente

        // creamos (en memoria) el dir que apunta al dir existente en
        // bad_cluster porq como estamos inicializando el arbol, esta vacio
        fat_file bb_dir =
            fat_file_init_orphan_dir("/bb", vol->table, bad_cluster);
        vol->file_tree = fat_tree_insert(vol->file_tree, root_dir_node, bb_dir);

        fat_tree_node bb_dir_node = fat_tree_node_search(vol->file_tree, "/bb");
        // leemos los children que ya esta en disco
        fat_fuse_load_directory_children(vol, bb_dir_node);

        // no hace falta dentry_add_child() porq estos datos ya estan en el
        // disco lo que falta nomas es cargarlos en memoria
    }
    return vol;
}

fat_volume fat_fuse_init(fat_volume vol) {
    errno = 0;
    fat_file root_dir =
        fat_file_init_orphan_dir("/", vol->table, vol->root_dir_start_cluster);
    vol->file_tree = fat_tree_init();
    vol->file_tree = fat_tree_insert(vol->file_tree, NULL, root_dir);

    // Load the content of / to the tree
    fat_tree_node root_dir_node = fat_tree_node_search(vol->file_tree, "/");
    // iniciamos el vigilante
    fat_fuse_load_directory_children(vol, root_dir_node);
    vol = vigilante(vol, root_dir_node);

    // impresion para debug
    fat_tree_print_preorder(vol->file_tree);
    return vol;
}

/* Get file attributes (file descriptor version) */
int fat_fuse_fgetattr(const char *path, struct stat *stbuf,
                      struct fuse_file_info *fi) {
    fat_file file = (fat_file)fat_tree_get_file((fat_tree_node)fi->fh);
    fat_file_to_stbuf(file, stbuf);
    return 0;
}

/* Load the entire hierarchy of parent directories in the in-memory tree.
 * IMPORTANT: Asumes the rood directory / has been already loaded
 */
static int fat_fuse_load_path(const char *path) {
    fat_volume vol = get_fat_volume();
    size_t len = strlen(path);
    char *prefix = calloc(len + 1, sizeof(char));
    for (size_t i = 1; i <= len; i++) {
        if (path[i] != '/')
            continue;
        if (path[i] == '\0')
            break; // End of path
        // We found a path delimiter /, we take everything up to there
        memcpy(prefix, path, i);
        prefix[i] = '\0';

        fat_tree_node dir_node = fat_tree_node_search(vol->file_tree, prefix);
        if (dir_node == NULL) {
            // This doesn't happen because the parent was added in the
            // previous iteration.
            DEBUG("Directory %s not found in tree\n", prefix);
            errno = ENOENT;
            return -errno;
        }
        fat_file dir = fat_tree_get_file(dir_node);
        if (!fat_file_is_directory(dir)) {
            DEBUG("Directory %s is not a directory\n", prefix);
            errno = ENOTDIR;
            return -errno;
        }
        if (dir->children_read != 1) {
            fat_fuse_load_directory_children(vol, dir_node);
        }
    }
    free(prefix);
    return 0;
}

/* Get file attributes (path version) */
int fat_fuse_getattr(const char *path, struct stat *stbuf) {
    fat_volume vol;
    fat_file file;

    vol = get_fat_volume();

    // Check all parent directories in path are loaded in the tree
    fat_fuse_load_path(path);

    file = fat_tree_search(vol->file_tree, path);
    if (file == NULL) {
        errno = ENOENT;
        return -errno;
    }
    fat_file_to_stbuf(file, stbuf);
    return 0;
}

/* Open a file */
int fat_fuse_open(const char *path, struct fuse_file_info *fi) {
    fat_volume vol;
    fat_tree_node file_node;
    fat_file file;

    vol = get_fat_volume();
    file_node = fat_tree_node_search(vol->file_tree, path);
    if (!file_node)
        return -errno;
    file = fat_tree_get_file(file_node);
    if (fat_file_is_directory(file))
        return -EISDIR;
    fat_tree_inc_num_times_opened(file_node);
    fi->fh = (uintptr_t)file_node;
    return 0;
}

/* Open a directory */
int fat_fuse_opendir(const char *path, struct fuse_file_info *fi) {
    fat_volume vol = NULL;
    fat_tree_node file_node = NULL;
    fat_file file = NULL;

    vol = get_fat_volume();
    file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL) {
        return -errno;
    }
    file = fat_tree_get_file(file_node);
    if (!fat_file_is_directory(file)) {
        return -ENOTDIR;
    }
    fat_tree_inc_num_times_opened(file_node);
    fi->fh = (uintptr_t)file_node;
    return 0;
}

/* Add entries of a directory in @fi to @buf using @filler function. */
int fat_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi) {
    fat_volume vol = get_fat_volume();
    errno = 0;
    fat_tree_node dir_node = (fat_tree_node)fi->fh;
    fat_file dir = fat_tree_get_file(dir_node);
    fat_file *children = NULL, *child = NULL;
    int error = 0;

    // Insert first two filenames (. and ..)
    if ((*filler)(buf, ".", NULL, 0) || (*filler)(buf, "..", NULL, 0)) {
        return -errno;
    }
    if (!fat_file_is_directory(dir)) {
        errno = ENOTDIR;
        return -errno;
    }
    if (dir->children_read != 1) {
        fat_fuse_load_directory_children(vol, dir_node);
        if (errno < 0) {
            return -errno;
        }
    }

    children = fat_tree_flatten_h_children(dir_node);
    child = children;
    while (*child != NULL) {
        // comparo el hijo actual para ver si es el vigilate,
        // cuando lo sea lo omite por ende no va a aparecer al hacer ls
        if (strcmp((*child)->name, "bb") == 0) {
            child++;
            continue;
        }
        error = (*filler)(buf, (*child)->name, NULL, 0);
        if (error != 0) {
            DEBUG("Error in readdir: %s\n", dir->filepath);
            break;
        }
        child++;
    }
    return 0;
}

/* Read data from a file */
int fat_fuse_read(const char *path, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi) {
    errno = 0;
    int bytes_read;
    fat_tree_node file_node = (fat_tree_node)fi->fh;
    fat_file file = fat_tree_get_file(file_node);
    fat_file parent = fat_tree_get_parent(file_node);

    bytes_read = fat_file_pread(file, buf, size, offset, parent);
    fat_fuse_log_activity(".read", file);

    return bytes_read;
}

/* Write data from a file */
int fat_fuse_write(const char *path, const char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
    int bytes_written;
    fat_tree_node file_node = (fat_tree_node)fi->fh;
    fat_file file = fat_tree_get_file(file_node);
    fat_file parent = fat_tree_get_parent(file_node);

    if (size == 0)
        return 0; // Nothing to write
    if (offset > file->dentry->file_size)
        return -EOVERFLOW;

    bytes_written = fat_file_pwrite(file, buf, size, offset, parent);
    fat_fuse_log_activity(".write", file);

    return bytes_written;
}

/* Close a file */
int fat_fuse_release(const char *path, struct fuse_file_info *fi) {
    fat_tree_node file = (fat_tree_node)fi->fh;
    fat_tree_dec_num_times_opened(file);
    return 0;
}

/* Close a directory */
int fat_fuse_releasedir(const char *path, struct fuse_file_info *fi) {
    fat_tree_node file = (fat_tree_node)fi->fh;
    fat_tree_dec_num_times_opened(file);
    return 0;
}

int fat_fuse_mkdir(const char *path, mode_t mode) {
    errno = 0;
    fat_volume vol = NULL;
    fat_file parent = NULL, new_file = NULL;
    fat_tree_node parent_node = NULL;

    // The system has already checked the path does not exist. We get the parent
    vol = get_fat_volume();
    parent_node = fat_tree_node_search(vol->file_tree, dirname(strdup(path)));
    if (parent_node == NULL) {
        errno = ENOENT;
        return -errno;
    }
    parent = fat_tree_get_file(parent_node);
    if (!fat_file_is_directory(parent)) {
        fat_error("Error! Parent is not directory\n");
        errno = ENOTDIR;
        return -errno;
    }

    // init child
    new_file = fat_file_init(vol->table, true, strdup(path));
    if (errno != 0) {
        return -errno;
    }
    // insert to directory tree representation
    vol->file_tree = fat_tree_insert(vol->file_tree, parent_node, new_file);
    // write file in parent's entry (disk)
    fat_file_dentry_add_child(parent, new_file);
    return -errno;
}

/* Creates a new file in @path. @mode and @dev are ignored. */
int fat_fuse_mknod(const char *path, mode_t mode, dev_t dev) {
    errno = 0;
    fat_volume vol;
    fat_file parent, new_file;
    fat_tree_node parent_node;

    // The system has already checked the path does not exist. We get the parent
    vol = get_fat_volume();
    parent_node = fat_tree_node_search(vol->file_tree, dirname(strdup(path)));
    if (parent_node == NULL) {
        errno = ENOENT;
        return -errno;
    }
    parent = fat_tree_get_file(parent_node);
    if (!fat_file_is_directory(parent)) {
        fat_error("Error! Parent is not directory\n");
        errno = ENOTDIR;
        return -errno;
    }
    new_file = fat_file_init(vol->table, false, strdup(path));
    if (new_file == NULL) {
        return -errno;
    }
    // insert to directory tree representation
    vol->file_tree = fat_tree_insert(vol->file_tree, parent_node, new_file);
    // Write dentry in parent cluster
    fat_file_dentry_add_child(parent, new_file);
    return -errno;
}

int fat_fuse_utime(const char *path, struct utimbuf *buf) {
    errno = 0;
    fat_file parent = NULL;
    fat_volume vol = get_fat_volume();
    fat_tree_node file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL || errno != 0) {
        errno = ENOENT;
        return -errno;
    }
    parent = fat_tree_get_parent(file_node);
    if (parent == NULL || errno != 0) {
        DEBUG("WARNING: Setting time for parent ignored");
        return 0; // We do nothing, no utime for parent
    }
    fat_utime(fat_tree_get_file(file_node), parent, buf);
    return -errno;
}

/* Shortens the file at the given offset.*/
int fat_fuse_truncate(const char *path, off_t offset) {
    errno = 0;
    fat_volume vol = get_fat_volume();
    fat_file file = NULL, parent = NULL;
    fat_tree_node file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL || errno != 0) {
        errno = ENOENT;
        return -errno;
    }
    file = fat_tree_get_file(file_node);
    if (fat_file_is_directory(file))
        return -EISDIR;

    parent = fat_tree_get_parent(file_node);
    fat_tree_inc_num_times_opened(file_node);
    fat_file_truncate(file, offset, parent);
    return -errno;
}

// Delete a File.
int fat_fuse_unlink(const char *path) {
    errno = 0;
    fat_volume vol;
    fat_file parent, file;
    fat_tree_node file_node;

    vol = get_fat_volume();

    // 1. Se busca el nodo de archivo a eliminar
    file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL || errno != 0) {
        errno = ENOENT;
        return -errno;
    }

    // 2. Archivo a eliminar
    file = fat_tree_get_file(file_node);
    if (file == NULL) {
        return -errno;
    }

    if (fat_file_is_directory(file)) {
        errno = EISDIR;
        return -errno;
    }

    parent = fat_tree_get_parent(file_node);
    // Se valida que sea un directorio
    if (parent == NULL || !fat_file_is_directory(parent)) {
        fat_error("Error! Parent is not directory\n");
        errno = ENOTDIR;
        return -errno;
    }
    // 4. Se elimina de memoria y disco
    // Libera todos los clusters del archivo
    fat_file_free_clusters(file);
    // Elimina todas las entradas del directorio
    fat_file_remove_dentry(file, parent);
    // Elimina el nodo del archivo desde el tree
    vol->file_tree = fat_tree_delete(vol->file_tree, path);

    return -errno;
}

// Delete directory.
int fat_fuse_rmdir(const char *path) {
    errno = 0;
    fat_volume vol = NULL;
    fat_file parent = NULL, directory = NULL;
    fat_tree_node dir_node_a_delete = NULL;

    vol = get_fat_volume();

    // 1. Busco el nodo del directorio a eliminar
    dir_node_a_delete = fat_tree_node_search(vol->file_tree, path);
    if (dir_node_a_delete == NULL) {
        errno = ENOENT;
        return -errno;
    }
    // 2. Directorio a eliminar
    directory = fat_tree_get_file(dir_node_a_delete);

    // 3. Verifico que es un directorio
    if (!fat_file_is_directory(directory)) {
        fat_error("Error! not is directory \n");
        errno = ENOTDIR;
        return -errno;
    }

    // 4. Se elimina recursivamente el contenido del directorio sino esta vacio
    GList *children_list = fat_file_read_children(directory);
    // Se elimina todos los hijos
    for (GList *l = children_list; l != NULL; l = l->next) {
        fat_file child = (fat_file)l->data;
        printf("archivos a eliminar: %s\n", child->name);
        if (fat_file_is_directory(child)) {
            // Se llama recursivamente a la funcion eliminar directorio con el
            // path
            fat_fuse_rmdir(child->filepath);
        } else {
            fat_fuse_unlink(child->filepath);
        }
    }

    // libera lista
    if (children_list) {
        g_list_free(children_list);
    }

    parent = fat_tree_get_parent(dir_node_a_delete);
    if (parent == NULL || !fat_file_is_directory(parent)) {
        errno = ENOTDIR;
        return -errno;
    }

    // 6. Se elimina de memeoria y del disco
    fat_file_remove_dentry(directory, parent);
    fat_file_free_clusters(directory);
    vol->file_tree = fat_tree_delete(vol->file_tree, path);

    return -errno;
}