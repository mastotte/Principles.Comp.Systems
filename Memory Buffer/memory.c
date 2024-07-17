#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#define BUFFER_SIZE 4096

int main() {
    int BUFSIZE = 5000;
    const char s[2] = "\\";
    int res = 0;
    char com[4] = "";
    char cur[2] = "";
    char str[BUFSIZE];
    char *file;

    int count = 0;
    do {
        read(STDIN_FILENO, cur, 1);
        count++;

        strncat(com, cur, 1);
        //printf("Com: %s\n",com);
    } while (cur[0] != 32 && cur[0] != 10 && count < 5);

    if (count == 5) { // 4 characters before a space occured
        fprintf(stderr, "Invalid Command\n");
        return 1;
    }
    int i = 0;
    do {
        //fd1 = read(STDIN_FILENO,str,BUFFER_SIZE);
        read(STDIN_FILENO, cur, 1);
        if (cur[0] != 92 && cur[0] != 10)
            strncat(str, cur, 1);
        i++;
        //printf("\n%s      (%s)",str,cur);
    } while (cur[0] != 92 && cur[0] != 10 && i <= 4096);

    if ((strcmp(com, "get ") != 0 && strcmp(com, "set ") != 0) || (i > 4096)) {
        fprintf(stderr, "Invalid Command\n");
        return 1;
    }

    if (strlen(str) > PATH_MAX) {
        fprintf(stderr, "Invalid Command\n");
        return 1;
    }
    char buf[BUFSIZE];
    if (strcmp(com, "get ") == 0) {
        //printf("starting get\n");
        //read(STDIN_FILENO, cur, 1);
        //read(STDIN_FILENO, cur, 1);
        if (cur[0] != 10) {

            fprintf(stderr, "Invalid Command\n");
            return 1;
        }
        int fd = open(str, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }
        //*************************************
        int bytes_read = 0;
        char buf[BUFFER_SIZE];
        do {
            bytes_read = read(fd, buf, BUFFER_SIZE);
            if (bytes_read < 0) {
                fprintf(stderr, "Invalid Command\n");
                return 1;
            } else if (bytes_read > 0) {
                int bytes_written = 0;
                do {
                    int bytes
                        = write(STDOUT_FILENO, buf + bytes_written, bytes_read - bytes_written);
                    if (bytes <= 0) {
                        fprintf(stderr, "Invalid Command\n");
                        return 1;
                    }
                    bytes_written += bytes;
                } while (bytes_written < bytes_read);
            }
        } while (bytes_read > 0);
        //***************************************
        //printf("OK\n");
        close(fd);
        return 0;
    }
    if (strcmp(com, "set ") == 0) {
        //printf("starting set\n");
        int fd = creat(str, 0666);
        //read(STDIN_FILENO, cur, 1);
        /*if(cur[0] == 10){
            return 0;
        }*/
        //*************************************
        int bytes_read = 0;
        char buf[BUFFER_SIZE];
        do {
            bytes_read = read(STDIN_FILENO, buf, BUFFER_SIZE);
            if (bytes_read < 0) {
                fprintf(stderr, "Invalid Command\n");
                return 1;
            } else if (bytes_read > 0) {
                int bytes_written = 0;
                do {
                    int bytes = write(fd, buf + bytes_written, bytes_read - bytes_written);
                    if (bytes <= 0) {
                        fprintf(stderr, "Invalid Command\n");
                        return 1;
                    }
                    bytes_written += bytes;
                } while (bytes_written < bytes_read);
            }
        } while (bytes_read > 0);
        //****************************************
        printf("OK\n");
        close(fd);
        return 0;
    }
    return 0;
}
