#include "parsing.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "command.h"
#include "parser.h"

static scommand parse_scommand(Parser p) {
    /* Devuelve NULL cuando hay un error de parseo */
    scommand cmd = scommand_new();

    arg_kind_t type;
    char *arg;
    arg = parser_next_argument(p, &type);
    
    while (arg != NULL) {
        if (type == ARG_NORMAL) {
            scommand_push_back(cmd, arg);
        } else if (type == ARG_INPUT){
            scommand_set_redir_in(cmd, arg);
        } else if (type == ARG_OUTPUT) {
            scommand_set_redir_out(cmd, arg);
        } else {
            scommand_destroy(cmd);
            return NULL;
        }
        arg = parser_next_argument(p, &type);
    }
    return cmd;
}

pipeline parse_pipeline(Parser p) {
    
    parser_skip_blanks(p);

    if (parser_at_eof(p)) {
        return NULL;
    }
    pipeline result = pipeline_new();
    scommand cmd = NULL;
    bool error = false, another_pipe = false;
    cmd = parse_scommand(p);
    error = (cmd==NULL); /* Comando inválido al empezar */

    if (error) {
        result = pipeline_destroy(result);
        return NULL;
    }

    another_pipe = true;
    while (another_pipe && !error){
        parser_skip_blanks(p);
        pipeline_push_back(result, cmd);
        parser_op_pipe(p, &another_pipe);
        if (another_pipe) {
            cmd = parse_scommand(p);
            parser_skip_blanks(p);
            error = (cmd == NULL);
        }
        parser_skip_blanks(p);
    }
    /* Opcionalmente un OP_BACKGROUND al final */
    bool is_background = false;
    parser_op_background(p, &is_background);
    pipeline_set_wait(result, !is_background);
    /* Si hubo error en el parseo de algún comando, 
    * limpiar y retornar NULL */
    bool garbage;
    parser_garbage(p, &garbage);

    /* Tolerancia a espacios posteriores */
    /* Consumir todo lo que hay inclusive el \n */
    if( pipeline_length(result) == 1 && scommand_length(pipeline_front(result)) == 0) {
        pipeline_destroy(result);
        return NULL;
    }
    

    return result;
}
