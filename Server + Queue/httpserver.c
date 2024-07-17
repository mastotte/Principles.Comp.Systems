// Asgn 2: A simple HTTP server.
// By: Eugene Chou
//     Andrew Quinn
//     Brian Zhao

#include "asgn2_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "response.h"
#include "request.h"
#include "queue.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <getopt.h>

void handle_connection(int);

void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);

pthread_t *threads;
pthread_mutex_t worker;
pthread_mutex_t FCL;
queue_t *q = NULL;

bool *idle;
void *retval;

int connfd;
int pop;

void *worker_job() {
    //int fd_test = creat("f_work.txt", 0666);
    //write(fd_test, "start\n", 6);
    while (1) {
        //pthread_mutex_lock(&worker);
        //queue_pop(args, (void **) &pop);
        queue_pop(q, (void **) &pop);
        //write(fd_test, "pop\n", 4);
        handle_connection(pop);
        //write(fd_test, "handle\n", 7);
        //close(pop);
        //pthread_mutex_unlock(&worker);
    }
}
int main(int argc, char **argv) {
    //------------------------------
    //int fd_test = creat("f_main.txt", 0666);
    //write(fd_test, "start\n", 6);
    //------------------------------
    if (argc < 2) {
        warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pthread_mutex_init(&FCL, NULL);
    pthread_mutex_init(&worker, NULL);

    char *endptr = NULL;
    int opt = 0;
    size_t thread_max = 0;
    size_t port = 0;
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't': thread_max = (size_t) strtoull(optarg, &endptr, 10); break;
        default: break;
        }
    }

    port = (size_t) strtoull(argv[argc - 1], &endptr, 10);
    if (endptr && *endptr != '\0') {
        warnx("invalid port number: %s", argv[1]);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);
    Listener_Socket sock;
    listener_init(&sock, port);
    //************** my code here ******************

    pthread_t threads[thread_max];

    q = queue_new(thread_max);
    for (size_t i = 0; i < thread_max; i++) {
        pthread_create(&threads[i], NULL, worker_job, NULL);
    }
    //**********************************************
    //write(fd_test, "while\n", 6);
    while (1) {
        int connfd = listener_accept(&sock);
        queue_push(q, (void *) (uintptr_t) connfd);
        //write(fd_test, "new push\n", 9);
    }
    //write(fd_test, "end while\n", 10);
    return EXIT_SUCCESS;
}

void handle_connection(int connfd) {
    //------------------------------
    //int fd_test = creat("f_handle.txt", 0666);
    //write(fd_test, "start\n", 6);
    //------------------------------
    flock(connfd, LOCK_EX); // new addition
    conn_t *conn = conn_new(connfd);
    const Response_t *res = conn_parse(conn);

    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        debug("%s", conn_str(conn));
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_GET) {
            handle_get(conn);
        } else if (req == &REQUEST_PUT) {
            handle_put(conn);
        } else {
            handle_unsupported(conn);
        }
    }
    conn_delete(&conn);

    // added
    close(connfd);
    //write(fd_test, "end\n", 4);
}
// printf "PUT /bar.txt HTTP/1.1\r\nRequest-Id: 4\r\nContent-Length: 12\r\n\r\nHello World!" | ./cse130_nc localhost 1306
void handle_get(conn_t *conn) {
    //------------------------------
    //int fd_test = creat("f_get.txt", 0666);
    //write(fd_test, "start\n", 6);
    pthread_mutex_lock(&FCL);
    const Response_t *res = NULL;
    size_t size = 0;
    //------------------------------
    char *uri = conn_get_uri(conn);
    debug("GET request not implemented. But, we want to get %s", uri);
    //write(fd_test, "check 1\n", 8);
    // What are the steps in here?
    struct stat st;
    // 1. Open the file.
    // If  open it returns < 0, then use the result appropriately
    //   a. Cannot access -- use RESPONSE_FORBIDDEN
    //   b. Cannot find the file -- use RESPONSE_NOT_FOUND
    //   c. other error? -- use RESPONSE_INTERNAL_SERVER_ERROR
    // (hint: check errno for these cases)!
    int fd = 0;
    bool existed = access(uri, F_OK) == 0;
    if (!existed) {
        res = &RESPONSE_NOT_FOUND;
        //write(fd_test, "Not Found\n", 10);
        goto out;
    }
    //write(fd_test, "check 2\n", 8);
    fd = open(uri, O_RDONLY, 0600);
    flock(fd, LOCK_EX);
    if (fd < 0) {
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            res = &RESPONSE_FORBIDDEN;
            //write(fd_test, "Forbidden\n", 10);
            goto out;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
            //write(fd_test, "ISE\n", 4);
            goto out;
        }
    }
    //write(fd_test, "check 3\n", 8);

    // 2. Get the size of the file.
    // (hint: checkout the function fstat)!

    // Get the size of the file.
    fstat(fd, &st);
    size = st.st_size;
    // 3. Check if the file is a directory, because directories *will*
    // open, but are not valid.
    // (hint: checkout the macro "S_IFDIR", which you can use after you call fstat!)
    //if(S_IFDIR(stbuf.st_mode)){
    // it's a directory
    //    return;
    //}
    // 4. Send the file
    // (hint: checkout the conn_send_file function!)
    //write(fd_test, "check 4\n", 8);
    res = conn_send_file(conn, fd, (uint64_t) size);
    //conn_send_file(conn, fd, 50);
    if (res == NULL) {
        res = &RESPONSE_OK;
    }
    //write(fd_test, "check 5\n", 8);

out:

    fprintf(stderr, "GET,%s,%hu,%s\n", uri, response_get_code(res),
        conn_get_header(conn, "Request-Id"));

    //write(fd_test, "\ncheck 6\n", 9);
    close(fd);
    //write(fd_test, "check 7\n", 8);
    pthread_mutex_unlock(&FCL); //
    //write(fd_test, "end\n", 4);
}

void handle_unsupported(conn_t *conn) {
    debug("handling unsupported request");

    // send responses
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
}
// <Oper>,<URI>,<Status-Code>,<RequestID header value>\n
// PUT,/b.txt,201,3
void handle_put(conn_t *conn) {
    // my code -------------
    //------------------------------
    //int fd_test = creat("f_put.txt", 0666);
    //write(fd_test, "start put\n", 10);
    //------------------------------
    //printf("handle put\n");
    pthread_mutex_lock(&FCL);
    //----------------------
    char *uri = conn_get_uri(conn);
    //write(fd_test, uri, strlen(uri));
    const Response_t *res = NULL;
    debug("handling put request for %s", uri);

    // Check if file already exists before opening it.
    bool existed = access(uri, F_OK) == 0;
    debug("%s existed? %d", uri, existed);
    // if doesnt exist, create it
    int fd = 0;
    if (!existed) {
        fd = creat(uri, 0666);
        flock(fd, LOCK_EX);
    } else {
        fd = open(uri, O_TRUNC | O_WRONLY, 0600);
        flock(fd, LOCK_EX);
    }

    if (fd < 0) {
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            res = &RESPONSE_FORBIDDEN;
            goto out;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
            goto out;
        }
    }

    ftruncate(fd, 0);
    // pthread_mutex_unlock(&FCL); moved to 231

    // Open the file..
    res = conn_recv_file(conn, fd);

    if (res == NULL && existed) {
        res = &RESPONSE_OK;
    } else if (res == NULL && !existed) {
        res = &RESPONSE_CREATED;
    }

out:
    conn_send_response(conn, res);

    fprintf(stderr, "PUT,%s,%hu,%s\n", uri, response_get_code(res),
        conn_get_header(conn, "Request-Id"));

    close(fd);
    pthread_mutex_unlock(&FCL); //
    //write(fd_test, "\nending put\n", 12);
}
