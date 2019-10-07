/* 
   rsingh12
   115240766
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sysexits.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "command.h"
#include "executor.h"

static void print_tree(struct tree *t);

int execute(struct tree *t) {

   execute_aux(t, STDIN_FILENO, STDOUT_FILENO);
   
   return 0;
}

int execute_aux(struct tree *t, int input_fd, int output_fd) {
  
   /* Base case */
   if (t->conjunction == NONE) {
     
     pid_t pid;
     
     if (strcmp(t->argv[0], "exit") == 0) {
       exit(0);
     }
     
     else if (strcmp(t->argv[0], "cd") == 0) {
       
       char* path = t->argv[1];
       
       if (t->argv[1] == NULL) {
	 path = getenv("HOME");
       }
       
       if (chdir(path) == -1) {
	 return 0;
       }
     }
     
     else {

       if (t->input != NULL) {
	 
	 int current_fd = open(t->input, O_RDONLY);
	 
	 if (dup2(current_fd, input_fd) < 0) {
	   err(EX_OSERR, "dup2 error");
	 }
	 
	 close(current_fd);
       }
       
       if (t->output != NULL) {
	 
	 int current_fd = open(t->output, O_CREAT | O_WRONLY, 0664);
	 
	 if (dup2(current_fd, input_fd) < 0) {
	   err(EX_OSERR, "dup2 error");
	 }

	 close(current_fd);
       }
       
       if ((pid = fork()) < 0) {
	 err(EX_OSERR, "fork error");
       }
       
       /* parent */
       if (pid) {
	 
	 int status;
	 wait(&status);
	 
	 if (!WIFEXITED(status) || WEXITSTATUS(status)) {
	   return 0;
	 }
       }
       
       /* child */
       else {
	 execvp(t->argv[0], t->argv);
	 exit(EX_OSERR);
       }
     }
     
     return 1;
   }
   
   else if (t->conjunction == AND) {
     
     if (t->input != NULL) {
       
       int current_fd = open(t->input, O_RDONLY);
       
       if (dup2(current_fd, input_fd) < 0) {
	 err(EX_OSERR, "dup2 error");
       }
       
       close(current_fd);
     }
     
     if (t->output != NULL) {
       
       int current_fd = open(t->output, O_CREAT | O_WRONLY, 0664);
       
       if (dup2(current_fd, input_fd) < 0) {
	 err(EX_OSERR, "dup2 error");
       }
       
       close(current_fd);
     }
     
     return execute_aux(t->left, input_fd, output_fd) && execute_aux(t->right, input_fd, output_fd);
   }
   
   else if (t->conjunction == PIPE) {
     
     int pipe_fd[2];
     pid_t child_pid_one, child_pid_two;
     
     if (t->input != NULL) {
       
       int current_fd = open(t->input, O_RDONLY);
       
       if (dup2(current_fd, input_fd) < 0) {
	 err(EX_OSERR, "dup2 error");
       }
       
       close(current_fd);
     }
     
     if (t->output != NULL) {
       
       int current_fd = open(t->output, O_CREAT | O_WRONLY, 0664);
       
       if (dup2(current_fd, input_fd) < 0) {
	 err(EX_OSERR, "dup2 error");
       }
       
       close(current_fd);
     }
     
     /* Check for ambiguity */
     if (t->left->output != NULL) {
       printf("Ambiguous output redirect.\n");
       return 0;
     }
     
     if (t->right->input != NULL) {
       printf("Ambiguous input redirect.\n");
       return 0;
     }
     
     if (pipe(pipe_fd) < 0) {
       err(EX_OSERR, "pipe error");
     }
     
     if ((child_pid_one = fork()) < 0) {
       err(EX_OSERR, "fork error");
     }
     
     /* Left side */
     if (child_pid_one == 0) {
       
       close(pipe_fd[0]); /* Don't need read end */
       
       if (dup2(pipe_fd[1], output_fd) < 0) {
	 err(EX_OSERR, "dup2 error");
       }
       
       close(pipe_fd[1]);
       
       execute_aux(t->left, input_fd, output_fd);
       
       exit(0);
     }
     
     /* Right side */
     else { /* Parent's code */

       /* Second child */
       if ((child_pid_two = fork()) < 0) {
	 err(EX_OSERR, "fork error");
       }
       
       if (child_pid_two == 0) {
	 
	 close(pipe_fd[1]); /* Don't need the write end */
	 
	 if (dup2(pipe_fd[0], input_fd) < 0) {
	   err(EX_OSERR, "dup2 error");
	 }
	 
	 close(pipe_fd[0]);
	 
	 execute_aux(t->right, input_fd, output_fd);
	 
	 exit(1);
       }
       
       else { /* Parent */
	 
	 int status_one, status_two;
	 
	 close(pipe_fd[0]);
	 close(pipe_fd[1]);
	 
	 wait(&status_one);
	 wait(&status_two);
	 
	 if ((!WIFEXITED(status_one) || WEXITSTATUS(status_one)) || (!WIFEXITED(status_two) || WEXITSTATUS(status_two))) {
	   return 0;
	 } 
	 
	 return 1;
       }
     }
   }
   
   else if (t->conjunction == SUBSHELL) {
     
     pid_t pid;
      
      if (t->input != NULL) {
	
	int current_fd = open(t->input, O_RDONLY);
	
	if (dup2(current_fd, input_fd) < 0) {
	  err(EX_OSERR, "dup2 error");
	}
	
	close(current_fd);
      }
      
      if (t->output != NULL) {
	
	int current_fd = open(t->output, O_CREAT | O_WRONLY, 0664);
	
	if (dup2(current_fd, input_fd) < 0) {
	  err(EX_OSERR, "dup2 error");
	}
	
	close(current_fd);
      }
      
      if ((pid = fork()) < 0) {
	err(EX_OSERR, "fork error");
      }
      
      /* parent */
      if (pid) {
	
	int status;
	wait(&status);
	
	if (!WIFEXITED(status) || WEXITSTATUS(status)) {
	  return 0;
	} 
	
	return 1;
      }
      
      /* child */
      else {
	execute_aux(t->left, input_fd, output_fd);
	exit(0);
      }
   }
   
   return 1;
}

static void print_tree(struct tree *t) {
   if (t != NULL) {
     print_tree(t->left);
     
     if (t->conjunction == NONE) {
       printf("NONE: %s, ", t->argv[0]);
     } else {
       printf("%s, ", conj[t->conjunction]);
     }
     printf("IR: %s, ", t->input);
     printf("OR: %s\n", t->output);
     
     print_tree(t->right);
   }
}

