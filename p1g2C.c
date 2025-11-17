// Live Chat Room - Multi-threaded TCP Client
// Real-time chat with authentication and dual I/O threads

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include "protocol.h"

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RED     "\033[31m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_MAGENTA "\033[35m"

// Global state
int global_sock = 0;
volatile int keep_running = 1;
char my_username[MAX_USERNAME];

// Signal handler
void signal_handler(int sig);
void display_welcome_banner(const char *username);
void send_disconnect_message(const char *username);
void *receive_thread(void *arg);

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n%s[!] Caught interrupt signal, disconnecting...%s\n",
               COLOR_YELLOW, COLOR_RESET);
        keep_running = 0;
    }
}

// Display welcome banner after authentication
void display_welcome_banner(const char *username) {
    printf("\n");
    printf("%s╔════════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║     Live Chat Room - Connected!       ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s╟────────────────────────────────────────╢%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║%s  Username: %s%-27s%s%s║%s\n",
           COLOR_CYAN, COLOR_RESET, COLOR_GREEN, username, COLOR_RESET, COLOR_CYAN, COLOR_RESET);
    printf("%s║%s  Server:   127.0.0.1:%-16d%s║%s\n",
           COLOR_CYAN, COLOR_RESET, SERVER_PORT, COLOR_CYAN, COLOR_RESET);
    printf("%s╟────────────────────────────────────────╢%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║%s  Commands:                             %s║%s\n",
           COLOR_CYAN, COLOR_RESET, COLOR_CYAN, COLOR_RESET);
    printf("%s║%s   - Type messages to chat              %s║%s\n",
           COLOR_CYAN, COLOR_RESET, COLOR_CYAN, COLOR_RESET);
    printf("%s║%s   - 'quit' or Ctrl+D to exit           %s║%s\n",
           COLOR_CYAN, COLOR_RESET, COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════╝%s\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
}

// Send disconnect message to server
void send_disconnect_message(const char *username) {
    if (global_sock > 0) {
        char disconnect_msg[BUFFER_SIZE];
        format_disconnect_message(disconnect_msg, username);
        send(global_sock, disconnect_msg, strlen(disconnect_msg), 0);
    }
}

// Receive thread - listens for incoming messages from server
void *receive_thread(void *arg) {
    (void)arg;  // Unused parameter

    char buffer[BUFFER_SIZE];

    while (keep_running) {
        memset(buffer, 0, BUFFER_SIZE);

        int valread = read(global_sock, buffer, BUFFER_SIZE - 1);

        if (valread <= 0) {
            if (keep_running) {
                printf("\n%s[!] Disconnected from server%s\n", COLOR_RED, COLOR_RESET);
                keep_running = 0;
            }
            break;
        }

        buffer[valread] = '\0';

        // Parse message
        message_t msg;
        if (parse_message(buffer, &msg) == 0) {
            // Move cursor to beginning of line and clear it
            printf("\r\033[K");

            if (strcmp(msg.type, MSG_TYPE_MESSAGE) == 0) {
                // Regular chat message
                if (strcmp(msg.sender, my_username) == 0) {
                    // My own message (echo from server)
                    printf("%s[You]%s %s\n", COLOR_MAGENTA, COLOR_RESET, msg.content);
                } else {
                    // Message from another user
                    printf("%s[%s]%s %s\n", COLOR_CYAN, msg.sender, COLOR_RESET, msg.content);
                }
            } else if (strcmp(msg.type, MSG_TYPE_NOTIFY) == 0) {
                // System notification
                printf("%s[*] %s%s\n", COLOR_YELLOW, msg.content, COLOR_RESET);
            } else if (strcmp(msg.type, MSG_TYPE_ERROR) == 0) {
                // Error message
                printf("%s[ERROR] %s%s\n", COLOR_RED, msg.content, COLOR_RESET);
            }

            // Re-display prompt
            printf("%s> %s", COLOR_GREEN, COLOR_RESET);
            fflush(stdout);
        } else {
            // Couldn't parse, display raw message
            printf("\r\033[K");
            printf("%s[Server] %s%s", COLOR_BLUE, buffer, COLOR_RESET);
            printf("%s> %s", COLOR_GREEN, COLOR_RESET);
            fflush(stdout);
        }
    }

    return NULL;
}

// Main client function
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char username[MAX_USERNAME] = {0};

    signal(SIGINT, signal_handler);

    // Display header
    printf("\n");
    printf("%s╔════════════════════════════════════════╗%s\n", COLOR_BLUE, COLOR_RESET);
    printf("%s║       Live Chat Room - Client         ║%s\n", COLOR_BLUE, COLOR_RESET);
    printf("%s╚════════════════════════════════════════╝%s\n", COLOR_BLUE, COLOR_RESET);
    printf("\n");

    // Get username from user
    printf("Enter your username: ");
    fflush(stdout);

    if (fgets(username, MAX_USERNAME, stdin) == NULL) {
        fprintf(stderr, "%sFailed to read username%s\n", COLOR_RED, COLOR_RESET);
        return -1;
    }

    // Remove trailing newline
    size_t len = strlen(username);
    if (len > 0 && username[len - 1] == '\n') {
        username[len - 1] = '\0';
    }

    // Validate username
    if (!validate_username(username)) {
        fprintf(stderr, "%sInvalid username! Use only letters, numbers, and underscores.%s\n",
                COLOR_RED, COLOR_RESET);
        fprintf(stderr, "%sUsername must be 1-%d characters long.%s\n",
                COLOR_RED, MAX_USERNAME - 1, COLOR_RESET);
        return -1;
    }

    strncpy(my_username, username, MAX_USERNAME - 1);
    my_username[MAX_USERNAME - 1] = '\0';

    printf("%sUsername: %s%s\n", COLOR_GREEN, username, COLOR_RESET);

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    // Set up server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "%sInvalid address / Address not supported%s\n",
                COLOR_RED, COLOR_RESET);
        close(sock);
        return -1;
    }

    // Connect to server
    printf("%sConnecting to server at 127.0.0.1:%d...%s\n",
           COLOR_YELLOW, SERVER_PORT, COLOR_RESET);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "%sConnection Failed%s\n", COLOR_RED, COLOR_RESET);
        fprintf(stderr, "Make sure the server is running on port %d\n", SERVER_PORT);
        close(sock);
        return -1;
    }

    printf("%s✓ Connected to server%s\n", COLOR_GREEN, COLOR_RESET);

    // Authenticate with server
    char auth_msg[BUFFER_SIZE];
    format_auth_message(auth_msg, username);

    printf("%sAuthenticating as '%s'...%s\n", COLOR_YELLOW, username, COLOR_RESET);

    if (send(sock, auth_msg, strlen(auth_msg), 0) < 0) {
        perror("Failed to send authentication");
        close(sock);
        return -1;
    }

    // Wait for authentication response
    char auth_response[BUFFER_SIZE] = {0};
    int valread = read(sock, auth_response, BUFFER_SIZE - 1);
    if (valread <= 0) {
        fprintf(stderr, "%sFailed to receive authentication response%s\n",
                COLOR_RED, COLOR_RESET);
        close(sock);
        return -1;
    }
    auth_response[valread] = '\0';

    // Extract first line only (in case multiple messages arrive together)
    char *newline = strchr(auth_response, '\n');
    if (newline != NULL) {
        *newline = '\0';
    }

    // Check authentication result
    if (strcmp(auth_response, AUTH_OK) == 0) {
        printf("%s✓ Authentication successful!%s\n", COLOR_GREEN, COLOR_RESET);
        display_welcome_banner(username);
    } else {
        fprintf(stderr, "%s✗ Authentication failed: %s%s\n",
                COLOR_RED, auth_response, COLOR_RESET);
        close(sock);
        return -1;
    }

    // Start receive thread
    global_sock = sock;

    pthread_t recv_tid;
    if (pthread_create(&recv_tid, NULL, receive_thread, NULL) != 0) {
        perror("Failed to create receive thread");
        close(sock);
        return -1;
    }

    // Main loop - send messages from user input
    char input[MAX_MESSAGE];
    char formatted_msg[BUFFER_SIZE];

    while (keep_running) {
        printf("%s> %s", COLOR_GREEN, COLOR_RESET);
        fflush(stdout);

        // Read user input
        if (fgets(input, MAX_MESSAGE, stdin) == NULL) {
            // EOF (Ctrl+D) or error
            if (keep_running) {
                printf("\n%sDisconnecting...%s\n", COLOR_YELLOW, COLOR_RESET);
            }
            break;
        }

        if (!keep_running) break;  // Check flag again

        // Remove trailing newline
        size_t input_len = strlen(input);
        if (input_len > 0 && input[input_len - 1] == '\n') {
            input[input_len - 1] = '\0';
            input_len--;
        }

        // Skip empty messages
        if (input_len == 0) {
            continue;
        }

        // Check for quit command
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            printf("%sDisconnecting...%s\n", COLOR_YELLOW, COLOR_RESET);
            break;
        }

        // Validate message content
        if (!validate_message_content(input)) {
            fprintf(stderr, "%sMessage too long! Maximum %d characters.%s\n",
                    COLOR_RED, MAX_MESSAGE - 1, COLOR_RESET);
            continue;
        }

        // Format and send message
        format_chat_message(formatted_msg, username, input);
        if (send(sock, formatted_msg, strlen(formatted_msg), 0) < 0) {
            if (keep_running) {
                fprintf(stderr, "%sSend failed%s\n", COLOR_RED, COLOR_RESET);
            }
            break;
        }
    }

    // Cleanup and disconnect
    keep_running = 0;

    // Send disconnect notification (optional)
    send_disconnect_message(username);

    // Give server time to receive disconnect message
    usleep(100000);  // 100ms

    // Cancel and join receive thread
    pthread_cancel(recv_tid);
    pthread_join(recv_tid, NULL);

    // Close socket
    close(sock);

    // Display goodbye message
    printf("\n");
    printf("%s╔════════════════════════════════════════╗%s\n", COLOR_BLUE, COLOR_RESET);
    printf("%s║       Disconnected from server         ║%s\n", COLOR_BLUE, COLOR_RESET);
    printf("%s║           Goodbye, %-19s║%s\n", COLOR_BLUE, username, COLOR_RESET);
    printf("%s╚════════════════════════════════════════╝%s\n", COLOR_BLUE, COLOR_RESET);
    printf("\n");

    return 0;
}
