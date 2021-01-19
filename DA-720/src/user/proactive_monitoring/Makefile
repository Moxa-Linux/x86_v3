CC=gcc
STRIP=strip

CFLAGS=-I. `net-snmp-config --cflags`
# shared library flags (assumes gcc)
DLFLAGS=-fPIC -shared

all: ini.o pro_mon_fun.o snmpso

	@$(CC) pro_mon.c ini.o pro_mon_fun.o -lpthread -lncurses -o pro_mon
	@$(CC) pro_mond.c ini.o pro_mon_fun.o -lpthread -o pro_mond
	@$(CC) $(CFLAGS) $(DLFLAGS) -o proactiveMonitoring.so \
proactiveMonitoring.o logicProcessorTable.o partitionStatusTable.o \
networkStatusTable.o serialPortStatusTable.o

snmpso:
	@$(CC) $(CFLAGS) $(DLFLAGS) -c proactiveMonitoring.c
	@$(CC) $(CFLAGS) $(DLFLAGS) -c logicProcessorTable.c
	@$(CC) $(CFLAGS) $(DLFLAGS) -c partitionStatusTable.c
	@$(CC) $(CFLAGS) $(DLFLAGS) -c networkStatusTable.c
	@$(CC) $(CFLAGS) $(DLFLAGS) -c serialPortStatusTable.c

ini.o:
	@$(CC) -c ini.c
pro_mon_fun.o:
	@$(CC) -c pro_mon_fun.c
clean:
	@rm -f *.o
	@rm -f proactiveMonitoring.so
	@rm -f pro_mon
	@rm -f pro_mond
