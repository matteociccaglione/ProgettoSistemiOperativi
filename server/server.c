
#include"headerS.h" //definisce la maggior parte delle funzioni e gli header di libreria
void initialize_server(){
	riordinaFile();
	backup=fopen("server_backup_file.txt","r+");
	int codice=0,i=0;
	int len=0;
	char string[1024],*local;
	for(i=0;i<NUMROW*NUMCOL;i++){
		matrix[i]=1;
		sockets[i]=-1;
	}
	while(fscanf(backup,"%s\n",string)!=EOF){
		len=strlen(string);
		local=strtok(string,"#");
		codice=atoi(local);
		code[codice]=malloc(sizeof(char*)*len);
		strcpy(code[codice],"");
		local=strtok(NULL,"#");
		while(local!=NULL){
			strcat(code[codice],local);
			strcat(code[codice]," ");
			matrix[atoi(local)]=0;
			local=strtok(NULL,"#");
		}
		if(strcmp(code[codice],"")==0){
			code[codice]=NULL;
		}
		
	}
	fclose(backup);
}
	
void initialize_client(int sock){
	char string[100+NUMROW*NUMCOL];
	char str[NUMROW*NUMCOL+1];
	int i,c;
	sprintf(string,"%d,%d,",NUMROW,NUMCOL);
	for(i=0;i<NUMROW;i++){
		for(c=0;c<NUMCOL;c++){
			if(matrix[i*NUMCOL+c]==0){
				str[i*NUMCOL+c]='0';
			}
			else{
				str[i*NUMCOL+c]='1';
			}
		}
	}
	str[NUMROW*NUMCOL]='\0';
	strcat(string,str);
	if(write(sock,string,strlen(str))==-1){
	}
	pthread_mutex_lock(&init_mut);
	pthread_mutex_lock(&mutex_socket);
	for(i=0;i<NUMROW*NUMCOL;i++){
		if(sockets[i]==-1){
			sockets[i]=sock;
			if(i>max_client){
				max_client=i;
			}
			break;
		}
	}
	pthread_mutex_unlock(&mutex_socket);
	pthread_mutex_unlock(&init_mut);
}
void *sender_thread(void *Unused){
	msg *mex;
	int num;
	sigset_t set;
	sigaddset(&set,SIGPIPE);
	sigaddset(&set,SIGINT);
	sigprocmask(SIG_UNBLOCK,&set,NULL);
	list *local;
	struct sembuf oper;
	oper.sem_num=0;
	oper.sem_op=-1;
	while(1){
		semop(sem_elem,&oper,1);
		if(highprio_h!=NULL){
			mex=highprio_h->text;
			pthread_mutex_lock(&mutex_l);
			if(highprio_t==highprio_h){
				highprio_t=NULL;
			}
			local=highprio_h;
			highprio_h=highprio_h->next;
			pthread_mutex_unlock(&mutex_l);
			WriteCommand(mex);
			free(local);
			free(mex);
		}
		if(lowprio_h!=NULL){
			mex=lowprio_h->text;
			pthread_mutex_lock(&mutex_lp);
			if(lowprio_t==lowprio_h){
				lowprio_t=NULL;
			}
			local=lowprio_h;
			lowprio_h=lowprio_h->next;
			pthread_mutex_unlock(&mutex_lp);
			WriteCommand(mex);
			free(local);
			free(mex);
		}
	}
}



void *backupThread(void *unused){
	while(1){
	sleep(300);
	riordinaFile();
	}
}
	
void pipe_handler(int sig){
	printf("SIGPIPE received client disconnected...\n");
	removeSock(actual_sock);
}

void *thread_f(void *sock){
	int my_sock=*(int *)sock;
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_BLOCK,&set,NULL);
	msg *mex=malloc(sizeof(msg));
	while(1){
		mex=ReadCommand(my_sock);
		if(mex==NULL){
			removeSock(my_sock);
			pthread_exit(NULL);
		}
		ProcessCommand(mex,my_sock);
	}
}

void *thread_User(void *Unused){
		char risposta[1024],string[1024],newString[1024];
		char *local;
		char posto[1024];
		int row,col,num;
		int i,c,r;
		while(1){
		scanf("%s",risposta);
		if(strcmp(risposta,"status")==0){
			system("clear");
			riordinaFile();
			pthread_mutex_lock(&backup_mut);
			backup=fopen("server_backup_file.txt","r+");
			printf("Status del file backup: \n");
			while(fscanf(backup,"%s\n",string)!=EOF){
				local=strtok(string,"#");
				i=atoi(local);
				strcpy(newString,"");
				local=strtok(NULL,"#");
				while(local!=NULL){
					num=atoi(local);
					row=0;
					col=0;
					while(row*NUMCOL<num){
						row++;
					}
					if(row*NUMCOL!=num)
						row--;
					col=num-row*NUMCOL;
					sprintf(posto,"%d,%d",row,col);
					strcat(newString,posto);
					strcat(newString," ");
					local=strtok(NULL,"#");
				}
				printf("CODE: %d VALUE: %s\n",i,newString);
			}
			fclose(backup);
			pthread_mutex_unlock(&backup_mut);
			printf("\nStatus attuale della sala: \n");
			pthread_mutex_lock(&mutex_a);
			for (r=0;r<NUMROW;r++){
				printf("%d: ",r+1);
				for(c=0;c<NUMCOL;c++){
					if(matrix[r*NUMCOL +c]==0){
						printf("x ");
					}
					else{
						printf("%d ",r*NUMCOL+c);
					}
				}
				printf("\n");
			}
			pthread_mutex_unlock(&mutex_a);
		}
		else{
			if(strcmp(risposta,"quit")==0){
				printf("Good Bye\n");
				exit(1);
			}
		}
		fflush(stdin);
}
}
int main(int argc, char **argv){
	if(argc>2){
		printf("Avvia il server senza parametri o specificando il port number\n");
	}
	int port_number;
	if(argc==2){
		int charact=0;
		while(charact<strlen(argv[1])){
			if(argv[1][charact]<48||argv[1][charact]>57){
				printf("Input invalido un portNumber contiene solo numeri\n");
				exit(EXIT_FAILURE);
			}
			charact++;
		}
		port_number=atoi(argv[1]);
	}
	else{
		port_number=25001;
	}
	pthread_mutex_init(&init_mut,NULL);
	pthread_mutex_init(&backup_mut,NULL);
	sem_elem=semget(21,1,IPC_CREAT|0666);
	semctl(sem_elem,0,SETVAL,0);
	int on=1,r;
	code=(char **)malloc(sizeof(char*)*NUMROW*NUMCOL);
	for(r=0;r<NUMROW*NUMCOL;r++){
		code[r]=NULL;
	}
	initialize_server();
	pthread_mutex_init(&mutex_a,NULL);
	pthread_mutex_init(&mutex_d,NULL);
	pthread_mutex_init(&mutex_socket,NULL);
	pthread_mutex_init(&mutex_l,NULL);
	pthread_mutex_init(&mutex_lp,NULL);
	
	struct sigaction act;
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_BLOCK,&set,NULL);
	act.sa_handler=pipe_handler;
	act.sa_mask=set;
	sigaction(SIGPIPE,&act,NULL);
	int listen_s,cons_s;
	listen_s=socket(AF_INET,SOCK_STREAM,0);
	setsockopt(listen_s,SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	struct sockaddr_in addr;
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port_number);
	addr.sin_addr.s_addr=INADDR_ANY;
	if(bind(listen_s,(struct sockaddr *)&addr,sizeof(addr))==-1){
		printf("Fatal error impossibile connettere il server si prega di riavviare l'applicazione\n");
		exit(0);
	}
	else{
		printf("Server correttamente inizializzato\n");
	}
	pthread_t printThread;
	pthread_create(&printThread,NULL,thread_User,(void *)NULL);
	pthread_t backupThreadF;
	pthread_create(&backupThreadF,NULL,backupThread,(void *)NULL);
	listen(listen_s,50);
	struct sockaddr_in client_addr;
	pthread_t threads[NUMROW*NUMCOL];
	int client_size;
	int i=0;
	pthread_t sendert;
	pthread_create(&sendert,NULL,sender_thread,(void *)NULL);
	while(1){
		cons_s=accept(listen_s,(struct sockaddr*)&client_addr,&client_size);
		initialize_client(cons_s);
		pthread_create(threads+i,NULL,thread_f,(void *)&cons_s);
		i++;
	}
}

