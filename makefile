CC = cc
CFLAG = -I./include/util -I./include/server -g -D _D
LIBFLAG = -L lib/ -lFlyUtil -lpthread -lmysqlclient -ldl
CPATH = ./src/
OPATH = ./build/
OUTPATH = ./bin/
SOURCE = $(wildcard $(CPATH)*.c)
OBJECTS = $(patsubst $(CPATH)%.c, %.o, $(SOURCE))

vpath %.a lib
vpath %.o build
vpath %.c src

ALL: $(OBJECTS) main.o
	$(CC) $(patsubst %.o, $(OPATH)%.o, $^) $(LIBFLAG) -o $(OUTPATH)server
main.o: ./main.c
	$(CC) -c $(CFLAG) $< -o $(OPATH)$@
$(OBJECTS):%.o:%.c
	$(CC) -c $(CFLAG) $< -o $(OPATH)$@

.PHONY: clear

clear:
	-rm $(OPATH)*.o $(OUTPATH)server
