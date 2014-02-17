all: McprngTest.cc McprngTest.h
	rsync ~/workspace/sandridge/application/sandridge/SandridgeRandomGenerator.cc .
	rsync ~/workspace/sandridge/application/sandridge/SandridgeRandomGenerator.h .
	g++ -std=gnu++11 SandridgeRandomGenerator.cc McprngTest.cc -lcpptest -o testmcprng


.PHONY:
clean:
	rm -f mcprng
	rm -f *~
	rm -if SandridgeRandomGenerator.cc SandridgeRandomGenerator.h
