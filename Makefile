CC = g++
ODIR = obj
PROG = gmake
CXXFLAGS = -std=c++17 -lstdc++fs

OBJS = $(ODIR)/edge.o $(ODIR)/graph.o $(ODIR)/node.o $(ODIR)/utils.o $(ODIR)/gmake.o $(ODIR)/parser.o
$(PROG) : $(ODIR) $(OBJS)
	$(CC) -o $@ $(OBJS) $(CXXFLAGS)

$(ODIR)/edge.o : ./Graph/src/edge.cpp ./Graph/include/edge.h ./Graph/include/node.h ./Graph/include/dataInterface.h ./Graph/include/utils.h
	$(CC) -c $< -o $@ $(CXXFLAGS)

$(ODIR)/graph.o : ./Graph/src/graph.cpp ./Graph/include/graph.h ./Graph/include/graphEntry.h ./Graph/include/edge.h ./Graph/include/node.h ./Graph/include/dataInterface.h ./Graph/include/utils.h
	$(CC) -c $< -o $@ $(CXXFLAGS)

$(ODIR)/node.o : ./Graph/src/node.cpp ./Graph/include/node.h ./Graph/include/dataInterface.h ./Graph/include/utils.h
	$(CC) -c $< -o $@ $(CXXFLAGS)

$(ODIR)/utils.o : ./Graph/src/utils.cpp ./Graph/include/utils.h
	$(CC) -c $< -o $@ $(CXXFLAGS)

$(ODIR)/gmake.o : ./src/gmake.cpp ./Graph/include/graph.h ./Graph/include/graphEntry.h ./Graph/include/edge.h ./Graph/include/node.h ./Graph/include/dataInterface.h ./Graph/include/utils.h ./include/parser.h
	$(CC) -c $< -o $@ $(CXXFLAGS)

$(ODIR)/parser.o : ./src/parser.cpp ./include/parser.h
	$(CC) -c $< -o $@ $(CXXFLAGS)

$(ODIR) :
	if [ ! -d $(ODIR) ]; then mkdir $(ODIR); fi

.PHONY : clean
clean :
	if [ -d $(ODIR) ]; then rm $(ODIR) -r; fi
	if [ -f $(PROG) ]; then rm $(PROG); fi

.PHONY : install
install :
	sudo cp gmake /bin/gmake
