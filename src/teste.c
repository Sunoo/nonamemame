#include <unistd.h>
#include <process.h>

int main(){
   int pid;
        pid = spawnl(P_NOWAIT, "mpg123.exe","mpg123.exe","wild.mp3",0);
		logerror("MPG123 PID: %d", pid);
		kill(pid, 9);
}
