/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>

#include "variante.h"
#include "readcmd.h"


#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1
#include <libguile.h>



/* Structure de childs en execution en fond de tache
*/

struct child_background{
	pid_t pid;
	char *name;
	//struct child_background *prev; // a revoir
	struct child_background *next;
};

int wstatus;
struct child_background *background;

void add_child(pid_t pid, char *name);

void redirection(struct cmdline *l);

void jobs();

void piper(struct cmdline *l);

void pipe_shell(struct cmdline *line);

void executer_cmd(struct cmdline *l);


// Question 6: Insert your code to execute the command line
	// identically to the standard execution scheme:
	//parsecmd, then fork+execvp, for a single command.
	// pipe and i/o redirection are not required. */


int question6_executer(char *line) {

	struct cmdline *l = parsecmd(&line);
	for(int i = 0; l->seq[i] != 0; i++){

		int pid = fork();
		switch(pid){

			case -1:
				printf("error: %s\n",l->err);
				break;
			case 0:
				if(execvp(l->seq[i][0], l->seq[i]) == -1) printf("error: %s\n",l->err);
				break;
			default:
				if(l->bg == 0){ 
					waitpid(pid, &wstatus, 0);
				} else {
					add_child(pid, l->seq[i][0]);
				}
				
				break;
		}
	}

	return 0;
}

SCM executer_wrapper(SCM x)
{
        return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}
#endif


void terminate(char *line) {
#if USE_GNU_READLINE == 1
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
	  free(line);
	printf("exit\n");
	exit(0);
}


/*Insere le nouveau fils dans la liste des processus fils en background*/
// au lieu de line on peut mettre char *name
void add_child(pid_t pid, char *name){
	struct child_background *new_child = malloc(sizeof(struct child_background));
	new_child->name = calloc(1, sizeof(char*)); // calloc(1,...)
	strcpy(new_child->name, name);
	new_child->pid = pid;
	//new_child->prev = NULL;
	//new_child->next = NULL;


	if (background != NULL){
		new_child->next = background;
		//background = new_child;
	}
	/*else {
		new_child->prev = background;
		background->next = new_child;
		background = background->next;
	} */
	background = new_child;
}


/*void redirection(struct cmdline *l) {
	if(l->in != NULL){
		int input = open(l->in, O_RDONLY);
		if(input == -1){
			perror("An error ocurred with opening the file descriptor");
			exit(EXIT_FAILURE);
		}
		dup2(input, 0);
		close(input);
	}

	if(l->out != NULL){
		//int output = open(l->out, O_TRUNC | O_WRONLY | O_CREAT);
		int output = open(l->out , O_WRONLY | O_CREAT | S_IRUSR);
		if(output == -1){
			perror("An error ocurred with opening the file descriptor");
			exit(EXIT_FAILURE);
		}
		dup2(output, 1);
		close(output);
	}
} */

/*bool child_process_status(pid_t pid){

	if(waitpid(pid, &wstatus, WNOHANG) != 0) {
		// child process has finished
		return true;
	}
	
	else{
		// child process still running
		return false;
	}
} */



void jobs(){

	printf("liste des commandes en tache de fond\n");
	printf("-----------------------------\n");
	struct child_background * ptr_backg = background;
	//redirection(l);


	//while (ptr_backg != NULL){}
	while (ptr_backg != NULL){
		//bool status = child_process_status(ptr_backg->pid);
		//if(waitpid(ptr_backg->pid, &wstatus, WNOHANG) == 0){
		
		printf("num_pid :[%d]    nom_cmd :%s\n", ptr_backg->pid, ptr_backg->name);
			
			//printf("etat de fils --> %d\n", wait(&wstatus));
		ptr_backg = ptr_backg->next;
		
		//background = background->next; // ou bien ->prev
	}
}


void multiple_pipes(struct cmdline *l){
	pid_t pid;
	int fd[2];
	int i = 0;
	int fdd = 0;
	while(l->seq[i] != NULL){

		if (pipe(fd) == -1){
			exit(EXIT_FAILURE);
		
		} else {
			pid = fork();
			if(pid == -1){
				perror("fork");
				exit(1);
			}
			else if (pid == 0){
				dup2(fdd, 0);
				if(l->seq[i+1] != NULL){
					dup2(fd[1], 1);
				}
				close(fd[0]);
				execvp(l->seq[i][0], l->seq[i]);
				exit(1);
			}
			else {
				wait(NULL);
				close(fd[1]);
				fdd = fd[0];
				i++;
			}
		}
	}
}



/*void pipe_shell(struct cmdline *line){
	pid_t pid;
	int fd[2];
	if(line->seq[1] != NULL){
		pipe(fd);
		if (pipe(fd) == -1){
			printf("An error ocurred with opening the pipe");
		}
		pid = fork();
		if (pid == -1){
			printf("An error ocurred with creating a child process");
		}
		else if(pid == 0){
			close(fd[1]);
			dup2(fd[0], 0);
			close(fd[0]);
			execvp(line->seq[1][0], line->seq[1]);
		}
		else{
			close(fd[0]);
			dup2(fd[1], 1);
			close(fd[1]);
    	}
	}
} */






void executer_cmd(struct cmdline *l){
	pid_t pid;
	switch (pid = fork()){
		case -1:
			perror("fork:");
			exit(1);
		case 0:
			/*if (strcmp(l->seq[0][0], "jobs") == 0){
				jobs();
			} */


			if(l->seq[1] != NULL){
				multiple_pipes(l);
				//pipe_shell(l);
			}

			else{
				if (l->in != NULL){
					int input = open(l->in, O_RDONLY);
					if(input == -1){
						perror("An error ocurred with opening the file descriptor");
						exit(EXIT_FAILURE);
					}
					dup2(input, 0);
					close(input);
				}

				if (l->out != NULL){
					int output = open(l->in, O_WRONLY | O_CREAT);
					if(output == -1){
						perror("An error ocurred with opening the file descriptor");
						exit(EXIT_FAILURE);
					}
					dup2(output, 0);
					close(output);
				}

				if(execvp(l->seq[0][0], l->seq[0]) < 0){
					printf("*** ERROR: exec failed\n");
				}
			}
			break;
			

			
			/*else if (execvp(l->seq[0][0], l->seq[0]) < 0){
				printf("*** ERROR: exec failed\n");
			} */

			//exit(1);
		default:
			if (l->bg != 0){
				add_child(pid, l->seq[0][0]);
				//add_child(pid, l);
			}
			/*else if (waitpid(pid, &wstatus, WUNTRACED | WCONTINUED) == -1) {
				perror("waitpid");
				exit(1);
			}*/
			else{
				waitpid(pid, &wstatus, 0);
			}
			break;
	}
}

int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

	while (1) {
		struct cmdline *l;
		char *line=0;
		int i, j;
		char *prompt = "ensishell>";

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);
		if (line == 0 || ! strncmp(line,"exit", 4)) {
			terminate(line);
		}

#if USE_GNU_READLINE == 1
		add_history(line);
#endif


#if USE_GUILE == 1
		/* The line is a scheme command */
		if (line[0] == '(') {
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
			continue;
		}
#endif

		/* parsecmd free line and set it up to 0 */
		l = parsecmd( & line);

		/* If input stream closed, normal termination */
		if (!l) {
			terminate(0);
		}

		/*if(l){
			executer_cmd(l);
		} */

		
		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (strcmp(l->seq[0][0], "jobs") == 0){
			jobs();
		}

		else{
			executer_cmd(l);
		}


		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);
		if (l->bg) printf("background (&)\n");



		//Display each command of the pipe */
		for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
			for (j=0; cmd[j]!=0; j++) {
				printf("'%s' ", cmd[j]);
			}
			printf("\n");
		}
	}
}
