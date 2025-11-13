#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define PORT 8080
int main() {
int sock = 0;
struct sockaddr_in serv_addr;
char *hello = "Hello from client";
char buffer[1024] = {0};
// 1. Create a TCP socket (IPv4)
if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
perror("Socket creation error");
return -1;
}


// 2. Set up the server address structure
serv_addr.sin_family = AF_INET;
serv_addr.sin_port = htons(PORT);
// Convert IPv4 address from text to binary form
if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
perror("Invalid address/ Address not supported");
close(sock);
return -1;
}
// 3. Connect to the server
if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
perror("Connection Failed");
close(sock);
return -1;
}

// 4. Send a message to the server
send(sock, hello, strlen(hello), 0);
printf("Message sent from client.\n");
// 5. Receive the server's response
int valread = read(sock, buffer, sizeof(buffer));
if (valread < 0) {
perror("read");
}
 else {
printf("Server: %s\n", buffer);
}
// 6. Close the socket
close(sock);
return 0;
}

