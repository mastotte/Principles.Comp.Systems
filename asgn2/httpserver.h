#include <stdbool.h>

typedef enum { GET, PUT } command;
typedef enum {
    OK = 200,
    Created = 201,
    BadRequest = 400,
    Forbidden = 403,
    NotFound = 404,
    InternalServerError = 500,
    NotImplemented = 501,
    VersionNotSupported = 505
} status;
typedef struct {
    command cmd;
    status status_code;
    char *target_path;
    char *version;
    int content_length;
    int fd;
} Request;
