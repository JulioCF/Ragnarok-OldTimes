/*
    Este programa adiciona um usuario no account.txt
	Nunca use este programa quando o login-server estiver online.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *account_txt = "../save/account.txt";

//-----------------------------------------------------
// Function to suppress control characters in a string.
//-----------------------------------------------------
int remove_control_chars(unsigned char *str) {
	int i;
	int change = 0;

	for(i = 0; str[i]; i++) {
		if (str[i] < 32) {
			str[i] = '_';
			change = 1;
		}
	}

	return change;
}

int main(int argc, char *argv[]) {

	char username[24];
	char password[24];
	char sex[2];

	int next_id, id;
	char line[1024];

	// verificando a existecia do arquivo account_txt.
	printf("Verificando se o arquivo'%s' existe...\n", account_txt);
	FILE *FPaccin = fopen(account_txt, "r");
	if (FPaccin == NULL) {
		printf("Arquivo '%s' nao encontrado!\n", account_txt);
		printf("Reinstale o aplication.\n");
		exit(0);
	}

	next_id = 2000000;
	while(fgets(line, sizeof(line)-1, FPaccin)) {
		if (line[0] == '/' && line[1] == '/') { continue; }
		if (sscanf(line, "%d\t%%newid%%\n", &id) == 1) {
			if (next_id < id) {
				next_id = id;
			}
		} else {
			sscanf(line,"%i%[^	]", &id);
			if (next_id <= id) {
				next_id = id +1;
			}
		}
	}
	close(FPaccin);
	printf("O arquivo existe.\n");

	printf("Nunca crie uma conta se o login-server estiver online!!!\n");
	printf("Se o login-server estiver online, pressione ctrl+C agora para finalizar este programa.\n");
	printf("\n");

	strcpy(username, "");
	while (strlen(username) < 4 || strlen(username) > 23) {
		printf("Informe o nome de usuario (4-23 caracteres): ");
		scanf("%s", &username);
		username[23] = 0;
		remove_control_chars(username);
	}

	strcpy(password, "");
	while (strlen(password) < 4 || strlen(password) > 23) {
		printf("informe sua senha (4-23 characters): ");
		scanf("%s", &password);
		password[23] = 0;
		remove_control_chars(password);
	}

	strcpy(sex, "");
	while (strcmp(sex, "F") != 0 && strcmp(sex, "M") != 0) {
		printf("Informe o sexo (M para Masculino, F para feminino): ");
		scanf("%s", &sex);
	}

	FILE *FPaccout = fopen(account_txt, "r+");
	fseek(FPaccout, 0, SEEK_END);
	fprintf(FPaccout, "%i	%s	%s	-	%s	-\r\n", next_id, username, password, sex);
	close(FPaccout);

	printf("Conta criada com sucesso.\n");
}
