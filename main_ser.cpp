#include <iostream>
#include <getopt.h>
#include <sstream>
#include <fstream>

#include "client.h"
#include "server.h"

using namespace std;

void usage(){
    cout << "usage:" << endl;
    cout << "i - ip" << endl << "p - port" << "h - this help" << endl;
}

int main(int argc, char **argv)
{
    int opt = -1;
    string ip = "127.0.0.1";
    int port = 0;
    std::shared_ptr<Server>server(new Server());

    if(argc < 2){
        usage();
        exit(EXIT_SUCCESS);
    }
    while ((opt = getopt(argc, argv, "i:p:r:f:hs")) != -1) {
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
        //help
        case 'h':
        default:
            usage();
            exit(EXIT_SUCCESS);
        }
    }

    bool result = server->start(ip, port);
    if( !result ){
        cout << "unable to start server with "<< ip << ":" << port << endl;
        exit(EXIT_FAILURE);
    }
    server->join();


    exit(EXIT_SUCCESS);
}
