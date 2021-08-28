#include "httprequest.h"
#include <WinSock.h>
#include <iostream>

HttpRequest::HttpRequest(const std::string& ip, int port) : mIp(ip), mPort(port)
{
}


HttpRequest::~HttpRequest(void)
{
}

// Http GET����
std::string HttpRequest::httpGet(std::string req)
{
    std::string ret = ""; // ����Http Response
    try
    {
        // ��ʼ����socket��ʼ��
        WSADATA wData;
        ::WSAStartup(MAKEWORD(2, 2), &wData);

        SOCKET clientSocket = socket(AF_INET, 1, 0);
        struct sockaddr_in ServerAddr{};
        ServerAddr.sin_addr.s_addr = inet_addr(mIp.c_str());
        ServerAddr.sin_port = htons(mPort);
        ServerAddr.sin_family = AF_INET;
        int errNo = connect(clientSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr));
        if(errNo == 0)
        {
            std::string strSend = " HTTP/1.1\r\n"
                                  "Host: www.chessdb.cn\r\n"
                                  "Cookie:16888\r\n\r\n";
            strSend = "GET " + req + strSend;

            // ����
            errNo = send(clientSocket, strSend.c_str(), strSend.length(), 0);
            if(errNo > 0)
            {
                //cout << "���ͳɹ�" << endl;
            }
            else
            {
                std::cout << "errNo:" << errNo << std::endl;
                return ret;
            }

            // ����
            char bufRecv[3069] = {0};
            errNo = recv(clientSocket, bufRecv, 3069, 0);
            if(errNo > 0)
            {
                ret = bufRecv;// ������ճɹ����򷵻ؽ��յ���������
            }
            else
            {
                std::cout << "errNo:" << errNo << std::endl;
                return ret;
            }
        }
        else
        {
            errNo = WSAGetLastError();
            std::cout << "errNo:" << errNo << std::endl;
        }
        // socket��������
        ::WSACleanup();
    }
    catch (...)
    {
        return "";
    }
    return ret;
}

// �ָ��ַ���
std::vector<std::string> HttpRequest::split(const std::string &s, const std::string &seperator)
{
    std::vector<std::string> result;
    typedef std::string::size_type string_size;
    string_size i = 0;

    while(i != s.size()){
        // �ҵ��ַ������׸������ڷָ�������ĸ
        int flag = 0;
        while(i != s.size() && flag == 0){
            flag = 1;
            for(string_size x = 0; x < seperator.size(); ++x)
                if(s[i] == seperator[x]){
                    ++i;
                    flag = 0;
                    break;
                }
        }

        // �ҵ���һ���ָ������������ָ���֮����ַ���ȡ��
        flag = 0;
        string_size j = i;
        while(j != s.size() && flag == 0){
            for(string_size x = 0; x < seperator.size(); ++x)
                if(s[j] == seperator[x]){
                    flag = 1;
                    break;
                }
            if(flag == 0)
                    ++j;
        }
        if(i != j){
            result.push_back(s.substr(i, j-i));
            i = j;
        }
    }
    return result;
}
