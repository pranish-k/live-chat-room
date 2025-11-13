# Makefile for Live Chat Room Project
# Compiles both server and client with proper flags

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -pthread -std=c11

# Targets
SERVER = server
CLIENT = client
TARGETS = $(SERVER) $(CLIENT)

# Source files
SERVER_SRC = p1gxS.c
CLIENT_SRC = p1gxC.c
PROTOCOL_HEADER = protocol.h

# Default target: build everything
all: $(TARGETS)
	@echo ""
	@echo "╔════════════════════════════════════════╗"
	@echo "║     Build Complete!                    ║"
	@echo "╚════════════════════════════════════════╝"
	@echo ""
	@echo "To run the server: ./$(SERVER)"
	@echo "To run the client: ./$(CLIENT)"
	@echo ""

# Build server
$(SERVER): $(SERVER_SRC) $(PROTOCOL_HEADER)
	@echo "Compiling server..."
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC)
	@echo "✓ Server compiled successfully"

# Build client
$(CLIENT): $(CLIENT_SRC) $(PROTOCOL_HEADER)
	@echo "Compiling client..."
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)
	@echo "✓ Client compiled successfully"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGETS)
	@echo "✓ Clean complete"

# Run server
run-server: $(SERVER)
	@echo "Starting server..."
	./$(SERVER)

# Run client
run-client: $(CLIENT)
	@echo "Starting client..."
	./$(CLIENT)

# Test compilation (compile without warnings as errors)
test: CFLAGS += -Werror
test: clean all
	@echo ""
	@echo "✓ Test build successful (no warnings or errors)"

# Check for memory leaks with valgrind (if installed)
valgrind-server: $(SERVER)
	@echo "Running valgrind on server..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(SERVER)

valgrind-client: $(CLIENT)
	@echo "Running valgrind on client..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(CLIENT)

# Help target
help:
	@echo "Available targets:"
	@echo "  all            - Build both server and client (default)"
	@echo "  server         - Build server only"
	@echo "  client         - Build client only"
	@echo "  clean          - Remove compiled binaries"
	@echo "  run-server     - Compile and run server"
	@echo "  run-client     - Compile and run client"
	@echo "  test           - Test compilation with strict warnings"
	@echo "  valgrind-server- Run server with valgrind memory check"
	@echo "  valgrind-client- Run client with valgrind memory check"
	@echo "  help           - Show this help message"

# Phony targets (not actual files)
.PHONY: all clean run-server run-client test valgrind-server valgrind-client help
