

#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include <acl.h>
#include <atomic>


constexpr int BUFFER_SIZE = 8;

class TCPServer {
    int port;
    int max_clients;
    ACL acl{};
    std::atomic<bool> kill_acceptor{false};

    int server_socket;
    std::thread client_acceptor;
    static void accept_clients(std::atomic<bool> &kys, ACL &acl, int serverSocket);

public:
    TCPServer(int port, int max_clients);
    bool load_acl_from_disk(std::string path);
    bool start();
    void stop();

};




#endif //TCP_SERVER_H
