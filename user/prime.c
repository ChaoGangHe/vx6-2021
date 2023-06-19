#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MSGSIZE 35
#define ONE '1'
#define ZERO '0'
void prime(int pipe_read,int pipe_write)
{
    char buf[MSGSIZE];
    int val = 0;
    read(pipe_read,buf,MSGSIZE);
    for(int i=2;i<MSGSIZE;i++)
    {
        if(buf[i]==ONE)
        {
            val = i;
            break;
        }
    }
    if(val==0)exit(0);
    printf("prime %d\n",val);
    buf[val] = ZERO;
    for(int i=2;i<MSGSIZE;i++)
    {
        if(i%val==0) buf[i]=ZERO;
    }
    int pid = fork();
    if(pid>0) write(pipe_write,buf,MSGSIZE);
    else prime(pipe_read,pipe_write);
}
int main(int agrc,char* argv[])
{
    int fd[2];
    char buf[MSGSIZE];
    pipe(fd);
    for(int i=2;i<MSGSIZE;i++)
    {
        buf[i] = ONE;
    }
    int pid =fork();
    if(pid>0){
        buf[0] = ZERO;
        buf[1] = ZERO;
        write(fd[1],buf,MSGSIZE);
        wait(0);
    }else{
        prime(fd[0],fd[1]);
        wait(0);
    }
    exit(0);
    return 0;
}