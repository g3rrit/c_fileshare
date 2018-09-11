#include "host.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "tor_util.h"
#include "tor_service.h"

#define DELTA_WAIT_FOR_HOST_TIME 1500   //in ms
#define MAX_WAIT_FOR_HOST_TIME 5000     //in ms

sock_t host_start(char *port, void (*host_started)(char *host_id, void *env), void *host_env)
{
    //start tor hidden service
    char host_id[17] = {0,};
    if(!tor_service_add(port, host_id))
    {
        log_err("starting tor service\n");
        return 0;
    }
    //---

    //init host
    struct addrinfo hints;
    struct addrinfo *ptr = 0;
    struct addrinfo *res = 0;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    log_msg("getaddrinfo for port: %s\n", port);

    int rc = getaddrinfo("127.0.0.1", port, &hints, &res);
    if(rc != 0)
    {
        log_err("getting addrinfo\n");
        host_stop(host_id);
        return 0;
    }

    ptr = res;

    sock_t hs = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if(hs < 0)
    {
        log_err("creating host socket\n");
        host_stop(host_id);
        return 0;
    }

    log_msg("created socket\n");

    rc = bind(hs, ptr->ai_addr, ptr->ai_addrlen);
    if(rc == -1)
    {
        socket_close(hs);
        log_err("binding host socket\n");
        host_stop(host_id);
        return 0;
    }
    //-----

    //listen on host
    rc = listen(hs, 10);
    if(rc == -1)
    {
        socket_close(hs);
        log_err("listening on host\n");
        host_stop(host_id);
        return 0;
    }

    host_started(host_id, host_env);

    log_msg("listening\n");

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    fd_set master_set;
    fd_set working_set;
    FD_ZERO(&master_set);
    int max_sd = (int)hs;
    FD_SET(hs, &master_set);
    while(1)
    {
        memcpy(&working_set, &master_set, sizeof(master_set));
        rc = select(max_sd + 1, &working_set, 0, 0, &timeout);

        if(rc < 0)
        {
            log_err("host select\n");
            break;
        }
        if(rc == 0)
        {
            continue;
        }

        sock_t c_socket = -1;
        do
        {
            c_socket = accept(hs, 0, 0);
            if(c_socket != -1)
            {
                log_msg("accepted client connection");
                return c_socket;
            }
        } while(1);

    }

}

int host_stop(char *id)
{
    if(!tor_service_remove(id))
    {
        log_err("stopping tor service\n");
        return 0;
    }
    return 1;
}


