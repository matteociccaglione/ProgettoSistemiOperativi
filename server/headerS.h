
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
#include"describer.h"
#define fflush(stdin) while ((getchar())!='\n')
	typedef struct __cons_list{
	int sock;
	int available;
	struct __cons_list *next;
}cons_list;
pthread_mutex_t mutex_l;
pthread_mutex_t mutex_socket;
int sem_elem;
int matrix[NUMROW*NUMCOL];//voglio inizializzare tutto a 1
pthread_mutex_t mutex_a;
pthread_mutex_t mutex_lp;
pthread_mutex_t mutex_d;
pthread_mutex_t init_mut;
pthread_mutex_t backup_mut;
int actual_sock;
char **code;
int global_sock;
int sockets[NUMROW*NUMCOL];
int max_client=-1;
char postieffettivi[NUMROW*NUMCOL];
FILE *backup;
cons_list *header=NULL;
cons_list *tail=NULL;
typedef struct __message{
	int type; // 1=assegnazione posti, 2=disdire operazione, 3=gestione 
	char *text;//contenuto del messaggio
	int sock_dest; //unused for type 1 and 2. Se type=3 e il messaggio lo invia il server allora sock_dest è il socket destinazione.
}msg;

typedef struct __list{
	msg *text;
	struct __list *next;
}list;
list *highprio_t=NULL;
list *lowprio_t=NULL;
list *highprio_h=NULL;
list *lowprio_h=NULL;

void removeSock(int sock);
int getCode(char *mex);
void riordinaFile();
void inserisciLista(list *lista, list *newElement,int num);
msg *ReadCommand(int sock);
int WriteCommand(msg* messaggio);
int ProcessCommand(msg *messaggio, int my_sock);
void removeSock(int sock){
	pthread_mutex_lock(&mutex_socket);
	int i;
	for(i=0;i<max_client+1;i++){
		if(sockets[i]==sock){
			sockets[i]=-1;
			close(sock);
			if(i==max_client)
				max_client--;
			
		}
	}
	pthread_mutex_unlock(&mutex_socket);
}
int getCode(char *mex){
	
	int i,count=0;
	char *local_string;
	char string[NUMROW*NUMCOL];
	char *token;
	for(i=0;i<NUMROW*NUMCOL;i++){
		if(code[i]==NULL){
			code[i]=(char *)malloc(sizeof(char)*strlen(mex)+ 6);
			if(code[i]==NULL){
				printf("Impossibile fare malloc\n");
			}
			sprintf(code[i],"%s",mex);
			local_string=malloc(sizeof(char)*strlen(mex));
			strcpy(local_string,mex);
			token=strtok(local_string," ");
			sprintf(string,"%d",i);
			strcat(string,"#");
			strcat(string,token);
			token=strtok(NULL," ");
			while(token!=NULL){
				strcat(string,"#");
				strcat(string,token);
				token=strtok(NULL," ");
			}
			pthread_mutex_lock(&backup_mut);
			backup=fopen("server_backup_file.txt","a");
			if(backup==NULL){
							printf("Fatal error\n");
							exit(0);
						}
			if(fprintf(backup,"%s\n",string)<0){
				printf("Fatal error\n");
			}
			fclose(backup);
			pthread_mutex_unlock(&backup_mut);
			return i;
		}
	}
	return -1;
}



void riordinaFile(){
	pthread_mutex_lock(&backup_mut);
	backup=fopen("server_backup_file.txt","r");
	int code;
	char c;
	char local_string[4096];
	char *local;
	char **codes;
	codes=(char**)malloc(sizeof(char*)*NUMROW*NUMCOL);
	char string[4096];
	while(fscanf(backup,"%s\n",string)!=EOF){
		strcpy(local_string,string);
		code=atoi(strtok(local_string,"#"));
		codes[code]=(char *)malloc(sizeof(char)*strlen(string));
		local=strtok(NULL,"#");
		if(local!=NULL){
		strcpy(codes[code],local);
		local=strtok(NULL,"#");
		while(local!=NULL){
			strcat(codes[code],"#");
			strcat(codes[code],local);
			local=strtok(NULL,"#");
		}
		}else{
			codes[code]=NULL;
		}
		}
	fclose(backup);
	backup=fopen("server_backup_file.txt","w");
	for(int i=0;i<NUMROW*NUMCOL;i++){
		if(codes[i]!=NULL){
			fprintf(backup,"%d#%s\n",i,codes[i]);
		}
	}
	fclose(backup);
	pthread_mutex_unlock(&backup_mut);
}
void inserisciLista(list *lista, list *newElement,int num){
	struct sembuf oper;
	oper.sem_num=0;
	oper.sem_op=1;
	newElement->next=NULL;
	if(num==1){
		pthread_mutex_lock(&mutex_l);
	}
	else{
		pthread_mutex_lock(&mutex_lp);
	}
	if(lista==NULL){
					if(num==1){
						highprio_h=newElement;
						highprio_t=newElement;
					}
					else{
						lowprio_h=newElement;
						lowprio_t=newElement;
					}
				}
	else{
			lista->next=newElement;
			lista=newElement;
		}
	if(num==1){
		pthread_mutex_unlock(&mutex_l);
	}
	else{
		pthread_mutex_unlock(&mutex_lp);
	}
	semop(sem_elem,&oper,1);
}


msg *ReadCommand(int sock){     //La ReadCommand legge un byte per volta per assicurarsi di leggere uno e un solo messaggio alla volta. Evitiamo cosi lo scenario in cui piu messaggi arrivano alla socket e rimangono in attesa di lettura e viene letto più di uno.
	msg * messaggio=malloc(sizeof(msg));
	char character[3]=" ";
	int c=0;
	int x=2;
	int fine=0;
	char local[4096];
	ssize_t size;
		while(1){
leggi:		if(read(sock,character,x*sizeof(char))==0){
				return NULL;
			}
			if(x!=1){
			if(strcmp(character,"-a")==0){
				messaggio->type=1;
				x=1;
				character[0]=' ';
				character[1]='\0';
				goto leggi;
			}
			else{
			if(strcmp(character,"-d")==0){
				messaggio->type=2;
				x=1;
				character[0]=' ';
				character[1]='\0';
				goto leggi;
			}
			else{
			if(strcmp(character,"-c")==0){
				messaggio->type=3;
				x=1;
				goto leggi;
			}
			printf("Il messaggio ricevuto non ha senso connessione con client diverso da quello ufficiale... procedo alla disconnessione\n");
			removeSock(sock);
			}
			}
			}
			if(x==1){
				if(strcmp(character,"-")==0){
					fine=1;
				}
				else{
					local[c]=character[0];
					c++;
				}
			}
			if(fine==1 && strcmp(character,"f")==0){
				local[c-1]='\0';
				messaggio->text=(char *)malloc(sizeof(char)*strlen(local));
				strcpy(messaggio->text,local);
				return messaggio;
			}
		}
	}
	



int ProcessCommand(msg *messaggio, int my_sock){

	list *anotherElement=malloc(sizeof(list));
	int count=0;
	char string[NUMROW*NUMCOL];
	int i=0;
	int c=0;
	int d=0;
	char mess[1024];
	int scritto=0;
	char *local=(char*)malloc(sizeof(char)*50);
	char *local_code=(char*)malloc(sizeof(char)*50);
	int array[NUMROW*NUMCOL];
	msg *risposta=malloc(sizeof(msg));
	msg *risposta1=malloc(sizeof(msg));
	list *newElement=malloc(sizeof(list));
	pthread_mutex_lock(&mutex_a);
	if(messaggio->type==1){
		strcpy(mess,messaggio->text);
		risposta->type=1;
		risposta->text=(char *)malloc(sizeof(char)*strlen(messaggio->text));
		strcpy(risposta->text,messaggio->text);
		local=strtok(messaggio->text," ");
		while(local!=NULL){
			array[d]=atoi(local);
			d++;
			local=strtok(NULL," ");
		}
		for(i=0;i<d;i++){
			if(matrix[array[i]]==0){
				risposta->text="e";
				risposta->type=3;
				risposta->sock_dest=my_sock;
				newElement->text=risposta;
				inserisciLista(highprio_t,newElement,1);
				pthread_mutex_unlock(&mutex_a);
				return 0;
			}
		}
		for(i=0;i<d;i++){
			matrix[array[i]]=0;
		}
		risposta1->type=3;
		risposta1->sock_dest=my_sock;
		risposta1->text=malloc(sizeof(char)*10);
		sprintf(risposta1->text,"%d",getCode(mess));
		newElement->text=risposta1;
		inserisciLista(highprio_t,newElement,1);
		anotherElement->text=risposta;
		inserisciLista(lowprio_t,anotherElement,2);
	}
	if(messaggio->type==2){
		list *newElement=malloc(sizeof(list));
		local_code=strtok(messaggio->text," ");
		risposta->type=2;
		if(code[atoi(local_code)]!=NULL){
		risposta->text=(char*)malloc(sizeof(char)*strlen(code[atoi(local_code)]));
		strcpy(risposta->text,code[atoi(local_code)]);
		local=strtok(code[atoi(local_code)]," ");
		while(local!=NULL){
			matrix[atoi(local)]='1';
			local=strtok(NULL," ");
		}
		code[atoi(local_code)]=NULL;
		newElement->text=risposta;
		inserisciLista(lowprio_t,newElement,2);
		risposta1->text=malloc(sizeof(char)*4);
		strcpy(risposta1->text," ct");
		risposta1->type=3;
		anotherElement->text=risposta1;
		risposta1->sock_dest=my_sock;
		if(my_sock==-1){
			free(anotherElement);
			return -1;
		}
		inserisciLista(lowprio_t,anotherElement,1);
		pthread_mutex_lock(&backup_mut);
		backup=fopen("server_backup_file.txt","a");
		if(backup==NULL){
			printf("Fatal error\n");
			exit(0);
		}
		if(fprintf(backup,"%s\n",local_code)<0){
			printf("Fatal error\n");
			exit(0);
		}
		count=0;
		fclose(backup);
		pthread_mutex_unlock(&backup_mut);
		}
		else{
			risposta->type=3;
			risposta->sock_dest=my_sock;
			risposta->text=malloc(sizeof(char)*4);
			strcpy(risposta->text,"cnt");
			newElement->text=risposta;
			inserisciLista(highprio_t,newElement,1);
		}
	}
	pthread_mutex_unlock(&mutex_a);
}
	int WriteCommand(msg* messaggio){
		cons_list *local_head=header;		//lista definita nel server come lista di socket gestite
		pthread_mutex_lock(&mutex_socket);
		char testo[4096], buffer[4096];
		int done=0;
		int i;
		if(messaggio->type!=3){
			if(messaggio->type==1){
				sprintf(testo,"-a %s -f",messaggio->text);
			}
			if(messaggio->type==2){
				sprintf(testo,"-d %s -f",messaggio->text);
			}
			for(i=0;i<max_client+1;i++){
				if(sockets[i]!=-1){
					write(sockets[i],testo,strlen(testo));
				}
			}
		}
		else{
			sprintf(buffer,"%s",messaggio->text);
			sprintf(testo,"-c %s -f",messaggio->text);
			actual_sock=messaggio->sock_dest;
			if(write(actual_sock,testo,strlen(testo))==-1){
				messaggio->type=2;
				strcpy(messaggio->text,buffer);
				ProcessCommand(messaggio,-1);
			}
			else{
			done=1;
			}
		}
		pthread_mutex_unlock(&mutex_socket);
		return done;
}	
		