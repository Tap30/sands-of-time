CC = gcc
CFLAGS += -Wall -Wextra -Werror -fPIC
LIB_LDFLAGS += -shared
LDFLAGS += -lpthread
LDADD += -ldl -lm
MKDIR_P = mkdir -p
.PHONY: directories

all: directories
	${CC} ${CFLAGS}  ${LDFLAGS} ${LIB_LDFLAGS} ${LDADD}  -o builds/lib.so src/lib.c

directories:
	${MKDIR_P} builds

clean:
	rm -r builds