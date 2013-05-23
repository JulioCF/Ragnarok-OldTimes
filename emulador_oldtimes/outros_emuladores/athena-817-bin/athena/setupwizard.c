#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *char_config = "conf/char_athena.conf";

char *grf_config = "conf/grf-files.txt";
char *account_txt = "account.txt";
char *map_config = "conf/map_athena.conf";

char *char_config_bak = "conf/char_athena.conf-bak";
char *grf_config_bak = "conf/grf-files.txt-bak";
char *map_config_bak = "conf/map_athena.conf-bak";


int main() {

	int accountfound;

	char line[1000];
	char r1[1000], r2[1000];

	char IP[20];
	char username[24];
	char password[24];

//Make sex and go_on strings to keep from getting \n as input. 
	char sex[2];
	char go_on[2];

	char data[200];
	char sdata[200];
	char adata[200];
	
	printf("Welcome to the Athena Configuration Wizard!\n");
	printf("This will help you configure athena with ease.\n\n");
	printf("If you screw up using it, just run it again.\n\n");

	printf("Are you sure you want to continue? (Y for yes, N for no): ");
	scanf("%s", &go_on);

	if(go_on[0] == 'N') {
		exit(0);
	}

//Check to see if account.txt exists.

	printf("Checking if account.txt exists...\n");
	FILE *FPAC=fopen(account_txt, "r");
	if(FPAC != NULL){
		printf("account.txt found.\n");
		accountfound = 1;
	} else {
		printf("account.txt not found...  let's make it now.\n\n");
		accountfound = 0;
	}
	


//Configure

	//Move all changed config files to filename-bak for backup.
	rename(char_config, char_config_bak);
	rename(grf_config, grf_config_bak);
	rename(map_config, map_config_bak);

	FILE *file=fopen(char_config_bak, "r");
	if(file == NULL){
		printf("char_athena.conf not found!\n");
		exit(0);
	}

	while( fgets(line,1000,file) ) {
		if(line[0] == '/' && line[1] == '/') { continue; }
		sscanf(line,"%[^:]: %[^\r\n$]", r1, r2);
		if( strcmp(r1, "login_ip") == 0 ) {
			memcpy(IP,r2,20);
		}
	}

	FILE *file2=fopen(grf_config_bak, "r");
	if(file == NULL){
		printf("char_athena.conf not found!\n");
		exit(0);
	}

	FILE *file3=fopen(map_config_bak, "r");
        if(file == NULL){
                printf("conf/map_athena.conf not found!\n");
                exit(0);
        }

	while( fgets(line,1000,file2) ) {
		if(line[0] == '/' && line[1] == '/') { continue; }
		sscanf(line,"%[^:]: %[^\r\n$]", r1, r2);
		if( strcmp(r1, "data") == 0 ) {
			memcpy(data,r2,200);
		}
		if( strcmp(r1, "sdata") == 0 ) {
                        memcpy(sdata,r2,200);
                }
		if( strcmp(r1, "adata") == 0 ) {
                        memcpy(adata,r2,200);
                }
	}

	if (accountfound == 0) {
		printf("Enter a username: ");
                scanf("%s",&username);
                printf("Enter a password: ");
                scanf("%s",&password);
                printf("Enter a gender (M for male, F for female): ");
                scanf("%s", &sex);
	}

	printf("Enter your IP: [%s]: ", IP);
	scanf("%s", &IP);
	printf("Enter your path to data.grf: "); // data);
	scanf("%s", &data);
	printf("Enter your path to sdata.grf: "); // sdata);
        scanf("%s", &sdata);
	printf("Enter your path to adata.grf: "); // adata);
        scanf("%s", &adata);


	//Set file pointer back to 0
	rewind(file);


	FILE *file_in=fopen(char_config, "w");
	while( fgets(line,1000,file) ) {
                sscanf(line,"%[^:]: %[^\r\n$]", r1, r2);
                if( strcmp(r1, "login_ip") == 0 ) {
			fprintf(file_in, "login_ip: %s\r\n",IP);
		} else if ( strcmp(r1, "char_ip") == 0 ) {
			fprintf(file_in, "char_ip: %s\r\n",IP);
		} else {
			fprintf(file_in, "%s",line);
		}
	}
	close(file_in);

	FILE *file_in2=fopen(map_config, "w");
        while( fgets(line,1000,file3) ) {
                sscanf(line,"%[^:]: %[^\r\n$]", r1, r2);
                if( strcmp(r1, "map_ip") == 0 ) {
                        fprintf(file_in2, "map_ip: %s\r\n",IP);
                } else if ( strcmp(r1, "char_ip") == 0 ) {
                        fprintf(file_in2, "char_ip: %s\r\n",IP);
                } else {
                        fprintf(file_in2, "%s",line);
                }
        }
        close(file_in2);

	rewind(file2);

	FILE *file_in3=fopen(grf_config, "w");
        while( fgets(line,1000,file2) ) {
                sscanf(line,"%[^:]: %[^\r\n$]", r1, r2);
                if( strcmp(r1, "data") == 0 ) {
                        fprintf(file_in3, "data: %s\r\n",data);
                } else if ( strcmp(r1, "sdata") == 0 ) {
                        fprintf(file_in3, "sdata: %s\r\n",sdata);
		} else if ( strcmp(r1, "adata") == 0 ) {
			fprintf(file_in3, "adata: %s\r\n",adata);			
                } else {
                        fprintf(file_in3, "%s",line);
                }
        }
        close(file_in3);

	fclose(file);
	fclose(file2);
	fclose(file3);

	//fclose(FPAC);


	if (accountfound == 0) {
		FILE *FPaccout=fopen(account_txt, "w");
		fprintf(FPaccout, "1	s1	p1	-	S	-\r\n");
		fprintf(FPaccout, "1000000	%s	%s	-	%s	-\r\n", username, password, sex);
        	close(FPaccout);
	}

	printf("Athena is now setup.\n");

}
