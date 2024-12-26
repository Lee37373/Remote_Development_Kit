#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define CLIENT_MAX 5	//최대 접속대기 가능한 클라이언트수

int serv_sock;	//서버소켓 파일디스크립터
struct sockaddr_in serv_addr;	//서버 주소
int clnt_sock[CLIENT_MAX];	//클라이언트 소켓 파일디스크립터
struct sockaddr_in clnt_addr[CLIENT_MAX];	//클라이언트 주소
socklen_t clnt_addr_size[CLIENT_MAX];	//소켓 주소 크기
int thr_id, status;	//쓰레드 상태 관련 변수
int pid;	//최초의 pid

//오류 처리
void error_handling(char *message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	end(1);
}

void end(int sig) {	//서버 종료시 실행 함수
	char msg[100];
	if(pid == getpid())
	    for(int i0 = 0; i0 < CLIENT_MAX; i0++)	{	//서버 종료 시 사용한 자원 해제
				close(clnt_sock[i0]);

				//생성한 코드 및 실행파일 삭제
				snprintf(msg, 100, "rm -r tmpfile%d.c 2> /dev/null", i0);
				system(msg);
				snprintf(msg, 100, "rm -r tmpfile%d 2> /dev/null", i0);
				system(msg);
			}

	close(serv_sock);	//사용한 자원 해제
    exit(sig);	//종료
}

void *excode(void *_num) {
	int i0, i1, t0, t1, num = (int *)_num, fd, pid, stdin, stdout, status;
	char c, file[100], msg[100];
	//임시 c코드 파일 생성
	snprintf(file, 100, "./tmpfile%d.c",num);
    fd = open(file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);	//쓰기전용으로 열기,파일이 없으면 생성, rwx권한 부여

	//클라이언트로부터 파일을 1byte씩 전송받는다
	while(1) {
		t0 = read(clnt_sock[num], &c, sizeof(c));
		if(t0 == 0) {	//연결종료시 쓰레드 종료
			clnt_sock[num] = -1;
			pthread_exit(0);
		}
		if(c == 0)	//파일을 다 전송 받음
			break;
        t0 = write(fd, &c, 1);
	}
    close(fd);	//자원 해제

	//컴파일
	snprintf(msg, 100, "gcc -o tmpfile%d tmpfile%d.c", num, num);
	system(msg);

	//표준입출력을 따로 활용하기 위해 fork() 함수를 이용하여 새 프로세스로 분리
	pid = fork();
	if(pid) {	//부모
		//자원 해제
		close(clnt_sock[num]);
		clnt_sock[num] = -1;

		//실행한 프로그램이 종료될때까지 대기
		t0 = waitpid(pid, &status, 0);
		if(t0==-1)
			perror("waid failed");
	}
	else {	//자식
		//원본 stdin, stdout백업
		stdin = dup(STDIN_FILENO);
		stdout = dup(STDOUT_FILENO);

		//클라이언트의 소켓 파일디스크립터와 표준입출력간 데이터가 적절히 전달될 수 있도록 리다이렉션
		dup2(clnt_sock[num], STDIN_FILENO);
		dup2(clnt_sock[num], STDOUT_FILENO);

		//버퍼링 없이 컴파일 한 파일 실행
		snprintf(msg, 100, "stdbuf -o0 ./tmpfile%d", num);
		system(msg);

		//stdin, stdout 복원
		dup2(stdin, STDIN_FILENO);
		dup2(stdout, STDOUT_FILENO);

		//자원 해제
		close(clnt_sock[num]);
		snprintf(msg, 100, "rm -r tmpfile%d.c 2> /dev/null", num);
		system(msg);
		snprintf(msg, 100, "rm -r tmpfile%d 2> /dev/null", num);
		system(msg);

		//종료
		printf("[*] session closed\n");
		end(0);
	}
}
int main(int argc, char *argv[]) {
	int i0, i1, t0, t1;
	pid = getpid();

    //올바르지 않은 argc 실행시 오류 처리
	if(argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
	}

	signal(SIGINT, end);	//서버 강제 종료시 호출 함수

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);	//서버 소켓 생성
	if(serv_sock == -1) error_handling("socket() error");	//소켓 생성 오류 처리

	//서버 주소 설정
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	//바인딩
	if(bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error");

	//연결 요청 최대 대기 수 설정
	if(listen(serv_sock, CLIENT_MAX) == -1) error_handling("listen() error");

	//클라이언트 소켓 초기화
	for(i0 = 0; i0 < CLIENT_MAX; i0++)
		clnt_sock[i0] = -1;

	//클라이언트의 접속을 확인하며 반복적으로 처리
    while(1) {
		//접속 가능한 소켓을 찾고 가득찬 상태일 경우 대기
		for(i0 = 0; i0 < CLIENT_MAX; i0++)
			if(clnt_sock[i0] == -1)
				break;
		if (i0 == CLIENT_MAX) {
			printf("[*] full client ...\n");
			continue;
		}
		printf("[*] wait for client ...\n");

		//클라이언트와 연결
		clnt_addr_size[i0] = sizeof(clnt_addr[i0]);
		clnt_sock[i0] = accept(serv_sock, (struct sockaddr *)&clnt_addr[i0], &clnt_addr_size[i0]);
		if (clnt_sock[i0] == -1) {
			error_handling("accept() error");
			continue;
		}
		printf("[*] client connected\n");

		//연결된 이후 쓰레드로 실행
		int num = i0;
		pthread_t p_thread;
		thr_id = pthread_create(&p_thread, NULL, excode, (void *)num);
		if(thr_id < 0) {
			perror("thread create error : ");
			exit(0);
		}
	}

	return (0);
}

