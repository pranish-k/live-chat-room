#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define PORT 8080
int main() {
int server_fd, new_socket;
struct sockaddr_in address;
int addrlen = sizeof(address);
char buffer[1024] = {0};
char *message = "Hello from server";
// 1. Create a TCP socket (IPv4)
if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
perror("socket failed");
exit(EXIT_FAILURE);
}


// 2. Bind the socket to an IP/Port
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
address.sin_port = htons(PORT); // Convert port to network byte order
if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
perror("bind failed");
close(server_fd);
exit(EXIT_FAILURE);
}
 // 3. Listen for incoming connections (backlog of 3)
 // listen() call sets the maximum number of pending connections that can be queued waiting to be accepted by your server.
For example, here 5 is used.
if (listen(server_fd, 5) < 0) {
perror("listen");
close(server_fd);
exit(EXIT_FAILURE);
}
printf("Server is listening on port %d...\n", PORT);
// 4. Accept an incoming connection
if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
perror("accept");
close(server_fd);
exit(EXIT_FAILURE);
}

// 5. Read data sent by the client
int valread = read(new_socket, buffer, sizeof(buffer));
if (valread < 0) {
perror("read");
}
 else {
printf("Received from client: %s\n", buffer);
}
// 6. Send a response back to the client
send(new_socket, message, strlen(message), 0);
printf("Response sent to client.\n");
// 7. Close the sockets
close(new_socket);
close(server_fd);
return 0;
}

