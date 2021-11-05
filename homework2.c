 /*
 
 컴퓨터전자시스템공학부 201702653 이운문
 
 정상동작
 
 */
 #include <time.h>
 #include <sys/wait.h>
 #include <string.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 
 struct record{ // 계좌 정보를 담은 구조체
     char acc_no[6]; // 계좌 번호
     char name[10]; // 계좌 소유자 이름
     int balance; // 소지금
 };
 
 void *operation(void *order); // 쓰레드로 구현할 때 만든 operation 함수 프로세스로 바꾸면서 사용 안함
 int reclock (int fd, int recno, int len, int type); // record lock을 설정하기 위한 함수
 void depositview(struct record *curr, int id, int amount); // 입출 명령어를 보여주는 함수
 void withdrawview(struct record *curr, int id, int amount); // 출금 명령어를 보여주는 함수
 void inquiryview(struct record *curr, int id); // 계좌 정보를 보여주는 함수
 
 int main(int argc, char *argv[])
 {
     FILE *out; // 최종 출력을 위한 file stream 변수
     FILE *infile; // 계좌 정보 파일을 담을 file stream 변수
     FILE *order_fd; // 명령어 파일을 담을 file stream 변수
     pid_t mypid; // 현재 process의 id를 담는 변수
     struct record current; // 현재 record 정보를 출력할 때 구조체 크기만큼 임시로 담는 변수
     int i, n; // 반복문을 위한 임시 변수
     char buffer[100]; // 명령어를 한 줄 읽어올 때 담을 buffer
     int pos, record_pos; // 위치, 구조체 내 acc_no 위치
     int check_op, amount; // 받은 명령어를 확인(하나의 문자로 이뤄짐), 명령어에서 읽은 돈
     int fd; // 계좌 파일 descriptor를 담는 변수
 
     srand((unsigned)time(NULL)); // 난수를 생성을 초기화

     for(i = 0; i < 10; i++) // 반복문을 10번 반복하여 10개의 자식 프로세스를 생성한다.
     {
         int pid = fork(); // 자식 프로세스 fork함수로 생성
 
         if(pid == 0){ // 자식 프로세스일 경우
             //operation(op);
 
             mypid = getpid(); // 현재 프로세스의 id를 얻는다.

             if((order_fd = fopen("operation.dat", "r")) == NULL) // operation.dat 파일을 read모드로 열어 file stream에 담는다.
             {
                 printf("operation.dat 파일이 없습니다.\n");
                 exit(1);// operation.dat 파일이 없을 경우 종료
             }
 
             infile = fopen("account.dat", "r"); // 계좌 정보 파일을 read 모드로 열어 file stream에 담는다
 
             while(fgets(buffer, 100, order_fd) != NULL) // 더 이상 읽어올 명령어가 없을 경우 멈춘다.
             {// opeartion.dat file 에서 한 줄을 불러옴
 
                 char *ptr = strtok(buffer, "\t"); // 명령어를 \t 을 기준으로 나눔
                 char *sArr[10] = {NULL,};
                 int a = 0; // 명령어 갯수를 세기 위한 변수
                 int b = 0; // record에서 현재 acc_no이 있는지 확인하기 위한 변수
 
                 while(ptr != NULL) // token이 없을 경우 반복문을 멈춘다.
                 {
                     sArr[a] = ptr; // 첫 토큰을 sArr 배열에 담는다
                     a++;
                     ptr = strtok(NULL, "\t"); // sArr[0] = 1111 sArr[1] = c/r/d sArr[2] = 10000
                 }
 
                 check_op = *sArr[1]; // 특정 문자를 int 타입으로 바꾼다 -> 이후 switch문 사용을 위해
 
                 if(sArr[2] != NULL) // 만약 sArr[2] 값이 있을 경우 (즉 inquiry가 아닐 경우)
                     amount = strtol(sArr[2], NULL, 10); // amount -> int sArr[2] 값이 string 이므로 int type으로 바꾼다.
                 else
                     amount = 0; // inquiry일 경우 0으로 초기화 해준다.
 
                 while(fread(&current, sizeof(struct record), 1, infile)) // acc_no을 찾는 과정
                 { // account.dat 파일에서 record 구조체 크기만큼을 불러와 current에 담아서
                     if(strcmp(current.acc_no, sArr[0]) == 0) // 같은 acc_no 찾는다.
                     {
                         record_pos = b; // 있을 경우 acc_no 위치를 record_pos에 담는다.
                         break; // 더 이상 찾을 필요가 없으므로 정지
                     }
                     b++; // 나올 때 까지 b를 증가시켜 위치를 찾는다.
                     record_pos = b; // fread가 종료될 때까지 없을 경우 record_pos에 임의의 값을 담음
                 }
 
                 fseek(infile, 0, SEEK_SET); // 다음 acc_no을 찾기 위해 off_set을 처음으로 초기화 한다.
 
                 fd = open("account.dat", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR); // 계좌 파일을 r/w모드로 열어 fd에 file descriptor를 담는다.
 
                 if(record_pos < 5){ // record_pos가 있을 경우
 
                     switch(check_op){ // operation 확인
                         case 'w' : // withdraw일 경우
                             reclock(fd, record_pos, sizeof(struct record), F_WRLCK); // write record lock을 건다.
                             pos = record_pos * sizeof(struct record); // 위치는 record_pos (0 ~ 4) * 구조체 크기
                             lseek(fd, pos, SEEK_SET); // 파일 내 해당 구조체 위치로 이동한다.
                             n = read(fd, &current, sizeof(struct record)); // 현 위치에서 구조체 크기 만큼 읽는다.
                             current.balance -= amount; // 현 구조체의 balance에서 amount 만큼 값을 뺀다
                             lseek(fd, pos, SEEK_SET); // 다시 파일 내 해당 구조체 위치로 이동
                             write(fd, &current, sizeof(struct record)); // 현 위치에 구조체 내용을 쓴다.
                             withdrawview(&current, mypid, amount); // 현 명령어가 실행되었음을 출력한다.
                             reclock(fd, record_pos, sizeof(struct record), F_UNLCK); // write record lock을 해제한다.
                             break; // switch 종료
                         case 'i' :
                             reclock(fd, record_pos, sizeof(struct record), F_RDLCK); // read record lock을 건다.
                             pos = record_pos * sizeof(struct record); // 위치는 위와 동일
                             lseek(fd, pos, SEEK_SET);
                             n = read(fd, &current, sizeof(struct record));
                             inquiryview(&current, mypid); // 현 명령어가 실행되었음을 출력한다.
                             reclock(fd, record_pos, sizeof(struct record), F_UNLCK); // read record lock을 해제한다.
                             break; // switch 종료
                         case 'd' :
                             reclock(fd, record_pos, sizeof(struct record), F_WRLCK); // write record lock을 건다.
                             pos = record_pos * sizeof(struct record);
                             lseek(fd, pos, SEEK_SET);
                             n = read(fd, &current, sizeof(struct record)); // 구조체 값을 읽어온다
                             current.balance += amount; // 값을 더함
                             lseek(fd, pos, SEEK_SET);
                             write(fd, &current, sizeof(struct record)); // 값이 더 해진 정보를 해당 구조체에 쓴다.
                             depositview(&current, mypid, amount); // 현 명령어가 실행되었음을 출력한다.
                             reclock(fd, record_pos, sizeof(struct record), F_UNLCK); // write record lock을 해제한다.
                             break; // switch 종료
                         }
                 }
                 else // 만약 record_pos가 없을 경우 acc_no이 account.dat에 없을 경우
                     printf("계좌정보 없음\n"); // 계좌 정보가 없음을 출력한다.
 
                 close(fd); // 더 이상 account.dat file descriptor를 사용하지 않으므로 닫는다.
                 usleep(rand() % 1000001); // 명령어가 종료되었으므로 잠시 쉰다. 0 ~ 1000000 usec 중 랜덤
             }
         fclose(order_fd); // 더 이상 읽어올 명령어가 없으므로 operation.dat을 닫는다.
         fclose(infile); // account.dat도 닫는다.
         exit(0); // 10개 중 모든 명령어를 마친 프로세스는 닫는다.
         }
     }
     for(i = 0; i < 10; i++)
         wait(NULL); // 10개의 자식 프로세스가 종료되기를 기다린다.
 
     out = fopen("account.dat", "r"); // 현재 저장된 account.dat 파일을 연다
     for(i = 0; i < 5; i++) // 5개의 계좌 정보를 출력한다.
     {
         fread(&current, sizeof(struct record), 1, out); // 구조체 크기만큼 하나씩 읽어온다.
         printf("acc_no: %s name: %s balance: %d\n", current.acc_no, current.name, current.balance); // 계좌 정보를 출력
     }
     fclose(out); // account.dat 파일을 종료한다.
     exit(1); // main 프로세스가 종료된다.
 }
 
 void *operation(void *order){ // 쓰레드를 이용할 때 구현한 함수이므로 위의 자식 프로세스에서 동작하는 명령어와 같다.
                                 //사용하지 않음
     struct record current;
     FILE *infile;
     FILE *order_fd = order;
     pid_t pid;
     char buffer[100];
     int pos, record_pos, n;
     int check_op, amount;
     int fd;
 
     infile = fopen("account.dat", "r");
     pid = getpid();
 
     while(fgets(buffer, 100, order_fd) != NULL) // operation에서 명령어를 한 줄씩 불러옴
     {// 더 이상 읽어올 명령어가 없으면 멈춘다.
 
         char *ptr = strtok(buffer, "\t"); // 명령어를 \t 을 기준으로 나눔
         char *sArr[10] = {NULL,};
         int i = 0;
         int j = 0;
 
         while(ptr != NULL)
         {
             sArr[i] = ptr;
             i++;
             ptr = strtok(NULL, "\t"); // sArr[0] = 1111 sArr[1] = c/r/d sArr[2] = 10000
         }
 
         check_op = *sArr[1]; // 특정 문자
         if(sArr[2] != NULL)
             amount = strtol(sArr[2], NULL, 10); // amount -> intI
         else
             amount = 0;
 
         while(fread(&current, sizeof(struct record), 1, infile))
         {
             if(strcmp(current.acc_no, sArr[0]) == 0) // 같은 acc_no 찾는다.
             {
                 record_pos = j;
                 break;
             }
             j++;
             record_pos = j;
         }
         fseek(infile, 0, SEEK_SET);
 
         fd = open("account.dat", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
 
         if(record_pos < 5){
 
             switch(check_op){ // operation 확인
                 case 'w' :
                     reclock(fd, record_pos, sizeof(struct record), F_WRLCK);
                     pos = record_pos * sizeof(struct record);
                     lseek(fd, pos, SEEK_SET);
                     n = read(fd, &current, sizeof(struct record));
                     current.balance -= amount;
                     lseek(fd, pos, SEEK_SET);
                     write(fd, &current, sizeof(struct record));
                     withdrawview(&current, pid, amount);
                     reclock(fd, record_pos, sizeof(struct record), F_UNLCK);
                     break;
                 case 'i' :
                     reclock(fd, record_pos, sizeof(struct record), F_RDLCK);
                     pos = record_pos * sizeof(struct record);
                     lseek(fd, pos, SEEK_SET);
                     n = read(fd, &current, sizeof(struct record));
                     inquiryview(&current, pid);
                     reclock(fd, record_pos, sizeof(struct record), F_UNLCK);
                     break;
                 case 'd' :
                     reclock(fd, record_pos, sizeof(struct record), F_WRLCK);
                     pos = record_pos * sizeof(struct record);
                     lseek(fd, pos, SEEK_SET);
                     n = read(fd, &current, sizeof(struct record));
                     current.balance += amount; // 값을 더함
                     lseek(fd, pos, SEEK_SET);
                     write(fd, &current, sizeof(struct record));
                     depositview(&current, pid, amount);
                     reclock(fd, record_pos, sizeof(struct record), F_UNLCK);
                     break;
                 }
         }
         else
             printf("계좌정보 없음\n");
 
         close(fd);
         usleep(rand() % 1000001);
     }
     fclose(infile);
     exit(0);
 }
 
 int reclock (int fd, int recno, int len, int type) // file descriptor / record 번호 / 구조체 크기 / lock type
 {
     struct flock fl;
 
     switch (type) {
         case F_RDLCK: // read lock
         case F_WRLCK: // write lock
         case F_UNLCK: // unlock
             fl.l_type = type;
             fl.l_whence = SEEK_SET;
             fl.l_start = recno * len;
             fl.l_len = len;
             fcntl (fd, F_SETLKW, &fl); // file descriptor / lock을 설정 / 설정할 구조체
             return 1;
         default:
             return -1;
     };
 }
 
 
 void depositview(struct record *curr, int pid, int amount) // 입금 명령어가 수행되었음을 출력
 {
     printf("pid: %d acc_no: %s deposit: %d balance: %d \n", pid, curr->acc_no, amount, curr->balance);
 }
 
 void withdrawview(struct record *curr, int pid, int amount) // 출금 명령어가 수행되었음을 출력
 {
     printf("pid: %d acc_no: %s withdraw: %d balance: %d \n", pid, curr->acc_no, amount, curr->balance);
 }
 
 void inquiryview(struct record *curr, int pid) // 정보 명령어가 수행되었음을 출력
 {
     printf("pid: %d acc_no: %s inquiry \tbalance: %d \n", pid, curr->acc_no, curr->balance);
 }
