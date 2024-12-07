#include "server.h"

// Synchronization primitives
int numReaders = 0;
pthread_mutex_t rw_lock = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  

// Global variables
char const *server_MOTD = "Welcome to the BisonChat Server!\nchat>";
struct user_node *userList = NULL;  
struct room_node *roomList = NULL;  

void sigintHandler(int sig_num) {
    printf("\n[SERVER SHUTDOWN] Gracefully terminating...\n");

    pthread_mutex_lock(&mutex);

    // Close all user connections
    struct user_node *user = userList;
    while (user) {
        close(user->socket);
        user = user->next;
    }

    // Free memory
    freeUserList(userList);
    freeRoomList(roomList);

    pthread_mutex_unlock(&mutex);
    printf("All resources freed. Goodbye!\n");
    exit(0);
}

int main(int argc, char **argv) {
    signal(SIGINT, sigintHandler);

    // Create the default "Lobby" room
    pthread_mutex_lock(&mutex);
    createRoom(&roomList, "Lobby");
    pthread_mutex_unlock(&mutex);

    // Set up server socket
    int server_socket = get_server_socket();
    if (start_server(server_socket, BACKLOG) == -1) {
        perror("Failed to start server");
        exit(EXIT_FAILURE);
    }
    printf("[SERVER STARTED] Listening on PORT: %d\n", PORT);

    // Main server loop
    while (1) {
        int new_client = accept_client(server_socket);
        if (new_client != -1) {
            pthread_t client_thread;
            pthread_create(&client_thread, NULL, client_receive, (void *)&new_client);
        }
    }

    close(server_socket);
    return 0;
}

int get_server_socket() {
    int server_socket, opt = 1;
    struct sockaddr_in address;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Set socket options failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    return server_socket;
}

int start_server(int server_socket, int backlog) {
    return listen(server_socket, backlog);
}

int accept_client(int server_socket) {
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);

    if (client_socket < 0) {
        perror("Accept client failed");
        return -1;
    }
    return client_socket;
}
