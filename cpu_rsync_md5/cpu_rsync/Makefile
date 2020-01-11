server : server.o checksum.o md5.o hashtable.o
	gcc -o server server.o checksum.o md5.o hashtable.o

server.o : server.c checksum.c md5.c hashtable.c checksum.h hashtable.h
	gcc -c server.c checksum.c md5.c hashtable.c

checksum.o : checksum.c md5.c checksum.h
	gcc -c checksum.c md5.c

md5.o : md5.c checksum.h
	gcc -c md5.c

hashtable.o : hashtable.c hashtable.h
	gcc -c hashtable.c
clean:
	rm -rf *.o
	rm server
