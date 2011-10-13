/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <strings.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define F 0x7e
#define A_EMISSOR 0x03
#define A_RECETOR 0x01

#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0b
#define C_RR (00R0 + 0x1)
#define C_REJ (00R0 + 0x5)

/* trama 1- > F|A|C|bcc|F
 * F -> 0x7E
 * A -> 0x03 -> comandos enviados pelo emissor ou resp para o emissor
 *   -> 0x01 -> comandos enviados pelo receptor ou resp para o receptor
 * C -> SET -> 0x03
 *   -> UA  -> 0x07
 *   -> Disc-> 0x0b
 *   -> RR  -> 00R0 + 0x1
 *   -> REJ -> 00R0 + 0x5
 * bcc -> A^C //xor
 *****************************/


char decodeTramaControlo(char* a);
void enviarTrama(int fd, char a, char c);
void iniCon(int fd);
void semResposta(void);
int esperarTrama(int fd, char a, char c);
void disconnect(int fd);
char* encodeTrama(char* str, int len, int indT, char a);
char* decodeTrama(char* str, int len);


volatile int STOP=FALSE; 
int timeout;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

/***************
 * aula 2
 ***************/
    char fstring[255];
    int nfstring;
    nfstring = 0;



    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /*    
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    iniCon(fd);


      printf("DATA\n");
    while (STOP==FALSE) {       /* loop for input */
      res = read(fd,buf,1);   /* returns after 5 chars have been input */
      buf[res]=0;               /* so we can printf... */

      if( res>0) {
         printf(":%s:", buf);
        // HARDCODED para 1 BYTE
              fstring[nfstring++] = buf[0];
              if (buf[0]=='\0') STOP=TRUE;
              }
    }
    

    res = write(fd,fstring,nfstring);    
    printf("\nResponse sent: %d Bytes\n",res);

        disconnect(fd);

  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião 
  */


    sleep(1);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}

void semResposta(void){
    timeout = 1;
    printf("TIMEOUT\n");
    }

void disconnect(int fd){
	int ntries = 3;

        esperarTrama(fd, A_EMISSOR, C_DISC);

        enviarTrama(fd, A_RECETOR, C_DISC);
        
	if(!ntries)
		printf("Erro na disconeccao: TIMEOUT\n");
	else
		printf("\n\ndisconnected\n");

}

int esperarTrama(int fd, char a, char c){
	char buff[255];
	int res;
	char trama[5];
	char rightTrama[5];
	int indTrama = 0;
    
	//trama que correcta de resposta do receptor
	rightTrama[0] = F;
	rightTrama[1] = a;
	rightTrama[2] = c;
	rightTrama[3] = a ^ c;
	rightTrama[4] = F;

	// instala  funcao que atende interrupcao para timeout
    	//(void) signal(SIGALRM, semResposta);  

	timeout = 0;
	//alarm(3);   //poe o alarm a activar dentro de 3 segundos
    
	//printf("waiting for connection\n");
	
	//recepcao de resposta     
	while ( (indTrama!=5) && (timeout == 0) ){       
	
	    res = read(fd,buff,1);
	    
	    if( res>0){printf(":%x:", buff[0]);
	            //verifica se estamos a receber a trama na ordem correcta
	            if(buff[0] == rightTrama[indTrama])
	                    indTrama++;
	            else 
	                if(buff[0] == rightTrama[0])
	                        indTrama = 1;
	                else indTrama = 0;}


       }

       if(timeout)
            return -1;
       else{
            //cancela o alarm
            //signal(SIGALRM,SIG_IGN);
            return 1;
       }
}

void iniCon(int fd){
    int ntries = 3;
        
    if(esperarTrama(fd, A_EMISSOR, C_SET) < 0)
        ntries = 0;
        
    enviarTrama(fd, A_EMISSOR, C_UA);

   if(!ntries)
        printf("Erro na coneccao: TIMEOUT\n");
   else
        printf("\n\nconnected\n");
}


char decodeTramaControlo(char* a){
    


    if( a[0] == F && a[4] == F)
        if( a[1]^a[2] == a[3] )   
            return a[3];
       
    return NULL;
}

void enviarTrama(int fd, char a, char c){
    char buf[5];
    buf[0] = F;
    buf[1] = a;
    buf[2] = c;
    buf[3] = a^c;
    buf[4] = F;
   
    write(fd, buf, 5);
}

char* encodeTrama(char* str, int len, int indT, char a){

   char c;
   char bcc1;
   char bcc2;
   char *tudo;
   int j = 4;
 
   
   if(indT == 0)
         c=0;
   else
        c=2;     
    
   bcc1 = c^a;
    bcc2 = str[0];
   for(int i = 1; i<len; i++){
        bcc2 = bcc2^str[i];      
    }
   
    tudo = (char*) malloc (6+len);
    
    tudo[0] = F;
    tudo[1] = a;
    tudo[2]= c;
    tudo[3] = bcc1;
    
    for(int i = 0; i<len;i++,j++){
        tudo[j] = str[i];
    }
    tudo[j++]=bcc2;
    tudo[j++]=F;
       
    
   return tudo; 
  
}

char* decodeTrama(char* str, int len){

  char bcc1, bcc2;
  char* outravez;
  if(str[0]!=F || (str[1] != A_EMISSOR && str[1] != A_RECETOR) ){
        return NULL;
        
  } else 
  if(str[2]!= 0 && str[2]!= 2) 
        return NULL;
                
  else   
  if(str[1]^str[0] != str[3]) //bcc1
        return NULL;
        
  bcc2 = str[4];
  
  for(int i = 5; i< len-6;i++)
      bcc2 ^= str[i];  
  if(bcc2 != str[len-2]) //TESTAR
        return NULL;
        
  else if(str[len-1] != F)
        return NULL;
        

   outravez = (char*) malloc (len-6);  
   
   for(int i = 4; i<len-2; i++)
        outravez[i-4] = str[i];
   
   return outravez;   
}
