#include "history.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>

char *home = "/";
char *currentdir;
History *cmd_history;
char cmd_file[] = "cmd";
History *pid_history;
char pid_file[] = "pid";

void init(){
  currentdir = (char*) malloc (sizeof(char) * (strlen(home) + 1));
  strcpy(currentdir, home);
  cmd_history = history__load(cmd_history, cmd_file);
  pid_history = history__load(pid_history, pid_file);
  assert(currentdir && cmd_history && pid_history);
}

void byebye(){
  history__write(cmd_history, cmd_file);
  history__write(pid_history, pid_file);
  printf("byebye\n");
}

char *input_string(FILE *fp){
  int size = 10;
  char *str = (char*) malloc (sizeof(size * sizeof(char)));
  int str_counter = 0;
  char c;
  assert(str != NULL);

  while((c = fgetc(fp)) != '\n'){
    assert(c != EOF);
    if (str_counter == size){
      size *= 2;
      str = (char*) realloc(str, sizeof(char) * size);
      assert(str != NULL);
    }
    str[str_counter++] = c;
  }

  if (str_counter == size){
    size *= 2;
    str = (char*) realloc(str, sizeof(char) * size);
    assert(str != NULL);
  }

  str[str_counter++] = '\0';
  return str;
}

int checkdir(char* directory){
  DIR* dir = opendir(directory);
  if (dir)
    return 1;
  else
    return 0;
}

void movetodir(char const* const directory){
  char *actualdir;
  if (!directory)
    perror("Invalid directory");
  else if (directory[0] == '/'){
    actualdir = (char*) malloc (sizeof(char) * (strlen(directory) + 1));
    actualdir[0] = '\0';
    strcpy(actualdir, directory);
  }
  else if (!strcmp(currentdir, "/")){
    actualdir = (char*) malloc (sizeof(char) * (1 + strlen(directory) + 1));
    actualdir[0] = '/';
    actualdir[1] = '\0';
    strcat(actualdir, directory);
  }
  else{
    actualdir = (char*) malloc (sizeof(char) * (strlen(currentdir) + 1 + strlen(directory) + 1));
    actualdir[0] = '\0';
    strcat(strcat(strcat(actualdir, currentdir), "/"), directory);
  }
  if (checkdir(actualdir)){
    char *tmp = currentdir;
    currentdir = actualdir;
    free(tmp);
  }
  else{
    printf("Error, directory not found\n");
    free(actualdir);
  }
}

void whereami(){
  printf("%s\n", currentdir);
}

void parse_history(char const **argv){
  if (!strcmp(argv[0], "history")){
    if (argv[1] && !strcmp(argv[1], "[-c]")){
      history__destroy(cmd_history);
      cmd_history = history__init();
    }
    else
      history__print_backwards(cmd_history);
  }
}

// email professor of dual process in the same termnial
// can you show me how to make a terrabyte of random data
int run(char *const *argv){
  int pid = fork();
  if (pid > 0){
    int length = snprintf(NULL, 0, "%d", pid);
    char *pid_str = (char *) malloc (sizeof(char) * (length + 1));
    assert(pid_str != NULL);
    snprintf(pid_str, length + 1, "%d", pid);
    history__add(pid_history, pid_str);
    return pid;
  }
  else if (pid  == 0)
    execvp(argv[0], argv);
  perror("exec failed");
  return 0;
}

int run_wait(char *const *argv){
  int status;
  pid_t pid = run(argv);
  if (pid < 0)
    return pid;
  
  if (waitpid(pid, &status, 0) == -1){
    perror("waitpid error");
    exit(EXIT_FAILURE);
  }
  history__remove(pid_history);
  return pid;
}

int startProgram(char *const *argv, char *option){
  if ((strcmp(option, "BACKGROUND") && strcmp(option, "FOREGROUND")) | !argv[1]){
    perror("Invalid startProgram");
    return 0;
  }
  int argv_length = 0;
  while(argv[argv_length])
    argv_length++;
  argv_length++;
  char **argv2 = (char **) malloc(sizeof(char *) * (argv_length - 1));
  assert(argv2);
  for (int i = 0; i < argv_length - 1; i++){
    argv2[i] = argv[i + 1];
  }
  if (argv[1][0] == '/'){
    if (!strcmp(option, "FOREGROUND"))
      return run_wait(argv2);
    else
      return run(argv2);
  }
  else{
    int length = strlen(currentdir) + 1 + strlen(argv2[0]) + 1;
    char *program = (char*) malloc (sizeof(char) * length);
    assert(program);
    strcat(strcat(strcat(program, currentdir), "/"), argv2[0]);
    argv2[0] = program;
    history__remove(cmd_history);
    char *input_str;
    if (!strcmp(option, "BACKGROUND")){
      input_str = (char*) malloc (sizeof(char) * (sizeof("background ") + 1));
      strcpy(input_str, "background ");
    }
    else{
      input_str = (char*) malloc (sizeof(char) * (sizeof("start ") + 1));
      strcpy(input_str, "start");
    }
    argv_length = 0;
    while(argv2[argv_length]){
      input_str = (char*) realloc (input_str, (strlen(input_str) + 1 + strlen(argv2[argv_length]) + 1) * sizeof(char));
        strcat(strcat(input_str, " "), argv2[argv_length++]);
    }
    history__add(cmd_history, input_str);
    free(input_str);

    if (!strcmp(option, "FOREGROUND"))
      return run_wait((char *const *)argv2);
    else
      return run((char *const *)argv2);
  }
}

char ** parse_args(char *input_str){
  char **argv = (char**) malloc (sizeof(char*));
  int argv_counter = 0;
  char *token = strtok(input_str, " ");
  if (!token){
    free(argv);
    perror("No programm given");
    return NULL;
  }
  argv[0] = token;
  do{
    argv = (char**) realloc (argv, sizeof(char*) * (++argv_counter + 1));
    token = strtok(NULL, " ");
    argv[argv_counter] = token;
  } while(token);
  return argv;
}

int parse(char *input_str){
  if (!strcmp(input_str, ""))
    return 0;
  history__add(cmd_history, input_str);
  char const **argv = (char const **) parse_args(input_str);
  if (!strcmp(argv[0], "history"))
    parse_history(argv);
  else if (!strcmp(argv[0], "byebye")){
    byebye();
    exit(EXIT_SUCCESS);
  }
  else if (!strcmp(argv[0], "whereami"))
    whereami();
  else if (!strcmp(argv[0], "start"))
    startProgram((char**) argv, "FOREGROUND");
  else if (!strcmp(argv[0], "background"))
    printf("NEW PID: %d\n", startProgram((char**) argv, "BACKGROUND"));
  else if (!strcmp(argv[0], "movetodir")){
    if (!argv[1])
      printf("Error no directory given");
    else
      movetodir((char const* const) argv[1]);
  }
  else if (!strcmp(argv[0], "replay")){
    if (!argv[1]){
     printf("replay: not enough arguments");
     return 0;
    }
   long index = strtol(argv[1], NULL, 10) + 1;
   if (index == 0 && strcmp(argv[1], "0"))
     printf("replay: invalid argument");
   else{
     index = history__index(cmd_history) - index - 1;
     char *cmd = history__get(cmd_history, index);
     printf("%s\n", cmd);
     if(cmd)
       parse(cmd);
   }
  }
  else if (!strcmp(argv[0], "dalek")){
    pid_t index = (pid_t) strtol(argv[1], NULL, 10);
    printf("%d\n", kill(index, SIGKILL));
  }
  else{
    printf("unkown command\n");
  }
  free(argv);
  return 1;
}

int parse_prompt(){
  printf("# ");
  return parse(input_string(stdin));
}

int main(){
  init();
  while(1){
    parse_prompt();
  }
  return 0;
}
