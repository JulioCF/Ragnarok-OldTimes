/*
	This program adds a user to account.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *account_txt = "../account.txt";

int main(int argc, char *argv[]) {

	char username[24];
	char password[24];
	char sex[2];

	int nid,lid;
	char line[1000];


//Check to see if account.txt exists.

	printf("Checking if account.txt exists...\n");
	FILE *FPaccin=fopen(account_txt, "r");
	if(FPaccin == NULL){
		printf("account.txt not found!\nRun the setup wizard.\n");
		exit(0);
	}
	while( fgets(line,1000,FPaccin) ) {
                if(line[0] == '/' && line[1] == '/') { continue; }
                sscanf(line,"%i%[^	]", &lid);
	}
	close(FPaccin);

	nid = lid + 1;

	FILE *FPaccout=fopen(account_txt, "r+");

	printf("Enter a username: ");
	scanf("%s",&username);

	printf("Enter a password: ");
	scanf("%s",&password);

	printf("Enter a gender (M for male, F for female): ");
	scanf("%s", &sex);

	fseek(FPaccout, 0, SEEK_END);

	fprintf(FPaccout, "%i	%s	%s	-	%s	-\r\n", nid, username, password, sex);
	printf("Account added.\n");

	close(FPaccout);

}
