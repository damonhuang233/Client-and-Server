#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define BUF_SIZE 100000

void error(const char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[]) {

  int socketFD, portNumber, charsWritten, charsRead, textSize, keySize;
  struct sockaddr_in serverAddress;
  struct hostent* serverHostInfo;
  char buffer[BUF_SIZE];
  char keyBuffer[BUF_SIZE];
//check argv
  if(argc < 4) {
    fprintf(stderr, "USAGE: %s , plaintext key port\n", argv[0] );
    exit(0);
  }
//get text contents
  FILE *fp;

  fp = fopen(argv[1],"r");
  if(fp == NULL)
    error("ERROR opening plaintext file");
  memset(buffer, '\0', sizeof(buffer));
  while(fgets(buffer, sizeof(buffer)-1, fp));
  fclose(fp);
  textSize = strlen(buffer);
//check if text contains unvaild characters
  char vaildChars[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ \n";
  int i;
//if it does, print error and exit
  for(i = 0; i < textSize; i++)
    if(strchr(vaildChars,buffer[i]) == NULL){
      fprintf(stderr, "ERROR: text contains unvaild character.\n" );
      exit(1);
    }
//else, replace newline with # for send
  buffer[strcspn(buffer, "\n")] = '#';
//get key contents
  fp = fopen(argv[2],"r");
  if(fp == NULL)
    error("ERROR opening key file");
  memset(keyBuffer, '\0', sizeof(keyBuffer));
  while(fgets(keyBuffer, sizeof(keyBuffer)-1, fp));
  fclose(fp);
  keySize = strlen(keyBuffer);
//check if key contains unvaild characters
  for(i = 0; i < keySize; i++)
    if(strchr(vaildChars,keyBuffer[i]) == NULL){
      fprintf(stderr, "ERROR: key contains unvaild character.\n" );
      exit(1);
    }
//replace newline with * for send
  keyBuffer[strcspn(keyBuffer, "\n")] = '*';
//check if key too small for text
  if(keySize < textSize){
    fprintf(stderr, "ERROR: key size smaller than text size.\n");
    exit(1);
  }
//set up serverAddress
  memset((char *)&serverAddress, '\0', sizeof(serverAddress));
  portNumber = atoi(argv[3]);
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(portNumber);
  serverHostInfo = gethostbyname("localhost");
  if(serverHostInfo == NULL){
    fprintf(stderr, "CLINET: ERROR, no such host\n");
    exit(0);
  }
  memcpy((char *)&serverAddress.sin_addr.s_addr, (char *)serverHostInfo->h_addr, serverHostInfo->h_length);
//open socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if(socketFD < 0)
    error("CLIENT: ERROR opening socket");
//connect to server
  if(connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    error("CLIENT: ERROR connecting");
//send enc to make sure connecting to enc server
  charsWritten = send(socketFD, "enc!", 4 , 0);
  if(charsWritten < 0)
    error("CLINET: ERROR writing to socket");
  if(charsWritten < 4)
    printf("CLIENT: WARNING: Not all data written to socket!\n");
//check if connect to enc server
  char check[2] = "1\0";
  charsWritten = recv(socketFD, check, 1, 0); // if read 0, wrong server
  if(charsWritten < 0)
    error("CLINET: ERROR reading from socket");
  if(check[0] == '0')
    exit(2);
//send text
  charsWritten = send(socketFD, buffer, strlen(buffer), 0);
  if(charsWritten < 0)
    error("CLIENT: ERROR writing to socket");
  if(charsWritten < strlen(buffer))
    printf("CLIENT: WARNING: Not all data written to socket!\n");

  memset(buffer, '\0', sizeof(buffer));
//sent key
  charsWritten = send(socketFD, keyBuffer, strlen(keyBuffer), 0);
  if(charsWritten < 0)
    error("CLIENT: ERROR writing to socket");
  if(charsWritten < strlen(keyBuffer))
    printf("CLIENT: WARNING: Not all data written to socket!\n");

  memset(keyBuffer, '\0', sizeof(keyBuffer));
//read enc text, loop until read all characters
  int readLen = 0;
  do{
    charsRead = recv(socketFD, &buffer[readLen], 1000, 0);
    if(charsRead < 0)
      error("CLIENT: ERROR reading from socket");
    readLen += charsRead;
  }while(buffer[readLen-1] != '\0');
//print enc text
  printf("%s\n", buffer );
//close socket
  close(socketFD);

  return 0;
}
