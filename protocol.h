/*
 * Protocol Header for Live Chat Room System
 * Defines communication protocol between server and client
 *
 * Message Format: Text-based protocol with newline delimiters
 * All messages end with '\n' character
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

#define SERVER_PORT 8080
#define MAX_USERNAME 32
#define MAX_MESSAGE 256
#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024

// ============================================================================
// MESSAGE TYPE DEFINITIONS
// ============================================================================

#define MSG_TYPE_AUTH       "AUTH"
#define MSG_TYPE_MESSAGE    "MSG"
#define MSG_TYPE_NOTIFY     "NOTIFY"
#define MSG_TYPE_ERROR      "ERROR"
#define MSG_TYPE_DISCONNECT "DISCONNECT"

// ============================================================================
// RESPONSE CODE DEFINITIONS
// ============================================================================

#define AUTH_OK             "AUTH_OK"
#define AUTH_FAILED         "AUTH_FAILED:Username already taken"
#define AUTH_FAILED_INVALID "AUTH_FAILED:Invalid username"
#define MSG_DELIVERED       "MSG_OK"
#define SERVER_FULL         "ERROR:Server is full"
#define DISCONNECT_ACK      "DISCONNECT_ACK"

// ============================================================================
// MESSAGE STRUCTURE
// ============================================================================

typedef struct {
    char type[16];              // Message type (AUTH, MSG, NOTIFY, etc.)
    char sender[MAX_USERNAME];  // Username of sender
    char content[MAX_MESSAGE];  // Message content
} message_t;

// ============================================================================
// PROTOCOL FORMAT DOCUMENTATION
// ============================================================================

/*
 * TEXT-BASED PROTOCOL FORMAT:
 * All messages are newline-terminated strings
 *
 * 1. AUTHENTICATION (Client -> Server)
 *    Format: "AUTH:username\n"
 *    Example: "AUTH:alice\n"
 *
 * 2. AUTHENTICATION RESPONSE (Server -> Client)
 *    Success: "AUTH_OK\n"
 *    Failure: "AUTH_FAILED:Username already taken\n"
 *             "AUTH_FAILED:Invalid username\n"
 *
 * 3. CHAT MESSAGE (Client -> Server)
 *    Format: "MSG:username:message content\n"
 *    Example: "MSG:alice:Hello everyone!\n"
 *
 * 4. BROADCAST MESSAGE (Server -> All Clients)
 *    Format: "MSG:username:message content\n"
 *    Example: "MSG:alice:Hello everyone!\n"
 *
 * 5. NOTIFICATION (Server -> All Clients)
 *    Format: "NOTIFY:notification text\n"
 *    Example: "NOTIFY:alice joined the chat\n"
 *             "NOTIFY:bob left the chat\n"
 *
 * 6. ERROR MESSAGE (Server -> Client)
 *    Format: "ERROR:error description\n"
 *    Example: "ERROR:Message too long\n"
 *             "ERROR:Server is full\n"
 *
 * 7. DISCONNECT (Client -> Server)
 *    Format: "DISCONNECT:username\n"
 *    Example: "DISCONNECT:alice\n"
 *
 * 8. DISCONNECT ACK (Server -> Client)
 *    Format: "DISCONNECT_ACK\n"
 */

// ============================================================================
// PROTOCOL HELPER FUNCTIONS
// ============================================================================

/**
 * Format an authentication message
 * @param buffer Output buffer
 * @param username Username to authenticate
 * @return Length of formatted message
 */
static inline int format_auth_message(char *buffer, const char *username) {
    return snprintf(buffer, BUFFER_SIZE, "AUTH:%s\n", username);
}

/**
 * Format a chat message
 * @param buffer Output buffer
 * @param sender Username of sender
 * @param content Message content
 * @return Length of formatted message
 */
static inline int format_chat_message(char *buffer, const char *sender, const char *content) {
    return snprintf(buffer, BUFFER_SIZE, "MSG:%s:%s\n", sender, content);
}

/**
 * Format a notification message
 * @param buffer Output buffer
 * @param notification Notification text
 * @return Length of formatted message
 */
static inline int format_notification(char *buffer, const char *notification) {
    return snprintf(buffer, BUFFER_SIZE, "NOTIFY:%s\n", notification);
}

/**
 * Format an error message
 * @param buffer Output buffer
 * @param error Error description
 * @return Length of formatted message
 */
static inline int format_error_message(char *buffer, const char *error) {
    return snprintf(buffer, BUFFER_SIZE, "ERROR:%s\n", error);
}

/**
 * Format a disconnect message
 * @param buffer Output buffer
 * @param username Username disconnecting
 * @return Length of formatted message
 */
static inline int format_disconnect_message(char *buffer, const char *username) {
    return snprintf(buffer, BUFFER_SIZE, "DISCONNECT:%s\n", username);
}

/**
 * Parse a received message into message_t structure
 * @param raw_message Raw message string received
 * @param msg Output message structure
 * @return 0 on success, -1 on error
 */
static inline int parse_message(const char *raw_message, message_t *msg) {
    // Clear the message structure
    memset(msg, 0, sizeof(message_t));

    // Create a mutable copy for parsing
    char buffer[BUFFER_SIZE];
    strncpy(buffer, raw_message, BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';

    // Remove trailing newline if present
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    // Extract message type
    char *token = strtok(buffer, ":");
    if (token == NULL) return -1;
    strncpy(msg->type, token, sizeof(msg->type) - 1);

    // Parse based on message type
    if (strcmp(msg->type, "AUTH") == 0) {
        // AUTH:username
        token = strtok(NULL, ":");
        if (token == NULL) return -1;
        strncpy(msg->sender, token, sizeof(msg->sender) - 1);

    } else if (strcmp(msg->type, "MSG") == 0) {
        // MSG:sender:content
        token = strtok(NULL, ":");
        if (token == NULL) return -1;
        strncpy(msg->sender, token, sizeof(msg->sender) - 1);

        // Get the rest as content (may contain ':')
        token = strtok(NULL, "");
        if (token == NULL) return -1;
        strncpy(msg->content, token, sizeof(msg->content) - 1);

    } else if (strcmp(msg->type, "NOTIFY") == 0) {
        // NOTIFY:content
        token = strtok(NULL, "");
        if (token == NULL) return -1;
        strncpy(msg->content, token, sizeof(msg->content) - 1);

    } else if (strcmp(msg->type, "ERROR") == 0) {
        // ERROR:content
        token = strtok(NULL, "");
        if (token == NULL) return -1;
        strncpy(msg->content, token, sizeof(msg->content) - 1);

    } else if (strcmp(msg->type, "DISCONNECT") == 0) {
        // DISCONNECT:username
        token = strtok(NULL, ":");
        if (token == NULL) return -1;
        strncpy(msg->sender, token, sizeof(msg->sender) - 1);
    }

    return 0;
}

/**
 * Validate username according to protocol rules
 * @param username Username to validate
 * @return 1 if valid, 0 if invalid
 */
static inline int validate_username(const char *username) {
    if (username == NULL) return 0;

    size_t len = strlen(username);

    // Check length (must be between 1 and MAX_USERNAME-1)
    if (len == 0 || len >= MAX_USERNAME) return 0;

    // Check for invalid characters (only alphanumeric and underscore allowed)
    for (size_t i = 0; i < len; i++) {
        char c = username[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '_')) {
            return 0;
        }
    }

    return 1;
}

/**
 * Validate message content according to protocol rules
 * @param content Message content to validate
 * @return 1 if valid, 0 if invalid
 */
static inline int validate_message_content(const char *content) {
    if (content == NULL) return 0;

    size_t len = strlen(content);

    // Check length (must be between 1 and MAX_MESSAGE-1)
    if (len == 0 || len >= MAX_MESSAGE) return 0;

    return 1;
}

// ============================================================================
// CLIENT STATE STRUCTURE (for server-side tracking)
// ============================================================================

typedef struct {
    int socket_fd;                  // Client socket file descriptor
    char username[MAX_USERNAME];    // Authenticated username
    int authenticated;              // Authentication status (0 or 1)
} client_info_t;

// ============================================================================
// MESSAGE QUEUE STRUCTURE (for thread-safe messaging)
// ============================================================================

#define QUEUE_SIZE 100

typedef struct {
    message_t messages[QUEUE_SIZE]; // Circular buffer
    int head;                       // Write position
    int tail;                       // Read position
    int count;                      // Number of messages in queue
} message_queue_t;

/**
 * Initialize message queue
 * @param queue Queue to initialize
 */
static inline void init_message_queue(message_queue_t *queue) {
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

/**
 * Check if queue is empty
 * @param queue Queue to check
 * @return 1 if empty, 0 if not empty
 */
static inline int is_queue_empty(message_queue_t *queue) {
    return queue->count == 0;
}

/**
 * Check if queue is full
 * @param queue Queue to check
 * @return 1 if full, 0 if not full
 */
static inline int is_queue_full(message_queue_t *queue) {
    return queue->count >= QUEUE_SIZE;
}

/**
 * Enqueue a message (caller must handle mutex locking)
 * @param queue Queue to add to
 * @param msg Message to add
 * @return 0 on success, -1 if queue is full
 */
static inline int enqueue_message(message_queue_t *queue, const message_t *msg) {
    if (is_queue_full(queue)) return -1;

    queue->messages[queue->head] = *msg;
    queue->head = (queue->head + 1) % QUEUE_SIZE;
    queue->count++;

    return 0;
}

/**
 * Dequeue a message (caller must handle mutex locking)
 * @param queue Queue to read from
 * @param msg Output message
 * @return 0 on success, -1 if queue is empty
 */
static inline int dequeue_message(message_queue_t *queue, message_t *msg) {
    if (is_queue_empty(queue)) return -1;

    *msg = queue->messages[queue->tail];
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;
    queue->count--;

    return 0;
}

// ============================================================================
// PROTOCOL VERSION
// ============================================================================

#define PROTOCOL_VERSION "1.0"

#endif // PROTOCOL_H
