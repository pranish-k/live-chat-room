# Server Implementation Guide - Evolution from server.c to p1gxS.c

## Current State Analysis

**File:** `server.c` (Basic TCP Server)

**What You Already Have:** ✅
- Socket creation: `socket(AF_INET, SOCK_STREAM, 0)`
- Port binding: `bind()` to port 8080
- Listening: `listen()` with backlog of 5
- Accept connection: `accept()`
- Read/Write: `read()` and `send()`
- Error handling pattern established
- Clean socket closure

**What's Missing:** ❌
- Multi-threading (only handles 1 client)
- Authentication system
- Message broadcasting to all clients
- Continuous message loop (server exits after 1 message)
- Client management (tracking multiple clients)
- Thread-safe message queue
- Protocol integration

---

## Evolution Strategy

We'll transform `server.c` into `p1gxS.c` through **8 incremental steps**. Each step builds on the previous one and can be tested independently.

### Step-by-Step Roadmap

```
server.c (Current)
    ↓
Step 1: Add protocol.h integration
    ↓
Step 2: Change port to 5000, add continuous accept loop
    ↓
Step 3: Add multi-threading (one thread per client)
    ↓
Step 4: Add client tracking structure
    ↓
Step 5: Add authentication system
    ↓
Step 6: Add thread-safe message queue
    ↓
Step 7: Add broadcast thread
    ↓
Step 8: Add cleanup and signal handling
    ↓
p1gxS.c (Complete Server)
```

---

## Step 1: Protocol Integration

### Goal
Replace hardcoded messages with protocol.h functions.

### Code Changes

**Add at top of file:**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "protocol.h"  // ADD THIS

#define PORT 8080
```

**Replace message handling:**

**OLD CODE (lines 48-56):**
```c
int valread = read(new_socket, buffer, sizeof(buffer));
if (valread < 0) {
    perror("read");
} else {
    printf("Received from client: %s\n", buffer);
}
send(new_socket, message, strlen(message), 0);
printf("Response sent to client.\n");
```

**NEW CODE:**
```c
// Read message from client
char buffer[BUFFER_SIZE] = {0};
int valread = read(new_socket, buffer, BUFFER_SIZE - 1);
if (valread < 0) {
    perror("read");
    close(new_socket);
    close(server_fd);
    exit(EXIT_FAILURE);
}
buffer[valread] = '\0';

// Parse the message using protocol
message_t msg;
if (parse_message(buffer, &msg) == 0) {
    printf("Received [%s] from client: %s\n", msg.type, msg.content);
} else {
    printf("Failed to parse message: %s\n", buffer);
}

// Send response using protocol
char response[BUFFER_SIZE];
format_notification(response, "Server received your message");
send(new_socket, response, strlen(response), 0);
printf("Response sent to client.\n");
```

### Test
```bash
gcc server.c -o server
./server
```

Expected: Server compiles and runs as before, but now uses protocol functions.

**Checkpoint:** ✅ Protocol integration working

---

## Step 2: Continuous Accept Loop

### Goal
Make server stay alive and accept multiple clients sequentially (still not parallel yet).

### Code Changes

**REPLACE the main function structure:**

**OLD CODE (lines 39-61):**
```c
printf("Server is listening on port %d...\n", PORT);
// 4. Accept an incoming connection
if ((new_socket = accept(...)) < 0) {
    perror("accept");
    ...
}
// ... handle one client ...
close(new_socket);
close(server_fd);
return 0;
```

**NEW CODE:**
```c
#define PORT 5000  // Changed from 8080

printf("Server is listening on port %d...\n", PORT);

// Main server loop - accept multiple clients
while (1) {
    printf("Waiting for client connection...\n");

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t*)&addrlen)) < 0) {
        perror("accept failed");
        continue;  // Continue to next iteration instead of exiting
    }

    printf("Client connected!\n");

    // Handle client communication
    handle_client(new_socket);

    // Close this client's socket
    close(new_socket);
    printf("Client disconnected.\n");
}

close(server_fd);
return 0;
```

**Add handle_client function (before main):**
```c
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};

    // Read message from client
    int valread = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (valread < 0) {
        perror("read");
        return;
    }
    buffer[valread] = '\0';

    // Parse the message using protocol
    message_t msg;
    if (parse_message(buffer, &msg) == 0) {
        printf("Received [%s]: %s\n", msg.type, msg.content);
    } else {
        printf("Failed to parse message\n");
    }

    // Send response
    char response[BUFFER_SIZE];
    format_notification(response, "Message received");
    send(client_socket, response, strlen(response), 0);
}
```

### Test
```bash
gcc server.c -o server
./server
# In another terminal, run client multiple times
./client
./client
./client
```

Expected: Server handles each client one at a time, returns to waiting after each disconnects.

**Checkpoint:** ✅ Server stays alive and handles sequential clients

---

## Step 3: Add Multi-Threading (Critical Step!)

### Goal
Handle multiple clients **simultaneously** using pthread.

### Code Changes

**Add pthread header:**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>  // ADD THIS
#include "protocol.h"

#define PORT 5000
```

**Modify handle_client to be thread function:**

**OLD:**
```c
void handle_client(int client_socket) {
    // ...
}
```

**NEW:**
```c
void *handle_client(void *arg) {
    int client_socket = *(int*)arg;
    free(arg);  // Free the allocated memory for socket fd

    char buffer[BUFFER_SIZE] = {0};

    printf("[Thread %ld] Client connected\n", pthread_self());

    // Read message from client
    int valread = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (valread < 0) {
        perror("read");
        close(client_socket);
        return NULL;
    }
    buffer[valread] = '\0';

    // Parse the message using protocol
    message_t msg;
    if (parse_message(buffer, &msg) == 0) {
        printf("[Thread %ld] Received [%s]: %s\n",
               pthread_self(), msg.type, msg.content);
    }

    // Send response
    char response[BUFFER_SIZE];
    format_notification(response, "Message received");
    send(client_socket, response, strlen(response), 0);

    close(client_socket);
    printf("[Thread %ld] Client disconnected\n", pthread_self());

    return NULL;
}
```

**Modify main loop to create threads:**

**OLD:**
```c
while (1) {
    printf("Waiting for client connection...\n");

    if ((new_socket = accept(...)) < 0) {
        perror("accept failed");
        continue;
    }

    printf("Client connected!\n");
    handle_client(new_socket);
    close(new_socket);
}
```

**NEW:**
```c
while (1) {
    printf("Waiting for client connection...\n");

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (new_socket < 0) {
        perror("accept failed");
        continue;
    }

    printf("Client connected!\n");

    // Allocate memory for socket fd to pass to thread
    int *client_sock = malloc(sizeof(int));
    *client_sock = new_socket;

    // Create thread to handle client
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, (void*)client_sock) != 0) {
        perror("pthread_create failed");
        close(new_socket);
        free(client_sock);
        continue;
    }

    // Detach thread so it cleans up automatically
    pthread_detach(thread_id);
}
```

### Compile with pthread
```bash
gcc server.c -o server -pthread
./server
```

### Test
```bash
# Terminal 1: Start server
./server

# Terminal 2: Start first client
./client

# Terminal 3: Start second client (while first is still connected)
./client

# Terminal 4: Start third client
./client
```

Expected: All clients can connect simultaneously!

**Checkpoint:** ✅ Multi-threading works - multiple clients can connect at once

---

## Step 4: Add Client Tracking Structure

### Goal
Track all connected clients (socket + username) for later broadcasting.

### Code Changes

**Add global structures (after includes, before functions):**
```c
#include <pthread.h>
#include "protocol.h"

#define PORT 5000
#define MAX_CLIENTS 50

// Global client list
client_info_t clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
```

**Add helper functions:**
```c
// Add client to the list
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

    printf("Client '%s' added. Total clients: %d\n", username, client_count);

    pthread_mutex_unlock(&clients_mutex);
    return 0;
}

// Remove client from the list
void remove_client(int socket_fd) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket_fd == socket_fd) {
            printf("Removing client '%s'\n", clients[i].username);

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

// Check if username already exists
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
```

### Test
```bash
gcc server.c -o server -pthread
./server
```

Expected: Compiles successfully. Client tracking structure ready.

**Checkpoint:** ✅ Client tracking data structure implemented

---

## Step 5: Add Authentication System

### Goal
Implement username authentication as first message exchange.

### Code Changes

**Modify handle_client function:**

```c
void *handle_client(void *arg) {
    int client_socket = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE] = {0};
    char username[MAX_USERNAME] = {0};

    printf("[Thread %ld] New client connected, waiting for authentication\n",
           pthread_self());

    // ========================================
    // STEP 1: AUTHENTICATION
    // ========================================

    // Read authentication message
    int valread = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (valread <= 0) {
        printf("[Thread %ld] Failed to read auth message\n", pthread_self());
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
        return NULL;
    }

    // Validate username
    if (!validate_username(auth_msg.sender)) {
        char response[BUFFER_SIZE];
        strcpy(response, AUTH_FAILED_INVALID);
        strcat(response, "\n");
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        return NULL;
    }

    // Check if username already taken
    if (username_exists(auth_msg.sender)) {
        char response[BUFFER_SIZE];
        strcpy(response, AUTH_FAILED);
        strcat(response, "\n");
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        return NULL;
    }

    // Authentication successful
    strncpy(username, auth_msg.sender, MAX_USERNAME - 1);

    // Add client to tracking list
    if (add_client(client_socket, username) != 0) {
        char response[BUFFER_SIZE];
        strcpy(response, SERVER_FULL);
        strcat(response, "\n");
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        return NULL;
    }

    // Send AUTH_OK
    char response[BUFFER_SIZE];
    strcpy(response, AUTH_OK);
    strcat(response, "\n");
    send(client_socket, response, strlen(response), 0);

    printf("[Thread %ld] User '%s' authenticated successfully\n",
           pthread_self(), username);

    // ========================================
    // STEP 2: MESSAGE LOOP (for now, just echo)
    // ========================================

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        valread = read(client_socket, buffer, BUFFER_SIZE - 1);

        if (valread <= 0) {
            // Client disconnected
            break;
        }

        buffer[valread] = '\0';
        printf("[%s] %s", username, buffer);

        // Echo back for now
        send(client_socket, buffer, strlen(buffer), 0);
    }

    // Cleanup
    remove_client(client_socket);
    close(client_socket);
    printf("[Thread %ld] User '%s' disconnected\n", pthread_self(), username);

    return NULL;
}
```

### Test
Update your client.c to send authentication first (or create test client):

```c
// In client, before sending regular message:
char auth_msg[256];
sprintf(auth_msg, "AUTH:testuser\n");
send(sock, auth_msg, strlen(auth_msg), 0);

// Read response
char buffer[1024];
read(sock, buffer, sizeof(buffer));
printf("Auth response: %s\n", buffer);
```

**Checkpoint:** ✅ Authentication system working

---

## Step 6: Add Thread-Safe Message Queue

### Goal
Create a queue where client threads can put messages, and a broadcast thread can retrieve them.

### Code Changes

**Add global message queue:**
```c
// After client tracking globals
message_queue_t msg_queue;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
```

**Initialize in main(), before server loop:**
```c
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Initialize message queue
    init_message_queue(&msg_queue);

    // Create socket...
    // ... rest of code
}
```

**Modify handle_client to enqueue messages:**

Replace the message loop section:
```c
// ========================================
// STEP 2: MESSAGE LOOP
// ========================================

while (1) {
    memset(buffer, 0, BUFFER_SIZE);
    valread = read(client_socket, buffer, BUFFER_SIZE - 1);

    if (valread <= 0) {
        // Client disconnected
        break;
    }

    buffer[valread] = '\0';

    // Parse message
    message_t msg;
    if (parse_message(buffer, &msg) == 0) {
        printf("[%s] Received: %s\n", username, msg.content);

        // Add to message queue for broadcasting
        pthread_mutex_lock(&queue_mutex);
        if (enqueue_message(&msg_queue, &msg) == 0) {
            pthread_cond_signal(&queue_cond);  // Wake up broadcast thread
        } else {
            printf("Message queue full!\n");
        }
        pthread_mutex_unlock(&queue_mutex);
    }
}
```

**Checkpoint:** ✅ Messages are queued (not yet broadcast)

---

## Step 7: Add Broadcast Thread

### Goal
Create a dedicated thread that reads from the queue and broadcasts to all clients.

### Code Changes

**Add broadcast thread function:**
```c
void *broadcast_thread(void *arg) {
    (void)arg;  // Unused

    printf("[Broadcast Thread] Started\n");

    while (1) {
        message_t msg;

        pthread_mutex_lock(&queue_mutex);

        // Wait for messages in queue
        while (is_queue_empty(&msg_queue)) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
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
                send(clients[i].socket_fd, broadcast, strlen(broadcast), 0);
            }
            pthread_mutex_unlock(&clients_mutex);
        } else {
            pthread_mutex_unlock(&queue_mutex);
        }
    }

    return NULL;
}
```

**Start broadcast thread in main():**
```c
int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Initialize message queue
    init_message_queue(&msg_queue);

    // Create broadcast thread
    pthread_t broadcast_tid;
    if (pthread_create(&broadcast_tid, NULL, broadcast_thread, NULL) != 0) {
        perror("Failed to create broadcast thread");
        exit(EXIT_FAILURE);
    }
    pthread_detach(broadcast_tid);

    printf("Broadcast thread started\n");

    // Create socket and start server...
    // ... rest of code
}
```

### Test
```bash
gcc server.c -o server -pthread
./server

# In multiple terminals, send AUTH then MSG messages
# All clients should receive broadcasts!
```

**Checkpoint:** ✅ Broadcasting works! All clients receive all messages.

---

## Step 8: Add Cleanup and Signal Handling

### Goal
Handle Ctrl+C gracefully and clean up all resources.

### Code Changes

**Add volatile flag:**
```c
#include <signal.h>

// Global flag
volatile int server_running = 1;
```

**Add signal handler:**
```c
void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n[Server] Shutting down...\n");
        server_running = 0;
    }
}
```

**Modify main to handle shutdown:**
```c
int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Register signal handler
    signal(SIGINT, signal_handler);

    // Initialize message queue
    init_message_queue(&msg_queue);

    // Create broadcast thread
    pthread_t broadcast_tid;
    pthread_create(&broadcast_tid, NULL, broadcast_thread, NULL);
    pthread_detach(broadcast_tid);

    // Socket creation and binding...

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Main accept loop
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (!server_running) break;  // Check if shutdown signal received

        if (new_socket < 0) {
            if (server_running) {
                perror("accept failed");
            }
            continue;
        }

        printf("New client connected\n");

        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client_sock) != 0) {
            perror("pthread_create failed");
            close(new_socket);
            free(client_sock);
            continue;
        }

        pthread_detach(thread_id);
    }

    // Cleanup
    printf("[Server] Closing all connections...\n");

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        close(clients[i].socket_fd);
    }
    pthread_mutex_unlock(&clients_mutex);

    close(server_fd);

    // Destroy mutexes
    pthread_mutex_destroy(&clients_mutex);
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);

    printf("[Server] Shutdown complete\n");

    return 0;
}
```

**Checkpoint:** ✅ Server shuts down gracefully with Ctrl+C

---

## Final Code Structure

Your complete `p1gxS.c` should have:

```c
// Headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include "protocol.h"

// Defines
#define PORT 5000
#define MAX_CLIENTS 50

// Global Variables
client_info_t clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

message_queue_t msg_queue;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

volatile int server_running = 1;

// Function Declarations
void signal_handler(int sig);
int add_client(int socket_fd, const char *username);
void remove_client(int socket_fd);
int username_exists(const char *username);
void *handle_client(void *arg);
void *broadcast_thread(void *arg);

// Function Implementations
// ... (all the functions from steps above)

// Main Function
int main() {
    // ... (implementation from steps above)
}
```

---

## Compilation

```bash
gcc -Wall -Wextra -pthread p1gxS.c -o server

# Or with Makefile:
# make server
```

---

## Testing Checklist

### Basic Functionality
- [ ] Server starts and listens on port 5000
- [ ] Single client can connect and authenticate
- [ ] Multiple clients can connect simultaneously
- [ ] Messages from one client broadcast to all others
- [ ] Duplicate usernames are rejected
- [ ] Invalid usernames are rejected

### Edge Cases
- [ ] Client disconnects gracefully
- [ ] Client disconnects abruptly (Ctrl+C)
- [ ] Server handles full client list (50 clients)
- [ ] Message queue handles high volume
- [ ] Server shuts down cleanly with Ctrl+C

### Memory & Thread Safety
- [ ] No memory leaks (test with valgrind)
- [ ] No race conditions
- [ ] Mutexes properly locked/unlocked
- [ ] Threads cleaned up properly

---

## Common Issues & Solutions

### Issue 1: "Address already in use"
**Solution:** Add socket option before bind:
```c
int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

### Issue 2: Messages not broadcasting
**Solution:** Check that:
- Messages are being enqueued (add printf in handle_client)
- Broadcast thread is signaled (pthread_cond_signal)
- Client list is not empty

### Issue 3: Server crashes on client disconnect
**Solution:** Check return value of read/recv:
```c
int valread = read(client_socket, buffer, BUFFER_SIZE);
if (valread <= 0) {
    // Handle disconnect
    break;
}
```

### Issue 4: Deadlock
**Solution:**
- Never hold two mutexes at once
- Always unlock mutexes in reverse order of locking
- Use pthread_mutex_trylock for debugging

---

## Next Steps

Once server is complete:
1. Test with basic client.c
2. Create mock test clients
3. Run stress tests (10+ clients)
4. Check with valgrind for memory leaks
5. Document your code
6. Proceed to integration testing with full client

---

## Summary

You've successfully evolved from a basic single-client echo server to a complete multi-threaded chat server with:

✅ Multi-threading (pthread)
✅ Client authentication
✅ Message broadcasting
✅ Thread-safe data structures
✅ Proper resource management
✅ Signal handling

**Congratulations!** Your server is now ready for integration with the client implementation.
