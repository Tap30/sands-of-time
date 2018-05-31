all:
	gcc -Wall -Wextra -shared -ldl -fPIC -o lib.so src/lib.c

clean:
	rm lib.so