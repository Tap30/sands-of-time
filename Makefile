CC = gcc
CFLAGS += -Wall -Wextra -Werror -fPIC
LIB_LDFLAGS += -shared
LDFLAGS += -lpthread
LDADD += -ldl -lm

all:
	${CC} ${CFLAGS}  ${LDFLAGS} ${LIB_LDFLAGS} ${LDADD}  -o lib.so src/lib.c

clean:
	rm lib.so
