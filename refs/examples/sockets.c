#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

void failOnError();
int createSocketOldWay(const char* host, int port);
int createSocketNewWay(const char* host, const char* port);

int main(int argc, char **argv)
{
    printf("Starting server...\n");

    //int serverSocket = createSocketOldWay("0.0.0.0", 8888);
    int serverSocket = createSocketNewWay(NULL, "8888");

    if (listen(serverSocket, 1) < 0)
        failOnError();

    printf("Waiting for the client...");
    fflush(stdout);

    int canWork = 1;
    while (canWork)
    {
        int clientSocket = accept(serverSocket, NULL, 0);
        if (clientSocket < 0)
            failOnError();

        char buffer[1024];
        ssize_t actualReceived = read(clientSocket, buffer, 1024);
        if (actualReceived < 0)
            failOnError();

        ssize_t actualSent = write(clientSocket, buffer, actualReceived);
        if (actualSent < 0)
            failOnError();

        actualReceived = recv(clientSocket, buffer, 1024, 0); // MSG_DONTWAIT MSG_PEEK
        if (actualReceived < 0)
            failOnError();

        actualSent = send(clientSocket, buffer, actualReceived, 0);
        if (actualSent < 0)
            failOnError();

        canWork = 0;
    }

    printf("Done");
    int x;
    scanf("%d", &x);

    close(serverSocket);

    return 0;
}

int createSocketNewWay(const char* host, const char* port)
{
    int result;

    struct addrinfo hints;
    struct addrinfo *head, *cursor;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // AF_INET6 или AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM; // 0 само подберется, не очень понятно зачем нужно
    hints.ai_flags = AI_PASSIVE; // для создания адреса INADDR_ANY, host надо передать как NULL

    if (getaddrinfo(host, port, &hints, &head) != 0)
        failOnError();

    for (cursor = head, result = -1; cursor != NULL && result < 0; cursor = cursor->ai_next)
    {
        result = socket(cursor->ai_family, cursor->ai_socktype, cursor->ai_protocol);
        if (result >= 0)
        {
            if (bind(result, cursor->ai_addr, cursor->ai_addrlen) != 0)
            {
                close(result);
                result = -1;
            }
        }
    }

    if (result < 0)
        failOnError();

    freeaddrinfo(head);

    return result;
}

int createSocketOldWay(const char* host, int port)
{
    int result;

    result = socket(AF_INET, SOCK_STREAM, 0);
    if (result < 0)
        failOnError();

    // такой адрес нет смысла объявлять - просто тип данных для приведения и передачи в bind
    struct sockaddr serverAddrGeneral;

    // можно руками собрать адрес на котором будет работать сервер
    struct sockaddr_in serverInetAddress;
    serverInetAddress.sin_family = AF_INET;
    serverInetAddress.sin_port = htons(port); // порт
    serverInetAddress.sin_addr.s_addr = INADDR_ANY; // 4 байта IP адреса, s_addr - uint32_t

    // можно сконвертировать 4-ех байтовое значение из строки в виде IP адреса
    if (inet_aton(host, &serverInetAddress.sin_addr) == 0)
        failOnError();

    // или вот так, на ошибку можно проверить через INADDR_NONE, но он совпадает с 255.255.255.255, лучше не использовать
    serverInetAddress.sin_addr.s_addr = inet_addr(host);

    // данная функция менят порядок байт на машинное представление
    //serverInetAddress.sin_addr.s_addr = inet_network("1.2.3.4");

    // можно получить строковое представление IP адреса
    printf("%s", inet_ntoa(serverInetAddress.sin_addr));

    if (bind(result, (struct sockaddr*)&serverInetAddress, sizeof(serverInetAddress)) != 0)
        failOnError();

    return result;
}

void failOnError()
{
    printf("Error! ");

    switch (errno)
    {
    case EPERM:
        printf("Operation not permitted.\n");
        break;
    case EACCES:
        printf("Permission denied.\n");
        break;
    default:
        printf("Unknown error: %d\n", errno);
        break;
    }

    exit(-1);
}