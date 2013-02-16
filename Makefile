CC = g++
CFLAGS = -g -Wall
LIBS = -lm -lcv

all: main.cpp FinderPattern.cpp FinderPatternTrio.cpp qrReader.cpp
	$(CC) $(CFLAGS) $(LIBS) -o qrcode main.cpp FinderPattern.cpp FinderPatternTrio.cpp qrReader.cpp


clean:
	$(RM) *.o *~
    
