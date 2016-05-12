#pragma comment (lib,"ws2_32.lib")
#include <iostream>
#include <stdio.h>
#include <WinSock2.h>

using namespace std;
/*
* Structure used in select() call, taken from the BSD file sys/time.h.
*/
//struct timeval {
//  long    tv_sec;         /* seconds */
//  long    tv_usec;        /* and microseconds */
//};
// fd_set변수형 >>  fd0 = stdin fd1 = stdout fd2 = stderr fd3 = socket(리눅스)
//

//typedef struct fd_set { (윈도우 형 fdset)
//  u_int fd_count;               /* how many are SET? */
//  SOCKET  fd_array[FD_SETSIZE];   /* an array of SOCKETs */
//} fd_set;

void err_handling(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
void main() {
  WSADATA wsaData;
  SOCKET servSock, clntSock;
  SOCKADDR_IN servAddr, clntAddr;
  struct timeval timeout; // select함수는 변화가 생겨야 return하는데 변화가 생기지않으면 무한 blocking상태에 머물기 때문에 timeout 시간을 지정
  fd_set reads, cpyreads; // 소켓 디스크립터의 범위
  int select_result;
  char buf[BUFSIZ];

  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    err_handling("wsastartup err");
  }

  servSock = socket(PF_INET, SOCK_STREAM, 0);
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(9000);

  if (bind(servSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
    err_handling("bind() err");
  }
  if (listen(servSock, 5) == SOCKET_ERROR) {
    err_handling("listen() err");
  }

  FD_ZERO(&reads); // fd_set 변수 초기화
  FD_SET(servSock, &reads); // 데이터 수신여부를 관찰하는 대상에 서버소켓 포함 ( 서버소켓으로 수신된 데이터가 있다 = 클라이언트로부터 연결요청이 있다)

  cout << "servsock num : " << servSock << endl;
  cout << "reads.fd_array[0] : " << reads.fd_array[0] << endl;
  cout << "fdcount : " << reads.fd_count;
  //cout << reads.fd_array[1];
  while (1) {
    cpyreads = reads; // 원본 보존을 위한 디스크립터 카피

    timeout.tv_sec = 5;
    timeout.tv_usec = 5000; // microsecond

    if ((select_result = select(0, &cpyreads, 0, 0, &timeout)) == SOCKET_ERROR) { // select에러 -1 select read만 관찰 
      //***변화가 생긴 디스크립터 제외하고 0으로초기화됨 cpyreads쓰는이유 예) 0 0 0 1 0 0.... 변화된것만 찾는다!
      err_handling("select err");
      break;
    }

    if (select_result == 0) { // timeout
      printf("TIMEOUT\n");
      continue;
    }
    // select 함수가 1 이상으로 반환
    for (int i = 0; i < reads.fd_count; i++) {
      if (FD_ISSET(reads.fd_array[i], &cpyreads)) { //  변화가 있는 소켓디스크립터 찾기(cpyreads에는 현재!변화한 디스크립터만 있다)
        if (reads.fd_array[i] == servSock) { // 변화가 서버일때 ( 클라이언트 연결 시도)
          printf("reads.fd_array[%d] = %d\n", i, reads.fd_array[i]);
          int size = sizeof(clntAddr);
          clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &size);
          FD_SET(clntSock, &reads); // fd_set형 변수 reads에 clntsock 소켓 디스크립터 정보 등록
          printf("connected client = %d\n", clntSock);
        }
        else { //변화가 서버가 아닐때 즉, 연결된 클라이언트로부터 데이터를 받을때
          int recv_size;
          printf("reads.fd_array[%d] = %d\n", i, reads.fd_array[i]);
          recv_size = recv(reads.fd_array[i], buf, BUFSIZ, 0);
          if (recv_size == 0) {

            FD_CLR(reads.fd_array[i], &reads);
            closesocket(cpyreads.fd_array[i]);
            printf("closed client %d \n", cpyreads.fd_array[i]);
          }
          else {
            send(reads.fd_array[i], buf, BUFSIZ, 0);
          }
        }
      }
    }

  } // while

  closesocket(servSock);
  WSACleanup();
}