CC=gcc

CFLAGS= -g -I.
SQLFLAGS= `mysql_config --cflags --libs`
LDFLAGS=-O2 `mysql_config --libs`

LDLIBS=iniparser/libiniparser.a algos/libalgos.a sha3/libhash.a -lpthread -lgmp -lm -lstdc++ -lcrypto -lssl -lcurl
LDLIBS+=-lmysqlclient

SOURCES=stratum.cpp db.cpp coind.cpp coind_aux.cpp coind_template.cpp coind_submit.cpp util.cpp list.cpp \
	rpc.cpp job.cpp job_send.cpp job_core.cpp merkle.cpp share.cpp socket.cpp coinbase.cpp \
	client.cpp client_submit.cpp client_core.cpp client_difficulty.cpp remote.cpp remote_template.cpp \
	user.cpp object.cpp json.cpp base58.cpp \
        satoshi/uint256.cpp satoshi/utilstrencodings.cpp \
        ethash/lib/ethash/ethash.cpp ethash/lib/ethash/progpow.cpp \
        ethash/lib/keccak/keccak.c ethash/lib/keccak/keccakf800.c ethash/lib/keccak/keccakf1600.c ethash/lib/ethash/primes.c \
        kawpow.cpp

CFLAGS += -DHAVE_CURL
SOURCES += rpc_curl.cpp
LDCURL = $(shell /usr/bin/pkg-config --static --libs libcurl)
LDFLAGS += $(LDCURL)

OBJECTS=$(SOURCES:.cpp=.o)
OUTPUT=stratum

CODEDIR1=algos
CODEDIR2=sha3
CODEDIR3=iniparser

.PHONY: projectcode1 projectcode2 projectcode3

all: projectcode1 projectcode2 projectcode3 $(SOURCES) $(OUTPUT)

projectcode1:
	$(MAKE) -C $(CODEDIR1)

projectcode2:
	$(MAKE) -C $(CODEDIR2)

projectcode3:
	$(MAKE) -C $(CODEDIR3)

$(SOURCES): stratum.h util.h

$(OUTPUT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDLIBS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $(SQLFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f algos/*.o
	rm -f algos/*.a
	rm -f sha3/*.o
	rm -f sha3/*.a
	rm -f algos/ar2/*.o
	rm -f ethash/lib/ethash/*.o
	rm -f ethash/include/ethash/*.o

install: clean all
	strip -s stratum
	cp stratum /usr/local/bin/
	cp stratum ../bin/

