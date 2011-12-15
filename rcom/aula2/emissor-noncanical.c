/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <strings.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
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


volatile int STOP=FALSE;


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

char decodeTrama(char* a);
void enviarTrama(int fd, char a, char c);
void iniCon(int fd);
void semResposta(void);

int timeout;

int main(int argc, char** argv)
{
    int fd,c, res, tamanho;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;
    
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
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 1 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");


    //inicia coneccao (Com  3 timeouts de 3s)
    iniCon(fd);
  
    gets (buf);
    
    tamanho=strlen(buf);
    buf[tamanho++]='\0';
    res = write(fd,buf,tamanho);   
    printf("%d bytes written\n", res);
  
    while (STOP==FALSE) {       /* loop for input */
      res = read(fd,buf,1);   /* returns after 1 chars have been input */
      if(res > 0)
              buf[res]=0;               /* so we can printf... */
              printf(":%s:\n", buf);
              if (buf[0]=='\0') STOP=TRUE;
    }


  /* 
    O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar 
    o indicado no gui�o 
  */


   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    
    




    close(fd);
    return 0;
}

void semResposta(void){
    timeout = 1;
    printf("TIMEOUT\n");
    }

void disconnect(int fd){
	int ntries = 3;

	while(ntries){
		enviarTrama(fd, A_EMISSOR, C_DISC);

		if(esperarTrama(fd, A_EMISSOR, C_DISC)==-1)
			printf("Timeout: %d\n", ntries--); 
	}
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
    	(void) signal(SIGALRM, semResposta);  

	timeout = 0;
	alarm(3);   //poe o alarm a activar dentro de 3 segundos
    
	//printf("waiting for connection\n");
	
	//recepcao de resposta     
	while ( (indTrama!=5) && (timeout == 0) ){       
	
	    res = read(fd,buff,1);
	    //printf(":%x:", buf[0]);
	    if( res>0)
	            //verifica se estamos a receber a trama na ordem correcta
	            if(buff[0] == rightTrama[indTrama])
	                    indTrama++;
	            else 
	                if(buff[0] == rightTrama[0])
	                        indTrama = 1;
	                else indTrama = 0;


       }

       if(timeout)
            return -1;
       else{
            //cancela o alarm
            signal(SIGALRM,SIG_IGN);
            return 1;
       }
}

void iniCon(int fd){
    int ntries = 3;
     
    while(ntries){
        enviarTrama(fd,A_EMISSOR,C_SET); //envia pedido de coneccao
        
	if(esperarTrama(fd, A_EMISSOR, C_UA)==-1)
		printf("Timeout: %d\n", ntries--); 
   }
   if(!ntries)
        printf("Erro na coneccao: TIMEOUT\n");
   else
        printf("\n\nconnected\n");
}


char decodeTrama(char* a){
    


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
