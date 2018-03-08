CC = g++
ODIR = obj
PROG = gmake
CXXFLAG = -std=c++11 -lboost_system -lboost_filesystem

$(PROG) : $(ODIR) $(ODIR)/Graph.o $(ODIR)/Utils.o $(ODIR)/Node.o $(ODIR)/Vertex.o $(ODIR)/gmake.o $(ODIR)/parser.o
	$(CC) -o $@ $(ODIR)/Graph.o $(ODIR)/Utils.o $(ODIR)/Node.o $(ODIR)/Vertex.o $(ODIR)/gmake.o $(ODIR)/parser.o $(CXXFLAG)

$(ODIR)/Graph.o : ./Graph/src/Graph.cpp ./Graph/include/Graph.h ./Graph/include/Vertex.h ./Graph/include/Utils.h
	$(CC) -c $< -o $@ $(CXXFLAG)

$(ODIR)/Utils.o : ./Graph/src/Utils.cpp ./Graph/include/Utils.h
	$(CC) -c $< -o $@ $(CXXFLAG)

$(ODIR)/Node.o : ./Graph/src/Node.cpp ./Graph/include/Node.h
	$(CC) -c $< -o $@ $(CXXFLAG)

$(ODIR)/Vertex.o : ./Graph/src/Vertex.cpp ./Graph/include/Vertex.h ./Graph/include/Node.h
	$(CC) -c $< -o $@ $(CXXFLAG)

$(ODIR)/gmake.o : ./src/gmake.cpp ./Graph/include/Graph.h ./include/parser.h
	$(CC) -c $< -o $@ $(CXXFLAG)

$(ODIR)/parser.o : ./src/parser.cpp ./include/parser.h
	$(CC) -c $< -o $@ $(CXXFLAG)

$(ODIR) :
	if [ ! -d $(ODIR) ]; then mkdir $(ODIR); fi

.PHONY : clean
clean :
	if [ -d $(ODIR) ]; then rm $(ODIR) -r; fi
	if [ -f $(PROG) ]; then rm $(PROG); fi

.PHONY : install
install :
	sudo cp gmake /bin/gmake
