# Project 1: Live Chat Room - Implementation Plan

**Total Points:** 100
**Team Size:** Maximum 2 students per group
**Language:** C
**Submission Format:** p1gxS.c, p1gxC.c, protocol.h, p1gxDoc (pdf/docx)

---

## Table of Contents
1. [Current Implementation Status](#current-implementation-status)
2. [Project Overview](#project-overview)
3. [Three-Phase Workflow](#three-phase-workflow)
4. [Developer 1: Server Implementation](#developer-1-server-implementation)
5. [Developer 2: Client Implementation](#developer-2-client-implementation)
6. [Protocol Design (protocol.h)](#protocol-design-protocolh)
7. [Documentation Requirements](#documentation-requirements)
8. [Testing Strategy](#testing-strategy)
9. [Submission Checklist](#submission-checklist)

---

## Current Implementation Status

### What We Have (Basic Foundation)

#### Server (server.c) - Basic TCP Server ✅
Current implementation provides:
- TCP socket creation (IPv4, SOCK_STREAM)
- Socket binding to port 8080
- Listening for connections (backlog of 5)
- Accepting single client connection
- Reading one message from client
- Sending one response back
- Proper socket cleanup

**Limitations:**
- ❌ Only handles ONE client at a time
- ❌ No multi-threading
- ❌ No authentication system
- ❌ No message broadcasting
- ❌ No continuous message loop
- ❌ Server exits after one client disconnects

#### Client (client.c) - Basic TCP Client ✅
Current implementation provides:
- TCP socket creation
- Server address configuration (127.0.0.1:8080)
- Connection to server
- Sending one message
- Receiving one response
- Proper socket cleanup

**Limitations:**
- ❌ No authentication
- ❌ No continuous chat capability
- ❌ No multi-threaded I/O (can't send and receive simultaneously)
- ❌ No user interface for username input
- ❌ Client exits after one message exchange

#### Protocol (protocol.h) - Communication Contract ✅
**COMPLETED:** Full protocol implementation with:
- Message type definitions (AUTH, MSG, NOTIFY, ERROR, DISCONNECT)
- Response codes (AUTH_OK, AUTH_FAILED, etc.)
- Message structures and parsing functions
- Helper functions for formatting messages
- Input validation (username, message content)
- Message queue structure for thread-safe operations
- Client info structure for server-side tracking

### Evolution Path

```
CURRENT STATE              →    TARGET STATE
─────────────────               ─────────────────
[server.c]                      [p1gxS.c]
- Single client                 - Multiple concurrent clients
- No threads                    - Multi-threaded (pthread)
- Simple echo                   - Message queue + broadcasting
                                - Authentication system
                                - Client management

[client.c]                      [p1gxC.c]
- One message                   - Continuous chat
- Blocking I/O                  - Non-blocking I/O (2 threads)
- No username                   - Username authentication
                                - Real-time message display
```

---

## Project Overview

### Objective
Evolve the basic client-server implementation into a complete live chat room system using socket programming with multi-threading that allows multiple users to communicate in real-time.

### Key Requirements
- **Server:** Multi-threaded, handles multiple clients, broadcasts messages
- **Client:** Connects to server, sends/receives messages, simple UI
- **Protocol:** Reliable communication between server and clients (✅ DONE)
- **Authentication:** Basic username-based system
- **Real-time:** Messages delivered instantly to all participants

### Points Distribution
| Component | Developer | Points |
|-----------|-----------|--------|
| Server Program | Dev 1 | 50 |
| Client Program | Dev 2 | 30 |
| Code Style (Comments, Formatting) | Both | 10 |
| Documentation | Both | 10 |
| **TOTAL** | | **100** |

### Foundation Already Built
✅ **Socket Programming Basics:**
- Both developers understand socket(), bind(), listen(), accept()
- Both understand connect(), send(), recv()
- Error handling patterns established
- Port configuration (moving from 8080 → 5000)

✅ **Protocol Definition:**
- Complete protocol.h with all message formats
- Helper functions ready to use
- Validation functions implemented

---

## Three-Phase Workflow

### PHASE 1: Protocol Definition ✅ COMPLETED

**The communication protocol has been fully defined in `protocol.h`**

#### What We Decided:

1. **Message Format** ✅
   - Text-based protocol with newline delimiters
   - Format: `TYPE:DATA\n`
   - Easy to debug and parse

2. **Authentication Format** ✅
   - Client request: `AUTH:username\n`
   - Server success: `AUTH_OK\n`
   - Server failure: `AUTH_FAILED:reason\n`

3. **System Messages** ✅
   - Notifications: `NOTIFY:message\n`
   - Errors: `ERROR:description\n`
   - Examples: `NOTIFY:alice joined the chat\n`

4. **Message Constraints** ✅
   - Maximum username: 32 characters
   - Maximum message: 256 characters
   - Character encoding: ASCII/UTF-8
   - Username rules: alphanumeric and underscore only

#### Implemented in `protocol.h`:

```c
✅ Constants: MAX_USERNAME (32), MAX_MESSAGE (256), SERVER_PORT (5000)
✅ Message types: AUTH, MSG, NOTIFY, ERROR, DISCONNECT
✅ Response codes: AUTH_OK, AUTH_FAILED, MSG_DELIVERED
✅ Data structures: message_t, client_info_t, message_queue_t
✅ Helper functions:
   - format_auth_message()
   - format_chat_message()
   - format_notification()
   - format_error_message()
   - parse_message()
   - validate_username()
   - validate_message_content()
✅ Queue operations:
   - init_message_queue()
   - enqueue_message()
   - dequeue_message()
```

**Status:** Protocol is ready to use! Both server and client can now include `protocol.h` and use these functions.

---

### PHASE 2: Independent Development (Parallel Work) 🔀

**Each developer evolves their basic implementation into the full system.**

**Starting Point:**
- **Dev 1 (Server):** Has basic server.c with socket setup, accept, send/recv
- **Dev 2 (Client):** Has basic client.c with socket connection, send/recv

**Key Insight:** You already understand sockets! Now add:
- Multi-threading (pthread)
- Protocol integration (use protocol.h)
- Message handling
- Authentication

**Testing Strategy:**
- **Dev 1 (Server):** Tests with mock client (~20 lines) OR use basic client.c
- **Dev 2 (Client):** Tests with mock server (~20 lines) OR use basic server.c

**Evolution Steps:**
- Start from working server.c/client.c
- Add features incrementally
- Test after each addition
- Use protocol.h functions for all communication

See detailed implementation guides:
- [server-implementation.md](server-implementation.md) - Step-by-step server evolution
- [client-implementation.md](client-implementation.md) - Step-by-step client evolution

**⏰ Time Estimate:** 3-5 days of independent work

---

### PHASE 3: Integration Testing (Final Day, Collaborative) 🔗

**Connect real server + real client together.**

Because both followed the protocol, they should work with minimal debugging.

**Integration Testing Checklist:**
- [ ] Multiple clients can connect simultaneously
- [ ] Authentication works for all clients
- [ ] Messages broadcast correctly to all users
- [ ] Clients can disconnect gracefully
- [ ] Server handles disconnections properly
- [ ] Error cases handled appropriately

**⏰ Time Estimate:** 4-6 hours of testing and debugging

---

## Developer 1: Server Implementation (50 pts)

**File:** `p1gxS.c`  
**Reference:** Project Spec Section 2.1-2.4

### Your Responsibility
Build a server that listens for connections, receives messages following the protocol, and broadcasts them to all connected clients.

### Tasks (In Order):

#### Task 1: Socket Setup ✅
**Goal:** Create a server socket that listens for incoming connections

```c
// Pseudo-code structure
int server_socket;
struct sockaddr_in server_addr;

// Create socket
// Bind to port (e.g., 5000)
// Listen for connections
```

**Test:** Can you successfully bind and listen?

**Reference:** Spec 2.1 - "Create a server application that listens for incoming connections"

---

#### Task 2: Client Connection Handler (Threading) ✅
**Goal:** Handle multiple clients simultaneously using threads

```c
// When client connects:
pthread_t thread_id;
int *client_socket = malloc(sizeof(int));
*client_socket = accept(server_socket, ...);
pthread_create(&thread_id, NULL, handle_client, client_socket);
```

**Key Points:**
- Each client gets its own thread
- Thread runs `handle_client()` function
- Use `pthread_create()`
- Detach threads with `pthread_detach()`

**Test:** Can multiple mock clients connect?

**Reference:** Spec 2.1 - "Using multi-threading, manage multiple client connections simultaneously"

---

#### Task 3: Receive Authentication ✅
**Goal:** Authenticate users by receiving and storing usernames

```c
// In each client thread:
// 1. Read first message (should be AUTH message per protocol)
// 2. Parse username
// 3. Store in client list
// 4. Send AUTH_OK or AUTH_FAILED
```

**Test:** Does username get stored correctly?

**Reference:** Spec 2.4 - "Implement a basic user authentication system"

---

#### Task 4: Message Queue (Thread-Safe) ✅
**Goal:** Create a thread-safe queue for messages from all clients

```c
// Shared data structure
typedef struct {
    message_t messages[100];  // Simple circular buffer
    int head, tail;
    pthread_mutex_t lock;
} message_queue_t;

// Functions needed:
// enqueue_message() - Add message (called by client threads)
// dequeue_message() - Get message (called by broadcast thread)
```

**Critical:** Use mutexes to protect shared data!

```c
pthread_mutex_lock(&queue->lock);
// Add/remove message
pthread_mutex_unlock(&queue->lock);
```

**Test:** Do messages get queued without corruption?

**Reference:** Spec 2.1 - "Implement a message queue to handle incoming messages"

---

#### Task 5: Receive Messages Loop ✅
**Goal:** Each client thread continuously reads and queues messages

```c
void *handle_client(void *arg) {
    int socket = *(int*)arg;
    
    // Authenticate first (Task 3)
    
    while (1) {
        // Read message from socket
        // Parse according to protocol
        // Add to message queue (Task 4)
        // Break on disconnect
    }
    
    // Cleanup
}
```

**Test:** Do messages from multiple clients get queued?

---

#### Task 6: Broadcast Thread ✅
**Goal:** Continuously read from queue and send to ALL clients

```c
void *broadcast_messages(void *arg) {
    while (server_running) {
        // Dequeue message
        // Send to all connected clients
        // Handle disconnected clients
    }
}
```

**Key Challenge:** Maintain list of connected clients safely

**Test:** Do all connected mock clients receive messages?

**Reference:** Spec 2.3 - "Ensure messages are delivered in real-time to all participants"

---

#### Task 7: Track Connected Clients ✅
**Goal:** Maintain a list of all active connections

```c
typedef struct {
    int socket;
    char username[MAX_USERNAME];
} client_info_t;

client_info_t clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_lock;
```

**Operations:**
- Add client on connect
- Remove client on disconnect
- Protect with mutex!

**Test:** Does list update correctly on connects/disconnects?

---

#### Task 8: Error Handling & Cleanup ✅
**Goal:** Handle errors gracefully and clean up resources

**What to handle:**
- Socket creation/bind/listen failures
- Client disconnections (detect via recv() return value)
- Thread creation failures
- Memory allocation failures

**Cleanup checklist:**
- Close all sockets
- Free all malloc'd memory
- Join/cancel all threads
- Destroy mutexes

---

### Testing Your Server (Independent, Phase 2)

**Build a mock client (~20 lines):**

```c
// mock_client.c
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    connect(sock, (struct sockaddr*)&server, sizeof(server));
    
    // Send authentication
    send(sock, "AUTH:testuser\n", 15, 0);
    
    char buffer[1024];
    recv(sock, buffer, 1024, 0);
    printf("Server response: %s\n", buffer);
    
    // Send a message
    send(sock, "MSG:testuser:hello world\n", 26, 0);
    
    // Receive broadcast
    recv(sock, buffer, 1024, 0);
    printf("Received: %s\n", buffer);
    
    close(sock);
    return 0;
}
```

**Compile:** `gcc mock_client.c -o mock_client`

**Test Plan:**
1. Run your server: `./p1gxS`
2. In separate terminals, run multiple mock clients
3. Verify all clients receive broadcasted messages
4. Test disconnections

---

### Deliverable Checklist for Dev 1:
- [ ] `p1gxS.c` with clean, commented code
- [ ] Handles multiple simultaneous clients
- [ ] Authentication working
- [ ] Message broadcasting working
- [ ] Thread-safe message queue
- [ ] Graceful error handling
- [ ] No memory leaks
- [ ] Follows coding style guidelines

---

## Developer 2: Client Implementation (30 pts)

**File:** `p1gxC.c`  
**Reference:** Project Spec Section 2.2-2.4

### Your Responsibility
Build a client that connects to the server, authenticates, and allows users to send/receive messages in real-time.

### Tasks (In Order):

#### Task 1: Socket Connection ✅
**Goal:** Connect to the server

```c
int sock = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in server;
server.sin_family = AF_INET;
server.sin_port = htons(SERVER_PORT);
server.sin_addr.s_addr = inet_addr("127.0.0.1");

if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
    perror("Connection failed");
    exit(1);
}
```

**Error Handling:**
- Server not running
- Invalid host/port
- Network issues

**Test:** Can you connect to your mock server?

**Reference:** Spec 2.2 - "Develop a client application allowing users to connect to the server"

---

#### Task 2: User Interface - Input Prompt ✅
**Goal:** Ask user for username at startup

```c
char username[MAX_USERNAME];
printf("Enter username: ");
fgets(username, MAX_USERNAME, stdin);
username[strcspn(username, "\n")] = 0; // Remove newline
```

**UI Requirements:**
- Simple text-based interface (no GUI needed)
- Clear prompts
- Easy to use

**Test:** Does username get input correctly?

**Reference:** Spec 2.4 - "Provide options for users to choose unique usernames"  
**Reference:** Spec 2.2 - "Implement a simple user interface for ease of use"

---

#### Task 3: Send Authentication ✅
**Goal:** Send username to server following protocol

```c
// Format according to your protocol
char auth_msg[MAX_MESSAGE];
sprintf(auth_msg, "AUTH:%s\n", username);
send(sock, auth_msg, strlen(auth_msg), 0);

// Wait for response
char response[MAX_MESSAGE];
recv(sock, response, MAX_MESSAGE, 0);

if (strstr(response, "AUTH_OK")) {
    printf("Authentication successful!\n");
} else {
    printf("Authentication failed!\n");
    exit(1);
}
```

**Test:** Does auth message get sent correctly?

---

#### Task 4: Receive Thread ✅
**Goal:** Continuously receive and display messages from server

```c
void *receive_messages(void *arg) {
    int sock = *(int*)arg;
    char buffer[MAX_MESSAGE];
    
    while (1) {
        int bytes = recv(sock, buffer, MAX_MESSAGE, 0);
        if (bytes <= 0) {
            printf("Disconnected from server\n");
            break;
        }
        
        buffer[bytes] = '\0';
        // Display message (Task 5)
    }
    
    return NULL;
}

// In main:
pthread_t recv_thread;
pthread_create(&recv_thread, NULL, receive_messages, &sock);
```

**Critical:** This thread must NOT block user input!

**Test:** Can you receive messages while typing?

**Reference:** Spec 2.3 - "Ensure messages are delivered in real-time"

---

#### Task 5: User Interface - Display Messages ✅
**Goal:** Format and display incoming messages

```c
// Parse message according to protocol
// Example format: MSG:alice:hello world
void display_message(char *raw_msg) {
    char sender[MAX_USERNAME];
    char content[MAX_MESSAGE];
    
    // Parse based on your protocol
    // Example: sscanf(raw_msg, "MSG:%[^:]:%[^\n]", sender, content);
    
    printf("[%s]: %s\n", sender, content);
    printf("> "); // Re-display prompt
    fflush(stdout);
}
```

**UI Tips:**
- Clear message display
- Show sender name
- Re-display input prompt after messages
- Consider timestamps (optional)

**Test:** Are messages displayed clearly?

**Reference:** Spec 2.2 - "Enable users to send and receive messages"

---

#### Task 6: Input Thread ✅
**Goal:** Continuously read user input without blocking

```c
void *send_messages(void *arg) {
    int sock = *(int*)arg;
    char input[MAX_MESSAGE];
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, MAX_MESSAGE, stdin) == NULL) {
            break; // EOF (Ctrl+D)
        }
        
        input[strcspn(input, "\n")] = 0; // Remove newline
        
        if (strlen(input) == 0) continue;
        
        // Format and send (Task 7)
    }
    
    return NULL;
}

// In main:
pthread_t send_thread;
pthread_create(&send_thread, NULL, send_messages, &sock);
```

**Test:** Can you type while receiving messages?

---

#### Task 7: Send Messages ✅
**Goal:** Format and send messages to server

```c
// Format according to protocol
char msg[MAX_MESSAGE];
sprintf(msg, "MSG:%s:%s\n", username, input);
send(sock, msg, strlen(msg), 0);
```

**Test:** Do messages get sent correctly?

**Reference:** Spec 2.2 - "Enable users to send and receive messages"

---

#### Task 8: Graceful Exit ✅
**Goal:** Clean up on exit

```c
// Handle Ctrl+D (EOF)
// Handle Ctrl+C (SIGINT)
void cleanup() {
    close(sock);
    pthread_cancel(recv_thread);
    pthread_join(recv_thread, NULL);
    printf("Disconnected. Goodbye!\n");
}

// Register signal handler
signal(SIGINT, cleanup);
```

**Cleanup checklist:**
- Close socket
- Cancel/join threads
- Free any allocated memory
- Display goodbye message

---

### Testing Your Client (Independent, Phase 2)

**Build a mock server (~20 lines):**

```c
// mock_server.c
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    server.sin_addr.s_addr = INADDR_ANY;
    
    bind(server_sock, (struct sockaddr*)&server, sizeof(server));
    listen(server_sock, 5);
    
    printf("Mock server listening on port 5000...\n");
    
    int client_sock = accept(server_sock, NULL, NULL);
    
    char buffer[1024];
    // Receive auth
    recv(client_sock, buffer, 1024, 0);
    printf("Received: %s", buffer);
    
    // Send AUTH_OK
    send(client_sock, "AUTH_OK\n", 9, 0);
    
    // Echo loop
    while (1) {
        int bytes = recv(client_sock, buffer, 1024, 0);
        if (bytes <= 0) break;
        printf("Received: %s", buffer);
        send(client_sock, buffer, bytes, 0); // Echo back
    }
    
    close(client_sock);
    close(server_sock);
    return 0;
}
```

**Compile:** `gcc mock_server.c -o mock_server`

**Test Plan:**
1. Run mock server: `./mock_server`
2. Run your client: `./p1gxC`
3. Test authentication
4. Test sending messages
5. Test receiving messages
6. Test multiple client instances
7. Test graceful disconnect (Ctrl+D)

---

### Deliverable Checklist for Dev 2:
- [ ] `p1gxC.c` with clean, commented code
- [ ] Connects to server successfully
- [ ] Authentication working
- [ ] Non-blocking I/O (can send and receive simultaneously)
- [ ] Clean message display
- [ ] Graceful exit handling
- [ ] No memory leaks
- [ ] Follows coding style guidelines

---

## Protocol Design (protocol.h)

This is the **contract** between server and client. Define together in Phase 1.

### Template Structure

```c
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Configuration
#define SERVER_PORT 5000
#define MAX_USERNAME 32
#define MAX_MESSAGE 256
#define MAX_CLIENTS 10

// Message Types
#define MSG_TYPE_AUTH       1
#define MSG_TYPE_MESSAGE    2
#define MSG_TYPE_NOTIFY     3
#define MSG_TYPE_ERROR      4

// Response Codes
#define AUTH_OK             "AUTH_OK"
#define AUTH_FAILED         "AUTH_FAILED"
#define MSG_DELIVERED       "MSG_OK"

// Message Structure (if using structs)
typedef struct {
    uint8_t type;
    char sender[MAX_USERNAME];
    char content[MAX_MESSAGE];
    uint32_t timestamp;  // Optional
} message_t;

// Protocol Formats (if using text-based)
// Authentication: "AUTH:username\n"
// Message:        "MSG:sender:content\n"
// Notification:   "NOTIFY:content\n"
// Error:          "ERROR:message\n"

// Helper Functions (Optional)
void format_auth_message(char *buffer, const char *username);
void format_chat_message(char *buffer, const char *sender, const char *content);
int parse_message(const char *raw, message_t *msg);

#endif // PROTOCOL_H
```

### Design Decisions Checklist

- [ ] **Message format:** Struct-based or text-based?
- [ ] **Delimiter:** Newline, null, or length-prefix?
- [ ] **Encoding:** ASCII, UTF-8?
- [ ] **Size limits:** Username max? Message max?
- [ ] **Message types:** What types do we need?
- [ ] **Error handling:** How to report errors?
- [ ] **Timestamps:** Include or skip?
- [ ] **Endianness:** Big-endian or little-endian (if using binary)?

### Example Text-Based Protocol

**Simple and Recommended:**

```
Format: TYPE:DATA\n

AUTH:username\n
MSG:sender:message content\n
NOTIFY:system message\n
ERROR:error description\n

Server responses:
AUTH_OK\n
AUTH_FAILED\n
```

**Why text-based?**
- Easy to debug (human-readable)
- Simple to parse
- Works well with `send()` and `recv()`
- No endianness issues

### Example Binary Protocol

**More Complex:**

```c
typedef struct {
    uint8_t type;           // 1 byte
    uint8_t sender_len;     // 1 byte
    uint8_t content_len;    // 1 byte
    uint8_t reserved;       // 1 byte (padding)
    char data[508];         // Variable data
} __attribute__((packed)) packet_t;
```

**Why binary?**
- More efficient
- Fixed size can simplify parsing
- But: more complex, debugging harder

---

## Documentation Requirements

**File:** `p1gxDoc.pdf` or `p1gxDoc.docx`  
**Format:** 1+ pages, 1.5-line spacing, font size 11  
**Worth:** 10 points

### Required Sections:

#### 1. Problem-Solving Approach (Both developers write)
**What to include:**
- Overall architecture diagram
- How you divided the work
- Protocol design decisions
- Threading model explanation
- Data structures used
- Synchronization strategy (mutexes, etc.)

**Example:**
```
We designed a client-server architecture where:
- Server uses one thread per client plus one broadcast thread
- Message queue with mutex for thread-safe access
- Text-based protocol with newline delimiters for simplicity
- [Include diagram showing thread interactions]
```

---

#### 2. User Guide (Both developers write)
**What to include:**

**Step 1: Compilation**
```bash
# Compile server
gcc p1gxS.c -o server -pthread

# Compile client
gcc p1gxC.c -o client -pthread
```

**Step 2: Running the Server**
```bash
./server
# Server starts listening on port 5000
```

**Step 3: Running Clients**
```bash
# Terminal 1
./client
Enter username: alice
> hello everyone

# Terminal 2
./client
Enter username: bob
[alice]: hello everyone
> hi alice
```

**Step 4: Example Session**
- Include screenshots of multiple clients chatting
- Show connection messages
- Show disconnection handling

---

#### 3. Code Analysis

**Dev 1 writes (Server):**
- Threading implementation details
- Message queue implementation
- Client tracking mechanism
- Broadcast logic
- Error handling approach
- Memory management

**Example:**
```
The server creates a new pthread for each connected client using
pthread_create(). Each thread runs handle_client() which:
1. Authenticates the user
2. Enters a loop to receive messages
3. Adds messages to the shared queue
[Code snippet with explanation]
```

**Dev 2 writes (Client):**
- Socket connection process
- I/O threading model
- Message sending logic
- Message receiving/display logic
- User interface implementation
- Graceful shutdown handling

**Example:**
```
The client uses two threads:
1. Receive thread: Continuously reads from socket
2. Send thread: Reads user input
This allows simultaneous sending and receiving.
[Code snippet with explanation]
```

---

#### 4. Screenshots (Both developers)
**Required screenshots:**
- [ ] Server starting up
- [ ] First client connecting and authenticating
- [ ] Second client connecting
- [ ] Multiple clients exchanging messages
- [ ] Client disconnecting gracefully
- [ ] Server handling disconnection

**Tips:**
- Use clear terminal colors
- Show multiple terminal windows in one screenshot
- Annotate screenshots if needed

---

#### 5. Testing & Validation (Both developers)
**What to include:**
- Test cases you ran
- How you verified correctness
- Edge cases tested
- Any bugs found and fixed

**Example test cases:**
- Multiple clients connect simultaneously
- One client disconnects while others chat
- Very long messages
- Empty messages
- Special characters in messages

---

## Testing Strategy

### Phase 2: Independent Testing

#### Dev 1 (Server) Testing:
```bash
# Test 1: Single mock client
./server &
./mock_client

# Test 2: Multiple mock clients
./server &
./mock_client &
./mock_client &
./mock_client &

# Test 3: Disconnect handling
./server &
./mock_client &
# Kill one mock client with Ctrl+C
# Verify server still works
```

#### Dev 2 (Client) Testing:
```bash
# Test 1: Connect to mock server
./mock_server &
./client

# Test 2: Multiple clients
./mock_server &
./client &
./client &

# Test 3: Graceful exit
./mock_server &
./client
# Press Ctrl+D
# Verify clean shutdown
```

### Phase 3: Integration Testing

```bash
# Test 1: Basic functionality
./server &
./client  # Terminal 1
./client  # Terminal 2
# Chat between terminals

# Test 2: Stress test
./server &
# Run 10 clients in parallel
for i in {1..10}; do
    ./client &
done

# Test 3: Disconnect scenarios
./server &
./client  # Client 1
./client  # Client 2
# Kill Client 1 with Ctrl+C
# Verify Client 2 still works
```

---

## Submission Checklist

### Files to Submit (on Canvas):

- [ ] `p1gxS.c` - Server source code
- [ ] `p1gxC.c` - Client source code
- [ ] `protocol.h` - Protocol header file
- [ ] `p1gxDoc.pdf` or `p1gxDoc.docx` - Documentation
- [ ] Any additional header files (optional)

**Note:** Replace 'x' with your group number on Canvas

### Pre-Submission Checklist:

#### Code Quality:
- [ ] Code is well-commented
- [ ] Consistent indentation (4 spaces or tab)
- [ ] Meaningful variable names
- [ ] No compiler warnings
- [ ] No memory leaks (test with valgrind)
- [ ] Error handling implemented
- [ ] Code follows C best practices

#### Functionality:
- [ ] Server handles multiple clients
- [ ] Client can send and receive simultaneously
- [ ] Authentication works
- [ ] Message broadcasting works
- [ ] Disconnections handled gracefully
- [ ] No crashes on edge cases

#### Documentation:
- [ ] All required sections included
- [ ] Screenshots clear and annotated
- [ ] User guide is complete
- [ ] Code analysis is detailed
- [ ] 1+ pages, 1.5-line spacing, font 11
- [ ] Proper spelling and grammar

#### Compilation:
- [ ] Server compiles without errors
- [ ] Client compiles without errors
- [ ] Makefile included (optional but recommended)

---

## Timeline Recommendation

**Week 1:**
- **Day 1-2:** Phase 1 - Protocol design together (2-4 hours)
- **Day 3-5:** Phase 2 - Independent development
  - Dev 1: Work on server implementation
  - Dev 2: Work on client implementation
  - Daily: 2-3 hours of coding
  - Daily: Quick sync (15 min) to discuss progress

**Week 2:**
- **Day 6-7:** Phase 2 - Completion
  - Finish independent implementations
  - Test with mock components thoroughly
- **Day 8:** Phase 3 - Integration testing (4-6 hours)
  - Connect real server + real client
  - Test all functionality
  - Fix integration bugs
- **Day 9:** Documentation
  - Write documentation sections
  - Take screenshots
  - Review and polish
- **Day 10:** Final review and submission
  - Code review together
  - Final testing
  - Submit on Canvas

---

## Optional Makefile

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGETS = server client mock_server mock_client

all: $(TARGETS)

server: p1gxS.c protocol.h
	$(CC) $(CFLAGS) -o server p1gxS.c

client: p1gxC.c protocol.h
	$(CC) $(CFLAGS) -o client p1gxC.c

mock_server: mock_server.c protocol.h
	$(CC) $(CFLAGS) -o mock_server mock_server.c

mock_client: mock_client.c protocol.h
	$(CC) $(CFLAGS) -o mock_client mock_client.c

clean:
	rm -f $(TARGETS)

test: all
	@echo "Running server..."
	./server &
	sleep 1
	@echo "Running clients..."
	./client &
	./client &

.PHONY: all clean test
```

---

## Tips for Success

### Communication:
- Daily 15-minute sync meetings
- Use Git/GitHub for version control
- Share protocol.h immediately after Phase 1
- Test early, test often

### Development:
- Start with simple version, then add features
- Test each component independently
- Use version control (Git)
- Keep backups

### Debugging:
- Use print statements liberally during development
- Test with valgrind: `valgrind --leak-check=full ./server`
- Use GDB for debugging: `gdb ./server`
- Check return values of all system calls

### Common Pitfalls to Avoid:
- **Not protecting shared data with mutexes** → Race conditions
- **Forgetting to detach threads** → Zombie threads
- **Not handling disconnections** → Server crashes
- **Blocking operations in threads** → UI freezes
- **Not freeing memory** → Memory leaks
- **Not closing sockets** → Resource exhaustion

---

## Resources

### C Socket Programming:
- Beej's Guide to Network Programming
- `man socket`, `man pthread`, `man send`, `man recv`

### Debugging:
- GDB Tutorial
- Valgrind documentation

### Git:
- Basic Git workflow
- How to handle merge conflicts

---

## Questions to Ask During Phase 1

**Protocol Design Questions:**
1. Should we use text-based or binary protocol?
2. What delimiter should we use?
3. How do we handle message boundaries?
4. What's the maximum message size?
5. Do we need timestamps?
6. How do we handle errors?
7. Should usernames be case-sensitive?
8. What characters are allowed in usernames?

**Architecture Questions:**
1. How many threads does the server need?
2. How should we structure the message queue?
3. What data structures do we need?
4. How do we detect client disconnections?
5. How do we handle server shutdown?

---

## Why This Approach Works

### The Key Insight:
By defining the protocol first, you create a **contract** that both sides follow. This means:

1. **Dev 1** doesn't need the real client to test - just something that sends data in the protocol format
2. **Dev 2** doesn't need the real server to test - just something that responds with the protocol format
3. Both can work **completely independently**
4. Integration becomes trivial because both components already follow the same contract

### This is How Real Software Teams Work:
- Define interfaces first
- Implement independently
- Use mocks for testing
- Integrate at the end

This approach eliminates the circular dependency: "I need the server to test the client, but I need the client to test the server."

---

## Final Checklist Before Submission

**One Day Before:**
- [ ] Both components compile without warnings
- [ ] All features work in integration testing
- [ ] No memory leaks (tested with valgrind)
- [ ] Documentation is complete
- [ ] Screenshots are clear
- [ ] Code is commented
- [ ] File names are correct (p1gxS.c, p1gxC.c, etc.)

**Day of Submission:**
- [ ] Final test run
- [ ] Zip all files together
- [ ] Submit on Canvas **before** class
- [ ] Prepare for in-class presentation

---

## Good Luck! 🚀

Remember:
- **Communication** is key
- **Test early and often**
- **Follow the protocol strictly**
- **Ask questions** when stuck
- **Start early** - don't wait until last minute

This is a realistic software engineering project. The skills you learn here (protocol design, independent development, testing with mocks, integration) are directly applicable to industry work.

You've got this! 💪