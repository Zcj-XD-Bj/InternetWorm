#pragma once

//套接字
#include <WinSock2.h>

#include <queue>

//正则表达式
#include <regex>

//SSL解密
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "libeay32.lib" )
#pragma comment(lib, "ssleay32.lib" )

#pragma comment(lib, "WS2_32.lib")

using namespace std;

string g_Host;
string g_Object;

SOCKET g_sock;

SSL_CTX *sslContext;
SSL *sslHandle;

bool Analyse(string url);
bool Connect();
bool SSL_Connect();
string Utf8ToGbk(const char *);
bool Gethtml(string &html);
void StartCatch(string);