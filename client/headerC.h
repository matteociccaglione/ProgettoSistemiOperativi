#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<pthread.h>
#include<semaphore.h>
#include<sys/sem.h>
#include<sys/ipc.h>
#include <arpa/inet.h>
#include<netinet/in.h>
#include <ctype.h>


int NUMROW,NUMCOL;
int found;
char *posti;
int client_sock;
int lost=1;
char code[2048];
pthread_mutex_t av_mut;
pthread_mutex_t code_op;
struct sockaddr_in their_addr;
struct sockaddr_in my_addr;
typedef struct __message{
	int type; // 1=assegnazione posti, 2=disdire operazione, 3=gestione 
	char *text;//contenuto del messaggio
	int sock_dest; //unused for type 1 and 2. Se type=3 e il messaggio lo invia il server allora sock_dest è il socket destinazione.
}msg;


msg *ReadCommand(int sock){     //La ReadCommand legge un byte per volta per assicurarsi di leggere uno e un solo messaggio alla volta. Evitiamo cosi lo scenario in cui piu messaggi arrivano alla socket e rimangono in attesa di lettura e viene letto più di uno.
	msg * messaggio=malloc(sizeof(msg));
	char character[1]=" ";
	int c=0;
	int x=2;
	int fine=0;
	char local[4096];
	ssize_t size;
		if(read(sock,character,sizeof(char))==0){
			return NULL;
		}
		if(character[0]=='-'){
			character[0]=' ';
			character[1]='\0';
			if(read(sock,character,sizeof(char))==0){
				return NULL;
			}
			switch(character[0]){
				case 'a':
						messaggio->type=1;
						break;
				case 'd':
						messaggio->type=2;
						break;
				case 'c':
						messaggio->type=3;
						break;
				default:
						printf("Errore\n");
						return NULL;
			}
			while(character[0]!='f'){
				if(read(sock,character,sizeof(char))==0){
					return NULL;
				}
				if(character[0]=='-' || character[0]=='f'){
				}
				else{
					local[c]=character[0];
					c++;
				}
			}
			local[c]='\0';
		}
	messaggio->text=malloc(sizeof(char)*strlen(local));
	strcpy(messaggio->text,local);
	return messaggio;
}
	
	
void WriteCommand(int type,char *string, int sock){
	char local[3];
	char *final_s;
	if(type==1){
		strcpy(local,"-a");
	}
	if(type==2){
		strcpy(local,"-d");
	}
	final_s=(char *)malloc(sizeof(char)*(strlen(string)+1+strlen(local)+3));
	sprintf(final_s,"%s %s -f",local,string);
	write(sock,final_s,strlen(final_s)*sizeof(char));
}

void stampaPosti(){
	system("clear");
	int r,c;
	for(r=0;r<NUMROW;r++){
		printf("%d  ",r);
		for(c=0;c<NUMCOL;c++){
			if(posti[r*NUMCOL+c]=='0'){
				printf("X");
			}
			else{
				printf("%d",c);
			}
		}
		printf("\n\n");
	}
}
void init(int sock){
	char stringa[4098];
	char *local,*col,*pos;
	alarm(30);
	if(read(sock,stringa,NUMROW*NUMCOL*sizeof(char)+50)==0){
		printf("Inizializzazione fallita a causa di una disconnessione del server\n");
		printf("Riavvio la procedura di connessione\n");
	}
	else{
		alarm(0);
		local=strtok(stringa,",");

		NUMROW=atoi(local);
		
		col=strtok(NULL,",");
		if(col==NULL){
			printf("Errore riavvia l'applicazione\n");
			exit(EXIT_FAILURE);
		}
		
		NUMCOL=atoi(col);
		pos=strtok(NULL,",");
		if(pos==NULL){
			printf("Errore riavvia l'applicazione\n");
			exit(EXIT_FAILURE);
		}
		posti=(char *)malloc(sizeof(char)*NUMROW*NUMCOL);
		strcpy(posti,pos);
		printf("Inizializzazione completata\n");
	}
}
void connection(struct sockaddr_in their_addr,struct sockaddr_in my_addr){
	int scelta;
	int on=1;
	client_sock=socket(AF_INET,SOCK_STREAM,0);
retry:	if(connect(client_sock,(struct sockaddr *)&their_addr,sizeof(their_addr))==-1){
		printf("Impossibile stabilire una connessione\n");
		printf("Inserire 0 per chiudere e 1 per riprovare\n");
		scanf("%d",&scelta);
		if(scelta==1){
			sleep(2);
			goto retry;
		}
		else{
			exit(0);
		}
	}
	printf("Connessione con il server stabilita\n");
	init(client_sock);
}



char * convertiStringa(char *string){
	char *stringa=malloc(strlen(string)*sizeof(char));
	char **local=malloc(sizeof(char)*NUMROW*NUMCOL);
	char str_array[NUMROW*NUMCOL][1024];
	char *fila=malloc(sizeof(char)*10);
	char *poltrona=malloc(sizeof(char)*10);
	int c=0;
	int i=0;
	int n_poltrona,n_fila;
	local[i]=strtok(string," ");
	while(local[i]!=NULL){
		i++;
		local[i]=strtok(NULL," ");
	}
	while(c<i){
		fila=strtok(local[c],",");
		poltrona=strtok(NULL,",");
		if(fila==NULL || poltrona==NULL){
			return NULL;
		}
		n_fila=atoi(fila);
		n_poltrona=atoi(poltrona);
		if(n_poltrona>=NUMCOL || n_fila>=NUMROW || n_poltrona<0 || n_fila<0)
			return NULL;
		sprintf(str_array[c],"%d",n_poltrona+n_fila*NUMCOL);
		c++;	
	}
	if(i==1){
		strcpy(stringa,str_array[0]);
	}
	else{
		strcat(str_array[0]," ");
		strcat(str_array[0],str_array[1]);
		strcpy(stringa,str_array[0]);
		c=2;
		while(c<i){
			strcat(stringa," ");
			strcat(stringa,str_array[c]);
			c++;
		}
		strcat(stringa," ");
	}
	return stringa;
}

void ProcessCommand(msg *mex){
	char *string=malloc(sizeof(char)*50);
	int d=0, array[NUMROW*NUMCOL],i,valore;
	if(mex->type==1){
		string=strtok(mex->text," ");
		while(string!=NULL){
			valore=atoi(string);
			posti[valore]='0';
			string=strtok(NULL," ");
		}
	}
	if(mex->type==2){
		string=strtok(mex->text," ");
		while(string!=NULL){
			valore=atoi(string);
			posti[valore]='1';
			string=strtok(NULL," ");
		}
	}
	if(mex->type==3){
		string=strtok(mex->text," ");
		if(strcmp(string,"e")==0){
			printf("Impossibile completare la richiesta perchè i posti sono già occupati\n");
			strcpy(code,"NULL");
			pthread_mutex_unlock(&av_mut);
		}
		else{
		if(strcmp(string,"cnt")==0){
			printf("Code not found\n");
			found=0;
			pthread_mutex_unlock(&code_op);
		}
		else{
			if(strcmp(string,"ct")==0){
				found=1;
				pthread_mutex_unlock(&code_op);
			}
			else{
				strcpy(code,mex->text);
				pthread_mutex_unlock(&av_mut);
			}
		}
	}
	free(mex);
}
	}