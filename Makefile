CC = g++
Sources = main.cpp
Modules = loader.o edit_conf.o
CFlags = -c
WallFlag = #-Wall 
OutPut = PacManCPP
curses = -lncurses

all: edit_conf loader
	$(CC) $(CFlags) $(WallFlag) $(Sources) -o main.o
	$(CC) main.o $(Modules) -o $(OutPut) $(curses)
	chmod u=rwx,g=rx,o=rx ./$(OutPut)
	./$(OutPut)

edit_conf:
	$(CC) $(CFlags) $(WallFlag) ./header/edit_conf.cpp

loader:
	$(CC) $(CFlags) $(WallFlag) ./header/loader.cpp

clean:
	rm -rf *.o *.pac

clean-all:
	rm -rf *.o *.pac
	rm -rf $(OutPut)