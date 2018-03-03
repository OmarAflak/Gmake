CC = g++
ODIR = obj
PROG = gmake
CXXFLAG = -lboost_system -lboost_filesystem

$(PROG) : $(ODIR) $(ODIR)/$(PROG).o
	$(CC) -o $@ $(ODIR)/$(PROG).o $(ODIR)/Utils.o $(ODIR)/Node.o $(ODIR)/Vertex.o $(ODIR)/Graph.o $(CXXFLAG)

$(ODIR)/Utils.o : ./Graph/src/Utils.cpp ./Graph/include/Utils.h
	$(CC) -c $< -o $@

$(ODIR)/Node.o : ./Graph/src/Node.cpp ./Graph/include/Node.h
	$(CC) -c $< -o $@

$(ODIR)/Vertex.o : ./Graph/src/Vertex.cpp ./Graph/include/Vertex.h $(ODIR)/Node.o
	$(CC) -c $< -o $@

$(ODIR)/Graph.o : ./Graph/src/Graph.cpp ./Graph/include/Graph.h $(ODIR)/Vertex.o $(ODIR)/Utils.o
	$(CC) -c $< -o $@

$(ODIR)/$(PROG).o : ./src/gmake.cpp $(ODIR)/Graph.o
	$(CC) -c $< -o $@

$(ODIR) :
	if [ ! -d $(ODIR) ]; then mkdir $(ODIR); fi

.PHONY : clean
clean :
	if [ -d $(ODIR) ]; then rm $(ODIR) -r; fi
	if [ -f $(PROG) ]; then rm $(PROG); fi

.PHONY : install
install :
	sudo mv gmake /bin
