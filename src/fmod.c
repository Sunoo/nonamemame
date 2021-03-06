#include "fmod.h"
#include "fmoddyn.h"
#include "fmod_errors.h"
#include "wincompat.h"
#include <windows.h>
#include <conio.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/*
Play a sample file using the FMOD lib ... mp2 mp3 wav ogg wma asf
PlayFile(filename, looping, start point, end point);
*/
void PlayFile(int channel, char file[500], int looping, int start, int stop){
FSOUND_STREAM *stream;
FSOUND_SetOutput(FSOUND_OUTPUT_WINMM);
    if (!FSOUND_Init(44100, 16, 0))
    {
        printf("Error!\n");
        printf("%s\n", FMOD_ErrorString(FSOUND_GetError()));
        FSOUND_Close();
        return;
    }
	if(looping){
	stream = FSOUND_Stream_Open(file, FSOUND_LOOP_NORMAL , 0, 0);
	FSOUND_Stream_SetLoopPoints(stream, start, stop);
	}else{
	stream = FSOUND_Stream_Open(file, FSOUND_LOOP_OFF, 0, 0);
	}
FSOUND_Stream_Play(channel, stream);
}

/*
  StopFile(channel to be stopped);
*/
signed char StopFile(int channel){
return FSOUND_StopSound(channel);
}


int main(int argc, char *argv[]){
FILE *fp;
fp = fopen(argv[1], "rb");
  if(fp){
  fclose(fp);
  PlayFile(0, argv[1], 1, 0, 0);
  while(!kbhit()){Sleep(10);}
  }else{
  printf("Uso: fmod [arquivo mp2 mp3 wav ogg wma asf]\n");
  }
}




 