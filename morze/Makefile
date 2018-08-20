OPTIONS=-fsanitize=address -fsanitize=undefined
COMPILE=g++ -std=c++14 -Wall -Wextra -c $(OPTIONS)
LINK=-lsfml-graphics -lsfml-window -lsfml-system -lsfml-network $(OPTIONS)

all: main

main: main.o
	g++ -o main main.o $(LINK)

main.o: main.cpp
	$(COMPILE) main.cpp

clean:
	$(RM) main *.o
