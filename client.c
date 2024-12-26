#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_MAX 1<<15    //최대 버퍼 크기

char inputbuffer[BUFFER_MAX];    //입력 버퍼
char outputbuffer[BUFFER_MAX];    //출력버퍼
int sock;    //소켓 파일디스크립터
struct sockaddr_in serv_addr;    //서버 소켓 주소

//종료시 실행 함수
void end() {
    printf("[*] session closed\n");
    close(sock);	//자원해제
    exit(0);
}

//오류 처리
void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

//표준입력을 소켓으로 전달
void *inputFunc() {
    int len, t0;
    while(1) {
        if(fgets(inputbuffer, sizeof(inputbuffer), stdin) != NULL) {    //입력이 들어오면 버퍼에 저장
            len = strlen(inputbuffer);
            t0 = write(sock, inputbuffer, len);    //입력받은 길이만큼 서버로 전달
            if(t0 == 0) {    //서버와의 연결이 끊김
                end();
            }
        }
    }
}

//소켓으로부터 전달될 프로그램의 출력을 표준출력
void *outputFunc() {
    int i0, t0;
    outputbuffer[0] = 0;
    while(1) {
        t0 = read(sock, outputbuffer, sizeof(outputbuffer));    //프로그램의 출력을 버퍼에 저장
        outputbuffer[t0] = 0;    //길이만큼 출력을 위해 0으로 설정
        if(t0 == 0) {    //서버와의 연결이 끊김
            end();
        }
        else if(t0 > 0)    //출력
            printf("%s", outputbuffer);
    }
}

int main(int argc, char *argv[]) {
    int i0, i1, t0, t1, fd, len, thr_id, status;
    char c;
    pthread_t p_thread[2];

    //올바르지 않은 argc 실행시 오류 처리
    if(argc != 4) {
        printf("Usage : %s <IP> <port> code.c\n", argv[0]);
        exit(1);
    }

    //소켓 생성
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1) error_handling("socket() error");

    //서버 주소 설정
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    //서버에 연결 및 실패시 오류처리
    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    //실행할 코드파일을 읽어 서버로 전달
    fd = open(argv[3], O_RDONLY);
    for(i0 = 0; ; i0++) {
        t0 = read(fd, &c, 1);
        if(t0 == 0)
            break;
        t0 = write(sock, &c, 1);
    }
    //파일을 다 읽었음을 서버로 전달
    c = 0;
    t0 = write(sock, &c, 1);
    close(fd);    //자원 해제

    //표준입력을 소켓으로 전달하기 위한 쓰레드 생성 및 실행
    thr_id = pthread_create(&p_thread[0], NULL, inputFunc, NULL);
    if(thr_id < 0) {
        perror("thread create error : ");
        exit(0);
    }
    //소켓으로부터 전달될 프로그램의 출력을 표준출력을 위한 쓰레드 생성 및 실행
    thr_id = pthread_create(&p_thread[1], NULL, outputFunc, NULL);
    if(thr_id < 0) {
        perror("thread create error : ");
        exit(0);
    }

    //프로그램이 바로 종료되지 않도록 쓰레드 종료까지 대기
    pthread_join(p_thread[0], (void **)&status);
    pthread_join(p_thread[1], (void **)&status);

    //종료
    end();

    return (0);
}

