#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H


#if defined(WIN32) || defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <string>



namespace TimeSyncLibrary{
    
    
    class Socket
    {
    public:
        Socket();
        virtual ~Socket();

        int close();
        int connectTo(sockaddr *serverAddr, int len);

        int read(char *buf, int len);
        int write(const char *buf, int len);

    protected:
        int sockFd;
    };

    class Server: public Socket
    {
    public:
        Server();
        virtual ~Server();

        int listen(sockaddr *serverAddr, int len, int backLog);
    };

    std::string IpAddressToString(sockaddr *addr, int addr_len);


}








#endif // SOCKET_UTIL_H
