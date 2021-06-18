.PHONY: build

build: cgolfer

cgolfer: cgolfer.c
	gcc -g -O3 -std=gnu11 cgolfer.c libpthreadGC2.a -o cgolfer

clean:
	rm -rf cgolfer