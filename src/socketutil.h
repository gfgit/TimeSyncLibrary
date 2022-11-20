#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H


#if defined(WIN32) || defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <string>



namespace TimeSyncLibrary{
    
    /*!
    Classe che realizza la connessione del client
    */
    class Socket
    {
    public:
        Socket();
        virtual ~Socket();

        int close();

        /*! crea e connette socket all'indirizzo specificato, len indica ipv4,ipv6 (quanti byte è lungo sockaddr)*/
        int connectTo(sockaddr *serverAddr, int len);

        /*! legge un buffer lungo len */
        int read(char *buf, int len);
        int write(const char *buf, int len);

    protected:
        /*! descrittore del socket (id del socket)*/
        int sockFd;
    };

    /*!
    Classe che realizza un server
    */
    class Server: public Socket
    {
    public:
        Server();
        virtual ~Server();

        /*! crea socket e si mette in ascolto*/
        int listen(sockaddr *serverAddr, int len, int backLog);
    };

    /*! converte l'indirizzo in byte del sockaddr in una stringa leggibile*/
    std::string IpAddressToString(sockaddr *addr, int addr_len);


}




#endif // SOCKET_UTIL_H
