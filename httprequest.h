#pragma once

#include <string>
#include <vector>

class HttpRequest
{
public:
    HttpRequest(const std::string& ip, int port);
    ~HttpRequest(void);

    // Http GET����
    std::string httpGet(std::string req);

    // �ָ��ַ���
    static std::vector<std::string> split(const std::string &s, const std::string &seperator);

private:
    std::string         mIp;
    int             mPort;
};
