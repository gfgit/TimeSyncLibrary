#if defined(WIN32) || defined(_WIN32)
#define _WIN32_WINNT 0x0600
#endif

#include <cstring>    // sizeof()
#include <iostream>
#include <string>


#if defined(WIN32) || defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
// headers for socket(), getaddrinfo() and friends
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif


#include "src/socketutil.h"

using namespace std;

int main(int argc, char *argv[])
{
#if defined(WIN32) || defined(_WIN32)
    WSADATA wdata;
    if(WSAStartup(MAKEWORD(2, 2), &wdata) != 0)
        return 1;
#endif

    // Let's check if port number is supplied or not..
    std::string portStr;
    if (argc < 2)
    {
        std::cerr << "Run program as 'program <port>'\n";
        portStr = "1234";
    }
    else
    {
        portStr = argv[1];
    }

    const unsigned int backLog = 8;  // number of connections allowed on the incoming queue

    addrinfo hints, *res, *p;    // we need 2 pointers, res to hold and p to iterate over
    memset(&hints, 0, sizeof(hints));

    // for more explanation, man socket
    hints.ai_family   = AF_UNSPEC;    // don't specify which IP version to use yet
    hints.ai_socktype = SOCK_STREAM;  // SOCK_STREAM refers to TCP, SOCK_DGRAM will be?
    hints.ai_flags    = AI_PASSIVE;


    // man getaddrinfo
    int gAddRes = getaddrinfo(NULL, portStr.c_str(), &hints, &res);
    if (gAddRes != 0) {
        std::cerr << gai_strerror(gAddRes) << "\n";
        return -2;
    }

    std::cout << "Detecting addresses" << std::endl;

    int numOfAddr = 0;

    // Now since getaddrinfo() has given us a list of addresses
    // we're going to iterate over them and ask user to choose one
    // address for program to bind to
    for (p = res; p != NULL; p = p->ai_next)
    {
        std::string ipVer;

        // if address is ipv4 address
        if (p->ai_family == AF_INET)
        {
            ipVer = "IPv4";
        }
        else if (p->ai_family == AF_INET6)
        {
            ipVer = "IPv6";
        }
        else
        {
            //Unsupported protocol
            ipVer = "UNK?";
        }

        numOfAddr++;

        // convert IPv4 and IPv6 addresses from binary to text form
        std::cout << "(" << numOfAddr << ") " << ipVer << " : " << TimeSyncLibrary::IpAddressToString(p->ai_addr, (int)p->ai_addrlen)
                  << std::endl;
    }

    // if no addresses found :(
    if (!numOfAddr) {
        std::cerr << "Found no host address to use\n";
        return -3;
    }

    // ask user to choose an address
    std::cout << "Enter the number of host address to bind with: ";
    int choice = 0;
    bool madeChoice     = false;
    do {
        std::cin >> choice;
        if (choice > (numOfAddr + 1) || choice < 1) {
            madeChoice = false;
            std::cout << "Wrong choice, try again!" << std::endl;
        } else
            madeChoice = true;
    } while (!madeChoice);


    // Apply user choice
    p = res;
    for(int i = 1; p && i < choice; i++)
    {
        p = p->ai_next;
    }

    if(!p)
        return -3;

    // let's create a new socket, socketFD is returned as descriptor
    // man socket for more information
    // these calls usually return -1 as result of some error
    TimeSyncLibrary::Socket mySock;
    TimeSyncLibrary::Server myServer;

    // start server
    if (myServer.listen(p->ai_addr, p->ai_addrlen, backLog) < 0)
    {
        std::cerr << "Error while binding socket\n";
        // if some error occurs, make sure to close socket and free resources
        freeaddrinfo(res);
        return -5;
    }
//    uncomment to use ipv4
//    struct sockaddr_in loopback;
//    /*Convert port number to integer*/
//    loopback.sin_family = AF_INET;       /* Internet domain */
//    loopback.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
//    loopback.sin_port = ((sockaddr_in*)p->ai_addr)->sin_port;

    struct sockaddr_in6 loopback6 = IN6ADDR_LOOPBACK_INIT;
    /*Convert port number to integer*/
    loopback6.sin6_family = AF_INET6;       /* Internet domain */
    loopback6.sin6_addr = in6addr_loopback;
    loopback6.sin6_port = ((sockaddr_in*)p->ai_addr)->sin_port;

    if (mySock.connectTo((sockaddr*)&loopback6, sizeof(loopback6)) < 0)
    {
        std::cerr << "Error while creating socket\n";
        freeaddrinfo(res);
        return -4;
    }

    // structure large enough to hold client's address
    sockaddr_storage client_addr;
    int client_addr_size = sizeof(client_addr);


    const std::string response = "Hello World";


    // a fresh infinite loop to communicate with incoming connections
    // this will take client connections one at a time
    // in further examples, we're going to use fork() call for each client connection
    while (1) {

        // accept call will give us a new socket descriptor
        TimeSyncLibrary::Socket clientSock;

        // <= 0 beacuse linux returns -1 on error, windows returns 0
        if (myServer.accept(clientSock,(sockaddr*) &client_addr, &client_addr_size) <= 0)
        {
            std::cerr << "Error while Accepting on socket\n";
            continue;
        }

        // send call sends the data you specify as second param and it's length as 3rd param, also returns how many bytes were actually sent
        
        auto bytes_sent = clientSock.write(response.data(), (int)response.length());
        cout << bytes_sent << endl;
        break;
    }

    char buff[50] = {0};
    int bytes_read = mySock.read(buff,50);

    cout << bytes_read << " " << buff << endl;

    freeaddrinfo(res);

#if defined(WIN32) || defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}
