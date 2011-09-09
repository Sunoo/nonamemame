#include "process.h"

long mcd;

/* NINO */
/* Funcao criada para envio de comandos atraves do arquivo cmd.txt */
static int mpegcmd (char cmd[500]){
FILE *f;
int pid;
pid = getpid();
mcd++;

f = fopen("cmd.txt","w");
  if(f){
  fprintf(f, "%d=%s=%d", pid, cmd, mcd);
  }
fclose(f);
return 0;
}

