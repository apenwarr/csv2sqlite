default: all

all: csv2sqlite

csv2sqlite: csv2sqlite.o wvcsv.o

%: %.o
	$(CXX) -ggdb -o $@ $^ -lwvstreams -lsqlite3

%.o: %.cc
	$(CXX) -ggdb -Wall -I/usr/include/xplc-0.3.13 -I/usr/include/wvstreams \
		-o $@ -c $<

$(patsubst %.cc,%.o,$(wildcard *.cc)): $(wildcard *.h)

clean::
	rm -f *~ *.o csv2sqlite
