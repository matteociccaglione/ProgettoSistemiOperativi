install:
		gcc server_installer.c -o installer
		./installer
		gcc server.c -o serverApplication -lpthread
		
clear:
		rm installer
		rm serverApplication
		rm server_backup_file.txt
		rm describer.h