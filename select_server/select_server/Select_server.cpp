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
// fd_set������ >>  fd0 = stdin fd1 = stdout fd2 = stderr fd3 = socket(������)
//

//typedef struct fd_set { (������ �� fdset)
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
  struct timeval timeout; // select�Լ��� ��ȭ�� ���ܾ� return�ϴµ� ��ȭ�� ������������ ���� blocking���¿� �ӹ��� ������ timeout �ð��� ����
  fd_set reads, cpyreads; // ���� ��ũ������ ����
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

  FD_ZERO(&reads); // fd_set ���� �ʱ�ȭ
  FD_SET(servSock, &reads); // ������ ���ſ��θ� �����ϴ� ��� �������� ���� ( ������������ ���ŵ� �����Ͱ� �ִ� = Ŭ���̾�Ʈ�κ��� �����û�� �ִ�)

  cout << "servsock num : " << servSock << endl;
  cout << "reads.fd_array[0] : " << reads.fd_array[0] << endl;
  cout << "fdcount : " << reads.fd_count;
  //cout << reads.fd_array[1];
  while (1) {
    cpyreads = reads; // ���� ������ ���� ��ũ���� ī��

    timeout.tv_sec = 5;
    timeout.tv_usec = 5000; // microsecond

    if ((select_result = select(0, &cpyreads, 0, 0, &timeout)) == SOCKET_ERROR) { // select���� -1 select read�� ���� 
      //***��ȭ�� ���� ��ũ���� �����ϰ� 0�����ʱ�ȭ�� cpyreads�������� ��) 0 0 0 1 0 0.... ��ȭ�Ȱ͸� ã�´�!
      err_handling("select err");
      break;
    }

    if (select_result == 0) { // timeout
      printf("TIMEOUT\n");
      continue;
    }
    // select �Լ��� 1 �̻����� ��ȯ
    for (int i = 0; i < reads.fd_count; i++) {
      if (FD_ISSET(reads.fd_array[i], &cpyreads)) { //  ��ȭ�� �ִ� ���ϵ�ũ���� ã��(cpyreads���� ����!��ȭ�� ��ũ���͸� �ִ�)
        if (reads.fd_array[i] == servSock) { // ��ȭ�� �����϶� ( Ŭ���̾�Ʈ ���� �õ�)
          printf("reads.fd_array[%d] = %d\n", i, reads.fd_array[i]);
          int size = sizeof(clntAddr);
          clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &size);
          FD_SET(clntSock, &reads); // fd_set�� ���� reads�� clntsock ���� ��ũ���� ���� ���
          printf("connected client = %d\n", clntSock);
        }
        else { //��ȭ�� ������ �ƴҶ� ��, ����� Ŭ���̾�Ʈ�κ��� �����͸� ������
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