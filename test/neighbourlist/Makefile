all: NLTest.cc NLTest.h MockNeighbour.cc MockNeighbour.h
	rsync ~/workspace/sandridge/routing/neighbours/NeighbourList.cc .
	rsync ~/workspace/sandridge/routing/neighbours/NeighbourList.h .
	rsync ~/workspace/sandridge/routing/floodingRouting/FloodingNeighbourList.cc .
	rsync ~/workspace/sandridge/routing/floodingRouting/FloodingNeighbourList.h .
	rsync ~/workspace/sandridge/routing/randomRouting/RandomNeighbourList.cc .
	rsync ~/workspace/sandridge/routing/randomRouting/RandomNeighbourList.h .
	@sed -i "s/castaliaModule->trace()/cout/g" *.cc *.h
	@sed -i "s/\"Neighbour\.h\"/\"MockNeighbour\.h\"/g" *.cc *.h
	@sed -i "s/\.\.\/neighbours\/NeighbourList\.h/NeighbourList\.h/g" *.cc *.h
	@sed -i "s/\"VirtualMac\.h\"/<iostream>/g" NeighbourList.h
	@sed -i "s/<string>/<string>\n\#include\ <iostream>/g" RandomNeighbourList.h
	@sed -i "s/\"RandomNeighbourList\.h\"/\"RandomNeighbourList\.h\"\n\#include\ <stdio.h>\n\#include\ <cstdlib>\n\#include\ <cmath>\n\nusing\ namespace\ std;/g" RandomNeighbourList.cc
	g++ MockNeighbour.cc NeighbourList.cc FloodingNeighbourList.cc RandomNeighbourList.cc NLTest.cc -lcpptest -o nltest
	
#sed -i "s/include\"VirtualMac\.h\"/include\<stdio\>/g" NeighbourList.h


.PHONY:
clean:
	rm -f nltest
	rm -f *~
	rm -i NeighbourList.* FloodingNeighbour* RandomNeighbour*
