//-----------Importación de las librerías necesarias para la minishell-----------

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

#include "parser.h"



//-----------Subfunciones usadas dentro del main-----------

void prompt(); // Escribe el prompt por pantalla
void cd(tcommand cmd); // Mandato interno cd



//-----------Función main-----------

int main(void){ 
	char buf[1024];
	tline *line;
	int i,j;
    pid_t pid;

    prompt();
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
	while (fgets(buf, 1024, stdin)) {
	    
        	
		line = tokenize(buf);
    	if (line == NULL) {
            printf("Traza");
		//	continue;
		}
       /* if (line != NULL && line->commands[0].argv[0] == "cd"){
            if (line->ncommands > 1){
                fprintf(stderr, "El cd no se usa con pipes");
            } else {
                cd(line->commands[0]);
            }
            printf("Traza 4");
        }
        printf("Traza 2");*/
        
        
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



//-----------Implementación de las subfunciones usadas-----------

void prompt(){
    char dir[1024];
    getcwd(dir, 1024);
    printf("==> %s: ", dir);
}

void cd(tcommand cmd){
    char *dir;
    char *dir2;
    printf("Traza 3");
    if (cmd.argc < 2){
        dir = getenv("home");
    } else{
        dir = cmd.argv[1];
    }

    chdir(dir);
    getcwd(dir2, 1024);
}
