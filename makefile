CC=g++ 
CFLAGS=-Wall
INC= -I./rapidxml
LINKER_FLAGS = -lSDL2 -std=c++11

OBJS = ArcadeMenu.cpp audio.cpp
OBJ_NAME = ArcadeMenu

all : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)

clean:
	rm -f $(OBJ_NAME)
stop:
	-pkill ArcadeMenu
run: ArcadeMenu
	./ArcadeMenu
install:
	cp ./ArcadeMenu ~/ArcadeMenu/
	cp ./ArcadeMenuConfig.xml ~/ArcadeMenu/