#include "socketutil.h"

#include <iostream>

#if defined(WIN32) || defined(_WIN32)
#include <ws2tcpip.h>
#else
// headers for socket(), getaddrinfo() and friends
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>

#include <unistd.h>    // close()
#endif


std::string TimeSyncLibrary::IpAddressToString(sockaddr *addr, int addr_len)
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

TimeSyncLibrary::Socket::Socket()
{
    sockFd = -1;
}

TimeSyncLibrary::Socket::~Socket()
{
    close();
}

int TimeSyncLibrary::Socket::close()
{
    if(sockFd == -1)
        return -1;

#if defined(WIN32) || defined(_WIN32)
    int rc = ::closesocket(sockFd);
#else
    int rc = ::close(sockFd);
#endif

    sockFd = -1;
    return rc;
}

int TimeSyncLibrary::Socket::connectTo(sockaddr *serverAddr, int len)
{
    close();

    sockFd = ::socket(serverAddr->sa_family, SOCK_STREAM, 0);
    if(sockFd == -1)
        return -1;

    return ::connect(sockFd, serverAddr, len);
}

int TimeSyncLibrary::Socket::read(char *buf, int len)
{
    return ::recv(sockFd, buf, len, 0);
}

int TimeSyncLibrary::Socket::write(const char *buf, int len)
{
    return ::send(sockFd, buf, len, 0);
}

TimeSyncLibrary::Server::Server():
    Socket()
{

}

TimeSyncLibrary::Server::~Server()
{
    close();
}

int TimeSyncLibrary::Server::listen(sockaddr *serverAddr, int len, int backLog)
{
    close();

    sockFd = ::socket(serverAddr->sa_family, SOCK_STREAM, 0);
    if(sockFd == -1)
        return -1;

    ::bind(sockFd, serverAddr, len);

    return ::listen(sockFd, backLog);
}