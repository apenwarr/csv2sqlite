
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

production:
	if [ -z "$$INSTALLDIR" ]; then \
	    echo "Please specify an installation directory" && exit 1; \
	fi
	rm -rf "$$INSTALLDIR"/csv2sqlite
	mkdir -p "$$INSTALLDIR"/csv2sqlite
	cp csv2sqlite "$$INSTALLDIR"/csv2sqlite
