#!/bin/bash
# Test script for Live Chat Room

echo "╔════════════════════════════════════════╗"
echo "║     Live Chat Room - Test Script      ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Clean up any existing processes
echo "[Test] Cleaning up existing server processes..."
killall -9 server 2>/dev/null
sleep 1

# Start server in background
echo "[Test] Starting server..."
./server > server_output.log 2>&1 &
SERVER_PID=$!
sleep 2

# Check if server started
if ps -p $SERVER_PID > /dev/null; then
    echo "[Test] ✓ Server started successfully (PID: $SERVER_PID)"
else
    echo "[Test] ✗ Server failed to start"
    cat server_output.log
    exit 1
fi

echo ""
echo "Server is now running on port 5000"
echo "Server output is being logged to: server_output.log"
echo ""
echo "To test the chat:"
echo "  1. Open a new terminal and run: ./client"
echo "  2. Enter a username when prompted"
echo "  3. Open another terminal and run: ./client"
echo "  4. Enter a different username"
echo "  5. Start chatting between the terminals!"
echo ""
echo "To stop the server: kill $SERVER_PID"
echo "Or run: killall server"
echo ""
echo "Press any key to view live server logs (Ctrl+C to exit)..."
read -n 1
tail -f server_output.log
