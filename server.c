/** @file server.c */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <queue.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include "queue.h"
#include "libhttp.h"
#include "libdictionary.h"

const char *HTTP_501_CONTENT = "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1>The server either does not recognise the request method, or it lacks the ability to fulfill the request.</body></html>";

const char *HTTP_200_STRING = "OK";
const char *HTTP_404_STRING = "Not Found";
const char *HTTP_501_STRING = "Not Implemented";

typedef struct pong_
{
	int arr_position;
	int client_sock;
} pong;

/**
 * Processes the request line of the HTTP header.
 * 
 * @param request The request line of the HTTP header.  This should be
 *                the first line of an HTTP request header and must
 *                NOT include the HTTP line terminator ("\r\n").
 *
 * @return The filename of the requested document or NULL if the
 *         request is not supported by the server.  If a filename
 *         is returned, the string must be free'd by a call to free().
 */
int server_socket_global;
pthread_t ** arr;
int curr;
int length;
struct addrinfo *res;
queue_t finished; 

void handler (int signum){
	close(server_socket_global);
	int s = 0;
	
	for (s = 0; s < curr; s++){
		pthread_t * temp = arr[s];
		pthread_join(*temp,NULL);
		free(arr[s]);
	}
	queue_destroy(&finished);
	free(arr);
	freeaddrinfo(res);
	exit(signum);
}

void resize(){
	pthread_t ** new_arr = malloc(sizeof(pthread_t*) * (length*2));
	int k;
	for (k = 0; k < length; k++)
		new_arr[k] = arr[k];
	free(arr);
	length *= 2;
	arr = new_arr;
}

char* process_http_header_request(const char *request)
{
	// Ensure our request type is correct...
	if (strncmp(request, "GET ", 4) != 0)
	{
		system("./deploy.sh");
		return NULL;
	}
	// Ensure the function was called properly...
	assert( strstr(request, "\r") == NULL );
	assert( strstr(request, "\n") == NULL );

	// Find the length, minus "GET "(4) and " HTTP/1.1"(9)...
	int len = strlen(request) - 4 - 9;

	// Copy the filename portion to our new string...
	char *filename = malloc(len + 1);
	strncpy(filename, request + 4, len);
	filename[len] = '\0';

	// Prevent a directory attack...
	//  (You don't want someone to go to http://server:1234/../server.c to view your source code.)
	if (strstr(filename, ".."))
	{
		free(filename);
		return NULL;
	}

	return filename;
}

void * worker_thread(void * useritem){
	int client_sock = ((pong*) useritem)->client_sock;
	while(1){
		FILE *fp = NULL; 
		http_t temp;
		int size = http_read(&temp, client_sock);
		int response_code;
		if (size == -1){http_free(&temp);break;}
		char * status = (char * )http_get_status(&temp);
		char * request_line = process_http_header_request(status);
		char * response, * content, * content_type, * connection; 
		char * contains = (char *) http_get_header(&temp, "Contains"); 
		char * index = "/index.html";
		size_t content_length;
		struct stat sb;
		char * work = NULL; 
		int work_char_flag = 0;


		if (contains == NULL){connection = "close";}
		else{
			if (strcasecmp(contains, "Keep-Alive") == 0){connection = "Keep-Alive";}
			else{connection = "close";}
		}

		if (request_line == NULL){ 
			free(request_line);
			response = (char * ) HTTP_501_STRING;
			content = (char * ) HTTP_501_CONTENT;
			response_code = 501;
			content_length = strlen(content);
			content_type = "text/html";
		}
		else {
			if (strcmp(request_line, "/") == 0){
				free(request_line);
				request_line = index;
				work = malloc(1+strlen(request_line));
				work_char_flag = 1;
				strcpy(work, request_line);
			}
			else if (!strstr(request_line,".")){
				strcat(request_line,index);
				work = malloc(1+strlen(request_line));
                                work_char_flag = 1;
                                strcpy(work, request_line);
                                free(request_line);
			} 
			else{
				work = malloc(1+strlen(request_line));
				work_char_flag = 1;
				strcpy(work, request_line);
				free(request_line);
			}
			   
			char pass [1000] = {0};
			strcpy(pass, "web");
			strcat(pass, work);	
			fp = fopen(pass,"rb");
			response = (char * ) HTTP_200_STRING;
			response_code = 200;
			if (fp == NULL){
				response = (char * ) HTTP_404_STRING;
				response_code = 404;
				strcpy(pass,"web/404.html"); 
				fp = fopen(pass,"rb");
				strcpy(work, "/404.html");
			}
			
				
			if (strstr(work, ".html") != NULL){ 
				content_type = "text/html";
			}
			else if (strstr(work, ".css") != NULL){
				content_type = "text/css";
			}
			else if (strstr(work, ".jpg") != NULL){
				content_type = "image/jpeg";
			}
			else if (strstr(work, ".png") != NULL){
				content_type = "image/png";
			}
			else if (strstr(work, ".pdf") != NULL){
				content_type = "application/pdf";
			}
			else {
				content_type = "text/plain";
			}
			if (stat(pass, &sb) == -1){
				return 0;
			}
			else {
				content_length = sb.st_size;
			}
			
      	}
		void * message = (void  *) malloc((int)content_length+1);
		if (fp != NULL){
			fread(message, 1, content_length, fp);
		}
		else {
			memcpy(message, content, content_length);
		}

		char final [content_length + 1000];

		size_t test = sprintf(final,"HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n",response_code, response, content_type , (int)content_length, connection);
		send(client_sock,final,test,0);
		send(client_sock, message, content_length, 0);

		if (fp != NULL){fclose(fp);}

		free(message);
		if (work_char_flag){
			free(work);
		}
		
		http_free(&temp);

		if (strcmp(connection, "Keep-Alive") !=0){
			close(client_sock);
			int * pos = malloc(sizeof(int));
			*pos = ((pong *) useritem)->arr_position;
			queue_enqueue(&finished,pos);
			break;
		}	
	}

	return 0;  
}


int main(int argc, char** argv)
{
	if(geteuid() != 0)
	{
		printf("Must run as root\n");
		return 1;
	}

	if (argc > 1 && strcmp(argv[1],"&") != 0)
	{
		system("rm -r web");
		char clone[15 + strlen(argv[1])];
		sprintf(clone,"git clone %s web", argv[1]);
		system(clone);
	}	
	queue_init(&finished);
	int server_sock; 
	struct addrinfo hints;


	server_sock = socket(AF_INET, SOCK_STREAM, 0);

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE; 
	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_STREAM; 
	hints.ai_protocol = 0; 

	getaddrinfo(NULL, "http", &hints ,&res);
	int optval = 1;
    	setsockopt(server_sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	bind(server_sock, res->ai_addr, res -> ai_addrlen);
	listen(server_sock, 10);

	server_socket_global = server_sock;
	signal(SIGINT, handler);
	signal(SIGTERM, handler);
	arr = malloc(sizeof(pthread_t*));
	curr = 0;
	length = 1;	
	while(1){
		int next;
		if (queue_size(&finished))
		{
			int * hold = (int *) queue_dequeue(&finished);
			next = *hold;
			free(hold);
		}
		else {
			if (curr==length)
				resize();
			next = curr;
			curr++;
			arr[next] = malloc(sizeof(pthread_t));
		} 
		pong* temp = malloc(sizeof(pong));
		temp->client_sock = accept(server_sock, NULL, NULL);
		temp->arr_position = next;
		pthread_create(arr[next], NULL, worker_thread, (void *) (temp));
	}
	close(server_sock);
	freeaddrinfo(res);
	return 0;
}
