 /*
 
 ttoro
 
 
 */
 
 #include <stdio.h>
 #include <unistd.h>
 #include <ctype.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <stdlib.h>
 #include <string.h>
 
 
 const int MAXLINE = 256; // 버퍼 크기는 256으로 설정
 
 void ToUp(char *p) // 소문자를 대문자로 바꿔주는 함수
 {
     while(*p)
     {
         *p = toupper(*p); // ctype.h 파일에서 지원하는 함수
         p++; // 문자 하나하나 접근하여 대문자로 바꿔줌
     }
 }
 
 int main(int argc, char *argv[])
 {
     int fd[2], fd2[2], fd3[2]; // 3개의 파이프를 만듬
     pid_t pid; // p - q fork() 받아오기위함
     pid_t pid2; // q - r fork() 받아오기 위함
     FILE* input; // 첫 인자 파일을 열기 위한 파일 fd 변수
     FILE* outer; // 출력 파이프를 위한 fd 변수
     int n; // 단어 수를 확인하기 위한 변수 n
     char line[MAXLINE], output[MAXLINE]; // 파일에서 문장을 받아올 변수, 파이프에서 문장을 받아올 변수
 
     pipe(fd); // p - q 파이프
     pipe(fd2); // q - r 파이프
     pipe(fd3); // r - p 파이프
 
     input = fopen(argv[1], "r"); // 파일을 read 모드를 열어 input에 fd를 넣어둠
 
     if ((pid = fork()) > 0 ) // 부모일 때
     {
         close(fd[0]); // p-q read 닫음
         close(fd2[0]); // q-r read 닫음
         close(fd2[1]); // q-r write 닫음
         close(fd3[1]); // r-q write 닫음
 
         while(fgets(line, MAXLINE, input) != NULL){ // 줄 별로 읽어옴
             n = strlen(line); // 문장 단어 수
             if((write(fd[1], line, n) != n)){ // 문장만큼 못 읽어 올 경우

             printf("부모 - 파이프에 내용을 쓸 수 없습니다!\n");
 
             }
         }
         fclose(input); // 더 이상 파일 fd는 필요 없으므로 close
			close(fd[1]); // p - q 에서 write pipe도 더 이상 필요 없으므로 close
 
         waitpid(pid, NULL, 0); // 부모 프로세스는 자식 프로세스가 종료될 때 까지 기다린다.
 
         outer = fdopen(fd3[0], "r"); // r - p 에서 read pipe를 read 모드로 fd를 얻어옴
 
         while(fgets(output, MAXLINE, outer) != NULL){ // 줄 별로 데이터를 읽어옴
             printf("%s", output); // stdout에 출력
         }
         fclose(outer); // 더 이상 r - p 의 read pipe fd 가 필요 없으므로 close
         close(fd3[0]); // r - p 의 read pipe가 필요 없으므로 close
 
         printf("Goodbye from P\n"); // 부모 프로세스 종료
 
     }
 
     else // 자식 프로세스일 때
     {
 
         if((pid2 = fork()) > 0 ) // 자식과 자손 관계에서 자식일 때
         {
             FILE* first; // p - q 에서 read pipe의 fd를 담을 변수
             char temp[MAXLINE]; // 문장을 읽어와 저장할 변수
             int linecheck; // 읽어 온 문장의 수를 셀 함수
 
             fclose(input); // 처음 파일 fd도 상속 받기 때문에 닫아 둠
             close(fd[1]); // p - q write
             close(fd2[0]); // q - r read
             close(fd3[0]); // r - p read
             close(fd3[1]); // r - p write
 
             first = fdopen(fd[0], "r"); // p - q 에서 read pipe를 read모드로 열고 fd를 저장
 
             while(fgets(temp, MAXLINE, first) != NULL){ // 줄 별로 데이터를 읽어옴
                 linecheck = strlen(temp); // 읽어온 문장의 길이를 확인
                 ToUp(temp); // 각 문장을 대문자로 바꿔주는 함수를 통해 대문자로 바꿔줌
                 write(fd2[1], temp, linecheck); // 읽어온 문장의 길이 만큼 q - r write pipe에 쓴다.
             }
             fclose(first); // 더 이상 p - q read pipe의 fd가 필요 없으므로 close
             close(fd[0]); // 더 이상 p -  q read pipe가 필요 없으므로 close
             close(fd2[1]); // 더 이상 q - r write pipe가 필요 없으므로 close
 
             waitpid(pid2, NULL, 0); // 자손 프로세스가 종료될 때 까지 대기
 
             printf("Goodbye from Q\n"); // 자식 프로세스 종료
             exit(2);
 
         }
         else // 자손 프로세스일 때
         {
             char tmp[MAXLINE]; // 문장을 읽어와 저장할 변수
             int begin_word = 0; // 단어인지 확인하기 위한 정수
             int i = 0; // 단어 확인 반복문을 위한 변수
             int l = 0; // 전체 line 수를 저장할 변수
             int count = 0; // 읽어들인 문장의 길이를 저장할 변수
             int word = 0; // 읽어들인 전체 단어의 수를 저장할 변수
             FILE *rread; // q - r 의 read pipe의 fd를 저장할 변수
 
             fclose(input); // 파일 fd도 상속 받으므로 닫아줌
             close(fd[0]); // p - q read
             close(fd[1]); // p - q write
             close(fd2[1]); // q - r write
             close(fd3[0]); // r - p read
 
             rread = fdopen(fd2[0], "r"); // r - q read pipe 를 read 모드로 열어 fd를 얻음
 
             while(fgets(tmp, MAXLINE, rread) != NULL) // 줄 별로 읽어 들임
             {
                 count = strlen(tmp); // 읽어 들인 문장의 길이를 구함
                 for(i = 0; i < count; i++)
                 {
                     if(tmp[i] == ' ' || tmp[i] == '\n' || tmp[i] == '\n') // 각 라인 별 공백 줄 바꿈 이후 단어를 센다.
                     {
                         if(begin_word){
                             word++; // 단어 수 증가
                             begin_word = 0; // 단어 시작 상태  초기화
                         }
                     }
                     else{
                         if(!begin_word) // 단어 시작 상태를 1로 바꿈
                             begin_word = 1;
                     }
                 }
                 l += 1; // line 전체 라인 수를 구함
                 write(fd3[1], tmp, count); // 읽어 들인 문장을 읽어 들인 문장의 길이만큼 r - p write pipe에 쓴다.
             }
             sprintf(tmp, "단어 수 : %d, 문장 수 : %d\n", word, l); // sprintf 를 사용하여 단어 수와 문장 수를 tmp에 적는다.
             write(fd3[1], tmp, strlen(tmp)); // 문장의 가장 마지막에 쓸 내용을 r - p write pipe에 tmp 길이 만큼 적는다.
 
             fclose(rread); // 더 이상 q - r read pipe의 fd를 사용하지 않으므로 종료
             close(fd2[0]); // 더 이상 q - r read pipe를 사용하지 않으므로 종료
             close(fd3[1]); // 더 이상 r - p write pipe를 사용하지 않으므로 종료
 
             printf("Goodbye from R\n"); // 자손 프로세스 종료
 
             exit(3);
 
         }
     }
     return 0;
 }