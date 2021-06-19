.PHONY: build

build: cgolfer

cgolfer: cgolfer.c
	gcc -g -O3 -std=gnu17 -pthread cgolfer.c -o cgolfer

clean:
	rm -rf cgolfer
