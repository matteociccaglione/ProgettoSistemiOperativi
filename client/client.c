#include "headerC.h"
#define fflush(stdin) while ((getchar())!='\n')
void alarm_handler(int sig){
	printf("Timeout scaduto closing connection\n");
	close(client_sock);
	printf("l'applicazione sta per chiudersi\n");
	exit(0);
}

void pipe_handler(int sig){
	printf("Lost connection with server\n");
	close(client_sock);
	printf("l'applicazione sta per chiudersi\n");
	exit(0);
}


void *polling_thread(void *i){
	sigset_t set;
	sigfillset(&set);
	sigdelset(&set,SIGPIPE);
	sigprocmask(SIG_BLOCK,&set,NULL);
	int sock=*(int*)i;
	msg *mex=(msg *)malloc(sizeof(msg));
	while(1){
		mex=ReadCommand(sock);
		if(mex==NULL){
			printf("Server disconnected\n");
			raise(SIGPIPE);
			close(client_sock);
			lost=1;
		}
		ProcessCommand(mex);
	}
}

int main(int argc, char **argv){
	if(argc!=3){
		printf("Errore devi inserire 2 valori di input\n ipNumber portNumber");
		exit(EXIT_FAILURE);
	}
	char *portNumber=argv[2];
	char * ipNumber=argv[1];
	printf("Welcome in the client\n");
	posti=(char *)malloc(sizeof(char)*NUMROW*NUMCOL+1);
	int result;
	int c;
	int on=1;
	char* tipoLetto;
	int contatore=0;
	pthread_mutex_init(&av_mut,NULL);
	pthread_mutex_lock(&av_mut);
	pthread_mutex_init(&code_op,NULL);
	pthread_mutex_lock(&code_op);
	char st;
	pthread_t thread;
	struct sigaction act;
	sigset_t set;
	sigfillset(&set);
	act.sa_handler=pipe_handler;
	act.sa_mask=set;
	sigaction(SIGPIPE,&act,NULL);
	act.sa_handler=alarm_handler;
	sigaction(SIGALRM,&act,NULL);
	char *string=malloc(sizeof(char)*4096);
	if(string==NULL){
		perror("Impossibile\n");
	}
	their_addr.sin_family=AF_INET;
	their_addr.sin_port=htons(atoi(portNumber));
	inet_aton(ipNumber,&their_addr.sin_addr);
	char *converted;
	int type;
	pthread_create(&thread,NULL,polling_thread,(void *)&client_sock);
start:	while(1){
		if(lost){
			alarm(0);
			connection(their_addr,my_addr);
			sleep(5);
			stampaPosti();
			lost=0;
		}
		printf("Scegliere l'operazione da eseguire: 0 per prenotare posti e 1 per disdire prenotazione 2 per chiudere l'applicazione\n");
		alarm(300);
		
retry:		tipoLetto=malloc(sizeof(char)*1023);
			
			if(fscanf(stdin,"%s",tipoLetto)!=EOF){
			c=0;
			while(c<strlen(tipoLetto)){
				
				if(tipoLetto[c]<48||tipoLetto[c]>57){
					
					break;
				}
				c++;
			}
			if(c>=strlen(tipoLetto)-1)
				type=atoi(tipoLetto);
			
			else{
				printf("Inserisci un valore tra 0,1 o 2\n");
				fflush(stdin);
				free(tipoLetto);
				goto retry;
				
			}
			free(tipoLetto);
		}
		fflush(stdin);
		alarm(500);
		if(type==0){
			stampaPosti();
		printf("Prego scegliere i posti da selezionare\n");
	    if(fgets(string,sizeof(char)*4096,stdin)==NULL ){
			goto start;
		}
		alarm(0);
		converted=convertiStringa(string);
		if(converted==NULL){
			printf("ERRORE RISPETTA LA SINTASSI\n");
		}
		else{
		WriteCommand(1,converted,client_sock);
		printf("Stiamo processando la tua richiesta rimani in attesa\n");
		pthread_mutex_lock(&av_mut);
		if(strcmp(code,"NULL")!=0)
			printf("Codice ricevuto: %s\n",code);
		printf("A breve sarà possibile effettuare una nuova operazione\n");
		sleep(10);
		stampaPosti();
		}
		}
		if(type==1){
			stampaPosti();
			printf("Inserire il codice di prenotazione\n");
reinserisci:	scanf("%s",string);
				contatore=0;
				while(contatore<strlen(string)){
					if(string[contatore]<48 || string[contatore]>57){
						printf("Ti ho chiesto di inserire un codice!\n");
						fflush(stdin);
						goto reinserisci;
					}
					contatore++;
			}
			
			alarm(0);
			printf("Codice acquisito\n");
			WriteCommand(2,string,client_sock);
			pthread_mutex_lock(&code_op);
			if(found!=0){
			printf("Operazione conclusa good bye\n");
			}
			else{
				printf("Impossibile completare la richiesta\n");
			}
			printf("A breve sarà possibile effettuare una nuova operazione\n");
			sleep(10);
			stampaPosti();
		}
		if(type==2){
			printf("L'applicazione sta per chiudersi\n");
			sleep(2);
			exit(0);
		}
		if(type!=0&&type!=1&&type!=2){
			printf("Ti ho detto di inserire una cifra tra 0 1 e 2\n");
			goto start;
		}
	}
}
			
		
		
	