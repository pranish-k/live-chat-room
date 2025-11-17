# Live Chat Room

A multi-threaded TCP chat system in C using POSIX sockets and pthreads.

## Features

**Server (p1g2S.c):**
- Multi-threaded architecture (one thread per client + broadcast thread)
- Username-based authentication with uniqueness checking
- Real-time message broadcasting to all connected clients
- Thread-safe message queue (circular buffer)
- Graceful shutdown handling (Ctrl+C)
- Support for up to 50 concurrent clients
- Client join/leave notifications

**Client (p1g2C.c):**
- Multi-threaded I/O (separate send and receive threads)
- Username validation and authentication
- Real-time message display with colorized output
- Support for quit command, Ctrl+D, and Ctrl+C exit methods
- Disconnect notifications to server

**Protocol (protocol.h):**
- Text-based protocol with newline delimiters
- Message types: AUTH, MSG, NOTIFY, ERROR, DISCONNECT
- Helper functions for message formatting and parsing
- Input validation for usernames and messages
- Thread-safe circular message queue

## Project Structure

```
live-chat-room/
├── protocol.h           # Communication protocol and shared structures
├── p1g2S.c              # Server implementation
├── p1g2C.c              # Client implementation
└── README.md            # This file
```

## Compilation

### Manual Compilation

```bash
# Compile server
gcc -pthread -o server p1g2S.c

# Compile client
gcc -pthread -o client p1g2C.c
```

**Requirements:**
- GCC compiler
- POSIX threads support
- C11 standard

## Running

### Start Server (Terminal 1)

```bash
./server
```

Output:
```
╔════════════════════════════════════════╗
║     Live Chat Room - Server           ║
╚════════════════════════════════════════╝

[Server] Message queue initialized
[Server] Broadcast thread started
[Server] Listening on port 8080
[Server] Maximum clients: 50
[Server] Press Ctrl+C to shutdown
```

### Start Clients (Terminal 2+)

```bash
./client
```

Enter username:
```
Enter your username: alice
✓ Connected to server
✓ Authentication successful!
```

### Chat

```
> Hello everyone!
[You] Hello everyone!
[bob] Hi alice!
> quit
Disconnected from server
```

## Protocol Specification

All messages are text-based with newline delimiters.

### Message Formats

- **AUTH** → `AUTH:username\n`
- **AUTH_OK** → `AUTH_OK\n`
- **AUTH_FAILED** → `AUTH_FAILED:reason\n`
- **MSG** → `MSG:username:content\n`
- **NOTIFY** → `NOTIFY:text\n`
- **ERROR** → `ERROR:description\n`
- **DISCONNECT** → `DISCONNECT:username\n`

### Configuration

- **SERVER_PORT:** 8080
- **MAX_USERNAME:** 32 characters
- **MAX_MESSAGE:** 256 characters
- **MAX_CLIENTS:** 50 concurrent
- **QUEUE_SIZE:** 100 messages

## Testing

### Basic Test

```bash
# Terminal 1: Start server
./server

# Terminal 2: Start first client
./client
# Enter: alice

# Terminal 3: Start second client
./client
# Enter: bob

# Both clients should receive notifications and be able to chat
```

### Stress Test

Connect 5+ clients and send messages rapidly to verify:
- No message loss
- No crashes or segfaults
- All clients receive all messages

## Common Issues

### "Address already in use"

```bash
# Kill existing server
killall server

# Wait a moment, then restart
./server
```

If that doesn't work, wait 60 seconds before restarting (TCP TIME_WAIT).

### "Connection Failed"

- Ensure server is running
- Verify port 8080 is not blocked
- Check with: `lsof -i :8080`

## Quick Reference

```bash
# Compile
gcc -Wall -Wextra -pthread -std=c11 -o server p1g2S.c
gcc -Wall -Wextra -pthread -std=c11 -o client p1g2C.c

# Run server
./server

# Run client
./client

# Kill server
killall server
```

## Architecture

### Threading Model

**Server:**
```
Main Thread (accept loop)
    ├─ Client Thread 1 (handle alice)
    ├─ Client Thread 2 (handle bob)
    └─ Broadcast Thread (distribute messages)
```

**Client:**
```
Main Thread (user input)
    └─ Receive Thread (listen for messages)
```

### Message Flow

1. Client sends: `MSG:alice:Hello!\n`
2. Server receives in client handler thread
3. Server enqueues message
4. Broadcast thread dequeues and sends to all
5. All clients receive: `MSG:alice:Hello!\n`

## Thread Safety

- Client list protected by `clients_mutex`
- Message queue protected by `queue_mutex` + `queue_cond`
- Condition variable for efficient thread synchronization
- No busy-waiting or race conditions

---

**Status:** Complete and tested 
