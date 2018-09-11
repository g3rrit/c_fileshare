#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#include "socket.h"
#include "tor_service.h"
#include "host.h"

void send_file(FILE *file, sock_t s) {

	uint64_t total_bytes = 0;
	unsigned int bytes = 0;

	uint8_t buffer[1024];
	memset(buffer, 0, 1024);
	uint64_t read_bytes = 0;

	int bps_t = 0;
	int bps = 0;

	uint64_t time_l = 0;
	uint64_t time_n = 0;
	float time_t = 0;

	while ((read_bytes = fread(buffer, 1, 1024, file)) > 0)
	{
		time_n = clock();

		time_t += ((float)(time_n - time_l))/(float)CLOCKS_PER_SEC;
		time_l = time_n;

		bytes = send(s, (const char*)buffer, read_bytes, 0);

		if(bytes <= 0)
			return;

		total_bytes += bytes;

		bps_t += bytes;
		if (time_t >= 1.)
		{
			bps = ((float)bps_t)/time_t;
			time_t = .0;
			bps_t = 0;
		}

		printf("\rsent: %14" PRIu64 " bytes | %10i B/s", total_bytes, bps);
		memset(buffer, 0, 1024);
	}

	printf("\nfinished sending file\n");
}

void recv_file(FILE *file, sock_t s) {

#define BUFFER_SIZE 1024
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);

	uint64_t total_bytes = 0;

	int bytes = 0;

	int bps_t = 0;
	int bps = 0;

	uint64_t time_l = 0;
	uint64_t time_n = 0;
	float time_t = 0;

	do
	{
		time_n = clock();

		time_t += ((float)(time_n - time_l)) / (float)CLOCKS_PER_SEC;
		time_l = time_n;

		bytes = recv(s, buffer, BUFFER_SIZE, 0);

		if (bytes <= 0)
			break;

		total_bytes += bytes;
		fwrite(buffer, 1, bytes, file);

		memset(buffer, 0, BUFFER_SIZE);

		bps_t += bytes;
		if (time_t >= 1.)
		{
			bps = ((float)bps_t) / time_t;
			time_t = .0;
			bps_t = 0;
		}

		printf("\rreceived: %14" PRIu64 " bytes | %10i B/s", total_bytes, bps);

	} while (bytes > 0);

	printf("\nreceived complete file\n");
}

void start() {
    socket_init();
    tor_service_init(".");
}

void stop() {
    tor_service_delete();
    socket_delete();
}

struct host_cb_t {
    char id[17];
};

void host_cb(char *id, struct host_cb_t *env) {
    printf("host started with address: [ %s ]\n", id); 
    strcpy(env->id, id);
}

int main() {

    start();

    char input = 0;
    printf("(h)ost file or (r)eiceive file?: ");
    scanf("%c", &input);

    if(input == 'h') {
        char line[1024] = {0};
        printf("specify file location: ");
        scanf("%s", line);
        
        FILE *file = fopen(line, "rb");
        if(!file) {
            printf("unable to find file[%s]! quitting...\n", line);
            stop();
            return 0;
        }

        printf("specify host port: ");
        scanf("%s", line);
        if(strlen(line) > 5) {
            printf("invalid port[%s]! quitting...\n", line);
            fclose(file);
            stop();
            return 0;
        }

        struct host_cb_t h_env = { .id = {0} };

        sock_t s = host_start(line, &host_cb, &h_env);
        if(!s) {
            printf("unable to start host server! quitting...\n");
            fclose(file);
            stop();
            return 0;
        }

        send_file(file, s);
        fclose(file);

        if(!host_stop(h_env.id)) {
            printf("unable to stop host server! quitting...\n");
            stop();
            return 0;
        }

        if(!socket_close(s)) {
            printf("unable to close socket! quitting...\n");
            stop();
            return 0;
        }

    } else if(input == 'r') {
        char path[1024] = {0};
        printf("specify destination file: ");
        scanf("%s", path);
        FILE *file = fopen(path, "wb");
        if(!file) {
            printf("unable to open file for writting[%s]! quitting...\n", path);
            stop();
            return 0;
        }

        char host_id[1024] = {0};
        char port[1024] = {0};
        printf("host address: ");
        scanf("%s", host_id);
        if(strlen(host_id) != 16) {
            printf("invalid host address[%s]! quitting...\n", host_id);
            fclose(file);
            stop();
            return 0;
        }

        printf("specify host port: ");
        scanf("%s", port);
        if(strlen(port) > 5) {
            printf("invalid host port[%s]! quitting...\n", port);
            fclose(file);
            stop();
            return 0;
        }
        
        sock_t s = tor_connect(host_id, port);
        if(!s) {
            printf("unable to connect to server! quitting...\n");
            fclose(file);
            stop();
            return 0;
        }

        recv_file(file, s);
        fclose(file);

        if(!socket_close(s)) {
            printf("unable to close socket! quitting...\n");
            stop();
            return 0;
        }

    } else {
        printf("invalid choice! quitting...\n");
    }

    stop();
    
    return 0;
}
