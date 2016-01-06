prefix := /usr/local
CCFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -l json -l rf24-bcm 

# define all programs
PROGRAMS = sender_p
SOURCES = ${PROGRAMS:=.cpp}



all: ${PROGRAMS}

${PROGRAMS}: ${SOURCES}
	g++ `mysql_config --cflags` -o $@ -Wall -I../ $@.cpp ${CCFLAGS} `mysql_config --libs`

clean:
	rm -rf $(PROGRAMS)

