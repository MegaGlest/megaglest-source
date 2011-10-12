/* $Id: minihttptestserver.c,v 1.6 2011/05/09 08:53:15 nanard Exp $ */
/* Project : miniUPnP
 * Author : Thomas Bernard
 * Copyright (c) 2011 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution.
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <time.h>

#define CRAP_LENGTH (2048)

volatile int quit = 0;
volatile int child_to_wait_for = 0;

/**
 * signal handler for SIGCHLD (child status has changed)
 */
void handle_signal_chld(int sig)
{
	printf("handle_signal_chld(%d)\n", sig);
	++child_to_wait_for;
}

/**
 * signal handler for SIGINT (CRTL C)
 */
#if 0
void handle_signal_int(int sig)
{
	printf("handle_signal_int(%d)\n", sig);
	quit = 1;
}
#endif

/**
 * build a text/plain content of the specified length
 */
void build_content(char * p, int n)
{
	char line_buffer[80];
	int k;
	int i = 0;

	while(n > 0) {
		k = snprintf(line_buffer, sizeof(line_buffer),
		             "%04d_ABCDEFGHIJKL_This_line_is_64_bytes_long_ABCDEFGHIJKL_%04d\r\n",
		             i, i);
		if(k != 64) {
			fprintf(stderr, "snprintf() returned %d in build_content()\n", k);
		}
		++i;
		if(n >= 64) {
			memcpy(p, line_buffer, 64);
			p += 64;
			n -= 64;
		} else {
			memcpy(p, line_buffer, n);
			p += n;
			n = 0;
		}
	}
}

/**
 * build crappy content
 */
void build_crap(char * p, int n)
{
	static const char crap[] = "_CRAP_\r\n";
	int i;

	while(n > 0) {
		i = sizeof(crap) - 1;
		if(i > n)
			i = n;
		memcpy(p, crap, i);
		p += i;
		n -= i;
	}
}

/**
 * build chunked response.
 * return a malloc'ed buffer
 */
char * build_chunked_response(int content_length, int * response_len) {
	char * response_buffer;
	char * content_buffer;
	int buffer_length;
	int i, n;

	/* allocate to have some margin */
	buffer_length = 256 + content_length + (content_length >> 4);
	response_buffer = malloc(buffer_length);
	*response_len = snprintf(response_buffer, buffer_length,
	                         "HTTP/1.1 200 OK\r\n"
	                         "Content-Type: text/plain\r\n"
	                         "Transfer-Encoding: chunked\r\n"
	                         "\r\n");

	/* build the content */
	content_buffer = malloc(content_length);
	build_content(content_buffer, content_length);

	/* chunk it */
	i = 0;
	while(i < content_length) {
		n = (rand() % 199) + 1;
		if(i + n > content_length) {
			n = content_length - i;
		}
		/* TODO : check buffer size ! */
		*response_len += snprintf(response_buffer + *response_len,
		                          buffer_length - *response_len,
		                          "%x\r\n", n);
		memcpy(response_buffer + *response_len, content_buffer + i, n);
		*response_len += n;
		i += n;
		response_buffer[(*response_len)++] = '\r';
		response_buffer[(*response_len)++] = '\n';
	}
	memcpy(response_buffer + *response_len, "0\r\n", 3);
	*response_len += 3;
	free(content_buffer);

	printf("resp_length=%d buffer_length=%d content_length=%d\n",
	       *response_len, buffer_length, content_length);
	return response_buffer;
}

enum modes { MODE_INVALID, MODE_CHUNKED, MODE_ADDCRAP, MODE_NORMAL };
const struct {
	const enum modes mode;
	const char * text;
} modes_array[] = {
	{MODE_CHUNKED, "chunked"},
	{MODE_ADDCRAP, "addcrap"},
	{MODE_NORMAL, "normal"},
	{MODE_INVALID, NULL}
};

/**
 * write the response with random behaviour !
 */
void send_response(int c, const char * buffer, int len)
{
	int n;
	while(len > 0) {
		n = (rand() % 99) + 1;
		if(n > len)
			n = len;
		n = write(c, buffer, n);
		if(n < 0) {
			perror("write");
			return;
		} else {
			len -= n;
			buffer += n;
		}
		usleep(10000); /* 10ms */
	}
}

/**
 * handle the HTTP connection
 */
void handle_http_connection(int c)
{
	char request_buffer[2048];
	int request_len = 0;
	int headers_found = 0;
	int n, i;
	char request_method[16];
	char request_uri[256];
	char http_version[16];
	char * p;
	char * response_buffer;
	int response_len;
	enum modes mode;
	int content_length = 16*1024;

	/* read the request */
	while(request_len < sizeof(request_buffer) && !headers_found) {
		n = read(c,
		         request_buffer + request_len,
		         sizeof(request_buffer) - request_len);
		if(n < 0) {
			perror("read");
			return;
		} else if(n==0) {
			/* remote host closed the connection */
			break;
		} else {
			request_len += n;
			for(i = 0; i < request_len - 3; i++) {
				if(0 == memcmp(request_buffer + i, "\r\n\r\n", 4)) {
					/* found the end of headers */
					headers_found = 1;
					break;
				}
			}
		}
	}
	if(!headers_found) {
		/* error */
		return;
	}
	printf("headers :\n%.*s", request_len, request_buffer);
	/* the request have been received, now parse the request line */
	p = request_buffer;
	for(i = 0; i < sizeof(request_method) - 1; i++) {
		if(*p == ' ' || *p == '\r')
			break;
		request_method[i] = *p;
		++p;
	}
	request_method[i] = '\0';
	while(*p == ' ')
		p++;
	for(i = 0; i < sizeof(request_uri) - 1; i++) {
		if(*p == ' ' || *p == '\r')
			break;
		request_uri[i] = *p;
		++p;
	}
	request_uri[i] = '\0';
	while(*p == ' ')
		p++;
	for(i = 0; i < sizeof(http_version) - 1; i++) {
		if(*p == ' ' || *p == '\r')
			break;
		http_version[i] = *p;
		++p;
	}
	http_version[i] = '\0';
	printf("Method = %s, URI = %s, %s\n",
	       request_method, request_uri, http_version);
	/* check if the request method is allowed */
	if(0 != strcmp(request_method, "GET")) {
		const char response405[] = "HTTP/1.1 405 Method Not Allowed\r\n"
		                           "Allow: GET\r\n\r\n";
		/* 405 Method Not Allowed */
		/* The response MUST include an Allow header containing a list
		 * of valid methods for the requested resource. */
		write(c, response405, sizeof(response405) - 1);
		return;
	}

	mode = MODE_INVALID;
	/* use the request URI to know what to do */
	for(i = 0; modes_array[i].mode != MODE_INVALID; i++) {
		if(strstr(request_uri, modes_array[i].text)) {
			mode = modes_array[i].mode; /* found */
			break;
		}
	}

	switch(mode) {
	case MODE_CHUNKED:
		response_buffer = build_chunked_response(content_length, &response_len);
		break;
	case MODE_ADDCRAP:
		response_len = content_length+256;
		response_buffer = malloc(response_len);
		n = snprintf(response_buffer, response_len,
		             "HTTP/1.1 200 OK\r\n"
		             "Server: minihttptestserver\r\n"
		             "Content-Type: text/plain\r\n"
		             "Content-Length: %d\r\n"
		             "\r\n", content_length);
		response_len = content_length+n+CRAP_LENGTH;
		response_buffer = realloc(response_buffer, response_len);
		build_content(response_buffer + n, content_length);
		build_crap(response_buffer + n + content_length, CRAP_LENGTH);
		break;
	default:
		response_len = content_length+256;
		response_buffer = malloc(response_len);
		n = snprintf(response_buffer, response_len,
		             "HTTP/1.1 200 OK\r\n"
		             "Server: minihttptestserver\r\n"
		             "Content-Type: text/plain\r\n"
		             "\r\n");
		response_len = content_length+n;
		response_buffer = realloc(response_buffer, response_len);
		build_content(response_buffer + n, response_len - n);
	}

	if(response_buffer) {
		send_response(c, response_buffer, response_len);
		free(response_buffer);
	} else {
		/* Error 500 */
	}
}

/**
 */
int main(int argc, char * * argv) {
	int ipv6 = 0;
	int s, c, i;
	unsigned short port = 0;
	struct sockaddr_storage server_addr;
	socklen_t server_addrlen;
	struct sockaddr_storage client_addr;
	socklen_t client_addrlen;
	pid_t pid;
	int child = 0;
	int status;
	const char * expected_file_name = NULL;

	for(i = 1; i < argc; i++) {
		if(argv[i][0] == '-') {
			switch(argv[i][1]) {
			case '6':
				ipv6 = 1;
				break;
			case 'e':
				/* write expected file ! */
				expected_file_name = argv[++i];
				break;
			case 'p':
				/* port */
				if(++i < argc) {
					port = (unsigned short)atoi(argv[i]);
				}
				break;
			default:
				fprintf(stderr, "unknown command line switch '%s'\n", argv[i]);
			}
		} else {
			fprintf(stderr, "unkown command line argument '%s'\n", argv[i]);
		}
	}

	srand(time(NULL));
	signal(SIGCHLD, handle_signal_chld);
#if 0
	signal(SIGINT, handle_signal_int);
#endif

	s = socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
	if(s < 0) {
		perror("socket");
		return 1;
	}
	memset(&server_addr, 0, sizeof(struct sockaddr_storage));
	memset(&client_addr, 0, sizeof(struct sockaddr_storage));
	if(ipv6) {
		struct sockaddr_in6 * addr = (struct sockaddr_in6 *)&server_addr;
		addr->sin6_family = AF_INET6;
		addr->sin6_port = htons(port);
		addr->sin6_addr = in6addr_any;
	} else {
		struct sockaddr_in * addr = (struct sockaddr_in *)&server_addr;
		addr->sin_family = AF_INET;
		addr->sin_port = htons(port);
		addr->sin_addr.s_addr = htonl(INADDR_ANY);
	}
	if(bind(s, (struct sockaddr *)&server_addr,
	        ipv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
		return 1;
	}
	if(listen(s, 5) < 0) {
		perror("listen");
	}
	if(port == 0) {
		server_addrlen = sizeof(struct sockaddr_storage);
		if(getsockname(s, (struct sockaddr *)&server_addr, &server_addrlen) < 0) {
			perror("getsockname");
			return 1;
		}
		if(ipv6) {
			struct sockaddr_in6 * addr = (struct sockaddr_in6 *)&server_addr;
			port = ntohs(addr->sin6_port);
		} else {
			struct sockaddr_in * addr = (struct sockaddr_in *)&server_addr;
			port = ntohs(addr->sin_port);
		}
		printf("Listening on port %hu\n", port);
		fflush(stdout);
	}

	/* write expected file */
	if(expected_file_name) {
		FILE * f;
		f = fopen(expected_file_name, "wb");
		if(f) {
			char * buffer;
			buffer = malloc(16*1024);
			build_content(buffer, 16*1024);
			fwrite(buffer, 1, 16*1024, f);
			free(buffer);
			fclose(f);
		}
	}

	/* fork() loop */
	while(!child && !quit) {
		while(child_to_wait_for > 0) {
			pid = wait(&status);
			if(pid < 0) {
				perror("wait");
			} else {
				printf("child(%d) terminated with status %d\n", pid, status);
			}
			--child_to_wait_for;
		}
		/* TODO : add a select() call in order to handle the case
		 * when a signal is caught */
		client_addrlen = sizeof(struct sockaddr_storage);
		c = accept(s, (struct sockaddr *)&client_addr,
		           &client_addrlen);
		if(c < 0) {
			perror("accept");
			return 1;
		}
		printf("accept...\n");
		pid = fork();
		if(pid < 0) {
			perror("fork");
			return 1;
		} else if(pid == 0) {
			/* child */
			child = 1;
			close(s);
			s = -1;
			handle_http_connection(c);
		}
		close(c);
	}
	if(s >= 0) {
		close(s);
		s = -1;
	}
	if(!child) {
		while(child_to_wait_for > 0) {
			pid = wait(&status);
			if(pid < 0) {
				perror("wait");
			} else {
				printf("child(%d) terminated with status %d\n", pid, status);
			}
			--child_to_wait_for;
		}
		printf("Bye...\n");
	}
	return 0;
}

