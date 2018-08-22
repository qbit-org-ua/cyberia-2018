OPTIONS=-fsanitize=address -fsanitize=undefined -I../SFML-2.5.0/include -L../SFML-2.5.0/lib -D_GLIBCXX_USE_CXX11_ABI=0
COMPILE=g++ -std=c++14 -Wall -Wextra -c $(OPTIONS)
LINK=-lsfml-graphics -lsfml-window -lsfml-system -lsfml-network $(OPTIONS)

all: main

main: main.o
	g++ -o main main.o $(LINK)

main.o: main.cpp
	$(COMPILE) main.cpp

clean:
	$(RM) main *.o
