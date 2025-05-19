#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens* tokens);
int cmd_help(struct tokens* tokens);
int cmd_pwd(struct tokens* tokens);
int cmd_cd(struct tokens* tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens* tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t* fun;
  char* cmd;
  char* doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {cmd_help, "?", "show this help menu"},
    {cmd_exit, "exit", "exit the command shell"},
    {cmd_pwd, "pwd", "print current working directory"},
    {cmd_cd, "cd", "change current working directory to given directory"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens* tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens* tokens) { exit(0); }

/* Prints the current working directory to standard output */
int cmd_pwd(unused struct tokens* tokens) {
  char cwd[PATH_MAX];

  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  } else {
    perror("getcwd() error");
    return 1;
  }
  return 0;
}

/* Takes one argument, a directory path, and changes the current working directory to that directory */
int cmd_cd(struct tokens* tokens) {
  char* directory = tokens_get_token(tokens, 1);
  if (directory == NULL) {
    perror("missing directory argument");
    return 1;
  }

  if (chdir(directory) != 0) {
    printf("chdir failed with errno %d: %s\n", errno, strerror(errno));
    return 1;
  }

  return 0;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

char* resolve_program_path(char* program_name) {
  // Look for program on PATH
  char* path_env = getenv("PATH");
  if (path_env == NULL) {
    printf("execute_program: path environment variable not set\n");
    return NULL;
  }

  char* path = strdup(path_env);
  if (path == NULL) {
    perror("strdup failed");
    return NULL;
  }

  char* rest;
  char* directory = strtok_r(path, ":", &rest);
  size_t len_program_name = strlen(program_name);
  while (directory != NULL) {
    size_t len_directory = strlen(directory);
    char* program_path = malloc(len_directory + len_program_name + 2);
    if (program_path == NULL) {
      directory = strtok_r(NULL, ":", &rest);
      continue;
    }

    memcpy(program_path, directory, len_directory);
    program_path[len_directory] = '/';
    memcpy(program_path + len_directory + 1, program_name, len_program_name + 1);

    if (access(program_path, X_OK) == 0) {
      return program_path;
    }

    free(program_path);
    directory = strtok_r(NULL, ":", &rest);
  }

  return NULL;
}

bool has_stdin_redirect(struct tokens* tokens, char** file_name_in) {
  for (size_t i = 0; i < tokens_get_length(tokens); i++) {
    char* token = tokens_get_token(tokens, i);
    if (strcmp(token, "<") == 0) {
      *file_name_in = tokens_get_token(tokens, i + 1);
      if (*file_name_in == NULL) {
        fprintf(stderr, "syntax error: expected file after '<'\n");
        return false;
      }
      return true;
    }
  }
  return false;
}

bool has_stdout_redirect(struct tokens* tokens, char** file_name_out) {
  for (size_t i = 0; i < tokens_get_length(tokens); i++) {
    char* token = tokens_get_token(tokens, i);
    if (strcmp(token, ">") == 0) {
      *file_name_out = tokens_get_token(tokens, i + 1);
      if (*file_name_out == NULL) {
        fprintf(stderr, "syntax error: expected file after '>'\n");
        return false;
      }
      return true;
    }
  }
  return false;
}

void execute_program(struct tokens* tokens) {
  int fundex = lookup(tokens_get_token(tokens, 0));
  if (fundex >= 0) {
    cmd_table[fundex].fun(tokens);
    return;
  }

  char* program_argument = tokens_get_token(tokens, 0);
  if (program_argument == NULL) {
    printf("execute_program: missing program argument\n");
    return;
  }

  bool resolved_program_path = false;
  char* program_path = program_argument;
  // See if file exists and is executable
  if (access(program_argument, X_OK) != 0) {
    program_path = resolve_program_path(program_argument);

    if (program_path == NULL) {
      printf("execute_program: unable to find program %s \n", program_argument);
      return;
    } else {
      resolved_program_path = true;
    }
  }

  // Determine whether to redirect stdin
  char* file_name_in = NULL;
  bool redirect_stdin = has_stdin_redirect(tokens, &file_name_in);

  // Determine whether to redirect stdout
  char* file_name_out;
  bool redirect_stdout = has_stdout_redirect(tokens, &file_name_out);

  size_t tokens_length = tokens_get_length(tokens);
  char** argv = malloc(sizeof(char*) * (tokens_length + 1));
  if (argv == NULL) {
    printf("execute_program: failed to allocate memory for arguments");
    return;
  }

  size_t argv_index = 0;
  for (size_t i = 0; i < tokens_length; i++) {
    char* token = tokens_get_token(tokens, i);
    if (strcmp(token, "<") == 0 || strcmp(token, ">") == 0) {
      break;
    }
    argv[argv_index++] = token;
  }
  argv[argv_index] = NULL;

  pid_t pid = fork();
  if (pid == 0) {
    /* Set process group to itself */
    setpgrp();

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    tcsetpgrp(STDIN_FILENO, getpid());

    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);

    if (redirect_stdin) {
      int fd = open(file_name_in, O_RDONLY);

      if (fd < 0) {
        printf("execute_program: failed to open stdin redirect file %s\n", file_name_in);
        exit(1);
      }

      dup2(fd, STDIN_FILENO);
      close(fd);
    }

    if (redirect_stdout) {
      int fd = open(file_name_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0) {
        printf("execute_program: failed to open stdout redirect file %s\n", file_name_out);
        exit(1);
      }

      dup2(fd, STDOUT_FILENO);
      close(fd);
    }

    execv(program_path, argv);
    perror("execv failed");
    return;
  } else {
    setpgid(pid, pid);

    struct sigaction sa_old_int, sa_old_tstp, sa_ignore, sa_old_ttou, sa_old_ttin;
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    sigaction(SIGINT, &sa_ignore, &sa_old_int);
    sigaction(SIGTSTP, &sa_ignore, &sa_old_tstp);
    sigaction(SIGTTOU, &sa_ignore, &sa_old_ttou);
    sigaction(SIGTTIN, &sa_ignore, &sa_old_ttin);

    tcsetpgrp(STDIN_FILENO, pid);

    wait(NULL);

    tcsetpgrp(STDIN_FILENO, getpid());
    sigaction(SIGINT, &sa_old_int, NULL);
    sigaction(SIGTSTP, &sa_old_tstp, NULL);
    sigaction(SIGTTOU, &sa_old_ttou, NULL);
    sigaction(SIGTTIN, &sa_old_ttin, NULL);
  }

  if (resolved_program_path) {
    free(program_path);
  }
  free(argv);
}

size_t get_num_of_pipes(struct tokens* tokens) {
  size_t pipe_num = 0;
  /* Determine the number of pipes by counting the | words */
  for (size_t t = 0; t < tokens_get_length(tokens); t++) {
    if (strcmp(tokens_get_token(tokens, t), "|") == 0) {
      pipe_num++;
    }
  }
  return pipe_num;
}

void execute_with_pipes(struct tokens* tokens, size_t pipe_num) {
  /* TODO: process executing as the same job, e.g., a single input to the shell as a pipeline, are placed in the same group. 
  Also, the choice of process group id is the pid of the process. */

  /* Number of processes */
  int p_num = pipe_num + 1;
  int pipe_arr[pipe_num][2];

  /* Initialize n-1 pipes */
  for (int i = 0; i < pipe_num; i++) {
    if (pipe(pipe_arr[i]) != 0) {
      perror("pipe");
      return;
    }
  }

  int end_previous_token_sequence = -1;
  int tokens_length = tokens_get_length(tokens);
  for (int i = 0; i < p_num; i++) {
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      return;
    }

    if (pid == 0) {
      if (i > 0) {
        dup2(pipe_arr[i - 1][0], STDIN_FILENO);
      }

      if (i < p_num - 1) {
        dup2(pipe_arr[i][1], STDOUT_FILENO);
      }

      /* Close all pipe FDs in child */
      for (int j = 0; j < pipe_num; j++) {
        close(pipe_arr[j][0]);
        close(pipe_arr[j][1]);
      }

      int s = end_previous_token_sequence + 1;
      int e = s;

      while (e < tokens_length && strcmp(tokens_get_token(tokens, e), "|") != 0) {
        e++;
      }

      char* file_name_in = NULL;
      bool redirect_stdin = false;
      char* file_name_out = NULL;
      bool redirect_stdout = false;

      size_t argv_index = 0;
      char* argv[128];

      for (int k = s; k < e; k++) {
        char* token = tokens_get_token(tokens, k);
        if (strcmp(token, "<") == 0 && k + 1 < e) {
          file_name_in = tokens_get_token(tokens, ++k);
          redirect_stdin = true;
        } else if (strcmp(token, ">") == 0 && k + 1 < e) {
          file_name_out = tokens_get_token(tokens, ++k);
          redirect_stdout = true;
        } else {
          argv[argv_index++] = token;
        }
      }

      argv[argv_index] = NULL;

      if (argv[0] == NULL) {
        printf("missing program argument \n");
        return;
      }

      char* program_path = argv[0];
      bool resolved_program_path = false;
      if (access(argv[0], X_OK) != 0) {
        program_path = resolve_program_path(argv[0]);
        if (program_path == NULL) {
          printf("execute_program: unable to find program %s\n", argv[0]);
          exit(1);
        }
        resolved_program_path = true;
      }

      if (redirect_stdin) {
        int fd = open(file_name_in, O_RDONLY);
        if (fd < 0) {
          printf("execute_program: failed to open stdin file %s\n", file_name_in);
          exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
      }

      if (redirect_stdout) {
        int fd = open(file_name_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
          printf("execute_program: failed to open stdout file %s\n", file_name_out);
          exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
      }

      execv(program_path, argv);
      perror("execv failed");
      if (resolved_program_path)
        free(program_path);
    }

    while (end_previous_token_sequence + 1 < tokens_length &&
           strcmp(tokens_get_token(tokens, end_previous_token_sequence + 1), "|") != 0) {
      end_previous_token_sequence++;
    }
    end_previous_token_sequence++;
  }

  for (int i = 0; i < pipe_num; i++) {
    close(pipe_arr[i][0]);
    close(pipe_arr[i][1]);
  }

  for (int i = 0; i < p_num; i++) {
    wait(NULL);
  }
}

int main(unused int argc, unused char* argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens* tokens = tokenize(line);

    size_t pipe_num = get_num_of_pipes(tokens);
    if (pipe_num == 0) {
      execute_program(tokens);
    } else {
      execute_with_pipes(tokens, pipe_num);
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
