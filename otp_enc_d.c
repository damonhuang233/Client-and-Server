#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define BUF_SIZE 200000
#define TEXT_SIZE 100000

void error(const char *msg) {
  perror(msg);
  exit(1);
}
//convert char to int
int charToInt(char c){
  char array[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
  int i;
  for(i = 0; i < 27; i++){
    if(array[i] == c)
      break;
  }
  i++;
  return i;
}
//convert int to char
char intToChar(int i){
  char array[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
  return array[i-1];
}

int main(int argc , char * argv[]){

  int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
  socklen_t sizeOfClinetInfo;
  struct sockaddr_in serverAddress, clientAddress;
  char buffer[BUF_SIZE];
  char textBuffer[TEXT_SIZE];
  char keyBuffer[TEXT_SIZE];
  pid_t pid, wpid;
  int numOfProc = 0;
//check argvs
  if(argc < 2) {
    fprintf(stderr, "USAGE: %s port\n", argv[0] );
    exit(1);
  }
//set up serverAddress
  memset((char *)&serverAddress, '\0', sizeof(serverAddress));
  portNumber = atoi(argv[1]);
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(portNumber);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
//open socket
  listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocketFD < 0)
    error("ERROR opening socket");
//bind socket to server
  if(bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    error("ERROR on binding");
//start listen, up to 5 at the same time
  listen(listenSocketFD , 5);

  while(1){
//accept connection
    int status;
    sizeOfClinetInfo = sizeof(clientAddress);
    establishedConnectionFD = accept(listenSocketFD,(struct sockaddr *)&clientAddress, &sizeOfClinetInfo );
    if(establishedConnectionFD < 0)
      error("ERROR on accept");
//clean finished process
    wpid = waitpid(-1, &status ,WNOHANG);
    if(wpid > 0)
      numOfProc--;
//make sure only 5 process at most at the same time
    if(numOfProc >= 5){
      printf("Server busy. Waiting for process ...\n");
      while(numOfProc >= 5){
        wpid = waitpid(-1, &status ,WNOHANG);
        if(wpid > 0)
          numOfProc--;
      }
    }
//fork to background
    pid = fork();
    if(pid < 0){
      error("ERROR on fork");
    }
    else if(pid == 0){
//make sure connect from enc client
      do{
        charsRead = recv(establishedConnectionFD, buffer, 4 ,0);
        if(charsRead < 0)
          error("ERROR reading from socket");
      }while(buffer[3] != '!');
//if not, send 0 and exit
      if(buffer[0] != 'e' || buffer[1] != 'n' || buffer[2] != 'c'){
        fprintf(stderr, "ERROR: Not connect from enc clinet.\n");
        charsRead = send(establishedConnectionFD, "0", 1 ,0);
        exit(2);
      }
//else, send 1
      charsRead = send(establishedConnectionFD, "1", 1 ,0);

      memset(buffer, '\0', BUF_SIZE);
//read text and key from socket, loop until read * (end of key)
      int readLen = 0;
      do{
        charsRead = recv(establishedConnectionFD, &buffer[readLen], 1000, 0);
        if(charsRead < 0)
          error("ERROR reading from socket");
        readLen += charsRead;
      }while(buffer[readLen-1] != '*');
//get text from buffer
      int i = 0, k = 0;
      for(i = 0; buffer[i] != '#'; i++){
        textBuffer[i] = buffer[i];
      }
      textBuffer[i] = '\0';
//get key from buffer
      i++;
      for(k = 0; buffer[i+k] != '*'; k++){
        keyBuffer[k] = buffer[i+k];
      }
      keyBuffer[k] = '\0';
//enc text
      int num;
      for(i = 0; textBuffer[i] != '\0' ; i++){
        num = (charToInt(textBuffer[i]) + charToInt(keyBuffer[i]));
        if(num > 27)
          num -= 27;
        textBuffer[i] = intToChar(num);
      }
//sent enc text
      readLen = 0;
      do{
        charsRead = send(establishedConnectionFD, textBuffer, sizeof(textBuffer), 0);
        if(charsRead < 0)
          error("ERROR writing to socket");
        readLen += charsRead;
      }while (textBuffer[readLen-1] != '\0');
//close connection and exit
      close(establishedConnectionFD);
      exit(0);
    }
    else {
//parent process, increase process counter
        numOfProc++;
//clean finished process
        wpid = waitpid(-1, &status ,WNOHANG);
        if(wpid > 0)
          numOfProc--;
    }
  }
//close socket
  close(listenSocketFD);

  return 0;
}
