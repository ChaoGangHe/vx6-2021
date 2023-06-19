#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MSGSIZE 16
int main(int argv,char* argc[])
{
    char buf[MSGSIZE];
    int fd[2];
    pipe(fd);

    int pid = fork();
    if(pid>0){
        //printf("我的父进程\n");
        write(fd[1],"ping",4);
        wait(0);
        read(fd[0],buf,MSGSIZE);
        printf("父进程收到子进程的:%s\n",buf);
    }else{
        //printf("我是子进程\n");
        read(fd[0],buf,MSGSIZE);
        printf("子进程收到了父进程：%s\n",buf);
        write(fd[1],"pong",4);
    }
    return 0;
}