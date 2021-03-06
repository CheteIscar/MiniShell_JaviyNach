//-----------Importación de las librerías necesarias para la minishell-----------\\

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "parser.h"


//-----------Variables globales y estructuras-----------\\

typedef struct TProcesoBg
{
    int num;
    pid_t pid;
    char *comandos;
} ProcesoBg;

int nProcesos = 0;    
pid_t *pBgFinalizados;
int nProcesosFinalizados = 0;
int boolProcesosFinalizados = 0;
char buf[1024];
int status;

//-----------Subfunciones usadas dentro del main-----------\\

void prompt(); // Escribe el prompt por pantalla
void cd(tcommand cmd); // Mandato interno cd
int comandosCoincidentes(tcommand cmd, char *nombre); // Comprueba si dos mandatos son iguales
void redireccionEntrada(char *fichero); // Coge la entrada de un fichero dado en lugar de stdin
void redireccionSalida(char *fichero); // Pone la salida en un fichero dado en lugar de stout 
int existeComando(tline *line); // Comprueba si existe el comando pasado
void redireccionError(char *fichero); // Pone la salida de error en un fichero dado en lugar de stderr
void anadirBackground(pid_t pid, ProcesoBg **listaProcesos); // Mete en una lista todos los procesos que se ejecuten en background
void manejadorBg(); // Le dice al padre cuando un hijo en background ha terminado
int buscarPid(pid_t pid, ProcesoBg *listaProcesos); // Busca el pid de un proceso en la lista de procesos en background
void jobs(ProcesoBg *listaProcesos); // Mandato interno jobs
void eliminarFinalizados(ProcesoBg **listaProcesos, pid_t *pBgFinalizados); // Elimina todos los procesos finalizados de la liste de procesos en background
void eliminarProceso(pid_t pid, ProcesoBg **listaProcesos); // Elimina el proceso de pid = pid de la lista de procesos en background
void fg(tline *line, ProcesoBg **listaProcesos);


//-----------Función main-----------\\

int main(void){ 
	tline *line;
	int i,j;
    pid_t pid;
    pid_t *pHijos;
    int p[2];
    int pipeExit;
    ProcesoBg *listaProcesosBg;
    pid_t pid2; // Se usa para el fork del proceso de los comandos en background

    prompt();
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN); // Evito que se cierre la shell cuando recibe estas señales
    signal(SIGUSR1, manejadorBg);
	while (fgets(buf, 1024, stdin)) {
	    
        pBgFinalizados = malloc(sizeof(pid_t));
        listaProcesosBg = (ProcesoBg*)  malloc(sizeof(ProcesoBg));
    	line = tokenize(buf);
    	if (line == NULL) {
            if (boolProcesosFinalizados == 1){
                eliminarFinalizados(&listaProcesosBg, pBgFinalizados);
            }
			continue;
		}
        
        if (line->ncommands > 0){ // Números de mandatos mayor que 0
            if (comandosCoincidentes(line->commands[0],"cd") == 0){ 
                if (line->ncommands > 1){
                    fprintf(stderr, "El cd no se puede usar con pipes");
                }
                else{
                    cd(line->commands[0]); // Comprueba si el mandato pasado es cd y lo ejecuta 
                }
            }
            if (comandosCoincidentes(line->commands[0], "jobs") == 0){
                jobs(listaProcesosBg); // Comprueba si el mandato pasado es jobs y lo ejecuta
            }
            if (comandosCoincidentes(line->commands[0], "fg") == 0){
                fg(line, &listaProcesosBg); // Comprueba si el mandato pasado es fg y lo ejecuta
            }
            if (existeComando(line) == 0){
                if (comandosCoincidentes(line->commands[0], "cd") == 1 && comandosCoincidentes(line->commands[0], "fg") == 1 && comandosCoincidentes(line->commands[0], "jobs") == 1){
                    if (line->ncommands == 1){ // Número de mandatos igual a 1. Se excluyen jobs, cd y fg
                        if (line->background){
                            pid2 = fork();
                            if (pid2 == 0){
                                pid = fork();
                                if (pid < 0){
                                    fprintf(stderr, "Ha fallado el fork()\n");
                                    exit(-1);
                                }
                                else if (pid == 0){ // Proceso hijo
                                    signal(SIGINT, SIG_DFL); // Activo el funcionamiento por defecto de estas señales cuando se ejecutan mandatos
                                    signal(SIGQUIT, SIG_DFL);
                                    if (line->redirect_input != NULL){
                                        redireccionEntrada(line->redirect_input);
                                    }
                                    if (line->redirect_output != NULL){
                                        redireccionSalida(line->redirect_output); // Tratamiento de redirecciones
                                    }
                                    if (line->redirect_error != NULL){
                                        redireccionError(line->redirect_error);
                                    }
                                    execvp(line->commands[0].argv[0], line->commands[0].argv);
                                    fprintf(stderr, "\nError en la ejecución del exec: %s\n", strerror(errno));
                                    exit(1);
                                }
                                else{
                                    wait(&status);
                                }
                                kill(getppid(), SIGUSR1);
                                exit(0);
                            }
                            else{
                                anadirBackground(pid2, &listaProcesosBg);
                                fprintf(stderr, "[%d] %d\n", nProcesos , (listaProcesosBg + (nProcesos - 1))->pid);
                            }        
                        }
                        else{
                            pid = fork();
                            if (pid < 0){
                                fprintf(stderr, "Ha fallado el fork()\n");
                                exit(-1);
                            }
                            else if (pid == 0){ // Proceso hijo
                                signal(SIGINT, SIG_DFL); // Activo el funcionamiento por defecto de estas señales cuando se ejecutan mandatos
                                signal(SIGQUIT, SIG_DFL);
                                if (line->redirect_input != NULL){
                                    redireccionEntrada(line->redirect_input);
                                }
                                if (line->redirect_output != NULL){
                                    redireccionSalida(line->redirect_output); // Tratamiento de redirecciones
                                }
                                if (line->redirect_error != NULL){
                                    redireccionError(line->redirect_error);
                                }
                                execvp(line->commands[0].argv[0], line->commands[0].argv);
                                fprintf(stderr, "Error en la ejecución del exec: %s\n", strerror(errno));
                                exit(1);
                            }
                            else{ // Proceso padre
                                wait(&status);              // HAY QUE AÑADIR EL CONTROL DEL WAIT DESPUÉS
                                if (WIFEXITED(status) != 0){
                                    if (WEXITSTATUS(status) != 0){
                                        fprintf(stderr, "Ha ocurrido un error en la ejecución del comando");
                                    }
                                }
                            }
                        }
                    } // Fin de mandatos con un solo comando
                    else{ // Número de mandatos mayor o igual que dos, con pipes
                        pHijos = malloc(line->ncommands * sizeof(pid_t));
                        
                        if (line->background){
                            pid2 = fork();
                            if (pid2==0){
                                pipe(p); // Creación de la pipe

                                pHijos[0] = fork(); // Creación del primer hijo
                                if (pHijos[0] < 0){
                                    fprintf(stderr, "Ha fallado el fork\n");
                                    exit(-1);               
                                }
                                else if (pHijos[0] == 0){ // Proceso hijo
                                    close(p[0]); // Este hijo solo escribe en la pipe. Cierro el otro extremo
                                    signal(SIGINT, SIG_DFL); // Activo las señales igual que arriba
                                    signal(SIGQUIT, SIG_DFL);   
                                    if (line->redirect_input != NULL){
                                        redireccionEntrada(line->redirect_input);
                                    }
                                    dup2(p[1], STDOUT_FILENO); // Redirecciono la salida estándar a la pipe
                                    execvp(line->commands[0].argv[0], line->commands[0].argv);
                                    close(p[1]); // Cierro la pipe al terminar de usarla
                                    exit(1);
                                    }
                                for (i = 1; i < line->ncommands; i++){
                                    if (i != line->ncommands -1){ // Hijos intermedios. El último hay que hacerlo aparte, igual que con el primero
                                        close(p[1]); // Cierro la parte de la pipe que no voy a usar
                                        pipeExit = dup(p[0]); // Duplico la entrada de la pipe 
                                        pipe(p); // Creo una nueva pipe para evitar problemas después
                                        pHijos[i] = fork();
                                        if (pHijos[i] < 0){
                                            fprintf(stderr, "Ha fallado el fork\n");
                                            exit(-1);
                                        }
                                        else if (pHijos[i] == 0){
                                            close(p[0]);
                                            signal(SIGINT, SIG_DFL); // Volvemos a activar las señales
                                            signal(SIGQUIT, SIG_DFL);
                                            dup2(pipeExit, STDIN_FILENO); // Redirecciono la stdin a la salida de la pipe anterior (duplica en pipeExit)
                                            dup2(p[1], STDOUT_FILENO); // Redirecciono la stdout a la entrada de la nueva pipe creada
                                            execvp(line->commands[i].argv[0], line->commands[i].argv);
                                            close(p[1]);
                                            exit(1);
                                        }
                                    }
                                    else{ // Código para el último hijo
                                        pHijos[i] = fork();
                                       if (pHijos[i] < 0){
                                            fprintf(stderr, "Ha fallado el fork\n");
                                            exit(-1);
                                        }
                                        else if (pHijos[i] == 0){
                                            close(p[1]); // Este hijo no tiene que escribir en la pipe
                                            signal(SIGINT, SIG_DFL);
                                            signal(SIGQUIT, SIG_DFL);
                                            dup2(p[0], STDIN_FILENO);
                                            if (line->redirect_output != NULL){
                                                redireccionSalida(line->redirect_output);
                                            }
                                            if (line->redirect_error != NULL){ // Tratamiento de redirecciones de salida(estándar y error)
                                                redireccionError(line->redirect_error);
                                            }
                                            execvp(line->commands[i].argv[0], line->commands[i].argv);
                                            close(p[0]);
                                            exit(1);
                                        }
                                        else{
                                            close(p[0]);
                                            close(p[1]); // El padre no usa las pipes. Cierro ambos extremos
                                            for (j = 0; j < line->ncommands; j++){
                                                waitpid(pHijos[j], &status, 0);
                                            }
                                            free(pHijos); //Libero la memoria reservada anteriormente para el puntero
                                        }
                                    }
                                }
                                kill(getppid(), SIGUSR1);
                                exit(0);
                            }
                            else{
                                anadirBackground(pid2, &listaProcesosBg);
                                fprintf(stderr, "[%d] %d\n", nProcesos , (listaProcesosBg + (nProcesos - 1))->pid);
                            }
                      }
                      else {
                            pipe(p); // Creación de la pipe

                            pHijos[0] = fork(); // Creación del primer hijo
                            if (pHijos[0] < 0){
                                fprintf(stderr, "Ha fallado el fork\n");
                                exit(-1);               
                            }
                            else if (pHijos[0] == 0){ // Proceso hijo
                                close(p[0]); // Este hijo solo escribe en la pipe. Cierro el otro extremo
                                signal(SIGINT, SIG_DFL); // Activo las señales igual que arriba
                                signal(SIGQUIT, SIG_DFL);   
                                if (line->redirect_input != NULL){
                                    redireccionEntrada(line->redirect_input);
                                }
                                dup2(p[1], STDOUT_FILENO); // Redirecciono la salida estándar a la pipe
                                execvp(line->commands[0].argv[0], line->commands[0].argv);
                                close(p[1]); // Cierro la pipe al terminar de usarla
                                exit(1);
                                }
                            for (i = 1; i < line->ncommands; i++){
                                if (i != line->ncommands -1){ // Hijos intermedios. El último hay que hacerlo aparte, igual que con el primero
                                    close(p[1]); // Cierro la parte de la pipe que no voy a usar
                                    pipeExit = dup(p[0]); // Duplico la entrada de la pipe 
                                    pipe(p); // Creo una nueva pipe para evitar problemas después
                                    pHijos[i] = fork();
                                    if (pHijos[i] < 0){
                                        fprintf(stderr, "Ha fallado el fork\n");
                                        exit(-1);
                                    }
                                    else if (pHijos[i] == 0){
                                        close(p[0]);
                                        signal(SIGINT, SIG_DFL); // Volvemos a activar las señales
                                        signal(SIGQUIT, SIG_DFL);
                                        dup2(pipeExit, STDIN_FILENO); // Redirecciono la stdin a la salida de la pipe anterior (duplica en pipeExit)
                                        dup2(p[1], STDOUT_FILENO); // Redirecciono la stdout a la entrada de la nueva pipe creada
                                        execvp(line->commands[i].argv[0], line->commands[i].argv);
                                        close(p[1]);
                                        exit(1);
                                    }
                                }
                                else{ // Código para el último hijo
                                    pHijos[i] = fork();
                                   if (pHijos[i] < 0){
                                        fprintf(stderr, "Ha fallado el fork\n");
                                        exit(-1);
                                    }
                                    else if (pHijos[i] == 0){
                                        close(p[1]); // Este hijo no tiene que escribir en la pipe
                                        signal(SIGINT, SIG_DFL);
                                        signal(SIGQUIT, SIG_DFL);
                                        dup2(p[0], STDIN_FILENO);
                                        if (line->redirect_output != NULL){
                                            redireccionSalida(line->redirect_output);
                                        }
                                        if (line->redirect_error != NULL){ // Tratamiento de redirecciones de salida(estándar y error)
                                            redireccionError(line->redirect_error);
                                        }
                                        execvp(line->commands[i].argv[0], line->commands[i].argv);
                                        close(p[0]);
                                        exit(1);
                                    }
                                    else{
                                        close(p[0]);
                                        close(p[1]); // El padre no usa las pipes. Cierro ambos extremos
                                        for (j = 0; j < line->ncommands; j++){
                                            waitpid(pHijos[j], &status, 0);
                                        }
                                        free(pHijos); //Libero la memoria reservada anteriormente para el puntero
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        prompt(); 
	}
    free(listaProcesosBg);
    free(pBgFinalizados);
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

void redireccionEntrada(char *fichero){
    int fd;

    fd = open(fichero, O_RDONLY);
    if (fd == -1){
        fprintf(stderr, "Error al abrir el fichero: %s\n", strerror(errno));
    }
    else{
        dup2(fd, STDIN_FILENO);
    }
}

void redireccionSalida(char *fichero){
    int fd;
    
    fd = open(fichero, O_CREAT | O_TRUNC | O_WRONLY);
    if (fd == -1){
        fprintf(stderr, "Error al abrir o crear el fichero: %s\n", strerror(errno));
    }
    else{
        dup2(fd, STDOUT_FILENO);
    }
}

void redireccionError(char *fichero){
    int fd;

    fd = open(fichero, O_CREAT | O_TRUNC | O_WRONLY);
    if (fd == -1){
        fprintf(stderr, "Error al abrir o crear el fichero: %s\n", strerror(errno));
    }
    else{
        dup2(fd, STDERR_FILENO);
    }
}


int existeComando(tline *line){
    int i, totalComandos;

    totalComandos = line->ncommands;
    for (i = 0; i < totalComandos; i++){
        if ((line->commands[i].filename == NULL) && (comandosCoincidentes(line->commands[i], "cd") == 1) && (comandosCoincidentes(line->commands[i], "jobs") == 1) 
            && (comandosCoincidentes(line->commands[i], "fg") == 1)){
            fprintf(stderr, "No existe el comando %s: %s\n", line->commands[i].argv[0], strerror(errno));
            return 1;
        }
    }
    return 0;
}

void anadirBackground(pid_t pid, ProcesoBg **listaProcesos){
    (*listaProcesos + nProcesos)->num = nProcesos + 1;
    (*listaProcesos + nProcesos)->pid = pid;
 //   (*listaProcesos + nProcesos)->comandos = buf;
    nProcesos++;
    *listaProcesos = realloc(*listaProcesos, (nProcesos + 1) * sizeof(ProcesoBg));
} 

void manejadorBg(){
    *(pBgFinalizados + nProcesosFinalizados) = waitpid(-1, NULL, 0);
    nProcesosFinalizados++;
    boolProcesosFinalizados = 1;
    pBgFinalizados = realloc(pBgFinalizados, (nProcesosFinalizados + 1) * sizeof(pid_t));
}

int buscarPid(pid_t pid, ProcesoBg *listaProcesos){
    int i;

    for (i = 0; i < nProcesos; i++){
        if ((listaProcesos + i)->pid = pid){ // Puede faltar asterisco
            return 0; 
        }
    }
    return 1;
}

void eliminarFinalizados(ProcesoBg **listaProcesos, pid_t *pBgFinalizados){
    int i;

    boolProcesosFinalizados = 0;
    for (i = 0; i < nProcesosFinalizados; i++){
        if (buscarPid(*(pBgFinalizados + i), *listaProcesos) == 0){
            eliminarProceso(*(pBgFinalizados + i), listaProcesos);
        }
    }
}

void eliminarProceso(pid_t pid, ProcesoBg **listaProcesos){
    int i;
    ProcesoBg *aux;

    aux = malloc(sizeof(ProcesoBg));
    for(i = 0; i < nProcesos; i++){
        if ((*(listaProcesos) + i)->pid != pid){
            anadirBackground((*(listaProcesos) + i)->pid,  &aux);
        }
    }
    free(*listaProcesos);
    *listaProcesos = aux;
    nProcesos--;
}

void jobs(ProcesoBg *listaProcesos){
    int i;

    for (i = 0; i < nProcesos; i++){
        fprintf(stderr, "[%d] Ejecutando \t\t\t\t %s\n", (listaProcesos + i)->num, (listaProcesos + i)->comandos);
    }
}
      
void fg(tline *line, ProcesoBg **listaProcesos){
    int i;
    int nume;

    if (line->ncommands > 1){
        fprintf(stderr, "El fg no se puede usar con pipes\n");
    }
    else if (line->commands[0].argc == 2){
        nume = atoi(line->commands[0].argv[1]);
        while(nume != (*(listaProcesos) + i)->num && i < nProcesos){
            i++;
        }
        if ((*(listaProcesos) + i)->num != nume){
            fprintf(stderr, "Comando no encontrado en background o ya finalizado\n");
        }
        else{
            signal(SIGQUIT, SIG_DFL);
            signal(SIGINT, SIG_DFL);
            waitpid((*(listaProcesos) + i)->pid, &status, 0);
            eliminarProceso((*(listaProcesos) + i)->pid, listaProcesos);
            boolProcesosFinalizados=1;
        }
    }
    else{
        fprintf(stderr, "Uso inadecuado del fg");
    }
}
