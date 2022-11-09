# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy client hash

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

client.o: client.c csapp.h
	$(CC) $(CFLAGS) -c client.c

client: client.o csapp.o
	$(CC) $(CFLAGS) client.o csapp.o -o client 

hash.o: hash.c csapp.h
	$(CC) $(CFLAGS) -c hash.c

hash: hash.o csapp.o
	$(CC) $(CFLAGS) -c hash.o csapp.o -o hash

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

