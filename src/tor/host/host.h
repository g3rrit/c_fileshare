#ifndef HOST_H
#define HOST_H

#include "socket.h"

//todo add callback for address
sock_t host_start(char *port, void (*host_started)(char *host_id, void *env), void *host_env);

int host_stop(char *id);

#endif
