all: char-server

COMMON_OBJ = ../common/core.o ../common/socket.o ../common/timer.o ../common/db.o
COMMON_H = ../common/core.h ../common/socket.h ../common/timer.h ../common/db.h

char-server: char.o inter.o int_party.o int_guild.o int_storage.o int_pet.o $(COMMON_OBJ)
	$(CC) -o ../$@ $>

char.o: char.c char.h inter.h int_pet.h ../common/core.h ../common/socket.h ../common/timer.h ../common/mmo.h ../common/version.h
inter.o: inter.c inter.h int_party.h int_guild.h int_storage.h int_pet.h ../common/mmo.h char.h ../common/socket.h
int_party.o: int_party.c int_party.h inter.h ../common/mmo.h char.h ../common/socket.h
int_guild.o: int_guild.c int_guild.h inter.h ../common/mmo.h char.h ../common/socket.h
int_storage.o: int_storage.c int_storage.h inter.h ../common/mmo.h char.h ../common/socket.h
int_pet.o: int_pet.c int_pet.h inter.h ../common/mmo.h char.h ../common/socket.h
clean:
	rm -f *.o ../char-server
