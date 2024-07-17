// Asgn 2: A simple HTTP server.
// By: Eugene Chou
//     Andrew Quinn
//     Brian Zhao

#include "asgn2_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "response.h"
#include "request.h"

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
#include <regex.h>

void handle_connection(int);
int cursor;
int connfd;
int setting;
int num_elements;
int data_count;
int size;

typedef enum { FIFO, LRU, CLOCK } SETTING;
struct Node {
    char *data;
    struct Node *next;
};

struct Node *head = NULL;
void prepend(char *str) {

    struct Node *N = (struct Node *) malloc(sizeof(struct Node));
    N->data = calloc(1, sizeof(char *));
    strcpy(N->data, str);
    N->next = head;
    head = N;
}
void free_list() {
    struct Node *cur = head;
    struct Node *next = NULL;
    while (cur != NULL) {
        next = cur->next;
        free(cur);
        cur = next;
    }
    head = NULL;
}
int contains(char *str) {
    struct Node *N = head;
    while (N != NULL && strcmp(N->data, str) != 0) {
        N = N->next;
    }
    if (N == NULL) {
        return 0;
    } else {
        return 1;
    }
}
// for testing
void print_bits(int *bits) {
    for (int i = 0; i < size; i++) {
        printf("(%d)", bits[i]);
    }
    printf("\n");
}
// for testing
void print_cache(char **cache) {
    printf("\n");
    for (int i = 0; i < size; i++) {
        if (cache[i] == NULL) {
            printf("[N]");
        } else {
            printf("[%s]", cache[i]);
        }
    }
    printf("\n");
}
// Returns the index if found, and -1 if not found
int item_found(char **cache, char *str) {
    for (int i = 0; i < num_elements; i++) {

        if (cache[i] == NULL) {
            return -1;
        }

        if (strcmp(cache[i], str) == 0) {
            return i;
        }
    }
    return -1;
}
int FIFO_cache(char **cache, char *str) {
    // Check if HIT or MISS
    int index = item_found(cache, str);
    if (index != -1) {
        return 1;
    } else {
        if (num_elements == size) {
            strcpy(cache[cursor % size], str);
            cursor++;
        } else {
            strcpy(cache[num_elements], str);
            num_elements++;
        }
        return 0;
    }
}
int LRU_cache(char **cache, char *str) {
    // Check if HIT or MISS
    int index = item_found(cache, str);
    char *temp = malloc(sizeof(char *));
    // item is already at back
    if (index == num_elements - 1 && index != -1) {
        return 1;
    } else if (index != -1) {
        // HIT
        // put recently accessed in back
        for (int i = index; i < size - 1; i++) {
            strcpy(temp, cache[i + 1]);
            strcpy(cache[i + 1], cache[i]);
            strcpy(cache[i], temp);
        }
        free(temp);
        return 1;
    } else {
        // MISS
        if (num_elements == size) {
            // shift array forward 1, then overwrite back with str
            // -fast way is to use a cursor as an offset and rotate list, putting str
            // at cursor-1
            for (int i = 0; i < num_elements - 1; i++) {
                strcpy(cache[i], cache[i + 1]);
            }
            strcpy(cache[num_elements - 1], str);
        } else {
            strcpy(cache[num_elements], str);
            num_elements++;
        }
        return 0;
    }
}
int CLOCK_cache(char **cache, int *bits, char *str) {
    int return_value = 0;
    int index = item_found(cache, str);
    if (index != -1) {
        return_value = 1;
        bits[index] = 1;
    } else {
        return_value = 0;
        if (num_elements == size) {
            while (bits[cursor % size] == 1) {
                bits[cursor % size] = 0;
                cursor++;
            }
            strcpy(cache[cursor % size], str);
            cursor++;
        } else {
            strcpy(cache[num_elements], str);
            num_elements++;
        }
    }
    return return_value;
}
int main(int argc, char **argv) {
    static const char *const re = "([\n])";
    regex_t regex;
    regcomp(&regex, re, REG_EXTENDED);

    if (argc != 4) {
        warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "usage: %s <size> <policy>\n", argv[0]);
        return EXIT_FAILURE;
    }
    size = atoi(argv[2]);

    int bits[size];
    char **cache = (char **) malloc(size * sizeof(char *));

    int compulsory_misses = 0;
    int capacitory_misses = 0;

    for (int i = 0; i < size; i++) {
        cache[i] = calloc(1, sizeof(char *));
        bits[i] = 0;
    }
    char *tag = argv[argc - 1];
    setting = 0;
    data_count = 0;
    num_elements = 0;

    if (strcmp(tag, "-F") == 0) {
        setting = FIFO;
    } else if (strcmp(tag, "-L") == 0) {
        setting = LRU;
    } else if (strcmp(tag, "-C") == 0) {
        setting = CLOCK;
    } else {
        fprintf(stderr, "Invalid tag\n");
    }

    while (1) {
        char buf[3] = "";
        char str[2000] = "";

        int inf = 1;
        int val = 0;
        do {
            inf = read_until(STDIN_FILENO, buf, 1, NULL);
            strncat(str, buf, 1);
            val = regexec(&regex, str, 0, NULL, 0);
        } while (val != 0 && inf > 0);
        // remove trailing newline
        char *tok = strtok(str, "\n");

        if (tok == NULL) {
            break;
        }
        switch (setting) {
        case FIFO:
            if (FIFO_cache(cache, tok) == 1) {
                printf("HIT\n");
            } else {
                printf("MISS\n");
                if (data_count == 0) {
                    prepend(tok);
                    data_count++;
                    compulsory_misses++;

                } else if (contains(tok) == 0) {
                    compulsory_misses++;
                    prepend(tok);
                    data_count++;
                } else {
                    capacitory_misses++;
                }
            }
            break;
        case LRU:
            if (LRU_cache(cache, tok) == 1) {
                printf("HIT\n");
            } else {
                printf("MISS\n");
                if (data_count == 0) {
                    prepend(tok);
                    data_count++;
                    compulsory_misses++;

                } else if (contains(tok) == 0) {
                    compulsory_misses++;
                    prepend(tok);
                    data_count++;
                } else {
                    capacitory_misses++;
                }
            }
            break;
        case CLOCK:
            if (CLOCK_cache(cache, bits, tok) == 1) {
                printf("HIT\n");
            } else {
                printf("MISS\n");
                if (data_count == 0) {
                    prepend(tok);
                    data_count++;
                    compulsory_misses++;

                } else if (contains(tok) == 0) {
                    compulsory_misses++;
                    prepend(tok);
                    data_count++;
                } else {
                    capacitory_misses++;
                }
            }
            break;
        }
    }
    printf("%d %d\n", compulsory_misses, capacitory_misses);
    /*free_list();
    for(int i = 0; i < size; i++){
        free(cache[i]);
    }
    free(cache);*/
    return EXIT_SUCCESS;
}
