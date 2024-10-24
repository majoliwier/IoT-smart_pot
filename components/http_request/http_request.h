#include <stdio.h> // printf
#include <string.h> // snprintf i strlen
#include <sys/socket.h> // operacja na gniazdach
#include <netdb.h> // getaddrinfo
#include <unistd.h> // close
#include "esp_log.h" // log

void http_get_request(const char *);

