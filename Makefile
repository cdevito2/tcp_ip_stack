CC=gcc
CFLAGS=-g
TARGET=tcpstack.exe
LIBS=-lpthread -L ./CommandParser -lcli


OBJS=gluethread/glthread.o\
		  graph.o\
		  topologies.o\
		  net.o\
		  comm.o\
		  Layer2/layer2.o\
		  Layer3/layer3.o\
		  Layer5/layer5.o\
		  Layer5/ping.o\
		  nwcli.o\
		  utils.o\
		  Layer2/l2switch.o

tcpstack.exe:testapp.o ${OBJS} CommandParser/libcli.a
	${CC} ${CFLAGS} testapp.o ${OBJS} -o tcpstack.exe ${LIBS}

testapp.o:testapp.c
	${CC} ${CFLAGS} -c testapp.c -o testapp.o

gluethread/glthread.o:gluethread/glthread.c
	${CC} ${CFLAGS} -c -I gluethread gluethread/glthread.c -o gluethread/glthread.o

graph.o:graph.c
	${CC} ${CFLAGS} -c -I . graph.c -o graph.o

topologies.o:topologies.c
	${CC} ${CFLAGS} -c -I . topologies.c -o topologies.o

net.o:net.c
	${CC} ${CFLAGS} -c -I . net.c -o net.o

nwcli.o:nwcli.c
	${CC} ${CFLAGS} -c -I . nwcli.c -o nwcli.o

comm.o:comm.c
	${CC} ${CFLAGS} -c -I . comm.c  -o comm.o

utils.o:utils.c
	${CC} ${CFLAGS} -c -I . utils.c -o utils.o

Layer2/layer2.o:Layer2/layer2.c
	${CC} ${CFLAGS} -c -I . Layer2/layer2.c -o Layer2/layer2.o

Layer3/layer3.o:Layer3/layer3.c
	${CC} ${CFLAGS} -c -I . Layer3/layer3.c -o Layer3/layer3.o

Layer5/layer5.o:Layer5/layer5.c
	${CC} ${CFLAGS} -c -I . Layer5/layer5.c -o Layer5/layer5.o

Layer5/ping.o:Layer5/ping.c
	${CC} ${CFLAGS} -c -I . Layer5/ping.c -o Layer5/ping.o

Layer2/l2switch.o:Layer2/l2switch.c
	${CC} ${CFLAGS} -c -I . Layer2/l2switch.c -o Layer2/l2switch.o	 

CommandParser/libcli.a:
	(cd CommandParser; make)

clean:
	rm *.o
	rm gluethread/glthread.o
	rm *exe
	(cd CommandParser; make clean)

all:
	make
	(cd CommandParser; make)
