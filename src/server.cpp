#include "server.h"

#include <iostream>
#include <thread>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <mutex>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

Server::Server():sock_tcp(-1), sock_udp(-1), ip("127.0.0.1"), port(0), thr_serv_loop(nullptr)
{

}

bool Server::start(std::string ip, int port)
{
    bool res_tcp = false, res_udp = false;

    this->ip = ip;
    this->port = port;
    this->is_stop = false;

    //лямбда для потока(thread) сервера
    auto th_foo = [this]() ->void {
        this->server_loop();
    };


    // запуск tcp
    res_tcp = start_tcp();
    if(!res_tcp ){
        std::cout << "unable to start tcp" << std::endl;
        return false;
    }

    //запуск udp
    res_udp = start_udp();
    if( !res_udp ){
        std::cout << "unable to start udp" << std::endl;
        return false;
    }

    //поток с основным циклом
    this->thr_serv_loop = new std::thread(th_foo);
    return true;
}

void Server::stop()
{
    // устанавливаем флаг окончания
    this->mtx_stop.lock();
    this->is_stop = true;
    this->mtx_stop.unlock();
    // ожидаем завершения
    if( this->thr_serv_loop && this->thr_serv_loop->joinable()){
        this->thr_serv_loop->join();
        delete this->thr_serv_loop;
        this->thr_serv_loop = nullptr;
    }
}

void Server::join()
{
    //ожидаем завершения
    if( this->thr_serv_loop && this->thr_serv_loop->joinable()){
        this->thr_serv_loop->join();
    }
}

Server::~Server()
{
    //close server socket
    close_sock(sock_tcp);
    close_sock(sock_udp);
}

/**
 * @brief установить сокет в неблокирующий режим
 * @param sock - сокет
 * @return
 */
inline bool set_non_block(int sock)
{
    int flags = fcntl(sock,F_GETFL,0);
    if(flags == -1){
        return false;
    }
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    return true;
}


bool Server::start_udp()
{
    struct sockaddr_in address;

    if ( (sock_udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        return false;
    }

    std::memset(&address, 0, sizeof(address));
    address.sin_family    = AF_INET;
    if (inet_aton(this->ip.c_str(), &address.sin_addr) == 0) {
        return false;
    }
    address.sin_port = htons(this->port);

    //set non block
    if(!set_non_block(this->sock_udp)){
        close_sock(this->sock_udp);
        return false;
    }

    if ( bind(sock_udp, (struct sockaddr*) &address, sizeof(address)) < 0 )
    {
        close_sock(sock_udp);
        return false;
    }

    return true;
}

bool Server::start_tcp()
{
    struct sockaddr_in address;
    if ((sock_tcp = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        return false;
    }

    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    if (inet_aton(this->ip.c_str(), &address.sin_addr) == 0) {
        return false;
    }
    address.sin_port = htons( this->port );

    //set non block
    if(!set_non_block(sock_tcp)){
        close_sock(sock_tcp);
        return false;
    }


    if (bind(sock_tcp, (struct sockaddr*) &address, sizeof(address))<0)
    {
        close_sock(sock_tcp);
        return false;
    }

    if (listen(sock_tcp, 1) < 0)
    {
        close_sock(sock_tcp);
        return false;
    }


    return true;
}

void Server::close_sock(int &s)
{
    if(s != -1){
        close(s);
        s = -1;
    }
}

/**
 * @brief добавить сокет в список наблюдения для poll
 * @param fds - список наблюдения
 * @param sock - сокет
 */
inline void add_to_pool(std::vector <struct pollfd> &fds, int sock){
    struct pollfd poll_fd;
    poll_fd.fd = sock;
    poll_fd.events = POLLIN;
    fds.push_back(poll_fd);
}


bool Server::process_new_tcp_connection(std::vector <struct pollfd> &fds){
    struct sockaddr address;
    socklen_t addrlen = sizeof(address);
    int new_socket = -1;
    if ((new_socket = accept(this->sock_tcp, &address, &addrlen))<0)
    {
        return false;
    }

    //set non block
    if(!set_non_block(new_socket)){
        close_sock(new_socket);
        return false;
    }

    //add to poll
    add_to_pool(fds, new_socket);

    return true;
}

bool Server::process_udp(){
    int actual_read = -1;
    char buffer[this->max_msg_size] = {0};
    sockaddr_in from;

    actual_read  = recv_udp((uint8_t*) buffer, this->max_msg_size, &from);
    if( actual_read <= 0){
        std::cout << "udp: nothing to read" << std::endl;
    }else{
        std::cout << "udp: we got "<<actual_read<<" bytes : "<<buffer << std::endl;
        int actual_send = -1;
        std::string resp_str1, resp_str2;
        response(std::string(buffer), resp_str1, resp_str2);

        //part 1
        actual_send = send_udp(&from,(uint8_t*) resp_str1.c_str(), (uint32_t)resp_str1.length());
        if(actual_send == -1){
            std::cout << "unable to send response1" << std::endl;
        }

        //part 2
        actual_send = send_udp(&from,(uint8_t*) resp_str2.c_str(), (uint32_t)resp_str2.length());
        if(actual_send == -1){
            std::cout << "unable to send response2" << std::endl;
        }

    }
    return true;
}

bool Server::process_tcp_client(std::vector <struct pollfd> &fds, int num){
    char buffer[this->max_msg_size] = {0};
    int actual_read = -1;

    actual_read = recv_tcp(fds.at(num).fd,(uint8_t*) buffer, this->max_msg_size);
    if ( actual_read > 0){
        std::cout << "tcp: we got " << actual_read << " bytes: " << buffer << std::endl;
        int actual_send = -1;
        std::string resp_str1, resp_str2;
        response(std::string(buffer), resp_str1, resp_str2);

        //part 1
        actual_send = send_tcp(fds.at(num).fd,(uint8_t*) resp_str1.c_str(), (uint32_t)resp_str1.length());
        if(actual_send == -1){
            std::cout << "unable to send response1" << std::endl;
        }

        //part 2
        actual_send = send_tcp(fds.at(num).fd,(uint8_t*) resp_str2.c_str(), (uint32_t)resp_str2.length());
        if(actual_send == -1){
            std::cout << "unable to send response2" << std::endl;
        }
    }else{
        // удаляем сокет из списка наблюдаемых
        std::cout << "client disconnected or smth" << std::endl;
        close(fds.at(num).fd);
        fds.erase(fds.begin()+num);
    }

    return true;
}

void Server::server_loop()
{
    std::vector <struct pollfd> fds;

    //лямбда для добавения в список poll
    auto add_to_pool = [&](int sock) ->void {
        struct pollfd poll_fd;
        poll_fd.fd = sock;
        poll_fd.events = POLLIN;
        fds.push_back(poll_fd);
    };

    //add tcp server sock
    add_to_pool(this->sock_tcp);
    //add udp server sock
    add_to_pool(this->sock_udp);

    while( true ){
        if(this->mtx_stop.try_lock()){
            bool value = this->is_stop;
            this->mtx_stop.unlock();
            if(value){
                break;
            }
        }

        int ret = poll( fds.data(), fds.size(), poll_timeout);
        if ( ret == -1 ){
            //error poll
            std::cout << "dbg: poll error" << std::strerror(errno) << std::endl;
            break;
        }else if ( ret == 0 ){
            //timeout
        }else{
            //good
            for(std::vector<pollfd>::size_type i=0; i<fds.size(); i++){
                if( fds[i].revents & POLLIN ){
                    fds[i].revents = 0;
                    if( fds[i].fd == this->sock_tcp ){
                        if(!process_new_tcp_connection(fds)){
                            std::cout << "unable to process new tcp connection" << std::endl;
                            return;
                        }
                    }else if( fds[i].fd == this->sock_udp ){
                        if(!process_udp()){
                            std::cout << "unable to process udp" << std::endl;
                            return;
                        }
                    }else{
                        if(!process_tcp_client(fds, i)){
                            std::cout << "unable to process tcp clients" << std::endl;
                            return;
                        }
                    }
                }
            }
        }

        server_task();
    }
}

void Server::server_task()
{
    //std::cout << "do cool staff" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

void Server::response(const std::string &input, std::string &outstr1, std::string &outstr2)
{
    //parse
    std::stringstream inp_stream(input);
    std::string item;
    std::vector<int> values;
    while (std::getline(inp_stream, item, ' '))
    {
        int val;
        try{
            val = std::stoi(item);
        }catch(...){
            //unable to stoi
            continue;
        }
        values.push_back(val);
    }

    //sort
    std::sort(values.begin(), values.end());

    //sum
    int sum = std::accumulate(values.begin(), values.end(), 0);

    //out
    for(auto it=values.begin(); it!=values.end(); ++it){
        outstr1.append(std::to_string(*it));
        if(it+1 != values.end())
            outstr1 += " ";
    }
    outstr2.append(std::to_string(sum));
}

int Server::send_tcp(int fd, const uint8_t *data, uint32_t size)
{
    if(!data || (size > this->max_msg_size))
        return -1;

    uint32_t net_size = htonl(size);
    int actual_send = -1;
    actual_send = send(fd, &net_size, sizeof(uint32_t), 0);
    if( actual_send < (int)sizeof(uint32_t)){
        std::cout << "tcp: unable to send size" << std::endl;
        return -1;
    }
    actual_send = send(fd, data, size, 0);
    if( actual_send < (int)size){
        std::cout << "tcp: unable to send data" << std::endl;
        return -1;
    }

    return (int)size;
}

int Server::send_udp(struct sockaddr_in* dst, const uint8_t *data, uint32_t size)
{
    if(!data || !dst || (size > this->max_msg_size))
        return -1;

    uint32_t net_size = htonl(size);
    int actual_send = -1;
    actual_send = sendto(this->sock_udp, &net_size, sizeof(uint32_t), MSG_CONFIRM, (struct sockaddr*) dst, sizeof(struct sockaddr));
    if( actual_send < (int)sizeof(uint32_t)){
        std::cout << "udp: unable to send size" << std::endl;
        return -1;
    }
    actual_send = sendto(this->sock_udp, data, size, MSG_CONFIRM, (struct sockaddr*) (struct sockaddr*) dst, sizeof(struct sockaddr));
    if( actual_send < (int)size){
        std::cout << "udp: unable to send data" << std::endl;
        return -1;
    }

    return (int)size;
}

int Server::recv_tcp(int fd, uint8_t* buffer, uint32_t max_size)
{
    if(!buffer){
        return -1;
    }

    int actual_read = -1;
    uint32_t net_size = -1, size = -1;

    actual_read = read( fd, &net_size, sizeof(uint32_t));
    if(actual_read<(int)sizeof(uint32_t)){
        return -1;
    }
    size = ntohl(net_size);
    if(size > max_size){
        return -1;
    }
    actual_read = read( fd, buffer, size);
    if(actual_read<(int)size){
        return -1;
    }
    return (int)size;
}

int Server::recv_udp(uint8_t* buffer, uint32_t max_size, struct sockaddr_in *from)
{
    if(!buffer || !from){
        return -1;
    }

    int actual_read = -1;
    uint32_t net_size = -1, size = -1;
    socklen_t len = sizeof(*from);

    actual_read  = recvfrom(this->sock_udp, &net_size, sizeof(uint32_t), MSG_WAITALL, (struct sockaddr *) from, &len);
    if(actual_read<(int)sizeof(uint32_t)){
        return -1;
    }
    size = ntohl(net_size);
    if(size > max_size){
        return -1;
    }

    actual_read  = recvfrom(this->sock_udp, buffer, size, MSG_WAITALL, (struct sockaddr *) from, &len);
    if(actual_read<(int)size){
        return -1;
    }

    return (int)size;
}
