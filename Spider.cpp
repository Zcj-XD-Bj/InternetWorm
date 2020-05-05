// Spider.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>

#include "Sipder_m.h"

int main()
{
	cout << "请输入URL（只支持https）：";
	string url;
	cin >> url;

	StartCatch(url);

	return 0;
}


void StartCatch(string url)
{
	queue<string> q;
	q.push(url);

	int flag = 0;

	while (!q.empty())
	{
		string cururl = q.front();
		q.pop();

		if (!Analyse(cururl))
		{
			cout << "url解析失败" << endl;
			flag = 1;
			continue;
		}

		if (!Connect())
		{
			cout << "连接服务器失败" << endl;
			flag = 1;
			continue;
		}

		if (!SSL_Connect())
		{
			cout << "建立SSL连接失败" << endl;
			continue;
		}

		string html;
		if (!Gethtml(html))
		{
			cout << "获取网页数据失败" << endl;
			continue;
		}

		cout << html;
	
		//网页解析
		smatch mat;
		regex rgx;
	}

	//释放SSL资源
	if (0 == flag)
	{
		SSL_shutdown(sslHandle);
		SSL_free(sslHandle);
		SSL_CTX_free(sslContext);
	}
	closesocket(g_sock);
	WSACleanup();
}

//解析URL
bool Analyse(string url)
{
	if (string::npos == url.find("https://"))	//查找“https//”位置
		return false;

	int pos = url.find('/', 8);	//查找下一个‘/’的位置
	if (string::npos == pos)	//没查到‘/’
	{
		g_Host = url.substr(8);	//取“从第8个字符开始到字符串最后”的子串
		g_Object = '/';	
	}
	else
	{
		g_Host = url.substr(8, pos - 8);	//截取“从第8个字符到第一个/”的子串
		g_Object = url.substr(pos);	//截取/之后的子串
	}

	return true;
}


bool Connect()
{
	//初始化套接字
	//https://docs.microsoft.com/en-us/windows/win32/api/winsock2/ns-winsock2-wsadata
	WSADATA wsadata;

	//The current version of the Windows Sockets specification is version 2.2
	//https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsastartup
	if (SOCKET_ERROR == WSAStartup(MAKEWORD(2, 2), &wsadata))
		return false;

	//创建未连接的套接字
	//https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
	g_sock = socket(AF_INET, SOCK_STREAM, 0);	//AF_INET表示ipv4；SOCK_STREAM表示TCP连接；第3个参数详见文档
	if (INVALID_SOCKET == g_sock)	//失败则返回INVALID_SOCKET
		return false;

	//将域名转换为ip地址
	//https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-gethostbyname
	//https://docs.microsoft.com/zh-cn/windows/win32/api/winsock/ns-winsock-hostent
	//hostent是host entry的缩写，该结构记录主机的信息，包括主机名、别名、地址类型、地址长度和地址列表。
	//之所以主机的地址是一个列表的形式，原因是当一个主机有多个网络接口时，自然有多个地址。
	hostent *p = gethostbyname(g_Host.c_str()); //c_str()将string转化为char*[]
	if (!p) return false;

	//https://blog.csdn.net/will130/article/details/53326740
	sockaddr_in sa;	
	memcpy(&sa.sin_addr, p->h_addr, 4);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(443);	//host to network
	
	//加入超时机制
	/*int nNetTimeout = 1;	//1s
	fd_set rfd;//
	timeval timeout;//
	FD_ZERO(&rfd);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	u_long ul = 1;//
	ioctlsocket(g_sock, FIONBIO, &ul);//
	connect(g_sock, (sockaddr*)&sa, sizeof(sockaddr));
	FD_SET(g_sock, &rfd);
	auto ret = select(0, 0, &rfd, 0, &timeout);
	if (ret <= 0)
	{
		cout << "网络连接超时，TimeOut！请设置更长的超时时间" << endl;
		return false;
	}
	ul = 0;
	ioctlsocket(g_sock, FIONBIO, &ul);*/

	//建立TCP连接
	//https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect
	if (SOCKET_ERROR == connect(g_sock, (sockaddr*)&sa, sizeof(sockaddr)))
		return false;
	return true;
}


//https://www.openssl.org/docs/manmaster/man3/
//https://blog.csdn.net/xs574924427/article/details/17240793
bool SSL_Connect()
{	
	ERR_load_BIO_strings();
	//SSL库初始化，载入SSL所有算法，载入所有SSL错误信息
	SSL_library_init();
	OpenSSL_add_ssl_algorithms();
	SSL_load_error_strings();

	//选择会话协议,创建会话环境
	sslContext = SSL_CTX_new(SSLv23_method());
	if (!sslContext)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}

	//建立SSL套接字
	sslHandle = SSL_new(sslContext);
	if (!sslHandle)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}

	//Connect the SSL struct to our connection
	//https://www.openssl.org/docs/manmaster/man3/SSL_set_fd.html
	if (0 == SSL_set_fd(sslHandle, g_sock))
	{
		ERR_print_errors_fp(stderr);
		return false;
	}

	//完成SSL握手
	if (SSL_connect(sslHandle) != 1)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}
	return true;
}


bool Gethtml(string &html)
{
	string info = "\0";
	info += "GET " + g_Object + " HTTP/1.1\r\n";
	info += "Host: " + g_Host + "\r\n";
//	info += "Content - Type: text / html; charset = UTF - 8\r\n";
	info += "Connection:Close\r\n\r\n";

	SSL_write(sslHandle, info.c_str(), info.length());

	char buf;
	//https://www.openssl.org/docs/manmaster/man3/SSL_read.html
	while (SSL_read(sslHandle, &buf, sizeof(buf)) > 0)
	{
		html += buf;
	}
	return true;
}


