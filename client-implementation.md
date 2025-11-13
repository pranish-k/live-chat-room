# Client Implementation Guide - Evolution from client.c to p1gxC.c

## Current State Analysis

**File:** `client.c` (Basic TCP Client)

**What You Already Have:** ✅
- Socket creation: `socket(AF_INET, SOCK_STREAM, 0)`
- Server address setup: `inet_pton()` for IP conversion
- Connection to server: `connect()`
- Send message: `send()`
- Receive response: `read()`
- Error handling patterns
- Clean socket closure

**What's Missing:** ❌
- Username input from user
- Authentication handshake
- Continuous chat loop (client exits after 1 message)
- Multi-threaded I/O (can't send and receive simultaneously)
- Message display formatting
- Graceful disconnect handling
- Protocol integration

---

## Evolution Strategy

We'll transform `client.c` into `p1gxC.c` through **7 incremental steps**. Each step builds on the previous one and can be tested independently.

### Step-by-Step Roadmap

```
client.c (Current)
    ↓
Step 1: Add protocol.h integration
    ↓
Step 2: Change port to 5000, add username input UI
    ↓
Step 3: Add authentication handshake
    ↓
Step 4: Add continuous message sending loop
    ↓
Step 5: Add receive thread (multi-threading)
    ↓
Step 6: Add message formatting and display
    ↓
Step 7: Add graceful disconnect and signal handling
    ↓
p1gxC.c (Complete Client)
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
#include <arpa/inet.h>
#include <netinet/in.h>
#include "protocol.h"  // ADD THIS

#define PORT 8080
```

**Replace message handling:**

**OLD CODE (lines 38-47):**
```c
send(sock, hello, strlen(hello), 0);
printf("Message sent from client.\n");

int valread = read(sock, buffer, sizeof(buffer));
if (valread < 0) {
    perror("read");
} else {
    printf("Server: %s\n", buffer);
}
```

**NEW CODE:**
```c
// Send message using protocol
char message[BUFFER_SIZE];
format_chat_message(message, "testuser", "Hello from client");
send(sock, message, strlen(message), 0);
printf("Message sent from client.\n");

// Receive response using protocol
char buffer[BUFFER_SIZE] = {0};
int valread = read(sock, buffer, BUFFER_SIZE - 1);
if (valread < 0) {
    perror("read");
} else {
    buffer[valread] = '\0';

    // Parse response
    message_t msg;
    if (parse_message(buffer, &msg) == 0) {
        printf("Server [%s]: %s\n", msg.type, msg.content);
    } else {
        printf("Server: %s\n", buffer);
    }
}
```

### Test
```bash
gcc client.c -o client
./client  # With server running
```

Expected: Client compiles and communicates using protocol format.

**Checkpoint:** ✅ Protocol integration working

---

## Step 2: Username Input UI

### Goal
Prompt user for username at startup and change to port 5000.

### Code Changes

**Update port:**
```c
#define PORT 5000  // Changed from 8080
```

**Add username input before connection:**

**ADD THIS CODE after socket creation, before connect():**

```c
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char username[MAX_USERNAME] = {0};

    // ========================================
    // STEP 1: GET USERNAME FROM USER
    // ========================================

    printf("=================================\n");
    printf("   Live Chat Room Client\n");
    printf("=================================\n");
    printf("Enter your username: ");
    fflush(stdout);

    if (fgets(username, MAX_USERNAME, stdin) == NULL) {
        fprintf(stderr, "Failed to read username\n");
        return -1;
    }

    // Remove trailing newline
    size_t len = strlen(username);
    if (len > 0 && username[len - 1] == '\n') {
        username[len - 1] = '\0';
    }

    // Validate username
    if (!validate_username(username)) {
        fprintf(stderr, "Invalid username! Use only letters, numbers, and underscores.\n");
        fprintf(stderr, "Username must be 1-%d characters long.\n", MAX_USERNAME - 1);
        return -1;
    }

    printf("Username: %s\n", username);

    // ========================================
    // STEP 2: CREATE SOCKET
    // ========================================

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    // ... rest of code (address setup, connect, etc.)
}
```

### Test
```bash
gcc client.c -o client
./client
```

Expected: Client prompts for username and validates it.

**Checkpoint:** ✅ Username input UI working

---

## Step 3: Add Authentication Handshake

### Goal
Send authentication message as first exchange with server.

### Code Changes

**Replace the message sending section with authentication:**

**ADD AFTER connect() succeeds:**

```c
// ========================================
// STEP 3: CONNECT TO SERVER
// ========================================

if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("Connection Failed");
    close(sock);
    return -1;
}

printf("Connected to server at 127.0.0.1:%d\n", PORT);

// ========================================
// STEP 4: AUTHENTICATE
// ========================================

// Send authentication request
char auth_msg[BUFFER_SIZE];
format_auth_message(auth_msg, username);

printf("Authenticating as '%s'...\n", username);
if (send(sock, auth_msg, strlen(auth_msg), 0) < 0) {
    perror("Failed to send authentication");
    close(sock);
    return -1;
}

// Wait for authentication response
char auth_response[BUFFER_SIZE] = {0};
int valread = read(sock, auth_response, BUFFER_SIZE - 1);
if (valread <= 0) {
    fprintf(stderr, "Failed to receive authentication response\n");
    close(sock);
    return -1;
}
auth_response[valread] = '\0';

// Remove trailing newline
len = strlen(auth_response);
if (len > 0 && auth_response[len - 1] == '\n') {
    auth_response[len - 1] = '\0';
}

// Check authentication result
if (strcmp(auth_response, AUTH_OK) == 0) {
    printf("✓ Authentication successful!\n");
    printf("You can now start chatting. Type your messages below.\n");
    printf("Press Ctrl+D or type 'quit' to exit.\n");
    printf("=================================\n\n");
} else {
    fprintf(stderr, "✗ Authentication failed: %s\n", auth_response);
    close(sock);
    return -1;
}
```

### Test

First, create a simple mock server that responds to AUTH:

```c
// mock_auth_server.c (for testing)
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(5000), .sin_addr.s_addr = INADDR_ANY};
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("Mock server listening on 5000...\n");
    int client = accept(server_fd, NULL, NULL);

    char buffer[1024];
    read(client, buffer, 1024);
    printf("Received: %s", buffer);

    send(client, "AUTH_OK\n", 8, 0);

    while(1) {
        int n = read(client, buffer, 1024);
        if (n <= 0) break;
        printf("Got: %s", buffer);
    }

    close(client);
    close(server_fd);
}
```

**Checkpoint:** ✅ Authentication handshake working

---

## Step 4: Add Continuous Message Sending Loop

### Goal
Allow user to send multiple messages without exiting.

### Code Changes

**Replace single message send with loop:**

**ADD AFTER authentication success:**

```c
// ========================================
// STEP 5: MESSAGE SENDING LOOP
// ========================================

char input[MAX_MESSAGE];
char formatted_msg[BUFFER_SIZE];

while (1) {
    printf("> ");
    fflush(stdout);

    // Read user input
    if (fgets(input, MAX_MESSAGE, stdin) == NULL) {
        // EOF (Ctrl+D) pressed
        printf("\nDisconnecting...\n");
        break;
    }

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
        printf("Disconnecting...\n");
        break;
    }

    // Validate message
    if (!validate_message_content(input)) {
        fprintf(stderr, "Message too long! Maximum %d characters.\n", MAX_MESSAGE - 1);
        continue;
    }

    // Format and send message
    format_chat_message(formatted_msg, username, input);
    if (send(sock, formatted_msg, strlen(formatted_msg), 0) < 0) {
        perror("Send failed");
        break;
    }
}

// Cleanup
close(sock);
printf("Disconnected from server.\n");
return 0;
```

### Test
```bash
gcc client.c -o client
./client
# Type multiple messages
# Type 'quit' or Ctrl+D to exit
```

**Checkpoint:** ✅ Continuous messaging working (but can't receive yet)

---

## Step 5: Add Receive Thread (Multi-threading)

### Goal
Create separate thread to receive messages while user types.

### Code Changes

**Add pthread header:**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>  // ADD THIS
#include "protocol.h"

#define PORT 5000
```

**Add global socket variable (before main):**
```c
// Global socket for thread access
int global_sock = 0;
volatile int keep_running = 1;
```

**Add receive thread function (before main):**
```c
void *receive_thread(void *arg) {
    (void)arg;  // Unused

    char buffer[BUFFER_SIZE];

    while (keep_running) {
        memset(buffer, 0, BUFFER_SIZE);

        int valread = read(global_sock, buffer, BUFFER_SIZE - 1);

        if (valread <= 0) {
            if (keep_running) {
                printf("\n[Disconnected from server]\n");
                keep_running = 0;
            }
            break;
        }

        buffer[valread] = '\0';

        // Parse message
        message_t msg;
        if (parse_message(buffer, &msg) == 0) {
            // Print message on new line
            printf("\r");  // Clear current line

            if (strcmp(msg.type, MSG_TYPE_MESSAGE) == 0) {
                printf("[%s]: %s\n", msg.sender, msg.content);
            } else if (strcmp(msg.type, MSG_TYPE_NOTIFY) == 0) {
                printf("[*] %s\n", msg.content);
            } else if (strcmp(msg.type, MSG_TYPE_ERROR) == 0) {
                printf("[ERROR] %s\n", msg.content);
            }

            printf("> ");  // Re-display prompt
            fflush(stdout);
        } else {
            // Couldn't parse, just display raw
            printf("\n%s", buffer);
            printf("> ");
            fflush(stdout);
        }
    }

    return NULL;
}
```

**Modify main to create receive thread:**

**REPLACE the message loop section:**

```c
// ========================================
// STEP 5: START RECEIVE THREAD
// ========================================

global_sock = sock;  // Set global socket

pthread_t recv_tid;
if (pthread_create(&recv_tid, NULL, receive_thread, NULL) != 0) {
    perror("Failed to create receive thread");
    close(sock);
    return -1;
}

printf("Receive thread started\n\n");

// ========================================
// STEP 6: MESSAGE SENDING LOOP (Main Thread)
// ========================================

char input[MAX_MESSAGE];
char formatted_msg[BUFFER_SIZE];

while (keep_running) {
    printf("> ");
    fflush(stdout);

    // Read user input
    if (fgets(input, MAX_MESSAGE, stdin) == NULL) {
        // EOF (Ctrl+D) pressed
        printf("\nDisconnecting...\n");
        break;
    }

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
        break;
    }

    // Validate message
    if (!validate_message_content(input)) {
        fprintf(stderr, "Message too long! Maximum %d characters.\n", MAX_MESSAGE - 1);
        continue;
    }

    // Format and send message
    format_chat_message(formatted_msg, username, input);
    if (send(sock, formatted_msg, strlen(formatted_msg), 0) < 0) {
        perror("Send failed");
        break;
    }
}

// ========================================
// STEP 7: CLEANUP
// ========================================

keep_running = 0;

// Cancel receive thread
pthread_cancel(recv_tid);
pthread_join(recv_tid, NULL);

close(sock);
printf("Disconnected from server.\n");

return 0;
```

### Compile with pthread
```bash
gcc client.c -o client -pthread
./client
```

### Test
```bash
# Terminal 1: Start server
./server

# Terminal 2: Start first client
./client
# Enter username: alice

# Terminal 3: Start second client
./client
# Enter username: bob

# Now type messages in both clients
# You should see messages appear in both windows!
```

**Checkpoint:** ✅ Multi-threaded I/O working - can send and receive simultaneously

---

## Step 6: Enhanced Message Formatting and Display

### Goal
Improve message display with better formatting and user experience.

### Code Changes

**Add helper function for display (before main):**

```c
// ANSI color codes (optional, for better UI)
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RED     "\033[31m"
#define COLOR_CYAN    "\033[36m"

void display_welcome_banner(const char *username) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║     Live Chat Room - Connected!       ║\n");
    printf("╟────────────────────────────────────────╢\n");
    printf("║  Username: %-27s ║\n", username);
    printf("║  Server:   127.0.0.1:5000              ║\n");
    printf("╟────────────────────────────────────────╢\n");
    printf("║  Commands:                             ║\n");
    printf("║   - Type messages to chat              ║\n");
    printf("║   - 'quit' or Ctrl+D to exit           ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
}
```

**Update authentication success message:**

**REPLACE:**
```c
if (strcmp(auth_response, AUTH_OK) == 0) {
    printf("✓ Authentication successful!\n");
    printf("You can now start chatting. Type your messages below.\n");
    printf("Press Ctrl+D or type 'quit' to exit.\n");
    printf("=================================\n\n");
}
```

**WITH:**
```c
if (strcmp(auth_response, AUTH_OK) == 0) {
    display_welcome_banner(username);
}
```

**Update receive thread for better formatting:**

```c
void *receive_thread(void *arg) {
    (void)arg;

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
                printf("%s[%s]%s %s\n", COLOR_CYAN, msg.sender, COLOR_RESET, msg.content);
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
        }
    }

    return NULL;
}
```

**Update input prompt color:**

```c
while (keep_running) {
    printf("%s> %s", COLOR_GREEN, COLOR_RESET);
    fflush(stdout);

    // ... rest of loop
}
```

### Test
```bash
gcc client.c -o client -pthread
./client
```

Expected: Nice formatted display with colors and better UI.

**Checkpoint:** ✅ Enhanced UI and message formatting

---

## Step 7: Graceful Disconnect and Signal Handling

### Goal
Handle Ctrl+C and clean disconnection properly.

### Code Changes

**Add signal handler:**
```c
#include <signal.h>

// Add after includes

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n[Client] Caught interrupt signal, disconnecting...\n");
        keep_running = 0;
    }
}
```

**Add disconnect message to server (optional):**

```c
void send_disconnect_message(const char *username) {
    char disconnect_msg[BUFFER_SIZE];
    format_disconnect_message(disconnect_msg, username);
    send(global_sock, disconnect_msg, strlen(disconnect_msg), 0);
}
```

**Update main with signal handler and cleanup:**

```c
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char username[MAX_USERNAME] = {0};

    // Register signal handler for Ctrl+C
    signal(SIGINT, signal_handler);

    // ... (username input, socket creation, connection, auth)

    // After authentication success:
    global_sock = sock;

    pthread_t recv_tid;
    if (pthread_create(&recv_tid, NULL, receive_thread, NULL) != 0) {
        perror("Failed to create receive thread");
        close(sock);
        return -1;
    }

    // Message loop
    char input[MAX_MESSAGE];
    char formatted_msg[BUFFER_SIZE];

    while (keep_running) {
        printf("%s> %s", COLOR_GREEN, COLOR_RESET);
        fflush(stdout);

        if (fgets(input, MAX_MESSAGE, stdin) == NULL) {
            // EOF or error
            if (keep_running) {
                printf("\nDisconnecting...\n");
            }
            break;
        }

        if (!keep_running) break;  // Check flag again

        size_t input_len = strlen(input);
        if (input_len > 0 && input[input_len - 1] == '\n') {
            input[input_len - 1] = '\0';
            input_len--;
        }

        if (input_len == 0) continue;

        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            break;
        }

        if (!validate_message_content(input)) {
            fprintf(stderr, "%sMessage too long! Maximum %d characters.%s\n",
                    COLOR_RED, MAX_MESSAGE - 1, COLOR_RESET);
            continue;
        }

        format_chat_message(formatted_msg, username, input);
        if (send(sock, formatted_msg, strlen(formatted_msg), 0) < 0) {
            if (keep_running) {
                perror("Send failed");
            }
            break;
        }
    }

    // ========================================
    // CLEANUP
    // ========================================

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

    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║       Disconnected from server         ║\n");
    printf("║           Goodbye, %-19s║\n", username);
    printf("╚════════════════════════════════════════╝\n");

    return 0;
}
```

### Test
```bash
gcc client.c -o client -pthread
./client

# Test various exit methods:
# 1. Type 'quit'
# 2. Press Ctrl+D
# 3. Press Ctrl+C
# All should exit gracefully
```

**Checkpoint:** ✅ Graceful disconnect working

---

## Final Code Structure

Your complete `p1gxC.c` should have:

```c
// Headers
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

// Defines
#define PORT 5000

// ANSI Colors (optional)
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RED     "\033[31m"
#define COLOR_CYAN    "\033[36m"

// Global Variables
int global_sock = 0;
volatile int keep_running = 1;

// Function Declarations
void signal_handler(int sig);
void display_welcome_banner(const char *username);
void send_disconnect_message(const char *username);
void *receive_thread(void *arg);

// Function Implementations
// ... (all functions from steps above)

// Main Function
int main() {
    // ... (implementation from steps above)
}
```

---

## Compilation

```bash
gcc -Wall -Wextra -pthread p1gxC.c -o client

# Or with Makefile:
# make client
```

---

## Testing Checklist

### Basic Functionality
- [ ] Client prompts for username
- [ ] Invalid usernames are rejected locally
- [ ] Client connects to server successfully
- [ ] Authentication works (both success and failure cases)
- [ ] Can send messages continuously
- [ ] Can receive messages from other clients
- [ ] Messages display with proper formatting

### User Experience
- [ ] Welcome banner displays correctly
- [ ] Incoming messages don't interfere with typing
- [ ] Input prompt re-displays after received messages
- [ ] Colors display correctly (if supported)
- [ ] Clear error messages for failures

### Exit Handling
- [ ] 'quit' command exits gracefully
- [ ] Ctrl+D exits gracefully
- [ ] Ctrl+C exits gracefully
- [ ] Disconnect message sent to server
- [ ] Socket closed properly

### Edge Cases
- [ ] Empty messages are ignored
- [ ] Very long messages are rejected
- [ ] Server disconnect handled gracefully
- [ ] Network errors handled without crash

### Memory & Thread Safety
- [ ] No memory leaks (test with valgrind)
- [ ] Threads cleaned up properly
- [ ] No zombie threads

---

## Common Issues & Solutions

### Issue 1: Messages appearing while typing

**Solution:** Use `\r\033[K` to clear line before printing:
```c
printf("\r\033[K");  // Clear current line
printf("[%s]: %s\n", sender, content);
printf("> ");  // Re-display prompt
fflush(stdout);
```

### Issue 2: Client hangs on exit

**Solution:** Make sure to:
- Set `keep_running = 0`
- Use `pthread_cancel()` on receive thread
- Call `pthread_join()` to wait for thread

### Issue 3: "Broken pipe" error when sending

**Solution:** Check server is still connected:
```c
if (send(sock, msg, len, 0) < 0) {
    if (errno == EPIPE) {
        printf("Server disconnected\n");
    }
    break;
}
```

### Issue 4: Colors not showing

**Solution:**
- Some terminals don't support ANSI colors
- You can add a `--no-color` flag option
- Or just remove the color defines

### Issue 5: Input and output mixed up

**Solution:** This is expected in multi-threaded terminal I/O. The `\r\033[K` sequence helps, but some mixing is unavoidable. Advanced solution: use ncurses library for better terminal control.

---

## Optional Enhancements

### Enhancement 1: Command History (Arrow Keys)
Use `readline` library for command history:
```c
#include <readline/readline.h>
#include <readline/history.h>

// Instead of fgets:
char *line = readline("> ");
if (line && *line) {
    add_history(line);
    // Use line...
    free(line);
}
```

Compile with: `gcc client.c -o client -pthread -lreadline`

### Enhancement 2: Typing Indicator
Send special message when user is typing (not required for project).

### Enhancement 3: Private Messages
Add syntax like `@username message` for private messages.

### Enhancement 4: Message Timestamps
Display timestamp with each message:
```c
time_t now = time(NULL);
struct tm *t = localtime(&now);
printf("[%02d:%02d:%02d] [%s]: %s\n",
       t->tm_hour, t->tm_min, t->tm_sec, sender, content);
```

---

## Integration Testing with Server

Once both server and client are complete:

```bash
# Terminal 1: Start server
cd /path/to/project
gcc -pthread p1gxS.c -o server
./server

# Terminal 2: Start client 1
gcc -pthread p1gxC.c -o client
./client
Username: alice

# Terminal 3: Start client 2
./client
Username: bob

# Terminal 4: Start client 3
./client
Username: charlie

# Now chat between all clients!
```

### Expected Behavior:
1. All clients connect and authenticate
2. Messages from any client appear in all other clients
3. Clients can join/leave dynamically
4. Server handles disconnections gracefully
5. No crashes or hangs

---

## Mock Server for Independent Testing

If you want to test client before server is ready:

```c
// mock_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

void *handle_client(void *arg) {
    int sock = *(int*)arg;
    free(arg);

    char buffer[1024];

    // Receive AUTH
    read(sock, buffer, 1024);
    printf("Received: %s", buffer);

    // Send AUTH_OK
    send(sock, "AUTH_OK\n", 8, 0);

    // Echo loop
    while (1) {
        int n = read(sock, buffer, 1024);
        if (n <= 0) break;

        printf("Received: %s", buffer);
        send(sock, buffer, n, 0);  // Echo back
    }

    close(sock);
    return NULL;
}

int main() {
    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(5000),
        .sin_addr.s_addr = INADDR_ANY
    };

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    printf("Mock server listening on port 5000\n");

    while (1) {
        int *client = malloc(sizeof(int));
        *client = accept(server, NULL, NULL);

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client);
        pthread_detach(tid);
    }

    return 0;
}
```

Compile: `gcc mock_server.c -o mock_server -pthread`

---

## Summary

You've successfully evolved from a basic single-message client to a complete multi-threaded chat client with:

✅ Username input and validation
✅ Authentication handshake
✅ Multi-threaded I/O (send and receive simultaneously)
✅ Continuous chat capability
✅ Message formatting and display
✅ Graceful disconnect handling
✅ Signal handling (Ctrl+C)
✅ Enhanced user interface

**Congratulations!** Your client is now ready for integration with the server implementation.

---

## Next Steps

1. Test client with mock server thoroughly
2. Test client with real server implementation
3. Run stress tests (type rapidly, long messages, etc.)
4. Check with valgrind for memory leaks
5. Document your code
6. Take screenshots for documentation
7. Prepare for final submission

---

## Quick Reference: Client Lifecycle

```
1. Start client
2. Prompt for username
3. Validate username locally
4. Create socket
5. Connect to server
6. Send AUTH:username
7. Wait for AUTH_OK or AUTH_FAILED
8. If AUTH_OK:
   - Start receive thread
   - Enter message loop
   - Send MSG:username:content for each message
   - Display incoming messages from receive thread
9. On quit/EOF/Ctrl+C:
   - Set keep_running = 0
   - Optionally send DISCONNECT message
   - Cancel receive thread
   - Join thread
   - Close socket
   - Exit
```

Good luck with your implementation! 🚀
