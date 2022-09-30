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

#include <unistd.h>    // close()
#endif

int myCloseSocket(int sockFD)
{
#if defined(WIN32) || defined(_WIN32)
    return closesocket(sockFD);
#else
    return close(sockFD);
#endif
}

std::string IpAddressToString(sockaddr *addr, int addr_len)
{
    // ipv6 length makes sure both ipv4/6 addresses can be stored in this variable
    char ipStr[INET6_ADDRSTRLEN];

#if (defined(WIN32) || defined(_WIN32))
    //Copy and remove port
    sockaddr_in6 addr_copy;
    int addr_copy_len = sizeof (sockaddr_in6);
    memset(&addr_copy, 0, addr_copy_len);

    if(addr_len < addr_copy_len)
        addr_copy_len = addr_len;

    memcpy(&addr_copy, addr, addr_copy_len);

    if (addr->sa_family == AF_INET)
    {
        /* IPv4 */
        ((sockaddr_in *)&addr_copy)->sin_port = 0;
    }
    else if (addr->sa_family == AF_INET6)
    {
        /* IPv6 */
        ((sockaddr_in6 *)&addr_copy)->sin6_port = 0;
    }
    else
    {
        //Unsupperted
        return {};
    }

    DWORD requiredLength = sizeof (ipStr);
    int err = WSAAddressToStringA((sockaddr *)&addr_copy, addr_copy_len, nullptr, ipStr, &requiredLength);
    if(err)
    {
        std::cerr << "Socket name error" << std::endl;
    }
#else
    void *addr_ptr;

    // if address is ipv4 address
    if (addr->sa_family == AF_INET)
    {
        sockaddr_in *ipv4 = reinterpret_cast<sockaddr_in *>(addr);
        addr_ptr = &(ipv4->sin_addr);
    }
    else if(addr->sa_family == AF_INET6)
    {
        sockaddr_in6 *ipv6 = reinterpret_cast<sockaddr_in6 *>(addr);
        addr_ptr = &(ipv6->sin6_addr);
    }
    else
    {
        //Unsupperted
        return {};
    }

    inet_ntop(addr->sa_family, addr_ptr, ipStr, sizeof (ipStr));
#endif

    return std::string(ipStr);
}

int main(int argc, char *argv[])
{
#if defined(WIN32) || defined(_WIN32)
    WSADATA wdata;
    if(WSAStartup(MAKEWORD(2, 2), &wdata) != 0)
        return 1;
#endif

    // Let's check if port number is supplied or not..
    if (argc != 2) {
        std::cerr << "Run program as 'program <port>'\n";
        return -1;
    }

    auto &portNum = argv[1];
    const unsigned int backLog = 8;  // number of connections allowed on the incoming queue


    addrinfo hints, *res, *p;    // we need 2 pointers, res to hold and p to iterate over
    memset(&hints, 0, sizeof(hints));

    // for more explanation, man socket
    hints.ai_family   = AF_UNSPEC;    // don't specify which IP version to use yet
    hints.ai_socktype = SOCK_STREAM;  // SOCK_STREAM refers to TCP, SOCK_DGRAM will be?
    hints.ai_flags    = AI_PASSIVE;


    // man getaddrinfo
    int gAddRes = getaddrinfo(NULL, portNum, &hints, &res);
    if (gAddRes != 0) {
        std::cerr << gai_strerror(gAddRes) << "\n";
        return -2;
    }

    std::cout << "Detecting addresses" << std::endl;

    unsigned int numOfAddr = 0;

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
        std::cout << "(" << numOfAddr << ") " << ipVer << " : " << IpAddressToString(p->ai_addr, p->ai_addrlen)
                  << std::endl;
    }

    // if no addresses found :(
    if (!numOfAddr) {
        std::cerr << "Found no host address to use\n";
        return -3;
    }

    // ask user to choose an address
    std::cout << "Enter the number of host address to bind with: ";
    unsigned int choice = 0;
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
    int sockFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockFD == -1) {
        std::cerr << "Error while creating socket\n";
        freeaddrinfo(res);
        return -4;
    }


    // Let's bind address to our socket we've just created
    int bindR = bind(sockFD, p->ai_addr, p->ai_addrlen);
    if (bindR == -1) {
        std::cerr << "Error while binding socket\n";

        // if some error occurs, make sure to close socket and free resources
        myCloseSocket(sockFD);
        freeaddrinfo(res);
        return -5;
    }


    // finally start listening for connections on our socket
    int listenR = listen(sockFD, backLog);
    if (listenR == -1) {
        std::cerr << "Error while Listening on socket\n";

        // if some error occurs, make sure to close socket and free resources
        myCloseSocket(sockFD);
        freeaddrinfo(res);
        return -6;
    }


    // structure large enough to hold client's address
    sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);


    const std::string response = "Hello World";


    // a fresh infinite loop to communicate with incoming connections
    // this will take client connections one at a time
    // in further examples, we're going to use fork() call for each client connection
    while (1) {

        // accept call will give us a new socket descriptor
        int newFD
            = accept(sockFD, (sockaddr *) &client_addr, &client_addr_size);
        if (newFD == -1) {
            std::cerr << "Error while Accepting on socket\n";
            continue;
        }

        // send call sends the data you specify as second param and it's length as 3rd param, also returns how many bytes were actually sent
        auto bytes_sent = send(newFD, response.data(), response.length(), 0);
        myCloseSocket(newFD);
    }

    myCloseSocket(sockFD);

    freeaddrinfo(res);

#if defined(WIN32) || defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}
