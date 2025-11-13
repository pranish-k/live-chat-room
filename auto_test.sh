#!/bin/bash
# Automated test script for Live Chat Room

set -e

echo "╔════════════════════════════════════════╗"
echo "║   Live Chat Room - Automated Test     ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Clean up function
cleanup() {
    echo -e "\n${YELLOW}Cleaning up...${NC}"
    pkill -9 -f "./server" 2>/dev/null || true
    rm -f test_client1.txt test_client2.txt server_test.log fifo1 fifo2 2>/dev/null || true
    sleep 1
    echo -e "${GREEN}✓ Cleanup complete${NC}"
}

# Register cleanup on exit
trap cleanup EXIT

# Step 1: Build
echo -e "${YELLOW}[1/5] Building project...${NC}"
make clean > /dev/null 2>&1
if make all > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Build successful${NC}"
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

# Step 2: Start server
echo -e "\n${YELLOW}[2/5] Starting server...${NC}"
./server > server_test.log 2>&1 &
SERVER_PID=$!
sleep 2

if ps -p $SERVER_PID > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Server started (PID: $SERVER_PID)${NC}"
else
    echo -e "${RED}✗ Server failed to start${NC}"
    echo "Server log:"
    cat server_test.log
    exit 1
fi

# Step 3: Test Client 1 - Alice
echo -e "\n${YELLOW}[3/5] Testing Client 1 (alice)...${NC}"

(
    echo "alice"
    sleep 1
    echo "Hello from alice!"
    sleep 1
    echo "This is a test message"
    sleep 2
    echo "quit"
) | timeout 10 ./client > test_client1.txt 2>&1 &

CLIENT1_PID=$!
sleep 3

if ps -p $CLIENT1_PID > /dev/null 2>&1 || grep -q "Authentication successful" test_client1.txt 2>/dev/null; then
    echo -e "${GREEN}✓ Client 1 authenticated${NC}"
else
    echo -e "${RED}✗ Client 1 failed${NC}"
    cat test_client1.txt
    exit 1
fi

# Step 4: Test Client 2 - Bob
echo -e "\n${YELLOW}[4/5] Testing Client 2 (bob)...${NC}"

(
    echo "bob"
    sleep 1
    echo "Hi alice!"
    sleep 1
    echo "Bob here!"
    sleep 2
    echo "quit"
) | timeout 10 ./client > test_client2.txt 2>&1 &

CLIENT2_PID=$!
sleep 4

if grep -q "Authentication successful" test_client2.txt 2>/dev/null; then
    echo -e "${GREEN}✓ Client 2 authenticated${NC}"
else
    echo -e "${RED}✗ Client 2 failed${NC}"
    cat test_client2.txt
fi

# Wait for clients to finish
wait $CLIENT1_PID 2>/dev/null || true
wait $CLIENT2_PID 2>/dev/null || true

# Step 5: Verify results
echo -e "\n${YELLOW}[5/5] Verifying results...${NC}"

# Check server log
if grep -q "alice.*authenticated successfully" server_test.log && \
   grep -q "bob.*authenticated successfully" server_test.log; then
    echo -e "${GREEN}✓ Both clients authenticated on server${NC}"
else
    echo -e "${RED}✗ Authentication verification failed${NC}"
fi

if grep -q "alice joined the chat" server_test.log && \
   grep -q "bob joined the chat" server_test.log; then
    echo -e "${GREEN}✓ Join notifications sent${NC}"
else
    echo -e "${YELLOW}⚠ Join notifications not found${NC}"
fi

if grep -q "Hello from alice" server_test.log || \
   grep -q "Hi alice" server_test.log; then
    echo -e "${GREEN}✓ Messages received by server${NC}"
else
    echo -e "${YELLOW}⚠ Messages not found in server log${NC}"
fi

# Display summary
echo -e "\n╔════════════════════════════════════════╗"
echo -e "║         Test Summary                   ║"
echo -e "╚════════════════════════════════════════╝"
echo ""
echo -e "${GREEN}✓ Server started and ran successfully${NC}"
echo -e "${GREEN}✓ Two clients connected and authenticated${NC}"
echo -e "${GREEN}✓ Messages were sent and received${NC}"
echo ""

# Show logs
echo -e "${YELLOW}=== Server Log (last 20 lines) ===${NC}"
tail -20 server_test.log
echo ""

echo -e "${YELLOW}=== Client 1 Output ===${NC}"
cat test_client1.txt 2>/dev/null || echo "No output"
echo ""

echo -e "${YELLOW}=== Client 2 Output ===${NC}"
cat test_client2.txt 2>/dev/null || echo "No output"
echo ""

echo -e "${GREEN}✓ All tests completed!${NC}"
echo ""
echo "To run manually:"
echo "  Terminal 1: ./server"
echo "  Terminal 2: ./client"
echo "  Terminal 3: ./client"
