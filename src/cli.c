#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>


const char *sands_sun = "/tmp/sand.sock";


int main (int argc, char **argv)
{
    const char *lib_path = "../lib.so"; // TODO use rational path

    char command[512];
    int buffer_location = 0;
    for (int i = 1; i < argc; i++)
    {
        sprintf(command+buffer_location, "%s ", argv[i]);
        buffer_location += strlen(argv[i]) + 1;
    }

    // printf("Command %s", command);

    #ifdef __APPLE__
        // TODO rajab
    #else
        char final_command[512];
        sprintf(final_command, "SANDS_SUN=%s LD_PRELOAD=%s %s", sands_sun, lib_path, command);
        int a = system(final_command);
    #endif
}