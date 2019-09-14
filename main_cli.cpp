#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>

#include <arpa/inet.h>
#include <getopt.h>

#include "client.h"
#include "server.h"

using namespace std;

void usage(){
    cout << "usage:" << endl;
    cout << "client -i 127.0.0.1 -p 5555 -r udp [messgage]"<<endl;
    cout << "i - ip" << endl << "p - port" << endl << "r - protocol" << endl << "h - this help" << endl;
}

int main(int argc, char **argv)
{
    const int max_msg_size = 1024;

    int opt = -1;
    string ip = "127.0.0.1", message;
    int port = 0;
    bool is_tcp = false;
    std::shared_ptr<Client>client(new Client());

    if(argc < 2){
        usage();
        exit(EXIT_SUCCESS);
    }
    while ((opt = getopt(argc, argv, "i:p:r:hs")) != -1) {
        switch (opt) {
        //ip
        case 'i':
            ip = optarg;
            cout << "ip:" << ip << endl;
            break;
        //port
        case 'p':
            {
                string arg(optarg);
                try{
                    port = stoi(arg);
                }catch(...){
                    cout << "unable to parse this:" << optarg << endl;
                    exit(EXIT_FAILURE);
                }
                cout << "port:" << port << endl;
            }
            break;
        //protocol
        case 'r':            
            {
                string arg(optarg);
                if( arg == "tcp")
                    is_tcp = true;
                else if( arg == "udp")
                    is_tcp = false;
            }
            cout << "protocol:" << (is_tcp?"tcp":"udp") << endl;
            break;
        //help
        case 'h':
        default:
            usage();
            exit(EXIT_SUCCESS);
        }
    }

    for (int i = optind; i < argc; i++){
        message += argv[i];
        if(i+1 < argc)
            message += " ";
    }
    cout << "{" << message << "}" << endl;

    client->init(ip, port, is_tcp);
    client->send_data(message.c_str(), message.length());
    char buffer[max_msg_size] = {0};

    //part 1
    uint32_t net_size = 0;
    uint32_t size = 0;
    client->recv_data((char*)&net_size, sizeof(uint32_t) );
    size = ntohl(net_size);
    client->recv_data(buffer, size);
    cout << buffer<<" ";

    std::memset(buffer, 0, sizeof(buffer));

    //part 2
    client->recv_data((char*)&net_size, sizeof(uint32_t) );
    size = ntohl(net_size);
    client->recv_data(buffer, size);
    cout <<buffer<<endl;

    client->disconnect();

    exit(EXIT_SUCCESS);
}
