#pragma once

#include <string>
#include <vector>

class HttpRequest
{
public:
    HttpRequest(const std::string& ip, int port);
    ~HttpRequest(void);

    // Http GETÇëÇó
    std::string httpGet(std::string req);

    // ·Ö¸î×Ö·û´®
    static std::vector<std::string> split(const std::string &s, const std::string &seperator);

private:
    std::string         mIp;
    int             mPort;
};
