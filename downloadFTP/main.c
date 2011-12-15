#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <regex.h>


struct {
	char* ip;
	char* username;
	char* password;
	int porta;		//porta default 21
	int lport;		//porta para fazer listen
	char* path;		//path para o ficheiro


}Con;


int new_con(char* host, int port);
char* getIP(char* host);
int getArgs(char* arg);




int main(int argc, char *argv[] ){

	int sock,lsock;

	if( argc >= 2)
		if(getArgs(argv[1])<0){
			printf("usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
			return 0;
		}

	//init default Con vars
	Con.porta = 21;
	Con.lport = 0;
	printf("struct {\n");
	printf("	char* ip=%s\n",Con.ip);
	printf("	char* username=%s\n",Con.username);
	printf("	char* password=%s\n",Con.password);
	printf("	int porta=%d\n",Con.porta);
	printf("	int lport=%d\n",Con.lport);
	printf("	char* path=%s\n",Con.path);
	printf(")}Con;\n");


	sock = new_con(Con.ip,21);



	printf("parse feito, falta fazer o parse da connecção.\nsock=%d\n",sock);

	close(sock);
	return 0;

}





int new_con(char* host_ip, int port){
	int	sockfd;
	struct	sockaddr_in server_addr;

	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(host_ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);		/*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	return -1;
    	}
	/*connect to the server*/
    if(connect(sockfd,(struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        	perror("connect()");
        	return -1;
	}
    	/*send a string to the server*/


	return sockfd;
}
char* getIP(char* host){

	struct hostent *h;


/*
struct hostent {
	char    *h_name;		Official name of the host.
	char    **h_aliases;	A NULL-terminated array of alternate names for the host.
	int     h_addrtype;		The type of address being returned; usually AF_INET.
	int     h_length;		The length of the address in bytes.
	char    **h_addr_list;	A zero-terminated array of network addresses for the host.
							Host addresses are in Network Byte Order.
};

#define h_addr h_addr_list[0]	The first address in h_addr_list.
*/
        if ((h=gethostbyname(host)) == NULL) {
            herror("gethostbyname");
            return NULL;
        }

        printf("Host name  : %s\n", h->h_name);
        printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));

        return inet_ntoa(*((struct in_addr *)h->h_addr));

}

int getArgs(char* arg){
	//arg = ftp://[<user>:<password>@]<host>/<url-path>

	//0 - regex para fazer match a ftp://
	//1 - regex para fazer match a /username:
	//2 - regex para fazer match a :username@
	//3 - regex para fazer match a host_name/
	//4 - regex para fazer match an ip
	//5 - regex para fazer match a path

	char **regex;
	char **match;
	char *temp=0;
	int i = 6;
	int offset;
	regex_t re;
	char buf[256];
	regmatch_t pmatch[100];
	int status;
	char *ps;
	int eflag;


	regex = (char*)calloc(6, sizeof(char*));
	regex[0] = (char*)calloc(8, sizeof(char));
	regex[1] = (char*)calloc(20, sizeof(char));
	regex[2] = (char*)calloc(20, sizeof(char));
	regex[3] = (char*)calloc(95, sizeof(char));
	regex[4] = (char*)calloc(104, sizeof(char));
	regex[5] = (char*)calloc(34, sizeof(char));

	regex[0] = "^ftp://";
	regex[1] = "/[a-zA-Z0-9]{1,12}:";
	regex[2] = ":[a-zA-Z0-9]{1,12}@";
	regex[3] = "(([a-zA-Z]|[a-zA-Z][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z]|[A-Za-z][A-Za-z0-9\\-]*[A-Za-z0-9])/";
	regex[4] = "(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])";
	regex[5] = "(/[a-zA-Z0-9\\-]+)+\\.[a-zA-Z0-9]+$";

	match = (char*)calloc(6,sizeof(char*));
//	printf("\nmatch = %d\n",match);


//	printf("\n\n\n");
	while(i--){
//		 printf("regex[%d] = %s;\n",5-i,regex[5-i]);
		 if((status = regcomp( &re, regex[5-i], REG_EXTENDED))!= 0){
			 regerror(status, &re, buf, 120);
			 return -1;
		 }
		 ps = arg;
		 eflag = 0;

		 if( status = regexec( &re, ps, 1, pmatch, eflag)== 0){
			 offset = pmatch[0].rm_eo-pmatch[0].rm_so;

			 match[5-i] = (char*)calloc(offset,sizeof(char));
			 strncpy(match[5-i],ps+pmatch[0].rm_so,offset);
//			 printf("encontrado: %s-----%d-%d=%d\n\n",match[5-i], pmatch[0].rm_eo,pmatch[0].rm_so,offset);

		 }

		 regfree( &re);

	}

	if(match[0] != 0) //começa com ftp://?
		if(match[5]!=0){ //tem path?
			Con.path = (char*)calloc( strlen(match[5])-1, sizeof(char));
			strncpy(Con.path, match[5]+1, strlen(match[5])-1);
		} else return -1; //não tem! logo não se pode fazer download
	else return -1; //o arg não está na forma necessaria

	if((match[1]!=0) && (match[2]!=0)){ //tem username e password?
		Con.username = (char*)calloc( strlen(match[1])-2, sizeof(char));
		Con.password = (char*)calloc( strlen(match[2])-2, sizeof(char));
		strncpy(Con.username, match[1]+1, strlen(match[1])-2);
		strncpy(Con.password, match[2]+1, strlen(match[2])-2);
	} else {
		Con.username = 0;
		Con.password = 0;
	}

    if((match[3]!=0) && (match[4]==0)){//tem hostname mas ip não?
		Con.ip = (char*)calloc(4*3+3, sizeof(char));
		temp = (char*)calloc( strlen(match[3]), sizeof(char));
		strncpy( temp, match[3], strlen(match[3])-1 );
		//match[4]-> null => uso-o como uma var temp para obter o ip do hostname
		match[4] = getIP(temp);
		strncpy(Con.ip, match[4], strlen(match[4]));
	} else{
		Con.ip = (char*)calloc(4*3+3, sizeof(char));
		strncpy(Con.ip, match[4], strlen(match[4]));
	}


    i=-1;

    //limpar apontadores
//    while(i++<6){
//    	free(regex[i]);
//    	if(match[i]!=0)
//    		free(match[i]);
//    }

    free(regex);
    free(match);
    if(temp!=0)
    	free(temp);

	return 1; //é correcto
}
