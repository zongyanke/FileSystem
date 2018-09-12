build : main.o FileSystem.o
	g++ -o build main.o FileSystem.o
main.o : main.cpp FileSystem.h
	g++ -c main.cpp
FileSystem.o : FileSystem.h FileSystem.cpp
	g++ -c FileSystem.cpp

clean :
	rm main.o FileSystem.o build FileSystem