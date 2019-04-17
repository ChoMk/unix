#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#define MAX_CMD_GRP 10
#define MAX_CMD_ARG 10
#define MAX_CMD_PIPE 10
const char *prompt = "myshell> ";

char* cmdvector[MAX_CMD_ARG];
char  cmdline[BUFSIZ];
char* cmdgrps[MAX_CMD_GRP];
char* cmdpipe[MAX_CMD_PIPE];
int STDOUTFD;
int STDINFD;

void fatal(char *str){
	perror(str);
	exit(1);
}
int cd(char *path){
	return chdir(path);
}
int makelist(char *s, const char *delimiters, char** list, int MAX_LIST){	
  int i = 0;
  int numtokens = 0;
  char *snew = NULL;

  if( (s==NULL) || (delimiters==NULL) ) return -1;

  snew = s + strspn(s, delimiters);	/* delimiters를 skip */
  if( (list[numtokens]=strtok(snew, delimiters)) == NULL )
    return numtokens;
	
  numtokens = 1;
  
  while(1){
     if( (list[numtokens]=strtok(NULL, delimiters)) == NULL)
	break;
     if(numtokens == (MAX_LIST-1)) return -1;
     numtokens++;
  }
  return numtokens;
}


int makepipe(char *s, const char *delimiters, char**list, int MAX_LIST){

  int i = 0;
  int numtokens = 0;
  char *snew = NULL;
  if( (s==NULL) || (delimiters==NULL) ) return -1;

  snew = s + strspn(s, delimiters);     /* delimiters를 skip */
  if( (list[numtokens]=strtok(snew, delimiters)) == NULL )
    return numtokens;

  numtokens = 1;

  while(1){
     if( (list[numtokens]=strtok(NULL, delimiters)) == NULL)
        break;
     if(numtokens == (MAX_LIST-1)) return -1;
     numtokens++;
  }
  return numtokens;

}// |을 기준으로 리스트를 만들어주는 함수


int makeargv(char *s, const char *delimiters, char**list, int MAX_LIST){
	
  int i = 0;
  int numtokens = 0;
  char *snew = NULL;
  if( (s==NULL) || (delimiters==NULL) ) return -1;

  snew = s + strspn(s, delimiters);     /* delimiters를 skip */
  if( (list[numtokens]=strtok(snew, delimiters)) == NULL )
    return numtokens;

  numtokens = 1;

  while(1){
     if( (list[numtokens]=strtok(NULL, delimiters)) == NULL)
        break;
     if(numtokens == (MAX_LIST-1)) return -1;
     numtokens++;
  }
  return numtokens;

}

void execute_cmdgrp(int makeargvCount){
	
	execvp(cmdvector[0], cmdvector);
	fatal("exec error");
	

}
int execute_cmdline(char* cmdline){
	int count = 0;
	int i =0;
	int k =0;
	int makeargvCount = 0;
	int makepipeCount = 0;
	pid_t pid =0;
	pid_t ppid =0;
	count = makelist(cmdline, ";", cmdgrps, MAX_CMD_GRP);
	int pfd[2];
	for(i = 0; i<count; ++i){
		makepipeCount = makepipe(cmdgrps[i], "|", cmdpipe, MAX_CMD_PIPE);//|을 기준으로 나누어 주자.
		for(k=0; k<makepipeCount; k++){
		int continueVal =0;
		makeargvCount = makeargv(cmdpipe[k], " \t", cmdvector, MAX_CMD_ARG);//|을 기준으로 나누어진 배열을 다시 공백으로 나누자
	
		int j = 0;
		int fd = 0;
		for(j = 0; j<makeargvCount; j++){
			if(cmdvector[j] != NULL && (strcmp(cmdvector[j], "<")==0)){//표준 입력을 변경하고자 할 때
				if((fd = open(cmdvector[j+1], O_RDONLY|O_CREAT,0644))<0)//j+1에 들어있는 파일명을 오픈해준다.
					fatal("file open error");
				dup2(fd, STDIN_FILENO);//표준 입력을 오픈한 파일로 바꾼다.
				close(fd);
				cmdvector[j] = NULL;//표준 입력만 변경하면 더이상 필요하지 않기 때문에 null로 채움
				cmdvector[j+1] = NULL;//표준 입력만 변경하면 더이상 필요하지 않기 때문에 null로 채움	
			}else if(cmdvector[j] != NULL && (strcmp(cmdvector[j], ">")==0)){//표준 출력을 변경하고자 할 때
				if((fd = open(cmdvector[j+1], O_WRONLY|O_CREAT, 0644))<0)//j+1에 들어있는 파일명을 오픈해준다.
					fatal("file open error");
				dup2(fd, STDOUT_FILENO);//표준 출력을 오픈한 파일로 바꾼다
				close(fd);
				cmdvector[j] = NULL;//표준 출력만 변경하면 더이상 필요하지 않기 때문에 null로 채움
				cmdvector[j+1] = NULL;//표준 출력만 변경하면 더이상 필요하지 않기 때문에 null로 채움
				
				
			}//strcmp에서 null을 검사할 때 오류가 발생하기 때문에 우선 null인지 확인
		}	
		if(cmdvector[0] != NULL && strcmp(cmdvector[0], "cd") == 0){//strcmp에서 null을 검사할 때 오류가 발생하기 때문에 우선 null인지 확인
			
			if(cd(cmdvector[1])<0)
				perror(cmdvector[1]);
			continue;
		}else if(cmdvector[0] != NULL && strcmp(cmdvector[0], "exit")==0){//strcmp에서 null을 검사할 때 오류가 발생하기 때문에 우선 null인지 확인
			return 1;
		}else if(cmdvector[makeargvCount-1] != NULL && strcmp(cmdvector[makeargvCount-1], "&") == 0){//strcmp에서 null을 검사할 때 오류가 발생하기 때문에 우선 null인지 확인
			cmdvector[makeargvCount-1] = NULL;
			if(k+1 != makepipeCount){//파이프가 필요한지 확인을 하자
                	
			pipe(pfd);//파이프를 만들어주자
			
                	switch(ppid=fork()){
                        	case -1 : fatal("fork error");
                        	case 0 : close(pfd[0]);
					dup2(pfd[1], STDOUT_FILENO);
					signal(SIGQUIT, SIG_DFL);
                                        signal(SIGINT, SIG_DFL);
                                        signal(SIGTSTP, SIG_DFL);
					setpgid(0, 0);
                                        execute_cmdgrp(makeargvCount);
                                        break;//자식의 경우 우선 수행을 하고 그 결과를 파이프로 부모프로세스에게 넘겨주는 방식으로 구현함
                        	default : close(pfd[1]);
					dup2(pfd[0], STDIN_FILENO);//부모는 파이프로 자식이 보낸것을 받을 수 있게 구현하였다.
					//fflush(stdout);
                                        return 0;

                	}
			}
                	else{//파이프가 필요하지 않은 경우는 수행하던 방식으로 수행
			switch(fork()){
				case -1 : fatal("fokr error");
				case 0 :signal(SIGQUIT, SIG_DFL);
					signal(SIGINT, SIG_DFL);
					signal(SIGTSTP, SIG_DFL);
					setpgid(0, 0); 
					execute_cmdgrp(makeargvCount);
					break;
				default :fflush(stdout); 
					return 0;
				



			}
			}
		}else if(cmdvector[0] != NULL){
		if(k+1 != makepipeCount){
                        pipe(pfd);
                        switch(ppid = fork()){
                                case -1 : fatal("fork error");
                                case 0 : close(pfd[0]);
					dup2(pfd[1], STDOUT_FILENO);
                                        signal(SIGQUIT, SIG_DFL);
                                	signal(SIGINT, SIG_DFL);
                                	signal(SIGTSTP, SIG_DFL);//무시하지 않고 수행
                               	 	setpgid(0,0);//현제 자식의 프로세스 아이디로 프로세스 그룹아이디 설정
                                	tcsetpgrp(STDIN_FILENO, getpgid(0));//포그라운드일 때 터미널제어권을 받음
                                 	execute_cmdgrp(makeargvCount);//그리고 실행~
					break;//자식의 경우 우선 수행을 하고 그 결과를 파이프로 부모프로세스에게 넘겨주는 방식으로 구현함
                                default : close(pfd[1]);
					dup2(pfd[0], STDIN_FILENO);
					waitpid(ppid, NULL, 0);
					tcsetpgrp(STDIN_FILENO, getpgid(0));//터미널 제어권한을 돌려 받음
	                                //fflush(stdout);
									//부모는 파이프로 자식이 보낸것을 받을 수 있게 구현하였다.

                        }

                }else{//파이프가 필요하지 않은 경우는 수행하던 방식으로 수행
		switch(pid=fork()){//포그라운드 실행
			case -1 : fatal("fork error");
			case 0 :signal(SIGQUIT, SIG_DFL);
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);//무시하지 않고 수행
				setpgid(0,0);//현제 자식의 프로세스 아이디로 프로세스 그룹아이디 설정
				tcsetpgrp(STDIN_FILENO, getpgid(0));//포그라운드일 때 터미널제어권을 받음
				 execute_cmdgrp(makeargvCount);//그리고 실행~
				break;
				
			default :
				waitpid(pid, NULL, 0);
				tcsetpgrp(STDIN_FILENO, getpgid(0));//터미널 제어권한을 돌려 받음
				fflush(stdout);
		}}
		}
		}

	}
	return 0;
}
void zombie_handler(){
	int status;
	wait(&status);
}//자식이 종료된 경우 그 시체를 수거할 수 있게해주는 핸들러
int main(int argc, char**argv){
  int i=0;
  pid_t pid;
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  act.sa_handler = zombie_handler;
  act.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &act, NULL);//자식이 죽었을 때 좀비핸들러 호출
  signal(SIGINT, SIG_IGN);  
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);//기본 쉘창에서 종료 시그널 무시...
  STDOUTFD = dup(STDOUT_FILENO);//처음 키보드와 모니터에 대한 표준입출력을 저장
  STDINFD = dup(STDIN_FILENO);
  while (1) {
	dup2(STDOUTFD, STDOUT_FILENO);//표준 입출력 복원과정
	dup2(STDINFD, STDIN_FILENO);//표준 입출력 복원과정
       	fflush(stdout);
	fflush(stdin);
	fputs(prompt, stdout);
	fgets(cmdline, BUFSIZ, stdin);
	cmdline[ strlen(cmdline) -1] ='\0';
	if(execute_cmdline(cmdline)==1){
		break;}
	printf("\n");
  }
  return 0;
}
