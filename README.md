# Remote_Development_Kit

## 사용법
`code.c` : 실행할 c파일 (예시의 경우 두 정수를 입력받아 더하는 코드)

`server.c` : 원격으로 code.c를 실행할 리눅스에서 실행

`client.c` : 사용자 pc에서 실행

<br>

`client`
```
$ ./client <IP> <PORT> <code>.c


## server에 연결되면 터미널 입력

$ 1 2


## 실행한 내용이 redirect되어 출력

$ result: 3


## 수행한 프로그램 종료시

$ session closed


## 다시 터미널로 복귀

$ _
```

<br>

`server`
```
$ ./server <PORT>


## client 연결 대기

$ wait for client ...

$ client connected


## client로부터 받은 코드 실행중


## 원격 실행 프로그램 종료시

$ session closed


## client 연결 대기

$ wait for client ...
```

## 주의사항

`server.c`는 스레드를 포함하므로 컴파일 시 주의

본 프로젝트는 라즈베리파이의 리눅스 환경에서 정상적으로 실행됩니다.