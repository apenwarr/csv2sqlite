
WVBUILD=../wvbuild
WVSTREAMS=$(WVBUILD)/wvstreams
WVSTREAMSOBJ=$(WVBUILD)/$(shell uname -m)-linux
include $(WVSTREAMS)/wvrules.mk
LIBS+=-lsqlite3

default: all

all: csv2sqlite

csv2sqlite: csv2sqlite.o wvcsv.o $(LIBWVSTREAMS)

clean:
	rm -f *~ *.o csv2sqlite
