#ifndef _CLIENT_H_
#define _CLIENT_H_
#include <iostream>
#include <utility>
#include <string>

class Client{
    friend std::iostream& operator>>(std::iostream&, const Client&);
 public:
    Client() { }
 private:
    std::string id, id_region;
    int serv_duration;
    std::pair<int, int> time_windows;
};
#endif
