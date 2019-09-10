#ifndef TESTTASK_SERVERH
#define TESTTASK_SERVERH

#include <string>
#include <vector>
#include <thread>
#include <mutex>

//namespace

class Server{
    static const int max_msg_size = 1024;
    static const int poll_timeout = 2000;
public:
    /**
     * @brief Server
     */
    Server();

    /**
     * @brief запустить сервер
     * @param ip - ip сервера
     * @param port - порт сервера
     * @return
     */
    bool start(std::string ip, int port);

    /**
     * @brief остановить сервер
     * @return
     */
    void stop();

    /**
     * @brief ожидать окончания работы сервера
     */
    void join();

    // use it to close server socket
    ~Server();
private:
    /**
     * @brief запустить сервер udp
     * @return
     */
    bool start_udp();
    /**
     * @brief запустить сервер tcp
     * @return
     */
    bool start_tcp();

    /**
     * @brief закрытие сокета
     * @param s - сокет
     */
    void close_sock(int &s);

    /**
     * @brief основной цикл сервера
     */
    void server_loop();

    /**
     * @brief работа выолняемая сервером в свободеное от запросов время
     */
    void server_task();

    /**
     * @brief обработка сообщения от клиетна по udp
     * @return
     */
    bool process_udp();

    /**
     * @brief обработка запроса соединения
     * @param fds - вектор с дескрипторами для poll
     * @return
     */
    bool process_new_tcp_connection(std::vector <struct pollfd> *fds);

    /**
     * @brief обработка сообщения от клиетна по tcp
     * @param fds - вектор с дескрипторами для poll
     * @param num - номер клиетна в fds
     * @return
     */
    bool process_tcp_client(std::vector <struct pollfd> *fds, int num);

    /**
     * @brief подготовка ответа на основе запроса
     * @param input - входная строка
     * @param outstr1 - ответ1
     * @param outstr2 - ответ2
     */
    void response(const std::string &input, std::string *outstr1, std::string *outstr2);
private:
    int sock_tcp, sock_udp;
    std::string ip;
    int port;
    std::vector<int> tcp_clients;
    std::mutex mtx_stop;
    bool is_stop;
    std::thread *thr_serv_loop;
};

#endif
