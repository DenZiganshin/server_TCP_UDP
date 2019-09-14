#ifndef TESTTASK_CLIENTH
#define TESTTASK_CLIENTH

#include <string>

class Client{

public:
    /**
     * @brief Client
     */
    Client();

    /**
     * @brief настройка
     * @param ip - ip адрес
     * @param port - порт
     * @param is_tcp - тип соединения (true - tcp, false - udp)
     * @return
     */
    bool init(std::string ip, int port, bool is_tcp);

    /**
     * @brief отправка данных
     * @param data - буфер с данными для отправки
     * @param size - количество байт
     * @return количество отправленных байт или -1
     */
    int send_data(const uint8_t* data, uint32_t size);

    /**
     * @brief получение данных
     * @param buffer - буфер для данных
     * @param max_size - максимальный размер
     * @return количество прочитанных байт или -1
     */
    int recv_data(uint8_t* buffer, uint32_t max_size);

    /**
     * @brief закрытие сокетов
     */
    void disconnect();
private:
    bool is_tcp;
    int sock;
    std::string ip;
    int port;
};

#endif
