#include <stdbool.h> /* para tener bool */
#include <glib.h>    /* para poder usar GList */
#include <stdlib.h>  /* para poder usar malloc y otras */
#include <assert.h>  /* para poder usar assert */
#include <stdio.h>
#include "command.h"

struct scommand_s{
    GList *args;
    char *redir_in;
    char *redir_out;
    unsigned int size;
};

scommand scommand_new(void){
    scommand cmd = calloc(1, sizeof(struct scommand_s));
    cmd->args = NULL;
    cmd->redir_in = NULL;
    cmd->redir_out = NULL;
    cmd->size = 0;

    assert(cmd != NULL);
    assert(scommand_is_empty(cmd));
    assert((scommand_get_redir_in(cmd) == NULL));
    assert((scommand_get_redir_out(cmd) == NULL));
    return cmd;
}

scommand scommand_destroy(scommand self){
    assert(self != NULL);
     g_list_free_full(self->args, free);
     free(self->redir_in);
     free(self->redir_out);
     self->args = NULL;
     self->redir_in = NULL;
     self->redir_out = NULL;
     free(self);
    self = NULL;

    assert(self == NULL);
    return self;
}

void scommand_push_back(scommand self, char *argument){
    assert(self != NULL);
    assert(argument != NULL);

    self->args = g_list_append(self->args, argument);
    self->size++;
    assert(!scommand_is_empty(self));
}

void scommand_pop_front(scommand self){
    assert(self != NULL);
    assert(!scommand_is_empty(self));

    GList *front_elem = g_list_first(self->args);
    free(front_elem->data);
    self->args = g_list_delete_link(self->args, front_elem);
    self->size--;
}

void scommand_set_redir_in(scommand self, char *filename){
    assert(self != NULL);
    free(self->redir_in);
    self->redir_in = filename;
}

void scommand_set_redir_out(scommand self, char *filename){
    assert(self != NULL);
    free(self->redir_out);
    self->redir_out = filename;
}

bool scommand_is_empty(const scommand self)
{
    assert(self != NULL);
    return self->size == 0;
}

unsigned int scommand_length(const scommand self)
{
    assert(self != NULL);
    assert((self->size == 0) == scommand_is_empty(self));
    return self->size;
}

char *scommand_front(const scommand self)
{
    assert((self != NULL) && (!scommand_is_empty(self)));
    char *front_scommand_argument = NULL;
    front_scommand_argument = self->args->data;

    assert(front_scommand_argument != NULL);
    return front_scommand_argument;
}

char *scommand_get_redir_in(const scommand self)
{
    assert(self != NULL);
    return self->redir_in;
}

char *scommand_get_redir_out(const scommand self)
{
    assert(self != NULL);
    return self->redir_out;
}

char *scommand_to_string(const scommand self)
{
    assert(self != NULL);
    // declaracion de cadeana/string res.
    GString *res = g_string_new("");
    // recorrido del arreglo de strings de commans y se guarda en res.
    for (GList *i = self->args; i != NULL; i = i->next)
    {
        g_string_append(res, (char *)i->data);
        if (i->next)
        {
            g_string_append_c(res, ' ');
        }
    }
    // primer argumento
    if (self->redir_in != NULL)
    {
        g_string_append(res, " < ");
        g_string_append(res, self->redir_in);
    }
    // segunda argumento
    if (self->redir_out != NULL)
    {
        g_string_append(res, " > ");
        g_string_append(res, self->redir_out);
    }
    // Devuelve el contenido del GString como un char* normal.
    char *res_final = g_string_free(res, FALSE);

    assert(scommand_is_empty(self) || scommand_get_redir_in(self) == NULL || scommand_get_redir_out(self) == NULL || strlen(res_final) > 0);

    return res_final;
}

struct pipeline_s
{
    GList *commands;
    bool wait;
    unsigned int size;
};

pipeline pipeline_new(void)
{
    pipeline new = malloc(sizeof(struct pipeline_s));
    new->commands = NULL;
    new->wait = true;
    new->size = 0;

    assert(new != NULL && pipeline_is_empty(new) && pipeline_get_wait(new));
    return new;
}

pipeline pipeline_destroy(pipeline self)
{
    assert(self != NULL);
    while (self->size != 0)
    {
        pipeline_pop_front(self);
    }
    free(self);
    self = NULL;

    assert(self == NULL);
    return self;
}

void pipeline_push_back(pipeline self, scommand sc)
{
    assert(self != NULL);
    assert(sc != NULL);

    self->commands = g_list_append(self->commands, sc);
    self->size++;

    assert(!pipeline_is_empty(self));
}

void pipeline_pop_front(pipeline self)
{
    assert(self != NULL);
    assert(!pipeline_is_empty(self));

    GList *front_elem = g_list_first(self->commands);
    scommand_destroy(front_elem->data);
    self->commands = g_list_delete_link(self->commands, front_elem);
    self->size--;
}

void pipeline_set_wait(pipeline self, const bool w)
{
    assert(self != NULL);
    self->wait = w;
}

bool pipeline_is_empty(const pipeline self)
{
    assert(self != NULL);
    return self->size == 0;
}

unsigned int pipeline_length(const pipeline self)
{
    assert(self != NULL);
    assert((self->size == 0) == pipeline_is_empty(self));
    return self->size;
}

scommand pipeline_front(const pipeline self)
{
    assert((self != NULL) && (!pipeline_is_empty(self)));

    assert(self->commands->data != NULL);
    return self->commands->data;
}

bool pipeline_get_wait(const pipeline self)
{
    assert(self != NULL);
    return self->wait;
}

char *pipeline_to_string(const pipeline self)
{
    assert(self != NULL);

    // declaracion de string acumulador.
    GString *res = g_string_new("");

    // recorre listas de scommans.
    for (GList *i = self->commands; i != NULL; i = i->next)
    {
        scommand aux = ((scommand)i->data);
        char *cmd_str = scommand_to_string(aux);
        g_string_append(res, cmd_str);
        // liberar memoria.
        free(cmd_str);

        if (i->next)
        {
            g_string_append(res, " | ");
        }
    }
    // agregar "&" si no hay wait.
    if (!pipeline_get_wait(self))
    {
        g_string_append(res, " & ");
    }
    // Devuelve el contenido del GString como un char* normal.
    char *res_final = g_string_free(res, FALSE);

    assert(pipeline_is_empty(self) || pipeline_get_wait(self) || strlen(res_final) > 0);

    return res_final;
}

