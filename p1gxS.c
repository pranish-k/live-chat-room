/*
 * Live Chat Room - Server Implementation
 * File: p1gxS.c
 *
 * Multi-threaded TCP server that handles multiple clients concurrently.
 * Features:
 * - Username-based authentication
 * - Real-time message broadcasting to all connected clients
 * - Thread-safe message queue
 * - Graceful shutdown handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "protocol.h"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Client tracking
client_info_t clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Message queue for broadcasting
message_queue_t msg_queue;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// Server control
volatile int server_running = 1;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

void signal_handler(int sig);
int add_client(int socket_fd, const char *username);
void remove_client(int socket_fd);
int username_exists(const char *username);
void *handle_client(void *arg);
void *broadcast_thread(void *arg);
void broadcast_notification(const char *notification);

// ============================================================================
// SIGNAL HANDLER
// ============================================================================

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n[Server] Received shutdown signal...\n");
        server_running = 0;
    }
}

// ============================================================================
// CLIENT MANAGEMENT FUNCTIONS
// ============================================================================

/**
 * Add a new client to the tracking list
 * Thread-safe operation using mutex
 */
int add_client(int socket_fd, const char *username) {
    pthread_mutex_lock(&clients_mutex);

    if (client_count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&clients_mutex);
        return -1;  // Server full
    }

    clients[client_count].socket_fd = socket_fd;
    strncpy(clients[client_count].username, username, MAX_USERNAME - 1);
    clients[client_count].username[MAX_USERNAME - 1] = '\0';
    clients[client_count].authenticated = 1;
    client_count++;

    printf("[Server] Client '%s' added. Total clients: %d\n", username, client_count);

    pthread_mutex_unlock(&clients_mutex);
    return 0;
}

/**
 * Remove a client from the tracking list
 * Thread-safe operation using mutex
 */
void remove_client(int socket_fd) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket_fd == socket_fd) {
            printf("[Server] Removing client '%s'\n", clients[i].username);

            // Shift remaining clients
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

/**
 * Check if a username already exists
 * Thread-safe operation using mutex
 */
int username_exists(const char *username) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].username, username) == 0) {
            pthread_mutex_unlock(&clients_mutex);
            return 1;  // Username exists
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    return 0;  // Username available
}

/**
 * Broadcast a notification to all connected clients
 */
void broadcast_notification(const char *notification) {
    char notify_msg[BUFFER_SIZE];
    format_notification(notify_msg, notification);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        send(clients[i].socket_fd, notify_msg, strlen(notify_msg), 0);
    }
    pthread_mutex_unlock(&clients_mutex);

    printf("[Server] Notification broadcasted: %s\n", notification);
}

// ============================================================================
// BROADCAST THREAD
// ============================================================================

/**
 * Dedicated thread for broadcasting messages to all clients
 * Reads from message queue and sends to all connected clients
 */
void *broadcast_thread(void *arg) {
    (void)arg;  // Unused parameter

    printf("[Broadcast Thread] Started\n");

    while (server_running) {
        message_t msg;

        pthread_mutex_lock(&queue_mutex);

        // Wait for messages in queue
        while (is_queue_empty(&msg_queue) && server_running) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }

        if (!server_running) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        // Dequeue message
        if (dequeue_message(&msg_queue, &msg) == 0) {
            pthread_mutex_unlock(&queue_mutex);

            // Format broadcast message
            char broadcast[BUFFER_SIZE];
            format_chat_message(broadcast, msg.sender, msg.content);

            printf("[Broadcast] %s: %s\n", msg.sender, msg.content);

            // Send to all connected clients
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < client_count; i++) {
                if (send(clients[i].socket_fd, broadcast, strlen(broadcast), 0) < 0) {
                    perror("[Broadcast] Send failed");
                }
            }
            pthread_mutex_unlock(&clients_mutex);
        } else {
            pthread_mutex_unlock(&queue_mutex);
        }
    }

    printf("[Broadcast Thread] Exiting\n");
    return NULL;
}

// ============================================================================
// CLIENT HANDLER THREAD
// ============================================================================

/**
 * Handle individual client connection
 * Each client runs in its own thread
 */
void *handle_client(void *arg) {
    int client_socket = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE] = {0};
    char username[MAX_USERNAME] = {0};

    printf("[Thread %p] New client connected (socket %d)\n",
           (void*)pthread_self(), client_socket);

    // ========================================
    // AUTHENTICATION
    // ========================================

    // Read authentication message
    int valread = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (valread <= 0) {
        printf("[Thread %p] Failed to read auth message\n", (void*)pthread_self());
        close(client_socket);
        return NULL;
    }
    buffer[valread] = '\0';

    // Parse authentication message
    message_t auth_msg;
    if (parse_message(buffer, &auth_msg) != 0 ||
        strcmp(auth_msg.type, MSG_TYPE_AUTH) != 0) {

        // Invalid auth message
        char response[BUFFER_SIZE];
        format_error_message(response, "Invalid authentication format");
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        printf("[Thread %p] Invalid auth format\n", (void*)pthread_self());
        return NULL;
    }

    // Validate username
    if (!validate_username(auth_msg.sender)) {
        char response[BUFFER_SIZE];
        strcpy(response, AUTH_FAILED_INVALID);
        strcat(response, "\n");
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        printf("[Thread %p] Invalid username: %s\n", (void*)pthread_self(), auth_msg.sender);
        return NULL;
    }

    // Check if username already taken
    if (username_exists(auth_msg.sender)) {
        char response[BUFFER_SIZE];
        strcpy(response, AUTH_FAILED);
        strcat(response, "\n");
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        printf("[Thread %p] Username already taken: %s\n", (void*)pthread_self(), auth_msg.sender);
        return NULL;
    }

    // Authentication successful
    strncpy(username, auth_msg.sender, MAX_USERNAME - 1);
    username[MAX_USERNAME - 1] = '\0';

    // Add client to tracking list
    if (add_client(client_socket, username) != 0) {
        char response[BUFFER_SIZE];
        strcpy(response, SERVER_FULL);
        strcat(response, "\n");
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        printf("[Thread %p] Server full, rejecting client\n", (void*)pthread_self());
        return NULL;
    }

    // Send AUTH_OK
    char response[BUFFER_SIZE];
    strcpy(response, AUTH_OK);
    strcat(response, "\n");
    send(client_socket, response, strlen(response), 0);

    printf("[Thread %p] User '%s' authenticated successfully\n",
           (void*)pthread_self(), username);

    // Broadcast join notification
    char join_msg[BUFFER_SIZE];
    snprintf(join_msg, BUFFER_SIZE, "%s joined the chat", username);
    broadcast_notification(join_msg);

    // ========================================
    // MESSAGE RECEIVING LOOP
    // ========================================

    while (server_running) {
        memset(buffer, 0, BUFFER_SIZE);
        valread = read(client_socket, buffer, BUFFER_SIZE - 1);

        if (valread <= 0) {
            // Client disconnected
            printf("[Thread %p] User '%s' disconnected\n", (void*)pthread_self(), username);
            break;
        }

        buffer[valread] = '\0';

        // Parse message
        message_t msg;
        if (parse_message(buffer, &msg) == 0) {

            if (strcmp(msg.type, MSG_TYPE_MESSAGE) == 0) {
                // Regular chat message
                printf("[%s] %s\n", username, msg.content);

                // Copy username to message (in case client sent wrong username)
                strncpy(msg.sender, username, MAX_USERNAME - 1);

                // Add to message queue for broadcasting
                pthread_mutex_lock(&queue_mutex);
                if (enqueue_message(&msg_queue, &msg) == 0) {
                    pthread_cond_signal(&queue_cond);  // Wake up broadcast thread
                } else {
                    printf("[Thread %p] Message queue full!\n", (void*)pthread_self());
                }
                pthread_mutex_unlock(&queue_mutex);

            } else if (strcmp(msg.type, MSG_TYPE_DISCONNECT) == 0) {
                // Client requesting disconnect
                printf("[Thread %p] User '%s' requested disconnect\n",
                       (void*)pthread_self(), username);
                break;
            }
        } else {
            printf("[Thread %p] Failed to parse message from %s\n",
                   (void*)pthread_self(), username);
        }
    }

    // ========================================
    // CLEANUP
    // ========================================

    // Broadcast leave notification
    char leave_msg[BUFFER_SIZE];
    snprintf(leave_msg, BUFFER_SIZE, "%s left the chat", username);
    broadcast_notification(leave_msg);

    remove_client(client_socket);
    close(client_socket);

    printf("[Thread %p] Client handler for '%s' exiting\n", (void*)pthread_self(), username);

    return NULL;
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    int server_fd;
    struct sockaddr_in address;

    printf("╔════════════════════════════════════════╗\n");
    printf("║     Live Chat Room - Server           ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // Register signal handler for graceful shutdown
    signal(SIGINT, signal_handler);

    // Initialize message queue
    init_message_queue(&msg_queue);
    printf("[Server] Message queue initialized\n");

    // Create broadcast thread
    pthread_t broadcast_tid;
    if (pthread_create(&broadcast_tid, NULL, broadcast_thread, NULL) != 0) {
        perror("[Server] Failed to create broadcast thread");
        exit(EXIT_FAILURE);
    }
    pthread_detach(broadcast_tid);
    printf("[Server] Broadcast thread started\n");

    // ========================================
    // CREATE SOCKET
    // ========================================

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[Server] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("[Server] Setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // ========================================
    // BIND SOCKET
    // ========================================

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Server] Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // ========================================
    // LISTEN FOR CONNECTIONS
    // ========================================

    if (listen(server_fd, 5) < 0) {
        perror("[Server] Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[Server] Listening on port %d\n", SERVER_PORT);
    printf("[Server] Maximum clients: %d\n", MAX_CLIENTS);
    printf("[Server] Press Ctrl+C to shutdown\n");
    printf("========================================\n\n");

    // ========================================
    // MAIN ACCEPT LOOP
    // ========================================

    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (!server_running) break;  // Check shutdown flag

        if (new_socket < 0) {
            if (server_running) {
                perror("[Server] Accept failed");
            }
            continue;
        }

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        printf("[Server] New connection from %s\n", client_ip);

        // Allocate memory for socket fd to pass to thread
        int *client_sock = malloc(sizeof(int));
        if (client_sock == NULL) {
            perror("[Server] Memory allocation failed");
            close(new_socket);
            continue;
        }
        *client_sock = new_socket;

        // Create thread to handle client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client_sock) != 0) {
            perror("[Server] Thread creation failed");
            close(new_socket);
            free(client_sock);
            continue;
        }

        // Detach thread so it cleans up automatically
        pthread_detach(thread_id);
    }

    // ========================================
    // CLEANUP
    // ========================================

    printf("\n[Server] Shutting down...\n");

    // Close all client connections
    pthread_mutex_lock(&clients_mutex);
    printf("[Server] Closing %d client connection(s)\n", client_count);
    for (int i = 0; i < client_count; i++) {
        close(clients[i].socket_fd);
    }
    client_count = 0;
    pthread_mutex_unlock(&clients_mutex);

    // Close server socket
    close(server_fd);

    // Signal broadcast thread to exit
    pthread_cond_signal(&queue_cond);

    // Destroy synchronization primitives
    pthread_mutex_destroy(&clients_mutex);
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);

    printf("[Server] Shutdown complete\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║         Server Terminated              ║\n");
    printf("╚════════════════════════════════════════╝\n");

    return 0;
}
