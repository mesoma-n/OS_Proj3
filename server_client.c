#include "server.h"

#define DEFAULT_ROOM "Lobby"

// Synchronization primitives
extern int numReaders;
extern pthread_mutex_t rw_lock;
extern pthread_mutex_t mutex;

// Global variables
extern struct user_node *userList;
extern struct room_node *roomList;
extern char const *server_MOTD;

// Utility function to trim whitespace
char *trimwhitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// Thread to handle client commands and messages
void *client_receive(void *ptr) {
    int client = *(int *)ptr;
    char buffer[MAXBUFF], command[MAXBUFF], tmpbuff[MAXBUFF];
    char *args[MAX_ARGS];
    char username[20];
    int received;

    // Assign guest username and add to Lobby
    sprintf(username, "guest%d", client);
    pthread_mutex_lock(&mutex);
    addUser(&userList, client, username);
    joinRoom(&roomList, DEFAULT_ROOM, client, username);
    pthread_mutex_unlock(&mutex);

    // Welcome message
    send(client, server_MOTD, strlen(server_MOTD), 0);

    while ((received = read(client, buffer, MAXBUFF)) > 0) {
        buffer[received] = '\0';
        strcpy(command, buffer);

        // Tokenize input into command and arguments
        args[0] = strtok(command, " ");
        int i = 0;
        while (args[i] != NULL) {
            args[++i] = strtok(NULL, " ");
            if (args[i]) args[i] = trimwhitespace(args[i]);
        }

        pthread_mutex_lock(&mutex);
        if (strcmp(args[0], "login") == 0) {
            updateUsername(&userList, client, args[1]);
        } else if (strcmp(args[0], "create") == 0) {
            createRoom(&roomList, args[1]);
        } else if (strcmp(args[0], "join") == 0) {
            joinRoom(&roomList, args[1], client, username);
        } else if (strcmp(args[0], "leave") == 0) {
            leaveRoom(&roomList, args[1], client);
        } else if (strcmp(args[0], "users") == 0) {
            listUsers(userList, tmpbuff);
            send(client, tmpbuff, strlen(tmpbuff), 0);
        } else if (strcmp(args[0], "rooms") == 0) {
            listRooms(roomList, tmpbuff);
            send(client, tmpbuff, strlen(tmpbuff), 0);
        } else if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "logout") == 0) {
            removeUser(&userList, client);
            leaveAllRooms(&roomList, client);
            close(client);
            pthread_mutex_unlock(&mutex);
            break;
        } else {
            broadcastMessage(userList, roomList, client, username, buffer);
        }
        pthread_mutex_unlock(&mutex);

        memset(buffer, 0, MAXBUFF);
    }
    return NULL;
}