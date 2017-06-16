#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int num, i;
    sscanf(argv[1], "%d", &num);
    for (i = 0; i < num; i++) {
        pid_t pid = fork();
        if (pid != 0) {  
            char* args[3];
            args[0] = "./pcc_client";
            args[1] = "10005000";
            args[2] = NULL;
            execv("./pcc_client", args);
        }
    }
}