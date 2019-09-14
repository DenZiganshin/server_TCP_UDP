#include "client.h"

#include <string>
#include <iostream>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

Client::Client():is_tcp(false), sock(-1), port(0)
{
}

bool Client::init(std::string ip, int port, bool is_tcp)
{
    struct sockaddr_in serv_addr;

    this->ip = ip;
    this->port = port;
    this->is_tcp = is_tcp;

    if( is_tcp ){
        if ((this->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            return false;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(this->port);
        if(inet_pton(AF_INET, this->ip.c_str(), &serv_addr.sin_addr)<=0)
        {
            return false;
        }

        if (connect(this->sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            return false;
        }
    }else{
        if ( (this->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            return false;
        }
    }

    return true;
}

int Client::send_data(const uint8_t* data, uint32_t size)
{
    if(!data){
        throw "no data";
        return -1;
    }
    int actual_send = -1;

    uint32_t net_size = htonl(size);
    if( this->is_tcp ){
        actual_send = send(this->sock , &net_size, sizeof(uint32_t), 0 );
        actual_send = send(this->sock , data, size, 0 );
    }else{
        struct sockaddr_in serv_addr;
        // Filling server information
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(this->port);
        if(inet_pton(AF_INET, this->ip.c_str(), &serv_addr.sin_addr)<=0)
        {
            return -1;
        }
        actual_send = sendto(this->sock, &net_size, sizeof(uint32_t), MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if(actual_send < (int)sizeof(uint32_t)){
            return -1;
            std::cout << "unable to send size" << std::endl;
        }
        actual_send = sendto(this->sock, data, size, MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if(actual_send < (int)size){
            return -1;
            std::cout << "unable to send data" << std::endl;
        }
    }
    return (int)size;
}

int Client::recv_data(uint8_t* buffer, uint32_t max_size)
{
    if(!buffer){
        return -1;
    }

    int actual_read = -1;
    uint32_t net_size = -1, size = -1;

    if( this->is_tcp ){
        actual_read = read( this->sock , &net_size, sizeof(uint32_t));
        if(actual_read<(int)sizeof(uint32_t)){
            return -1;
        }
        size = ntohl(net_size);
        if(size > max_size){
            return -1;
        }
        actual_read = read( this->sock , buffer, size);
        if(actual_read<(int)size){
            return -1;
        }
    }else{
        struct sockaddr_in serv_addr;
        //memsets
        socklen_t len = sizeof(serv_addr);
        actual_read  = recvfrom(this->sock, &net_size, sizeof(uint32_t), MSG_WAITALL, (struct sockaddr *) &serv_addr, &len);
        if(actual_read<(int)sizeof(uint32_t)){
            return -1;
        }
        size = ntohl(net_size);
        if(size > max_size){
            return -1;
        }

        actual_read  = recvfrom(this->sock, buffer, size, MSG_WAITALL, (struct sockaddr *) &serv_addr, &len);
        if(actual_read<(int)size){
            return -1;
        }
    }
    return (int)size;
}

void Client::disconnect()
{
    if(this->sock == -1)
        return;
    close(this->sock);
    this->sock = -1;
}
