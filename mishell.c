//-----------Importación de las librerías necesarias para la minishell-----------\\

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include "parser.h"



//-----------Subfunciones usadas dentro del main-----------\\

void prompt(); // Escribe el prompt por pantalla
void cd(tcommand cmd); // Mandato interno cd
int comandosCoincidentes(tcommand cmd, char *nombre);


//-----------Función main-----------\\

int main(void){ 
	char buf[1024];
	tline *line;
	int i,j;
    pid_t pid;
    char *token;

    prompt();
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
	while (fgets(buf, 1024, stdin)) {
	    
    	line = tokenize(buf);
    	if (line == NULL) {
			continue;
		}
        
        if (line->ncommands > 0){ // Números de mandatos mayor que 0
            if (comandosCoincidentes(line->commands[0],"cd") == 0){ 
                cd(line->commands[0]); // Comprueba si el mandato pasado es cd y lo ejecuta 
            }
           /* if (comandosCoincidentes(line->commands[0], "jobs") == 0){
              //jobs(); // Comprueba si el mandato pasado es jobs y lo ejecuta
            }
            if (comandosCoincidentes(line->commands[0], "fg") == 0){
              //fg(); // Comprueba si el mandato pasado es fg y lo ejecuta
            }
            if (line->ncommands == 1){ // Número de mandatos igual a 1. Se excluyen jobs, cd y fg
                pid = fork();
*/
        }
	//	if (line->redirect_input != NULL) {
	//		printf("redirección de entrada: %s\n", line->redirect_input);
	//	}
	//	if (line->redirect_output != NULL) {
	//		printf("redirección de salida: %s\n", line->redirect_output);
	//	}
	//	if (line->redirect_error != NULL) {
	//		printf("redirección de error: %s\n", line->redirect_error);
	//	}
	//	if (line->background) {
	//		printf("comando a ejecutarse en background\n");
	//	} 
	//	for (i=0; i<line->ncommands; i++) {
	//		printf("orden %d (%s):\n", i, line->commands[i].filename);
	//		for (j=0; j<line->commands[i].argc; j++) {
	//			printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
	//		}
	//	}
        prompt(); 
	}
	return 0;
}



//-----------Implementación de las subfunciones usadas-----------\\

void prompt(){
    char dir[1024];
    getcwd(dir, 1024);
    printf("==> %s: ", dir);
}

void cd(tcommand cmd){
    char *dir;
    
    if (cmd.argc > 2){ // cd erróneo, más de 1 argumento pasado
        fprintf(stderr, "Uso incorrecto del mandato cd\n");
    } 
    if (cmd.argc == 1){ // cd sin argumentos, nos lleva a $HOME
        dir = getenv("HOME");
    } else{
        dir = cmd.argv[1]; // cd con un argumento, nos lleva al directorio pasado
    }

    if (chdir(dir) != 0){
        fprintf(stderr, "Error al cambiar de directorio: %s\n", strerror(errno));
    }
}

int comandosCoincidentes(tcommand cmd, char *nombre){
    char *nComando;

    nComando = cmd.argv[0];
    if (strcmp(nComando, nombre) == 0){
        return 0;
    }
    else{
        return 1;
    }
}
