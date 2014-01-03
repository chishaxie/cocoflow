#include "cocoflow-comm.h"

#if defined(_MSC_VER)
	#pragma warning(disable:4748)
#endif

namespace ccf {

/***** tools *****/

union addr_ip {
	struct sockaddr_in6 addr6;
	struct sockaddr_in  addr4;
};

struct sockaddr_in6 sockaddr_in_into_sockaddr_in6(const struct sockaddr_in& addr)
{
	union addr_ip uaddr;
	uaddr.addr4 = addr;
	return uaddr.addr6;
}

struct sockaddr_in sockaddr_in_outof_sockaddr_in6(const struct sockaddr_in6& addr)
{
	union addr_ip uaddr;
	uaddr.addr6 = addr;
	return uaddr.addr4;
}

struct sockaddr_in ip_to_addr(const char* ipv4, int port)
{
	return uv_ip4_addr(ipv4, port);
}

struct sockaddr_in6 ip_to_addr6(const char* ipv6, int port)
{
	return uv_ip6_addr(ipv6, port);
}

std::string ip_to_str(const struct sockaddr* addr)
{
	if (addr->sa_family == AF_INET)
		return ip_to_str(*reinterpret_cast<const struct sockaddr_in*>(addr));
	else if (addr->sa_family == AF_INET6)
		return ip_to_str(*reinterpret_cast<const struct sockaddr_in6*>(addr));
	else
		return std::string("Not IP Address");
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
		strcpy(str, "Illegal IPv4 Address");
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
		strcpy(str, "Illegal IPv6 Address");
	return std::string(str);
}

}
