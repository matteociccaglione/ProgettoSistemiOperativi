//Installer application for server

#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<sys/stat.h>
int main(){
		int newOut;
		printf("Welcome in the installer for server application\n");
		int row,col;
		char buffer[1024];
		printf("Please digit the number of rows and the number of place for rows in the follow format <numOfRows> <numOfPlaces>\n\n For example i have 6 rows and 5 places for rows so i digit 6 5 \n\n");
		scanf("%d %d",&row,&col);
		printf("Stiamo salvando le tue informazioni\n\n\n");
		int fd,dd,cd;
		fd=open("describer.h",O_CREAT|O_TRUNC|O_WRONLY,0666);
		sprintf(buffer,"\n#define NUMROW %d \n#define NUMCOL %d\n ",row,col);
		write(fd,buffer,strlen(buffer));
		int backup;
		backup=open("server_backup_file.txt",O_CREAT|O_TRUNC|O_WRONLY,0666);
		printf("Operazione completata\n");
}