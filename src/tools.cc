#include "cocoflow-comm.h"

namespace ccf {

/***** tools *****/

struct sockaddr_in ip_to_addr(const char* ip, int port)
{
	return uv_ip4_addr(ip, port);
}

std::string ip_to_str(const struct sockaddr_in &addr)
{
	char str[32], tmp[10];
	if (uv_ip4_name(const_cast<struct sockaddr_in*>(&addr), str, sizeof(str)) == 0)
	{
		sprintf(tmp, ":%u", ntohs(addr.sin_port));
		strcat(str, tmp);
	}
	else
		strcpy(str, "Illegal Address");
	return std::string(str);
}

}
