#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>    //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <string>
#include <iostream>

#define TRUE 1
#define FALSE 0
#define PORT 8888

void strinsert(char *dst, int len, const char *src, int offset, int finallen)
{
    char temp[finallen];

    strncpy(temp, dst + offset, strlen(dst + offset));
    strncpy(dst + offset, src, strlen(src));
    strcat(dst + offset + strlen(src), temp);
}
class ConnectedClient
{
    int socketId;
    std::string username = "";
    int messages = 0;

public:
    ConnectedClient(int socketId)
    {
        ConnectedClient::socketId = socketId;
    }
    ConnectedClient()
    {
        ConnectedClient::socketId = 0;
    }

    int GetSocketId()
    {
        return socketId;
    }

    void Close()
    {
        close(socketId);
        //Somebody disconnected , get his details and print
        struct sockaddr_in address;
        int addrlen = sizeof(address);
        getpeername(socketId, (struct sockaddr *)&address,
                    (socklen_t *)&addrlen);
        printf("Host disconnected , socketId %s, ip %s , port %d \n", socketId, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        socketId = 0;
    }

    void SetUsername(std::string username)
    {
        ConnectedClient::username = username;
        std::cout << "Changin username of " << socketId << " to " << ConnectedClient::username << std::endl;
    }
    std::string GetUsername()
    {
        return username;
    }
    void SendMessage(ConnectedClient sender, std::string text)
    {
        std::string header = sender.username + ": ";
        int length = text.length() + header.length();
        char buffer_send[length + 1];
        strinsert(buffer_send, length - 1, header.c_str(), 0, length);
        strinsert(buffer_send, length - 1, text.c_str(), header.length(), length);
        buffer_send[length] = '\0';
        send(socketId, buffer_send, strlen(buffer_send), 0);
    }

    bool IsConnected(){
        return socketId != 0;
    }

    bool CanSendMessage()
    {
        return !username.empty();
    }

    void OnMessage(std::string text);
};

class InputBuffer
{
    char *buffer;
    int length;
    int cursor = 0;

public:
    InputBuffer(char *buffer, int length)
    {
        InputBuffer::buffer = buffer;
        InputBuffer::length = length;
    }

    char ReadChar()
    {
        if (cursor >= length)
        {
            return '\0';
        }
        return buffer[cursor++];
    }
    int LeftBytes()
    {
        return length - cursor;
    }
    void Read(char *buffer, int length)
    {
        int start = cursor;
        for (int i = start; i < length; i++, cursor++)
        {
            buffer[i - start] = InputBuffer::buffer[i];
        }
    }
    std::string ReadString()
    {
        int length = ReadChar();
        char buffer[length];
        Read(buffer, length);
        return std::string(buffer);
    }
};

ConnectedClient *clients = new ConnectedClient[30];
int main(int argc, char *argv[])
{
    int opt = TRUE;
    int master_socket, addrlen, new_socket, activity, i, valread;
    int max_sd;
    struct sockaddr_in address;

    fd_set readfds;

    //create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,
    //this is just a good habit, it will work without this
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d\n", PORT);

    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    std::cout << "Waiting for connections...\n";

    while (TRUE)
    {
        FD_ZERO(&readfds);

        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        for (i = 0; i < 30; i++)
        {
            ConnectedClient *sd = (clients + i);

            if (sd->GetSocketId() == 0)
                continue;

            if (sd->GetSocketId() > 0)
                FD_SET(sd->GetSocketId(), &readfds);

            if (sd->GetSocketId() > max_sd)
                max_sd = sd->GetSocketId();
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            printf("select error");
        }

        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket,
                                     (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            printf("Connected, socketId is %d, ip is: %s, port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            for (i = 0; i < 30; i++)
            {
                ConnectedClient *sd = (clients + i);
                if (sd->GetSocketId() == 0)
                {
                    clients[i] = *(new ConnectedClient(new_socket));
                    printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        //else its some IO operation on some other socket
        for (i = 0; i < 30; i++)
        {
            ConnectedClient *sd = (clients + i);

            if (FD_ISSET(sd->GetSocketId(), &readfds))
            {
                char *buffer = new char[1500];
                if ((valread = read(sd->GetSocketId(), buffer, 1500)) == 0)
                {
                    sd->Close();
                }
                else
                {
                    InputBuffer input(buffer, valread);
                    char packetId = input.ReadChar();
                    if (packetId == 0x01)
                    { // Change username
                        sd->SetUsername(input.ReadString());
                    }
                    else if (packetId == 0x02)
                    { // Chat message
                        std::cout << "Received message from " << sd->GetUsername() << " (" << sd->GetSocketId() << ") ";
                        if (sd->CanSendMessage())
                        {
                            std::string text = input.ReadString();
                            std::cout << text;
                            sd->OnMessage(text);
                        }
                        std::cout << std::endl;
                    }
                }
                delete buffer;
            }
        }
    }

    return 0;
}
void ConnectedClient::OnMessage(std::string text)
{
    messages++;
    for (int i = 0; i < 30; i++)
    {
        ConnectedClient *client = (clients + i);
        if (client != this && client->IsConnected()){
            client->SendMessage(*this, text);
        }
    }
}
