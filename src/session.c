#include "server.h"

pthread_key_t login_key;

void socket_puts(int socket, char* message) {
    send(socket, message, strlen(message), MSG_NOSIGNAL);
}

void session(int socket) {
    pthread_key_create(&login_key, NULL);
    greet(socket);
    login_menu(socket);
    socket_puts(socket, "Goodbye!\n");
    pthread_key_delete(login_key);
}

void greet(int socket) {
    char* greet_text = "\
        \n\n$#$#$#$#$#$#$#$#$#$#$#$#$\n\
Hello user!\n\
Telegram was blocked, so i decided\n\
to write my own chat using C sockets!\n\
Sign in, join to some chat via id, enter password, and have fun!\n\
$#$#$#$#$#$#$#$#$#$#$#$#$\n\n\n";
    socket_puts(socket, greet_text);
}

int login_menu(int socket) {
    char* menu_text = "\
Menu (32 chars command max):\n\n\
1. Sign in\n\
2. Log in\n\
3. Exit\n\
:> ";
    socket_puts(socket, menu_text);
    char command[32];
    if(recv(socket, command, 256, 0) <= 0) return 1;
    char* login;
    switch(atoi(command)) {
        case 1:
            login = (char*)malloc(sizeof(char)*32);
            memset(login, 0, sizeof(char)*32);
            if(!sign_in(socket, login)) {
                pthread_setspecific(login_key, login);
                return menu(socket);
            }
            free(login);
            socket_puts(socket, "\nFailure on sign in\n");
            return login_menu(socket);
        case 2:
            login = (char*)malloc(sizeof(char)*32);
            memset(login, 0, sizeof(char)*32);
            if(!log_in(socket, login)) {
                pthread_setspecific(login_key, login);
                return menu(socket);
            }
            free(login);
            socket_puts(socket, "\nLog in failed\n");
            return login_menu(socket);
        case 3:
            return 0;
        default:
            socket_puts(socket, "No such command\n\n");
            return login_menu(socket);
    }
}

int menu(int socket) {
    char* menu_text = "\
Menu (32 chars command max):\n\n\
1. Create room\n\
2. Connect to room\n\
3. Exit\n\
:> ";
    socket_puts(socket, menu_text);
    char command[32];
    char* chat_name;
    if(recv(socket, command, 256, 0) <= 0) return 1;
    switch(atoi(command)) {
        case 1:
            chat_name = create_room(socket);
            if(chat_name == NULL) {
                socket_puts(socket, "\nFailure on creating chat room\n");
                free(chat_name);
                return menu(socket);
            }
            return chat_interface(socket, chat_name);
        case 2:
            chat_name = connect_to_room(socket);
            if(chat_name == NULL) {
                socket_puts(socket, "\nLog in failed\n");
                free(chat_name);
                return menu(socket);
            }
            return chat_interface(socket, chat_name);
        case 3:
            return 0;
        default:
            socket_puts(socket, "\nNo such command\n");
            return menu(socket);
    }
}

void flush_chat_history(int socket, char* chat_name){
    char* chat_file_path = (char*)malloc(sizeof(char)*40);
    stpcpy(chat_file_path, "./chats/\0");
    strcat(chat_file_path, chat_name);
    FILE* new_chat_file = fopen(chat_file_path, "r");
    free(chat_file_path);
    char* old_messages = (char*)malloc(sizeof(char) * 1024);
    fread(old_messages, 1, 1024, new_chat_file);
    fclose(new_chat_file);
    socket_puts(socket, old_messages);
    free(old_messages);
}

int lock_chat(char* chat_name) {
    char* chat_lock_file_path = (char*)malloc(sizeof(char)*46);
    stpcpy(chat_lock_file_path, "./chats/\0");
    strcat(chat_lock_file_path, chat_name);
    strcat(chat_lock_file_path, "_lock");

    FILE* chat_lock_file = fopen(chat_lock_file_path, "w");
    free(chat_lock_file_path);
    fclose(chat_lock_file);
    return 0;
}

int unlock_chat(char* chat_name) {
    char* chat_lock_file_path = (char*)malloc(sizeof(char)*46);
    stpcpy(chat_lock_file_path, "./chats/\0");
    strcat(chat_lock_file_path, chat_name);
    strcat(chat_lock_file_path, "_lock");

    remove(chat_lock_file_path);
    free(chat_lock_file_path);
    return 0;
}

int chat_locked(char* chat_name) {
    char* chat_lock_file_path = (char*)malloc(sizeof(char)*46);
    stpcpy(chat_lock_file_path, "./chats/\0");
    strcat(chat_lock_file_path, chat_name);
    strcat(chat_lock_file_path, "_lock");

    FILE* chat_lock_file = fopen(chat_lock_file_path, "r");
    free(chat_lock_file_path);
    if(!chat_lock_file) return 0;
    fclose(chat_lock_file);
    return 1;
}

int write_new_message_to_chat_file(int socket, char* chat_name, char* message, char* login){
    char* chat_file_path = (char*)malloc(sizeof(char)*41);
    stpcpy(chat_file_path, "./chats/\0");
    strcat(chat_file_path, chat_name);

    for(int tries = 0; chat_locked(chat_name); tries++){
        if(tries > 5) return 1;
        sleep(1);
    }

    lock_chat(chat_name);
    FILE* new_chat_file = fopen(chat_file_path, "a");
    free(chat_file_path);
    fprintf(new_chat_file, "%s: %s", login, message);
    fclose(new_chat_file);
    unlock_chat(chat_name);
    return 0;
}

int check_connection(int socket) {
    int resp = send(socket, "", 1, MSG_NOSIGNAL);
    return resp != -1;
}

void* listen_for_new_messages(void* _chat_data) {
    chat_data* ch_data = (chat_data*)(_chat_data);
    char* chat_file_path = (char*)malloc(sizeof(char)*40);
    stpcpy(chat_file_path, "./chats/\0");
    strcat(chat_file_path, ch_data->chat_name);
    int last_message = 1;
    while(check_connection(ch_data->socket)) {
        FILE* chat_file = fopen(chat_file_path, "r");
        char* line = (char*)malloc(sizeof(char) * 284);
        memset(line, 0, sizeof(char) * 284);
        int message_index = 0;
        while(fgets(line, 284, chat_file) != NULL){
            message_index++;
            continue;
        }
        puts(line);
        fclose(chat_file);
        char login[33];
        memset(login, 0, 32);
        int i;
        for(i=0; line[i]!=':' && i<32; i++){
            login[i] = line[i];
        }
        login[++i] = 0;

        if(strcmp(login, ch_data->login) && message_index>last_message) {
            last_message = message_index;
            socket_puts(ch_data->socket, line);
        }
        sleep(1);
    }
    free(chat_file_path);
    return 0;
}

int chat_interface(int socket, char* chat_name) {
    flush_chat_history(socket, chat_name);
    socket_puts(socket, "\n\
************************************\n\
******Connected to ");
    socket_puts(socket, chat_name);
    socket_puts(socket, "*****\n\
***********************************\n");
    char* login = (char*)pthread_getspecific(login_key);
    chat_data* ch_data = (chat_data*)malloc(sizeof(chat_data));
    ch_data->login = login;
    ch_data->chat_name = chat_name;
    ch_data->socket = socket;

    pthread_t listen_thread;
    if( pthread_create(&listen_thread, NULL, listen_for_new_messages, (void*)ch_data) < 0)
    {
        perror("could not create thread");
        return 1;
    }

    char* new_message = (char*)malloc(sizeof(char)*284);
    memset(new_message, 0, sizeof(char)*250);
    char* stop_word = "exit\n";
    int recv_size;
    while((recv_size = recv(socket, new_message, 250, 0)) != 0) {   
        if(!strcmp(new_message, stop_word)) {
            break;
        }
        socket_puts(socket, login);
        socket_puts(socket, ": ");
        socket_puts(socket, new_message);
        
        write_new_message_to_chat_file(socket, chat_name, new_message, login);
        memset(new_message, 0, sizeof(char)*250);
    }
    return 0;
}

char* create_room(int socket) {
    int recv_size;
    
    socket_puts(socket, "Enter chat name: ");
    char* chat_name = (char*)malloc(sizeof(char)*32);
    recv_size = recv(socket, chat_name, 32, 0);
    if(!recv_size) return NULL;
    chat_name[recv_size-1] = 0;

    socket_puts(socket, "Enter chat password: ");
    char chat_pass[32];
    recv_size = recv(socket, chat_pass, 32, 0);
    if(!recv_size) return NULL;
    chat_pass[recv_size-1] = 0;

    char* chat_file_path = (char*)malloc(sizeof(char)*40);
    stpcpy(chat_file_path, "./chats/\0");
    strcat(chat_file_path, chat_name);

    if(access(chat_file_path, F_OK) != -1) {
        socket_puts(socket, "Chat exists\n");
        free(chat_file_path);
        return NULL;
    }

    FILE* new_chat_file = fopen(chat_file_path, "w");
    free(chat_file_path);
    char* login = (char*)pthread_getspecific(login_key);
    fprintf(new_chat_file, "%s: *************** created chat %s***************\n", login, chat_name);
    fclose(new_chat_file);

    char* chat_pass_file_path = (char*)malloc(sizeof(char)*44);
    stpcpy(chat_pass_file_path, "./chats/\0");
    strcat(chat_pass_file_path, chat_name);
    strcat(chat_pass_file_path, "_pass");
    
    FILE* new_chat_pass_file = fopen(chat_pass_file_path, "w");
    free(chat_pass_file_path);
    fprintf(new_chat_pass_file, "%s", chat_pass);
    fclose(new_chat_pass_file);
    return chat_name;
} 

char* connect_to_room(int socket) {
    int recv_size;
    
    socket_puts(socket, "Enter chat name: ");
    char* chat_name = (char*)malloc(sizeof(char)*32);
    recv_size = recv(socket, chat_name, 32, 0);
    if(!recv_size) return NULL;
    chat_name[recv_size-1] = 0;

    socket_puts(socket, "Enter chat password: ");
    char chat_pass[32];
    recv_size = recv(socket, chat_pass, 32, 0);
    if(!recv_size) return NULL;
    chat_pass[recv_size-1] = 0;

    char* chat_pass_file_path = (char*)malloc(sizeof(char)*44);
    stpcpy(chat_pass_file_path, "./chats/\0");
    strcat(chat_pass_file_path, chat_name);
    strcat(chat_pass_file_path, "_pass");
    
    FILE* new_chat_pass_file = fopen(chat_pass_file_path, "r");
    free(chat_pass_file_path);
    if(!new_chat_pass_file) {
        socket_puts(socket, "No such chat with such password\n");
        return NULL;
    }
    char pass[32];
    fgets(pass, 32, new_chat_pass_file);
    fclose(new_chat_pass_file);
    if(strcmp(chat_pass, pass)) {
        socket_puts(socket, "No such chat with such password\n");
        return NULL;
    }
    return chat_name;
}

int sign_in(int socket, char* login) {
    int recv_size;

    socket_puts(socket, "Enter login: ");
    char input_login[32];
    recv_size = recv(socket, input_login, 32, 0);
    if(!recv_size) return 1;
    input_login[recv_size-1] = 0;

    socket_puts(socket, "Enter password: ");
    char password[32];
    recv_size = recv(socket, password, 32, 0);
    if(!recv_size) return 1;
    password[recv_size-1] = 0;
    
    char* user_file_path = (char*)malloc(sizeof(char)*40);
    stpcpy(user_file_path, "./users/\0");
    strcat(user_file_path, input_login);
 
    if(access(user_file_path, F_OK) != -1) {
        socket_puts(socket, "User exists\n");
        free(user_file_path);
        return 1;
    }

    FILE* new_user_file = fopen(user_file_path, "w");
    free(user_file_path);
    fprintf(new_user_file, "%s", password);
    fclose(new_user_file);
    stpcpy(login, input_login);
    return 0;
}

int log_in(int socket, char* login) {
    int recv_size;
    
    socket_puts(socket, "Enter login: ");
    char input_login[32];
    recv_size = recv(socket, input_login, 32, 0);
    if(!recv_size) return 1;
    input_login[recv_size-1] = 0;

    socket_puts(socket, "Enter password: ");
    char password[32];
    recv_size = recv(socket, password, 32, 0);
    if(!recv_size) return 1;
    password[recv_size-1] = 0;
    
    char* user_file_path = (char*)malloc(sizeof(char)*40);
    stpcpy(user_file_path, "./users/\0");
    strcat(user_file_path, input_login);

    FILE* new_user_file = fopen(user_file_path, "r");
    free(user_file_path);
    if(!new_user_file) {
        socket_puts(socket, "No such user with such password\n");
        return 1;
    }
    char user_password[32];
    fgets(user_password, 32, new_user_file);
    if(strcmp(user_password, password)) {
        socket_puts(socket, "No such user with such password\n");
        return 1;
    }
    socket_puts(socket, "\n\nHello ");
    socket_puts(socket, input_login);
    socket_puts(socket, "!\n\n\n");
    stpcpy(login, input_login);
    return 0;
}
