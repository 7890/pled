CC = g++

SRC = src

H = $(SRC)/eventual
EVENTUAL_H = $(H)/timeline.h $(H)/playlist.h $(H)/treenode.h $(H)/event.h $(H)/ops.h $(H)/restring.h $(H)/files.h $(H)/common.h

default: pled

pled: $(SRC)/pled.cc $(EVENTUAL_H)
	$(CC) -o pled $(SRC)/pled.cc
	echo -n "pl::\nplaylist 'x'\nfile 'a' 0 1000\ninsert_file 0 1\ncut 10 100\nprint\n::\n" | ./pled

clean:
	rm -f pled
