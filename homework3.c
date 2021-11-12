/*
 
 
 Ttoro
 
 서버 정상 동작
 
 */
 
 
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
 
#include "dict.h"

#define MAX 2000 // max client data length
#define PORT 32653 // server port #
#define BACKLOG 50 // queue length 요청받을 클라이언트 수 50
 
char buffer[MAX]; // 문장을 입력 받을 버퍼
char client_message[2000]; // client로 부터 받은 message
int id = 0; // client id를 저장할 변수
 
Dict d; // dictionary 변수
 
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // mutex lock 생성
 
void *socketThread(void *arg); // thread가 수행할 함수
 
 
int main (int argc, char *argv[])
 {
	int sd, nsd, threadaddrsize; // socket descriptor, 요청에 응답할 socket descriptor, thread 주소 크기
    struct sockaddr_in threadaddr, servaddr; // thread 주소 구조체, 서버 주소 구조체
 
     d = DictCreate(); // dictionary 생성
 
     if (( sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) { // 소켓 생성
         fprintf( stderr, "can’t open socket.\n");
         exit(1);
     }
     // to bind the server itself to the socket
     bzero ((char*) &servaddr, sizeof( servaddr)); // 서버 주소 초기화
     servaddr.sin_family = AF_INET; // 서버 주소는 internet type
     servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // host ip는 127.0.0.1
     servaddr.sin_port = htons (PORT); // host의 포트 번호
 
     if (bind (sd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) { // socket descriptor와 서버 주소를 연결
         fprintf (stderr, "can’t bind to socket.\n");
         exit(1);
     } // bind itself to the socket
 
     listen (sd, BACKLOG); // 요청을 받을 때 대기열 크기를 설정(최대 50개)
 
     pthread_t tid[50]; // 50개 쓰레드 변수
     int i = 0;
 
     while (1) { // a typical server waiting loop
         threadaddrsize = sizeof(threadaddr); // thread  주소 구조체의 크기를 담음
         if (( nsd = accept (sd, (struct sockaddr *) &threadaddr, &threadaddrsize)) < 0) { // thread 주소 구조체를 연결
             fprintf (stderr, "can’t accept connection.\n"); // 성공 시 nsd = client에 응답용 socket descriptor 생성
             exit(1);
         } // upon return: client addr. is known and a new socket is created
         if(pthread_create(&tid[i++], NULL, socketThread, &nsd) != 0){
             printf("Failed to create thread!\n");
         } // thread 생성
 
         if(i >= 50){ // 쓰레드가 50개 이상일 경우
             i = 0;
             while(i < 50){
                 pthread_join(tid[i++], NULL); // 다른 thread 종료를 기다린다.
             }
             i = 0;
         }
     }
     close(sd); // 사용하지 않는 소켓은 닫는다.
     close(nsd);// 더 이상 사용하지 않는 소켓은 닫는다.
 } /* main */
 
 void *socketThread(void *arg) // thread가 수행할 함수
 {
     int myid, bytes; // 내 client id, 보낼 데이터 크기
     char thread_buffer[MAX]; // 쓰레드 내에서 문자열을 담을 버퍼
 
     int newSocket = *(int*)arg; // 인자로 받은 소켓을 다시 변수에 담는다.
     myid = 0; // 아이디를 식별하기 위한 값 0으로 초기화
 
     while((bytes = recv(newSocket, client_message, 1999, 0)) > 0){ // thread가 종료될때까지 무한 루프돈다.
         pthread_mutex_lock(&lock); // mutex lock을 건다.
         if(myid == 0){ // 내 아이디가 아직 할당되지 않은 경우
             id++; // 1부터 시작하므로 1을 더한다.
             myid = id; // 아이디를 할당한다.
         }
         client_message[bytes] = '\0'; // 받은 문자열 끝에 string 끝을 표시한다.
         printf("client %d : %s\n", myid, client_message); // 요청을 받으면 내 아이디와 client msg를 출력한다.
 
         char *ptr = strtok(client_message, " \n"); // client msg를 공백과 행 변환을 기준으로 분리한다.
         int i = 0; // 분리한 문자열을 관리하기 위한 변수 i
         char *sArr[10] = {NULL,};
         buffer[0] = '\0';
 
         while(ptr != NULL) // 문자열이 분리될 때까지 반복한다.
         {
             sArr[i] = ptr;
             i++;
             ptr = strtok(NULL, " \n"); // sArr[0] : read / sArr[1] : key / sArr[2] : value
         }
         if(sArr[0] == NULL){
             continue;
         }
         if(strcmp(sArr[0], "read") == 0){ // 첫번째 명령어가 read 일 경우
             if(DictSearch(d, sArr[1]) != 0){ // key에 대해서 dictionary에 있는지 확인한다.
                 sprintf(buffer, "%d %s: %s %s\n", myid, sArr[0], sArr[1], DictSearch(d, sArr[1])); // buffer에 형식에 맞게 문자열을 저장한다.
                 bytes = sizeof(buffer); // buffer문자열 길이를 구한다.
                 send(newSocket, buffer, bytes, 0); // newSocket을 통해 buffer의 내용을 보낸다.
 
             }
             else{ // key에 대해 없을 경우
                 sprintf(buffer, "not key value, read Failed\n");
                 bytes = sizeof(buffer);
                 send(newSocket, buffer, bytes, 0); // 실패 내용을 보낸다.
             }
         }
         else if(strcmp(sArr[0], "update") == 0){ // update일 경우
             if(DictSearch(d, sArr[1]) != 0){ // key를 통해 dictionary에 있는지 확인한다.
                 DictUpdate(d, sArr[1], sArr[2]); // 있을 경우 value값을 업데이트 한다.
                 sprintf(buffer, "%d %s OK : %s %s\n", myid, sArr[0], sArr[1], DictSearch(d, sArr[1]));
                 bytes = sizeof(buffer);
                 send(newSocket, buffer, bytes, 0); // buffer에 내용을 보낸다.
             }
             else{ // key에 대해 value가 없는 경우
                 sprintf(buffer, "%d %s Failed\n", myid, sArr[0]);
                 bytes = sizeof(buffer);
                 send(newSocket, buffer, bytes, 0); // 실패 내용을 보낸다.
             }
         }
         else if(strcmp(sArr[0], "insert") == 0){ // insert일 경우
             if(DictInsert(d, sArr[1], sArr[2]) != -1){ // dictionary가 꽉 차있지 않을 경우
                 sprintf(buffer, "%d %s OK : %s %s\n", myid, sArr[0], sArr[1], DictSearch(d, sArr[1]));
                 bytes = sizeof(buffer);
                 send(newSocket, buffer, bytes, 0); // buffer 내용을 보낸다.
             }
             else{
                 sprintf(buffer, "%d %s Failed\n", myid, sArr[0]); // 꽉 찬 경우
                 bytes = sizeof(buffer);
                 send(newSocket, buffer, bytes, 0); // buffer 내용을 보낸다.
             }
         }
         else if(strcmp(sArr[0], "quit") == 0){ // quit가 올 경우
             sprintf(buffer, "quit_ack"); // quit_ack를 buffer에 담는다.
             bytes = sizeof(buffer);
             if(send(newSocket, buffer, bytes, 0) != bytes){ // 종료 메세지를 보낸다.
                 fprintf(stderr, "can't send data.\n");
             }
             pthread_mutex_unlock(&lock); // 쓰레드 종료 시 mutex를 반납한다.
             pthread_exit(NULL); // 쓰레드를 종료한다.
         }
         else{
             sprintf(buffer, "일치하는 명령어가 없음\n"); // 일치하는 명령어가 없는 경우
             bytes = sizeof(buffer);
             send(newSocket, buffer, bytes, 0);
         }
         pthread_mutex_unlock(&lock); // mutex를 돌려준다.
     }
 }
