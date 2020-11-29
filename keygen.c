#include <stdio.h>
#include <stdlib.h>

int main(int argc, char * argv[] ){
//check argvs
  if(argc < 2){
    fprintf(stderr, "Need number.\n");
    exit(0);
  }

  srand(time(NULL));
  int randm, i, len;
//get how many characters for key
  len = atoi(argv[1]);
//key buffer
  char key[len+1];
//generate random key
  for(i = 0; i < len; i++){
    randm = rand()%27;
    if(randm == 26){
      key[i] = ' ';
    } else {
      key[i] = (char)('A' + randm);
    }
  }
//set end of key
  key[len] = '\0';
//print key to stdout
  printf("%s\n",key );

  return 0;
}
