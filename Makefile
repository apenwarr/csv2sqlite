default: all

all: csv2sqlite

csv2sqlite: csv2sqlite.o wvcsv.o

%: %.o
	$(CXX) -g -o $@ $^ -lwvstreams -lsqlite3

%.o: %.cc
	$(CXX) -Wall -I/usr/include/xplc-0.3.13 -I/usr/include/wvstreams \
		-o $@ -c $<

clean::
	rm -f *~ *.o csv2sqlite
