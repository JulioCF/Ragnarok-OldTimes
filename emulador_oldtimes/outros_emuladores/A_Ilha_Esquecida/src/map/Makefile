
COMMON_OBJ = ../common/obj_all/core.o ../common/obj_all/socket.o ../common/obj_all/timer.o \
	../common/obj_all/db.o ../common/obj_all/plugins.o ../common/obj_all/lock.o \
	../common/obj_all/nullpo.o ../common/obj_all/malloc.o ../common/obj_all/showmsg.o \
	../common/obj_all/utils.o ../common/obj_all/strlib.o ../common/obj_all/grfio.o \
	../common/obj_all/mapindex.o ../common/obj_all/ers.o
COMMON_H = ../common/core.h ../common/socket.h ../common/timer.h \
	../common/db.h ../common/plugins.h ../common/lock.h \
	../common/nullpo.h ../common/malloc.h ../common/showmsg.h \
	../common/utils.h ../common/strlib.h ../common/grfio.h \
	../common/mapindex.h ../common/ers.h

COMMON_SQL_OBJ = ../common/obj_sql/sql.o
COMMON_SQL_H = ../common/sql.h

MAP_OBJ = map.o chrif.o clif.o pc.o status.o npc.o \
	npc_chat.o chat.o path.o itemdb.o mob.o script.o \
	storage.o skill.o atcommand.o charcommand.o battle.o \
	intif.o trade.o party.o vending.o guild.o pet.o \
	log.o mail.o date.o unit.o mercenary.o
MAP_TXT_OBJ = $(MAP_OBJ:%=obj_txt/%)
MAP_SQL_OBJ = $(MAP_OBJ:%=obj_sql/%)
MAP_H = map.h chrif.h clif.h pc.h status.h npc.h \
	chat.h itemdb.h mob.h script.h path.h \
	storage.h skill.h atcommand.h charcommand.h battle.h \
	intif.h trade.h party.h vending.h guild.h pet.h \
	log.h mail.h date.h unit.h mercenary.h

HAVE_MYSQL=yes
ifeq ($(HAVE_MYSQL),yes)
	ALL_DEPENDS=txt sql
	SQL_DEPENDS=map-server_sql
else
	ALL_TARGET=txt
	SQL_DEPENDS=needs_mysql
endif

HAVE_PCRE=
ifeq ($(HAVE_PCRE),yes)
	PCRE_CFLAGS=-DPCRE_SUPPORT 
else
	PCRE_CFLAGS=
endif



#####################################################################
.PHONY : all txt sql clean help

all: $(ALL_DEPENDS)

txt: map-server

sql: $(SQL_DEPENDS)

clean:
	rm -rf *.o obj_txt obj_sql ../../map-server.exe ../../map-server_sql.exe

help:
ifeq ($(HAVE_MYSQL),yes)
	@echo "possible targets are 'sql' 'txt' 'all' 'clean' 'help'"
	@echo "'sql'   - map server (SQL version)"
else
	@echo "possible targets are 'txt' 'all' 'clean' 'help'"
endif
	@echo "'txt'   - map server (TXT version)"
	@echo "'all'   - builds all above targets"
	@echo "'clean' - cleans builds and objects"
	@echo "'help'  - outputs this message"

#####################################################################

needs_mysql:
	@echo "MySQL not found or disabled by the configure script"
	@exit 1

# object directories
obj_txt:
	-mkdir obj_txt

obj_sql:
	-mkdir obj_sql

# executables
map-server: obj_txt $(MAP_TXT_OBJ) $(COMMON_OBJ)
	gcc  -L/usr/lib -o ../../map-server.exe $(MAP_TXT_OBJ) $(COMMON_OBJ) -lm -lz  

map-server_sql: obj_sql $(MAP_SQL_OBJ) $(COMMON_OBJ) $(COMMON_SQL_OBJ)
	gcc  -L/usr/lib -o ../../map-server_sql.exe $(MAP_SQL_OBJ) $(COMMON_OBJ) $(COMMON_SQL_OBJ) -lm -lz   -L/usr/local/lib/mysql -lmysqlclient -lz -lcrypt -lm

# map object files
obj_txt/%.o: %.c $(MAP_H) $(COMMON_H)
	gcc -g -O2 -pipe -ffast-math -Wall -Wno-sign-compare -Wno-unused-parameter -Wno-switch $(PCRE_CFLAGS) -DTXT_ONLY  -I../common -I/usr/include -DFD_SETSIZE=4096 -DCYGWIN -c $(OUTPUT_OPTION) $<

obj_sql/%.o: %.c $(MAP_H) $(COMMON_H) $(COMMON_SQL_H)
	gcc -g -O2 -pipe -ffast-math -Wall -Wno-sign-compare -Wno-unused-parameter -Wno-switch $(PCRE_CFLAGS) -I/usr/local/include/mysql  -I../common -I/usr/include -DFD_SETSIZE=4096 -DCYGWIN -c $(OUTPUT_OPTION) $<

# missing common object files
../common/obj_all/%.o:
	@$(MAKE) -C ../common txt

../common/obj_sql/%.o:
	@$(MAKE) -C ../common sql
