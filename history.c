#include "history.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct history{
  unsigned int history_size;
  unsigned int history_index;
  char **history;
};
  
History* history__init(){
  unsigned int history_size = 10;
  History* self = (History*) malloc (sizeof(History));
  assert(self);

  self -> history = (char**) malloc (sizeof(char*) * history_size);
  assert(self -> history);
  self -> history_size = history_size;
  self -> history_index = 0;
  return self;
}

int history__index(History const *self){
  if (self)
      return self -> history_index;
  return 0;
}

int history__size(History const *self){
  if (self)
    return self -> history_size;
  return 0;
}

char* history__get(History *self, unsigned int const index){
  if (!self || index >= self -> history_index)
    return NULL;
  return self -> history[index];
}

void history__add(History *self, char const *to_add){
  if (!self || !to_add){
    perror("Cannot add to history");
    return;
  }

  if (self -> history_index == self -> history_size){
    self -> history_size *= 2;
    self -> history = (char**) realloc(self -> history, sizeof(char*) * self -> history_size);
    assert(self -> history != NULL);
  }
  int size = strlen(to_add);
  self -> history[self -> history_index] = (char*) malloc ((size + 1) * sizeof(char));
  strcpy(self -> history[self -> history_index++], to_add);
}

void history__remove(History *self){
  free(self -> history[--self -> history_index]);
}

int history__write(History *self, char const *name){
  if (!self)
    return 0;

  int name_size = strlen(name);
  char *file_name = (char*) malloc ((name_size + 1) * sizeof(char));
  strcpy(file_name, name);
  strcat(file_name, ".txt");
  
  FILE *fp;
  fp = fopen(file_name, "w");
  free(file_name);
  if (!fp){
    perror("Cannot open file");
    return 0;
  }

  if (!fp){
    perror("Cannot open file");
    return 0;
  }
  
  for (int i = 0; i < self -> history_index; i++)
    fprintf(fp, "%s\n", self -> history[i]);
    
  fclose(fp);
  return 1;
}

History* history__load(History *self, char const *name){

  int name_size = strlen(name);
  char *file_name = (char*) malloc ((name_size + 5) * sizeof(char));
  strcpy(file_name, name);
  strcat(file_name, ".txt");
  FILE *fp;
  fp = fopen(file_name, "r");
  free(file_name);
  if (!fp){
    return history__init();
  }

  char c;
  int buffer_size = 20;
  int buffer_pointer = 0;
  char * buffer = (char*) malloc(sizeof(char) * (buffer_size));
  if (buffer == NULL){
    perror("Cannot load buffer");
    return NULL;
  }

  History *tmp = history__init();
  if (!tmp){
    perror("Cannot create new history");
    return NULL;
  }
  else{
    history__destroy(self);
    self = tmp;
  }

  //void *result = malloc(sizeof(char));
  do{
    c = (char) fgetc(fp);
    if (buffer_pointer == buffer_size){
	buffer = realloc(buffer, sizeof(char) * buffer_size * 2);
	assert(buffer != NULL);
     }
    
    if (c != '\n')
      buffer[buffer_pointer++] = c;
    
    else{
      buffer[buffer_pointer] = '\0';
      history__add(self, buffer);
      buffer_pointer = 0;
    }

    if (feof(fp))
      break;
    
  }while(1);

  fclose(fp);
  free(buffer);
  return self;
}

void history__destroy(History* self){
  if (!self)
    return;
  
  for (int i = 0; i < self -> history_index; i++)
    if (self -> history[i])
      free(self -> history[i]);
  free(self);
}

void history__print(History const *self){
  if (!self)
    return;
  
  printf("History: index %d, size %d\n", self -> history_index, self -> history_size);
  for (int i = 0; i < self -> history_index; i++)
    printf("%d: %s\n", i, self -> history[i]);
}

void history__print_backwards(History const *self){
  if (!self)
    return;

  printf("History: index %d, size %d\n", self -> history_index, self -> history_size);
  for (int i = self -> history_index - 1; i >= 0; i--)
    printf("%d: %s\n", self -> history_index - i - 1, self -> history[i]);
}
