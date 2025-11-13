# Live Chat Room - Project Documentation

## Project Overview

A fully-featured, multi-threaded live chat room system implemented in C using POSIX sockets and pthreads. This project demonstrates concurrent programming, network communication, and real-time messaging.

### Features Implemented ✅

**Server (p1gxS.c):**
- Multi-threaded architecture (one thread per client + broadcast thread)
- Username-based authentication
- Real-time message broadcasting to all connected clients
- Thread-safe message queue with mutex synchronization
- Graceful shutdown handling (Ctrl+C)
- Support for up to 50 concurrent clients
- Client connection/disconnection notifications

**Client (p1gxC.c):**
- Multi-threaded I/O (simultaneous send and receive)
- Username validation and authentication
- Real-time message display
- Colorized terminal output for better UX
- Graceful disconnect handling
- Support for multiple quit methods (quit command, Ctrl+D, Ctrl+C)

**Protocol (protocol.h):**
- Text-based protocol with newline delimiters
- Message types: AUTH, MSG, NOTIFY, ERROR, DISCONNECT
- Helper functions for formatting and parsing
- Input validation (username, message content)
- Thread-safe message queue implementation

---

## Project Structure

```
live-chat-room/
├── protocol.h              # Communication protocol definition
├── p1gxS.c                 # Server implementation
├── p1gxC.c                 # Client implementation
├── server.c                # Basic server (foundation)
├── client.c                # Basic client (foundation)
├── Makefile                # Build automation
├── requirement.txt         # Project requirements specification
├── implementation.md       # Implementation plan and overview
├── server-implementation.md # Step-by-step server guide
├── client-implementation.md # Step-by-step client guide
├── test_chat.sh            # Testing script
└── README.md               # This file
```

---

## Compilation

### Using Makefile (Recommended)

```bash
# Build both server and client
make all

# Build server only
make server

# Build client only
make client

# Clean build artifacts
make clean

# Test compilation with strict warnings
make test
```

### Manual Compilation

```bash
# Compile server
gcc -Wall -Wextra -pthread -std=c11 -o server p1gxS.c

# Compile client
gcc -Wall -Wextra -pthread -std=c11 -o client p1gxC.c
```

**Compilation Requirements:**
- GCC compiler
- POSIX threads support (-pthread)
- C11 standard or later

---

## Running the Chat Room

### Method 1: Manual Testing

#### Terminal 1 - Start Server
```bash
./server
```

Expected output:
```
╔════════════════════════════════════════╗
║     Live Chat Room - Server           ║
╚════════════════════════════════════════╝

[Server] Message queue initialized
[Server] Broadcast thread started
[Server] Listening on port 5000
[Server] Maximum clients: 50
[Server] Press Ctrl+C to shutdown
========================================
```

#### Terminal 2 - Start Client 1
```bash
./client
```

```
Enter your username: alice
✓ Authentication successful!
```

#### Terminal 3 - Start Client 2
```bash
./client
```

```
Enter your username: bob
✓ Authentication successful!
[*] alice joined the chat
```

#### Now Chat!
```
alice> Hello everyone!
[You] Hello everyone!

bob> Hi alice!
[alice]: Hi alice!
```

### Method 2: Using Test Script

```bash
./test_chat.sh
```

---

## Testing Checklist

### Basic Functionality
- ✅ Server starts without errors
- ✅ Multiple clients can connect
- ✅ Authentication works
- ✅ Messages broadcast to all clients
- ✅ Join/leave notifications work
- ✅ Graceful disconnect handling
- ✅ Server shutdown clean

### Memory & Thread Safety
- ✅ No memory leaks
- ✅ Thread-safe operations
- ✅ Proper mutex usage
- ✅ Clean resource cleanup

---

## Protocol Specification

All messages use text-based format with newline delimiters.

### Message Types

1. **AUTH** - `AUTH:username\n`
2. **MSG** - `MSG:sender:content\n`
3. **NOTIFY** - `NOTIFY:message\n`
4. **ERROR** - `ERROR:description\n`
5. **DISCONNECT** - `DISCONNECT:username\n`

### Constraints

- Max username: 32 characters
- Max message: 256 characters
- Max clients: 50
- Server port: 5000

---

## Common Issues & Solutions

### "Address already in use"
```bash
killall server
# Wait 1 second, then restart
./server
```

### "Connection Failed"
- Ensure server is running first
- Check server is on port 5000

---

## Quick Reference

```bash
# Build
make all

# Run server
./server

# Run client
./client

# Clean
make clean

# Kill server
killall server
```

---

**Status:** Complete and tested ✅
