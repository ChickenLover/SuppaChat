#include "server.h"

int main(int argc, char const* argv[]){
    int server_fd, client_socket;
    struct sockaddr_in server;

    int connection;
    struct sockaddr_in client;    

    if (argc < 2){
        perror("Not enough args passed");
        exit(EXIT_FAILURE);
    }
    uint16_t port = (uint16_t)atoi(argv[1]);
    if (!port){
        perror("Wrong port format");
        exit(EXIT_FAILURE);
    }
    uint16_t server_port = htons(port);
    if (!(server_fd = socket(AF_INET, SOCK_STREAM, 0))){
        perror("Socket failed to start");
        exit(EXIT_FAILURE);
    }
    
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = server_port;

    if (bind(server_fd , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("Socket failed to start");
        exit(EXIT_FAILURE);
    }
    listen(server_fd, 3);

    puts("Waiting for incoming connections...");
    connection = sizeof(struct sockaddr_in);
    while((client_socket = accept(server_fd, (struct sockaddr *)&client, (socklen_t*)&connection)) )
    {
        puts("Connection accepted");

        pthread_t session_thread;
        int* client_sock = malloc(1);
        *client_sock = client_socket;

        if( pthread_create( &session_thread , NULL ,  handle_session , (void*) client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }

        puts("Handler assigned");
    }
    if (client_socket < 0)
    {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    close(server_fd);
    puts("Server closed");
    return 0;
}

void* handle_session(void* socket){
    int sock = *(int*)socket;
    session(sock);
    close(sock);
    free(socket);
    puts("Connection closed");
    return 0;
}
