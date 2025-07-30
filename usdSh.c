#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define MAX_BUFFER 256
#define MAX_ARGS 32

/*
 * UCU Unix Distriution Standard Shell v0.4 (AHORA CON REDIRECCIÓN DE SALIDA)
 * ---------------
 * Santiago Blanco 2025
 * UCU Unix Standard Shell
 * */

void prompt();
void parseArgs(char *input, char **args, int *argc);
int handleRedirect(char** args);

// ------------------------
// ------------------------

int main() {
  if (tcgetpgrp(STDIN_FILENO) != getpgrp()) {
    tcsetpgrp(STDIN_FILENO, getpgrp());
  }

  char inputBuf[MAX_BUFFER];
  char *cmd1_args[MAX_ARGS];
  char *cmd2_args[MAX_ARGS];

  /* TODO ESTO ES NECESARIO PARA MANEJAR SIGNALS EN TTY
   * ES CRITICO MANTENERLO */

  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);

  pid_t shell_pgid = getpid();
  setpgid(shell_pgid, shell_pgid);
  tcsetpgrp(STDIN_FILENO, shell_pgid);

  for (;;) {
    prompt();

    int count = read(0, inputBuf, MAX_BUFFER);
    if (count <= 1) {
      continue;
    }

    inputBuf[count - 1] = '\0'; // arrasa con el '\n'

    if (strcmp(inputBuf, "help") == 0) {
      write(1, "UCU Unix Standard Shell v0.2\nSantiago Blanco 2025\n", 51);
      continue;
    }

    // DIVIDO ANTES Y DESPUES DEL PIPE
    char *cmd1 = strtok(inputBuf, "|");
    char *cmd2 = strtok(NULL, "|");

    // if (cmd1)
    //   while (*cmd1 == ' ')
    //     cmd1++;
    // if (cmd2)
    //   while (*cmd2 == ' ')
    //     cmd2++;
    //

    // SEPARO POR ARGUMENTOS
    int argc1;
    parseArgs(cmd1, cmd1_args, &argc1);

    if (argc1 == 0) {
      continue; // Empty command
    }

    // SI COMANDO UNICO, EJECUTO DE UNA
    if (cmd2 == NULL) {

      if (strcmp(cmd1_args[0], "exit") == 0) {
        exit(0);
      }

      if (strcmp(cmd1_args[0], "cd") == 0) {
        if (cmd1_args[1] == NULL) {
          chdir(getenv("HOME"));
        } else if (chdir(cmd1_args[1]) == -1) {
          perror("cd");
        }
        continue;
      }

      // El fork y exec propiamente dicho, crea un hijo y reemplaza su contenido
      // por el programa dado

      pid_t pid = fork();

      if (pid == -1) {
        perror("fork");
        continue;
      }

      if (pid == 0) {
        setpgid(0, 0);
        tcsetpgrp(STDIN_FILENO, getpid());
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);

        int out_fd = handleRedirect(cmd1_args);
        if (out_fd != -1) {
          dup2(out_fd, STDOUT_FILENO);
          close(out_fd);
        }
        execvp(cmd1_args[0], cmd1_args);
        perror("exec");
        exit(1);
      } else {
        // proceso padre (la shell)

        // aseguremus que el hijo esté en su grupo
        setpgid(pid, pid);

        // doy control de terminal a hijo
        tcsetpgrp(STDIN_FILENO, pid);

        int status;
        waitpid(pid, &status, 0);

        // recuperamos control del terminal
        tcsetpgrp(STDIN_FILENO, getpid());
        continue;
      }
    }
    // OJO QUE SE VIENE UN PIPE
    // SEPARO ARGUMENTOS DEL SEGUNDO COMANDO
    int argc2 = 0;
    parseArgs(cmd2, cmd2_args, &argc2);

    int fd[2];
    if (pipe(fd) == -1) {
      perror("pipe");
      continue;
    }

    pid_t pid1 = fork();

    if (pid1 == -1) {
      perror("fork");
      continue;
    }

    if (pid1 == 0) {
      // primer proceso (hijo 1) vomita al pipe
      dup2(fd[1], STDOUT_FILENO);
      close(fd[0]);
      close(fd[1]);
      execvp(cmd1_args[0], cmd1_args);
      perror("exec1");
      exit(1);
    }

    pid_t pid2 = fork();

    if (pid2 == -1) {
      perror("fork");
      continue;
    }

    if (pid2 == 0) {
      // segundo proceso (hijo 2) succiona del pipe
      dup2(fd[0], STDIN_FILENO);
      close(fd[1]);
      close(fd[0]);
      execvp(cmd2_args[0], cmd2_args);
      perror("exec2");
      exit(1);
    }

    // proc. padre
    close(fd[0]);
    close(fd[1]);
    // espero por ambos procesos
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
  }

  return 0;
}

// ---------------------------------
// ---------------------------------

void prompt() {
  char *cwd = getcwd(NULL, 0);
  write(1, cwd, strlen(cwd));
  write(1, " >> ", 4);
  free(cwd);
}

void parseArgs(char *input, char **args, int *argc) {
  *argc = 0;

  if (!input) {
    args[0] = NULL;
    return;
  }

  char *tok = strtok(input, " ");
  while (tok && *argc < MAX_ARGS - 1) {
    args[(*argc)++] = tok;
    tok = strtok(NULL, " ");
  }
  args[*argc] = NULL;
}

// esto marcha joyazo
int handleRedirect(char **args) {
  for (int i = 0; args[i] != NULL; i++) {

    // redirección de salida 
    if (strcmp(args[i], ">") == 0 && args[i + 1] != NULL) {
      int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0) {
        perror("open >");
        exit(1);
      }

      // eliminar > y archivo de args
      for (int j = i; args[j + 2] != NULL; j++) {
        args[j] = args[j + 2];
      }
      args[i] = NULL;
      return fd;
    }

    // redirección con append: >>
    if (strcmp(args[i], ">>") == 0 && args[i + 1] != NULL) {
      int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd < 0) {
        perror("open >>");
        exit(1);
      }

      // Eliminar >> y archivo de args
      for (int j = i; args[j + 2] != NULL; j++) {
        args[j] = args[j + 2];
      }
      args[i] = NULL;
      return fd;
    }
  }
  return -1; // no hay redirección :(
}

