# Build Q and QLOAD programs for Linux

Q_OBJS= \
  q.o        \
  qlmsg.o      \
  qlcs.o       \
  qlparse.o \
  qlaccess.o \
  qlstruct.o \
  qlvalues.o \
  qlutil.o


QLOAD_OBJS= \
  qload.o    \
  qloadfnc.o \
  qlcs.o       \
  qlvalues.o \
  qlutil.o
 
MQLIB=/opt/mqm/lib64
MQINC=/opt/mqm/inc
LIBS=-lpthread -ldl -lmqm
CFLAGS= -I$(MQINC) -I. -DAMQ_UNIX -D_LINUX_2
PLAT=linux_x64

all: q qload
	-mkdir -p ../ship/$(PLAT)
	rm -f ../ship/$(PLAT)/* *.o
	mv q qload ../ship/$(PLAT)
	cp *.txt ../ship/$(PLAT)


q: $(Q_OBJS) Makefile
	$(CC) -o $@ $(Q_OBJS) -L$(MQLIB) $(LIBS)

qload: $(QLOAD_OBJS) Makefile
	$(CC) -o $@ $(QLOAD_OBJS) -L$(MQLIB) $(LIBS) 
