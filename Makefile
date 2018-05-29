all:
	gcc -shared -fPIC -o lib.so src/lib.c

clean:
	rm lib.so