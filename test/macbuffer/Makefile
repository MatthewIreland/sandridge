all: MacBufferTest.cc MacBufferTest.h
	rsync ~/workspace/sandridge/mac/macBuffer/MacBuffer.cc .
	rsync ~/workspace/sandridge/mac/macBuffer/MacBuffer.h .
	rsync ~/workspace/sandridge/mac/macBuffer/MacBufferFullException.cc .
	rsync ~/workspace/sandridge/mac/macBuffer/MacBufferFullException.h .
	g++ MacBuffer.cc MacBufferTest.cc -lcpptest -o macbuffertest
#	g++ -Wall -pedantic -g -c MacBuffer.cc
#	g++ -Wall -pedantic -g -c MacBufferTest.cc
#	g++ -g MacBuffer.o MacBufferTest.o -lcpptest -o macbuffertest

.PHONY:
clean:
	rm -f macbuffertest
	rm -f *~
	rm -if MacBuffer.cc MacBuffer.h
