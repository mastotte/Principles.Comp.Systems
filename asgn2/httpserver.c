#include "httpserver.h"
#include "asgn2_helper_funcs.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#define BUFFER_SIZE     4096
#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

Request *newRequest(void) {
    Request *R = calloc(1, sizeof(Request));
    R->target_path = calloc(2048, sizeof(char));
    R->version = calloc(40, sizeof(char));
    //R->status_code = 0;
    return R;
}
void free_all(Request *R) {
    free(R->target_path);
    free(R->version);
    free(R);
}
// read 31 -> GET /t2.txt HTTP/1.1\r\n\r\n
// read 63 -> PUT /t1.txt HTTP/1.1\r\nContent-Length: 12\r\n\r\nHello World!

// PUT /qwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiop.txt HTTP/1.1\r\nContent-Length: 12\r\n\r\nHello World!
// ^([A-Z]{1,8}) +(/[a-zA-Z0-9._]{2,63}) +(HTTP/1\\.1)+([\\r])+([\\n])
//
// VALID = 1, INVALID = 0
int validate_input(char *s, Request *R) {
    // length
    //int fd_test = creat("v.txt",0666);
    if (strlen(s) > 2048) {
        printf("Bad request\n");
        R->status_code = BadRequest;
        return 0;
    }
    // forbidden
    int c_slash = 0;
    int c_dot = 0;
    for (unsigned long i = 0; i < strlen(R->target_path); i++) {
        if (R->target_path[i] == 47) {
            c_slash++;
        }
        if (R->target_path[i] == 46) {
            c_dot++;
        }
    }
    if (c_slash > 1 || c_dot != 1) {
        printf("Forbidden\n");
        //write(fd_test, "Forbidden\n", 10);
        R->status_code = Forbidden;
        return 0;
    }
    // version not supported
    if (strcmp(R->version, "1.1") != 0) {
        printf("R->version : (%s)\n", R->version);
        printf("Version Not Supported\n");
        //write(fd_test, R->version, strlen(R->version));
        //write(fd_test, "Version Not Supported\n", 22);
        R->status_code = VersionNotSupported;
        return 0;
    }
    // final status
    int fd = -1;
    if (R->cmd == GET) {
        fd = open(R->target_path, O_RDONLY);
        if (fd == -1) {
            printf("Not found\n");
            //write(fd_test, "Not found\n", 10);
            R->status_code = NotFound;
            return 0;
        } else {
            printf("OK\n");
            //write(fd_test, "OK\n", 3);
            R->fd = fd;
            R->status_code = OK;
            return 1;
        }

    } else if (R->cmd == PUT) {
        fd = open(R->target_path, O_WRONLY);
        if (fd == -1) {
            close(fd);
            fd = creat(R->target_path, 0666);
            printf("Created\n");
            //write(fd_test, "Created\n", 8);
            R->fd = fd;
            R->status_code = Created;
            return 1;
        } else {
            close(fd);
            fd = creat(R->target_path, 0666);
            printf("OK\n");
            //write(fd_test, "OK\n", 3);
            R->fd = fd;
            R->status_code = OK;
            return 1;
        }

    } else {
        printf("Not implemented\n");
        //write(fd_test, "Not Implemented\n", 16);
        R->status_code = NotImplemented;
        return 0;
    }
    //close(fd_test);
    printf("returning 1\n");
    return 1;
}
int parsing(char *s, Request *R) {
    //int fd_test = creat("t3.txt",0666);
    char *p = s;
    char *g = s;
    char *r = s;
    static const char *const PUT
        = "^([a-zA-Z]{1,8}) +\\/([a-zA-Z0-9.\\/]{2,63}) "
          "+(HTTP\\/)+([0-9].[0-9])+([\r\n])+([a-zA-Z]{1,20}): "
          "+([a-zA-Z]{1,20}):+([0-9]{1,5})+([\r\n])+([a-zA-Z-]{1,20}): "
          "+(curl\\/[0-9]\\.[0-9]{1,2}\\.[0-9])+([\r\n])+([a-zA-Z]{1,7}): "
          "\\*\\/\\*+([\r\n])+([a-zA-Z0-9._-]{1,128}): +([0-9]{1,4})+([\r\n\r\n])";
    static const char *const GET
        = "^([a-zA-Z]{1,8}) +\\/([a-zA-Z0-9.\\/]{2,63}) "
          "+(HTTP\\/)+([0-9].[0-9])+([\r\n])+([a-zA-Z]{1,20}): "
          "+([a-zA-Z]{1,20}):+([0-9]{1,5})+([\r\n])+([a-zA-Z-]{1,20}): "
          "+(curl\\/[0-9]\\.[0-9]{1,2}\\.[0-9])+([\r\n])+([a-zA-Z]{1,7}): \\*\\/\\*+([\r\n\r\n])";
    static const char *const REQ
        = "^([a-zA-Z]{1,8}) +\\/([a-zA-Z0-9.\\/]{2,63}) +(HTTP\\/)+([0-9].[0-9])";
    regex_t regex_put, regex_get, regex_req;
    regmatch_t pmatch_put[17];
    regmatch_t pmatch_get[14];
    regmatch_t pmatch_req[5];
    regoff_t len;
    int vg, vp, vr;
    char target[130];
    char version[20];
    char com[10];
    char vcheck[20];
    char content_length[5];
    int skip = 0;

    if (regcomp(&regex_put, PUT, REG_NEWLINE | REG_EXTENDED))
        printf("failure\n");
    if (regcomp(&regex_get, GET, REG_NEWLINE | REG_EXTENDED))
        printf("failure\n");
    if (regcomp(&regex_req, REQ, REG_NEWLINE | REG_EXTENDED))
        printf("failure\n");

    //write(fd_test, "check 1\n", 8);

    vg = regexec(&regex_get, g, ARRAY_SIZE(pmatch_get), pmatch_get, 0);
    vp = regexec(&regex_put, p, ARRAY_SIZE(pmatch_put), pmatch_put, 0);
    vr = regexec(&regex_req, r, ARRAY_SIZE(pmatch_req), pmatch_req, 0);
    if (!vr) {

        len = pmatch_req[1].rm_eo - pmatch_req[1].rm_so;
        strncpy(com, r + pmatch_req[1].rm_so, (int) len);
        com[(int) len] = '\0';
        if (strcmp(com, "GET") != 0 && strcmp(com, "PUT") != 0) {
            R->status_code = NotImplemented;
            skip = 1;
        }
        len = pmatch_req[4].rm_eo - pmatch_req[4].rm_so;
        strncpy(vcheck, r + pmatch_req[4].rm_so, (int) len);
        vcheck[(int) len] = '\0';
        if (strcmp(vcheck, "1.1") != 0 && skip == 0) {
            R->status_code = VersionNotSupported;
            skip = 1;
        }
    }
    if (vg && vp && skip == 0) {
        //write(fd_test, g, strlen(p));
        R->status_code = BadRequest;
    } else if (!vp) {
        //write(fd_test, "check p\n", 8);
        for (int j = 0; (unsigned long) j < ARRAY_SIZE(pmatch_put); j++) {
            if (pmatch_put[j].rm_so == -1)
                break;
            len = pmatch_put[j].rm_eo - pmatch_put[j].rm_so;
            switch (j) {
            case 1: R->cmd = 1; break;
            case 2:
                strncpy(target, p + pmatch_put[2].rm_so, (int) len);
                target[(int) len] = '\0';
                strcpy(R->target_path, target);
                break;
            case 4:
                strncpy(version, p + pmatch_put[4].rm_so, (int) len);
                version[(int) len] = '\0';
                strcpy(R->version, version);
                break;
            case 16:
                strncpy(content_length, p + pmatch_put[16].rm_so, (int) len);
                content_length[(int) len] = '\0';
                R->content_length = atoi(content_length);
                break;
            }
        }
        p += pmatch_put[0].rm_eo;
    } else if (!vg && vp) {
        //write(fd_test, "check g\n", 8);
        for (int j = 0; (unsigned long) j < ARRAY_SIZE(pmatch_get); j++) {
            if (pmatch_get[j].rm_so == -1)
                break;
            len = pmatch_get[j].rm_eo - pmatch_get[j].rm_so;
            switch (j) {
            case 1: R->cmd = 0; break;
            case 2:
                strncpy(target, p + pmatch_get[2].rm_so, (int) len);
                target[(int) len] = '\0';
                strcpy(R->target_path, target);
                break;
            case 4:
                strncpy(version, p + pmatch_get[4].rm_so, (int) len);
                version[(int) len] = '\0';
                strcpy(R->version, version);
                break;
            }
        }
        g += pmatch_get[0].rm_eo;
    }
    //write(fd_test, "[check 4\n]", 10);
    //write(fd_test, "[check 5\n]", 10);
    regfree(&regex_get);
    regfree(&regex_put);
    regfree(&regex_req);
    return skip;
}

/*
OK = 200, Created = 201, BadRequest = 400, Forbidden = 403,
                NotFound = 404, InternalServerError = 500, NotImplemented = 501,
                VersionNotSupported = 505
*/
void create_output(Request *R, int accept_fd) {
    char CL[20];
    char s[200] = "HTTP/1.1";
    //strcat(s,R->version);
    switch (R->status_code) {
    case OK:
        strcat(s, " 200 OK\r\nContent-Length: ");
        if (R->cmd == GET) {
            struct stat buf;
            fstat(R->fd, &buf);
            int size = buf.st_size;
            R->content_length = size;
            sprintf(CL, "%d", size);
            printf("CONTENT LENGTH: %d", R->content_length);
            strcat(s, CL);
            strcat(s, "\n\n");
        } else if (R->cmd == PUT) {
            printf("STATUS PUT OK\n");
            sprintf(CL, "%d", R->content_length);
            strcat(s, CL);
            strcat(s, "\r\n\r\nOK\n");
        }
        break;
    case Created:
        printf("\nCreated = %d, R->status_code = %u\n", Created, R->status_code);
        strcat(s, " 201 Created\r\nContent-Length: ");
        sprintf(CL, "%d", R->content_length);
        strcat(s, CL);
        strcat(s, "\r\n\r\nCreated\n");
        break;
    case BadRequest:
        strcat(s, " 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        break;
    case Forbidden: strcat(s, " 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n"); break;
    case NotFound: strcat(s, " 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n"); break;
    case InternalServerError:
        strcat(
            s, " 500 Internal Server Error\r\nContent-Length: 23\r\n\r\nInternal Server Error\n");
        break;
    case NotImplemented:
        strcat(s, " 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n");
        break;
    case VersionNotSupported:
        strcat(
            s, " 505 Version Not Supported\r\nContent-Length: 22\r\n\r\nVersion Not Supported\n");
        break;
    }
    // 12, 22

    //write(fd_test,"\nCREATE OUTPUT \n" , 16);
    write(accept_fd, s, strlen(s));
    //write(fd_test, s, strlen(s));
    //write(fd_test,"\nEND CREATE OUTPUT \n" , 20);
}
int write_status(int s_code) {
    if (s_code == 0) {
        (exit(200));
    }
    (exit(s_code));
}
// read 31 -> GET /test.txt HTTP/1.1\r\n\r\n
// read 63 -> PUT /bar.txt HTTP/1.1\r\nContent-Length: 12\r\n\r\nHello World!
int main(int argc, char *argv[]) {

    static const char *const re = "(\r\n\r\n)";
    int port = 0;
    int accept_fd = -1;
    Listener_Socket S;
    int valid = 1;

    argc++;
    //int fd_test = creat("t2.txt",0666);
    port = atoi(argv[1]);
    int init_fd = listener_init(&S, port);

    while (1) {
        Request *R = newRequest();
        accept_fd = -1;

        //write(fd_test, "accepting\n", 10);
        accept_fd = listener_accept(&S);

        //if(accept_fd == -1) break;
        printf("Init: (%d)\nAccept: (%d)\nPort: [%d]\n", init_fd, accept_fd, port);
        regex_t regex;
        regcomp(&regex, re, REG_EXTENDED);
        int val = 1;
        char buf[3] = "";
        char str[2100] = "";
        printf("str before read: (%s)\n", str);
        //write(fd_test, "Starting do while\n", 19);
        //write(fd_test, "\nSTART DO WHILE\n", 16);
        int inf = 1;

        do {
            inf = read_until(accept_fd, buf, 1, NULL);
            strncat(str, buf, 1);
            val = regexec(&regex, str, 0, NULL, 0);

            if (strlen(str) > 3) {
                //write(fd_test, "\n- ",3);
                //write(fd_test, str, strlen(str));
            }

        } while (val != 0 && inf > 0);
        //write(fd_test, "\nEND DO WHILE\n", 14);

        //write(fd_test, "\nBegin Parsing\n", 15);
        printf("str after read: (%s)\n", str);
        parsing(str, R);

        //write(fd_test, "\nEnd Parsing\n", 13);
        //write(fd_test, R->version, strlen(R->version));
        if (R->status_code != BadRequest && R->status_code != NotImplemented
            && R->status_code != VersionNotSupported) {
            valid = validate_input(str, R);
        }

        create_output(R, accept_fd);

        if (valid) {
            //write(fd_test, "\nvalid input\n", 13);
            //parse(R,str);

            if (R->cmd == GET) {
                val = 2;
                //write(fd_test, "\ncalling get\n", 13);
                //int fd_5 = open("t5.txt",O_RDONLY);
                while (val > 1) {

                    //write(fd_test, "val\n",4);
                    val = pass_bytes(R->fd, accept_fd, 200);
                }
                //write(fd_test, "end get\n", 8);
            } else if (R->cmd == PUT) {
                val = 2;
                //write(fd_test, "\ncalling put\n", 13);
                while (val > 1) {
                    val = pass_bytes(accept_fd, R->fd, 200);
                }
            } else {
                R->status_code = InternalServerError;
                printf("Internal Sever Error\n");
                // break;
            }
        }
        //write(fd_test, "Reached end\n", 13);
        close(accept_fd);
        printf("Status-code: %d\n", R->status_code);
        //int exit_code = R->status_code;
        regfree(&regex);
        free_all(R);
        //exit(exit_code);
    }
}
