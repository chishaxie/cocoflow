#include "cocoflow-comm.h"

namespace ccf {

/***** tools *****/

struct sockaddr_in6 sockaddr_in_into_sockaddr_in6(const struct sockaddr_in& addr)
{
	struct sockaddr_in6 addr6;
	memcpy(&addr6, &addr, sizeof(struct sockaddr_in));
	return addr6;
}

struct sockaddr_in sockaddr_in_outof_sockaddr_in6(const struct sockaddr_in6& addr)
{
	struct sockaddr_in addr4;
	memcpy(&addr4, &addr, sizeof(struct sockaddr_in));
	return addr4;
}

struct sockaddr_in ip_to_addr(const char* ipv4, int port)
{
	return uv_ip4_addr(ipv4, port);
}

struct sockaddr_in6 ip_to_addr6(const char* ipv6, int port)
{
	return uv_ip6_addr(ipv6, port);
}

std::string ip_to_str(const struct sockaddr_in& addr)
{
	char str[112], tmp[16];
	if (uv_ip4_name(const_cast<struct sockaddr_in*>(&addr), str, sizeof(str)) == 0)
	{
		sprintf(tmp, ":%u", ntohs(addr.sin_port));
		strcat(str, tmp);
	}
	else
		strcpy(str, "Illegal Address");
	return std::string(str);
}

std::string ip_to_str(const struct sockaddr_in6& addr)
{
	char str[112], tmp[16];
	if (uv_ip6_name(const_cast<struct sockaddr_in6*>(&addr), str+1, sizeof(str)-1) == 0)
	{
		str[0] = '[';
		sprintf(tmp, "]:%u", ntohs(addr.sin6_port));
		strcat(str, tmp);
	}
	else
		strcpy(str, "Illegal Address");
	return std::string(str);
}

}
