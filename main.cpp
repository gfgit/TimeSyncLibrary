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

    //Settings
    bool runAsServer = true;
    std::string serverAddressStr, portStr;

    //Parse cmd line
    if(argc > 1)
    {
        bool printHelp = false;

        for(int i = 0; i < argc; i++)
        {
            if(strcmp(argv[i], "--server") == 0)
            {
                runAsServer = true;
            }
            else if(strcmp(argv[i], "--client") == 0)
            {
                runAsServer = false;

                if(argc > i + 1)
                {
                    //Store address
                    i++;
                    serverAddressStr = argv[i];
                }
                else
                {
                    printHelp = true;
                    break;
                }
            }
            else if(strcmp(argv[i], "--port") == 0)
            {
                if(argc > i + 1)
                {
                    //Store port
                    i++;
                    portStr = argv[i];
                }
                else
                {
                    printHelp = true;
                    break;
                }
            }
        }

        if(printHelp)
        {
            //Bad argument, print help
            std::cout << "Usage: " << argv[0] << endl
                      << "--server starts a server and prints address\n"
                      << "--client <IP> starts a client\n"
                      << "--port <port> port, if not passed default to 1234\n"
                         "Bye bye." << endl;
            return 1;
        }
    }

    u_short portNum = 1234; //Default port
    if(!portStr.empty())
    {
        //Use port chosen by user
        portNum = atoi(portStr.c_str());
    }

    //Convert to network byte order
    portNum = htons(portNum);


    if(runAsServer)
    {
        TimeSyncLibrary::Server server;

        struct sockaddr_in any_addr_ipv4;
        any_addr_ipv4.sin_family = AF_INET; /* Internet domain */
        any_addr_ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
        any_addr_ipv4.sin_port = portNum;

        if(server.listen((sockaddr *)&any_addr_ipv4, sizeof(any_addr_ipv4), 8) < 0)
        {
            std::cerr << "Error listening server" << endl;
            return 2;
        }

        char host[256];
        struct hostent *host_info;
        char *IP;
        if(gethostname(host, sizeof(host)) == -1)
        {
            cout << "Error getting host name" << endl;
        }

        host_info = gethostbyname(host);
        if(host_info == nullptr)
        {
            cout << "Error getting host name" << endl;
        }
        int i = 0;
        while(host_info->h_addr_list[i] != nullptr)
        {
            IP = inet_ntoa(*(in_addr*)host_info->h_addr_list[i]);
            cout << "Server started on : " << IP << " Port : " << ntohs(portNum) << endl;
            i++;
        }


        while(true)
        {
            // structure large enough to hold client's address
            sockaddr_storage client_addr;
            int client_addr_size = sizeof(client_addr);

            TimeSyncLibrary::Server clientSock;

            // <= 0 beacuse linux returns -1 on error, windows returns 0
            if (server.accept(clientSock,(sockaddr*) &client_addr, &client_addr_size) <= 0)
            {
                std::cerr << "Error while Accepting on socket\n";
                continue;
            }
            IP = inet_ntoa(((sockaddr_in*)&client_addr)->sin_addr);
            cout << "Request accepted " << IP << endl;

            std::string response = "Hello World!";
            auto bytes_sent = clientSock.write(response.data(), (int)response.length());
            cout << bytes_sent << endl;
            break;
        }
    }
    else
    {
        TimeSyncLibrary::Socket client;

        struct sockaddr_in remoteServerAddr;
        remoteServerAddr.sin_family = AF_INET; /* Internet domain */
        remoteServerAddr.sin_addr.s_addr = inet_addr(serverAddressStr.c_str());
        remoteServerAddr.sin_port = portNum;

        if(client.connectTo((sockaddr *)&remoteServerAddr, sizeof(remoteServerAddr)) < 0)
        {
            std::cerr << "Error connecting to server\n";
            return 3;
        }

        char buff[50] = {0};
        int bytes_read = client.read(buff,50);

        cout << bytes_read << " " << buff << endl;
    }

//    // Let's check if port number is supplied or not..
//    std::string portStr;
//    if (argc < 2)
//    {
//        std::cerr << "Run program as 'program <port>'\n";
//        portStr = "1234";
//    }
//    else
//    {
//        portStr = argv[1];
//    }

//    const unsigned int backLog = 8;  // number of connections allowed on the incoming queue

//    addrinfo hints, *res, *p;    // we need 2 pointers, res to hold and p to iterate over
//    memset(&hints, 0, sizeof(hints));

//    // for more explanation, man socket
//    hints.ai_family   = AF_UNSPEC;    // don't specify which IP version to use yet
//    hints.ai_socktype = SOCK_STREAM;  // SOCK_STREAM refers to TCP, SOCK_DGRAM will be?
//    hints.ai_flags    = AI_PASSIVE;


//    // man getaddrinfo
//    int gAddRes = getaddrinfo(NULL, portStr.c_str(), &hints, &res);
//    if (gAddRes != 0) {
//        std::cerr << gai_strerror(gAddRes) << "\n";
//        return -2;
//    }

//    std::cout << "Detecting addresses" << std::endl;

//    int numOfAddr = 0;

//    // Now since getaddrinfo() has given us a list of addresses
//    // we're going to iterate over them and ask user to choose one
//    // address for program to bind to
//    for (p = res; p != NULL; p = p->ai_next)
//    {
//        std::string ipVer;

//        // if address is ipv4 address
//        if (p->ai_family == AF_INET)
//        {
//            ipVer = "IPv4";
//        }
//        else if (p->ai_family == AF_INET6)
//        {
//            ipVer = "IPv6";
//        }
//        else
//        {
//            //Unsupported protocol
//            ipVer = "UNK?";
//        }

//        numOfAddr++;

//        // convert IPv4 and IPv6 addresses from binary to text form
//        std::cout << "(" << numOfAddr << ") " << ipVer << " : " << TimeSyncLibrary::IpAddressToString(p->ai_addr, (int)p->ai_addrlen)
//                  << std::endl;
//    }

//    // if no addresses found :(
//    if (!numOfAddr) {
//        std::cerr << "Found no host address to use\n";
//        return -3;
//    }

//    // ask user to choose an address
//    std::cout << "Enter the number of host address to bind with: ";
//    int choice = 0;
//    bool madeChoice     = false;
//    do {
//        std::cin >> choice;
//        if (choice > (numOfAddr + 1) || choice < 1) {
//            madeChoice = false;
//            std::cout << "Wrong choice, try again!" << std::endl;
//        } else
//            madeChoice = true;
//    } while (!madeChoice);


//    // Apply user choice
//    p = res;
//    for(int i = 1; p && i < choice; i++)
//    {
//        p = p->ai_next;
//    }

//    if(!p)
//        return -3;

//    // let's create a new socket, socketFD is returned as descriptor
//    // man socket for more information
//    // these calls usually return -1 as result of some error
//    TimeSyncLibrary::Socket mySock;
//    TimeSyncLibrary::Server myServer;

//    // start server
//    if (myServer.listen(p->ai_addr, p->ai_addrlen, backLog) < 0)
//    {
//        std::cerr << "Error while binding socket\n";
//        // if some error occurs, make sure to close socket and free resources
//        freeaddrinfo(res);
//        return -5;
//    }
////    uncomment to use ipv4
////    struct sockaddr_in loopback;
////    /*Convert port number to integer*/
////    loopback.sin_family = AF_INET;       /* Internet domain */
////    loopback.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
////    loopback.sin_port = ((sockaddr_in*)p->ai_addr)->sin_port;

//    struct sockaddr_in6 loopback6 = IN6ADDR_LOOPBACK_INIT;
//    /*Convert port number to integer*/
//    loopback6.sin6_family = AF_INET6;       /* Internet domain */
//    loopback6.sin6_addr = in6addr_loopback;
//    loopback6.sin6_port = ((sockaddr_in*)p->ai_addr)->sin_port;

//    if (mySock.connectTo((sockaddr*)&loopback6, sizeof(loopback6)) < 0)
//    {
//        std::cerr << "Error while creating socket\n";
//        freeaddrinfo(res);
//        return -4;
//    }

//    // structure large enough to hold client's address
//    sockaddr_storage client_addr;
//    int client_addr_size = sizeof(client_addr);


//    const std::string response = "Hello World";


//    // a fresh infinite loop to communicate with incoming connections
//    // this will take client connections one at a time
//    // in further examples, we're going to use fork() call for each client connection
//    while (1) {

//        // accept call will give us a new socket descriptor
//        TimeSyncLibrary::Socket clientSock;

//        // <= 0 beacuse linux returns -1 on error, windows returns 0
//        if (myServer.accept(clientSock,(sockaddr*) &client_addr, &client_addr_size) <= 0)
//        {
//            std::cerr << "Error while Accepting on socket\n";
//            continue;
//        }

//        // send call sends the data you specify as second param and it's length as 3rd param, also returns how many bytes were actually sent
        
//        auto bytes_sent = clientSock.write(response.data(), (int)response.length());
//        cout << bytes_sent << endl;
//        break;
//    }

//    char buff[50] = {0};
//    int bytes_read = mySock.read(buff,50);

//    cout << bytes_read << " " << buff << endl;

//    freeaddrinfo(res);

#if defined(WIN32) || defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}
