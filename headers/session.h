#include "string.h"

typedef struct {
    int socket;
    char* login;
    char* chat_name;
}chat_data;

void socket_puts(int socket, char* message);
int check_connection(int socket);

void session(int socket);
void greet(int socket);

int login_menu(int socket);
int sign_in(int socket, char* login);
int log_in(int socket, char* login);

int lock_chat(char* chat_name);
int unlock_chat(char* chat_name);
int chat_locked(char* chat_name);

int menu(int socket);
char* create_room(int socket);
char* connect_to_room(int socket);
int chat_interface(int socket, char* chat_name);
void flush_chat_history(int socket, char* chat_name);
int write_new_message_to_chat_file(int socket, char* chat_name, char* message, char* login);
void* listen_for_new_messages(void* _chat_data);
