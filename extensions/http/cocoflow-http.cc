#include "cocoflow-comm.h"
#include "cocoflow-http.h"

#include <algorithm>

namespace ccf {

namespace http {

static char lowercase(char c)
{
	if (c >= 'A' && c <= 'Z')
		return c + ('a' - 'A');
	else
		return c;
}

static int url_parse(const char *url,
	std::string &protocol,
	std::string &user,
	std::string &password,
	std::string &host,
	int &port, //0 for none
	std::string &path,
	std::string &query,
	std::string &comment)
{
	protocol.clear();
	user.clear();
	password.clear();
	host.clear();
	port = 0;
	path.clear();
	query.clear();
	comment.clear();
	
	const char *p = url;
	
	/* protocol:// */
	
	while (*p && *p != ':')
	{
		if ((*p >= 'a' && *p <= 'z') ||
			(*p >= 'A' && *p <= 'Z') ||
			(*p >= '0' && *p <= '9') ||
			*p == '.' || *p == '+' || *p == '-')
			;
		else
			return -1; //协议非法字符
		p ++;
	}
	
	if (!*p || p == url)
		return -2; //无协议
	
	protocol.append(url, p);
	std::transform(protocol.begin(), protocol.end(), protocol.begin(), lowercase);
	
	if (*(p+1) != '/' || *(p+2) != '/')
		return -3; //缺少//
	p += 3;
	
	/* user:password@host:port */
	
	const char *user_bgn = NULL, *user_end = NULL;
	const char *pass_bgn = NULL, *pass_end = NULL;
	const char *host_bgn = NULL, *host_end = NULL;
	const char *port_bgn = NULL, *port_end = NULL;
	
	user_bgn = p;
	for (;; p++)
	{
		if (*p == '\0' || *p == '/' || *p == '?' || *p == '#') //host ending chars
		{
			if (user_bgn == p)
				user_bgn = NULL; //none
			else if (!user_end) //host
			{
				host_bgn = user_bgn;
				host_end = p;
				user_bgn = NULL;
			}
			else if (pass_bgn && !pass_end) //host:port
			{
				host_bgn = user_bgn;
				host_end = user_end;
				port_bgn = pass_bgn;
				port_end = p;
				user_bgn = NULL;
				pass_bgn = NULL;
			}
			else if (!host_end) //user@host || user:password@host
				host_end = p;
			else //user@host:port || user:password@host:port
				port_end = p;
			break;
		}
		else if (*p == ':')
		{
			if (!host_bgn)
			{
				if (!user_end)
					user_end = p;
				else
					return -4; //非法的:
				pass_bgn = p + 1;
			}
			else
			{
				if (!host_end)
					host_end = p;
				else
					return -5; //非法的:
				port_bgn = p + 1;
			}
		}
		else if (*p == '@')
		{
			if (!user_end)
				user_end = p;
			else if (pass_bgn && !pass_end)
				pass_end = p;
			else
				return -6; //非法的@
			host_bgn = p + 1;
		}
	}
	
	if (user_bgn && user_bgn != user_end)
		user.append(user_bgn, user_end);
	if (pass_bgn && pass_bgn != pass_end)
		password.append(pass_bgn, pass_end);
	if (host_bgn && host_bgn != host_end)
	{
		for (const char *i = host_bgn; i != host_end; i++)
		{
			if ((*i >= 'a' && *i <= 'z') ||
				(*i >= 'A' && *i <= 'Z') ||
				(*i >= '0' && *i <= '9') ||
				*i == '.' || *i == '_' || *i == '-')
				;
			else
				return -7; //host非法字符
		}
		host.append(host_bgn, host_end);
		std::transform(host.begin(), host.end(), host.begin(), lowercase);
	}
	if (port_bgn && port_bgn != port_end)
	{
		for (const char *i = port_bgn; i != port_end; i++)
		{
			if (*i >= '0' && *i <= '9')
				;
			else
				return -8; //port非法字符
		}
		port = atoi(port_bgn);
		if (port <= 0 || port > 65535)
			return -9; //port非法值
	}
	
	/* /path?query#comment */
	
	const char *path_bgn = NULL, *path_end = NULL;
	const char *qstr_bgn = NULL, *qstr_end = NULL;
	const char *note_bgn = NULL, *note_end = NULL;
	
	if (*p == '/')
		path_bgn = p;
	else if (*p == '?')
		qstr_bgn = p + 1;
	else if (*p == '#')
		note_bgn = p + 1;
	
	if (path_bgn)
	{
		for (;; p++)
		{
			if (*p == '\0' || *p == '?' || *p == '#')
			{
				path_end = p;
				if (*p == '?')
					qstr_bgn = p + 1;
				else if (*p == '#')
					note_bgn = p + 1;
				break;
			}
		}
		path.append(path_bgn, path_end);
	}
	if (path.empty())
		path.append("/");
	
	if (qstr_bgn)
	{
		for (;; p++)
		{
			if (*p == '\0' || *p == '#')
			{
				qstr_end = p;
				if (*p == '#')
					note_bgn = p + 1;
				break;
			}
		}
		query.append(qstr_bgn, qstr_end);
	}
	
	if (note_bgn)
	{
		for (;; p++)
		{
			if (*p == '\0')
			{
				note_end = p;
				break;
			}
		}
		comment.append(note_bgn, note_end);
	}
	
	return 0;
}

}

}
