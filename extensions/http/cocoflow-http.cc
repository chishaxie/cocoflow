#include "cocoflow-comm.h"
#include "cocoflow-http.h"

#include <algorithm>

namespace ccf {

namespace http {

#define HTTP_DEFAULT_PORT        80
#define HTTP_FIELD_NAME_MAX_LEN  19

/* url parse */

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

/* http parse */

//RFC 2616
namespace header {

enum field {
	Unknown = 0,
	Accept,
	AcceptCharset,
	AcceptEncoding,
	AcceptLanguage,
	AcceptRanges,
	Age,
	Allow,
	Authorization,
	CacheControl,
	Connection,
	ContentEncoding,
	ContentLanguage,
	ContentLength,
	ContentLocation,
	ContentMD5,
	ContentRange,
	ContentType,
	Date,
	ETag,
	Expect,
	Expires,
	From,
	Host,
	IfMatch,
	IfModifiedSince,
	IfNoneMatch,
	IfRange,
	IfUnmodifiedSince,
	LastModified,
	Location,
	MaxForwards,
	Pragma,
	ProxyAuthenticate,
	ProxyAuthorization,
	Range,
	Referer,
	RetryAfter,
	Server,
	TE,
	Trailer,
	TransferEncoding,
	Upgrade,
	UserAgent,
	Vary,
	Via,
	Warning,
	WWWAuthenticate
};

typedef struct {
	const char *name;
	field value;
} field_seek_unit;

const static field_seek_unit field_seek_arrays[] = {
	{ "accept",               Accept             },
	{ "accept-charset",       AcceptCharset      },
	{ "accept-encoding",      AcceptEncoding     },
	{ "accept-language",      AcceptLanguage     },
	{ "accept-ranges",        AcceptRanges       },
	{ "age",                  Age                },
	{ "allow",                Allow              },
	{ "authorization",        Authorization      },
	{ "cache-control",        CacheControl       },
	{ "connection",           Connection         },
	{ "content-encoding",     ContentEncoding    },
	{ "content-language",     ContentLanguage    },
	{ "content-length",       ContentLength      },
	{ "content-location",     ContentLocation    },
	{ "content-md5",          ContentMD5         },
	{ "content-range",        ContentRange       },
	{ "content-type",         ContentType        },
	{ "date",                 Date               },
	{ "etag",                 ETag               },
	{ "expect",               Expect             },
	{ "expires",              Expires            },
	{ "from",                 From               },
	{ "host",                 Host               },
	{ "if-match",             IfMatch            },
	{ "if-modified-since",    IfModifiedSince    },
	{ "if-none-match",        IfNoneMatch        },
	{ "if-range",             IfRange            },
	{ "if-unmodified-since",  IfUnmodifiedSince  },
	{ "last-modified",        LastModified       },
	{ "location",             Location           },
	{ "max-forwards",         MaxForwards        },
	{ "pragma",               Pragma             },
	{ "proxy-authenticate",   ProxyAuthenticate  },
	{ "proxy-authorization",  ProxyAuthorization },
	{ "range",                Range              },
	{ "referer",              Referer            },
	{ "retry-after",          RetryAfter         },
	{ "server",               Server             },
	{ "te",                   TE                 },
	{ "trailer",              Trailer            },
	{ "transfer-encoding",    TransferEncoding   },
	{ "upgrade",              Upgrade            },
	{ "user-agent",           UserAgent          },
	{ "vary",                 Vary               },
	{ "via",                  Via                },
	{ "warning",              Warning            },
	{ "www-authenticate",     WWWAuthenticate    }
};

class field_seek_unit_comp
{
public:
	bool operator()(const field_seek_unit &a, const field_seek_unit &b) const
	{
		return strcmp(a.name, b.name) < 0;
	}
};

static field_seek_unit_comp field_comp;

static field check_field(const char *name)
{
	field_seek_unit target;
	target.name = name;
	const field_seek_unit *ret = std::lower_bound(field_seek_arrays,
		field_seek_arrays + sizeof(field_seek_arrays)/sizeof(field_seek_arrays[0]), target, field_comp);
	if (ret != field_seek_arrays + sizeof(field_seek_arrays)/sizeof(field_seek_arrays[0]) && strcmp(ret->name, name) == 0)
		return ret->value;
	else
		return Unknown;
}

}

static int http_rsp_parse(const void *buf, size_t len,
	const std::set<header::field> &needs,
	int &status_code,
	std::string &reason_phrase,
	std::map<header::field, std::string> &header_fields)
{
	status_code = 0;
	reason_phrase.clear();
	header_fields.clear();
	
	const char *s = reinterpret_cast<const char *>(buf);
	size_t i = 0;
	size_t bgn, end;
	
	/* Status-Line */
	
	//HTTP-Version SP
	if (len < 9)
		return -1;
	if (memcmp("HTTP/1.1 ", &s[i], 9) != 0)
		return -1;
	i += 9; //HTTP-Version SP
	
	//Status-Code SP
	bgn = i;
	for (; i < len; i++)
	{
		if (s[i] >= '0' && s[i] <= '9')
			;
		else if (s[i] == ' ')
			break;
		else
			return -2;
	}
	if (i != bgn + 3 || i == len) //固定三字节
		return -2;
	status_code = atoi(&s[bgn]);
	i ++; //SP
	
	//Reason-Phrase CRLF
	bgn = i;
	for (; i + 1 < len; i++)
	{
		if (s[i] == '\r' && s[i+1] == '\n')
			break;
		else if ((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z') || s[i] == ' ' || s[i] == '-')
			;
		else
			return -3;
	}
	if (i == bgn || i + 1 == len)
		return -3;
	reason_phrase.append(bgn, i);
	i += 2; //CRLF
	
	/* Response Header Fields */
	for (;;)
	{
		if (i + 1 < len)
		{
			//CRLF (last)
			if (s[i] == '\r' && s[i+1] == '\n')
				break;
			
			char field_name[HTTP_FIELD_NAME_MAX_LEN + 1];
			
			//Key
			bgn = i;
			for (; i < len; i++)
			{
				if ((s[i] >= '0' && s[i] <= '9') ||
					(s[i] >= 'a' && s[i] <= 'z') ||
					(s[i] >= 'A' && s[i] <= 'Z') ||
					s[i] == '-')
					;
				else if (s[i] == ':')
					break;
				else
					return -5;
			}
			end = i;
			if (end == bgn)
				return -5;
			if (end - bgn <= HTTP_FIELD_NAME_MAX_LEN)
			{
				memcpy(field_name, &s[bgn], end - bgn);
				field_name[end - bgn] = '\0';
				std::transform(field_name, field_name + end - bgn, field_name, lowercase);
			}
			else
				field_name[0] = '\0';
			i ++; //:
			
			//Value
			for (; i < len; i++) //去除value的前缀空格
			{
				if (s[i] == ' ')
					;
				else
					break;
			}
			bgn = i;
			for (; i + 1 < len; i++)
			{
				if (s[i] == '\r' && s[i+1] == '\n')
					break;
				else
					;
			}
			end = i;
			while (end > bgn && s[end-1] == ' ') //去除value的后缀空格
				;
			i += 2; //CRLF
			
			//Put into header_fields
			if (field_name[0] != '\0')
			{
				header::field cur = header::check_field(field_name);
				if (cur != header::Unknown)
				{
					switch (cur)
					{
					case header::ContentLength:
						if (end > bgn)
						{
							for (size_t j = bgn; j < end; j++)
								if (s[j] >= '0' && s[j] <= '9')
									;
								else
									return -6;
						}
						else
							return -6;
						break;
					case header::TransferEncoding:
						if (end - bgn == 7 && memcmp("chunked", &s[bgn], 7) == 0)
							;
						else
							return -6;
						break;
					default:
						break;
					}
					if (needs.count(cur) && end > bgn)
					{
						if (!header_fields.insert(std::pair<header::field, std::string>(cur, std::string(bgn, cur))).second)
							return -7;
					}
				}
			}
		}
		else
			return -4;
	}
	i += 2; //CRLF
	if (i != len)
		return -4;
	
	return 0;
}

/* http::get */

get::get(int &ret, const char **errmsg, const char *url, void *buf, size_t &len)
	: ret(ret), errmsg(errmsg), url(url), buf(buf), len(len)
{
	CHECK(this->url != NULL);
	this->ret = unfinished;
	if (this->errmsg)
		*this->errmsg = NULL;
}

void get::run()
{
	int ret;
	const char *errmsg;
	
	/* url parse */
	
	std::string protocol;
	std::string user;
	std::string password;
	std::string host;
	int port;
	std::string path;
	std::string query;
	std::string comment;
	
	ret = url_parse(this->url, protocol, user, password, host, port, path, query, comment);
	if (ret)
	{
		this->ret = err_url_parse;
		if (this->errmsg)
		{
			switch (ret)
			{
			case -1:
				*this->errmsg = "Illegal character in protocol";
				break;
			case -2:
				*this->errmsg = "Missing protocol";
				break;
			case -3:
				*this->errmsg = "Missing \"//\"";
				break;
			case -4:
			case -5:
				*this->errmsg = "Illegal \":\"";
				break;
			case -6:
				*this->errmsg = "Illegal \"@\"";
				break;
			case -7:
				*this->errmsg = "Illegal character in host";
				break;
			case -8:
				*this->errmsg = "Illegal character in port";
				break;
			case -9:
				*this->errmsg = "Illegal value in port";
				break;
			default:
				*this->errmsg = "Failed in url parse";
				break;
			}
		}
		return;
	}
	
	if (protocol != "http")
	{
		this->ret = err_url_parse;
		if (this->errmsg)
			*this->errmsg = "Only supported protocol \"http\"";
		return;
	}
	
	if (!user.empty() || !password.empty())
	{
		this->ret = err_url_parse;
		if (this->errmsg)
			*this->errmsg = "Unsupported user/password";
		return;
	}

	if (host.empty())
	{
		this->ret = err_url_parse;
		if (this->errmsg)
			*this->errmsg = "Missing host";
		return;
	}
	
	/* dns resolve */
	
	bool is_ipv4;
	struct sockaddr_in ipv4;
	struct sockaddr_in6 ipv6;
	
	{
		struct addrinfo *result;
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC; //Allow IPv4 or IPv6
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		getaddrinfo dns(ret, &result, &errmsg, host.c_str(), NULL, &hints);
		await(dns);
		if (ret)
		{
			this->ret = err_dns_resolve;
			if (this->errmsg)
			{
				if (errmsg && errmsg[0] != '\0')
					*this->errmsg = errmsg;
				else
					*this->errmsg = "Failed in dns resolve";
			}
			return;
		}
		
		CHECK(result != NULL);
		CHECK(result->ai_addr != NULL);
		
		if (result->ai_addr->sa_family == AF_INET)
		{
			is_ipv4 = true;
			ipv4 = *reinterpret_cast<struct sockaddr_in *>(result->ai_addr);
			ipv4.sin_port = htons(port? port: HTTP_DEFAULT_PORT);
		}
		else if (result->ai_addr->sa_family == AF_INET6)
		{
			is_ipv4 = false;
			ipv6 = *reinterpret_cast<struct sockaddr_in6 *>(result->ai_addr);
			ipv6.sin6_port = htons(port? port: HTTP_DEFAULT_PORT);
		}
		else
			CHECK(0);
		
		getaddrinfo::freeaddrinfo(result);
	}

	/* connect */
	
	tcp::connected sock;
	if (is_ipv4)
	{
		tcp::connect conn(ret, sock, ipv4);
		await(conn);
	}
	else
	{
		tcp::connect conn(ret, sock, ipv6);
		await(conn);
	}
	if (ret != tcp::success)
	{
		this->ret = err_tcp_connect;
		if (this->errmsg)
			*this->errmsg = "Failed in tcp connect";
		return;
	}
	
	/* send */
	{
		char header[4096];
		int bytes;
		if (port)
			bytes = snprintf(header, sizeof(header),
				"GET %s HTTP/1.1\r\n"
				"Host: %s:%d\r\n"
				"\r\n",
				path.c_str(), host.c_str(), port);
		else
			bytes = snprintf(header, sizeof(header),
				"GET %s HTTP/1.1\r\n"
				"Host: %s\r\n"
				"\r\n",
				path.c_str(), host.c_str());
		CHECK(bytes > 0 && bytes <= (int)sizeof(header));
		ccf::tcp::send s(ret, sock, header, bytes);
		await(s);
		if (ret != tcp::success)
		{
			this->ret = err_send_request;
			if (this->errmsg)
				*this->errmsg = "Failed in http send header";
			return;
		}
	}
	
	/* recv */
	{
		char header[4096];
		size_t bytes = sizeof(header);
		ccf::tcp::recv_till rt(ret, sock, header, bytes, "\r\n\r\n", 4);
		await(rt);
		if (ret != tcp::success)
		{
			this->ret = err_recv_response;
			if (this->errmsg)
				*this->errmsg = "Failed in http recv header";
			return;
		}
		
		//TODO
		header[bytes] = '\0';
		printf("%s", header);
		
		//http response parse
		std::set<header::field> needs;
		int status_code;
		std::string reason_phrase;
		std::map<header::field, std::string> header_fields;
		
		needs.insert(header::ContentLength);
		needs.insert(header::TransferEncoding);
		
		ret = http_rsp_parse(header, bytes, needs, status_code, reason_phrase, header_fields);
		if (ret)
		{
			this->ret = err_response_header_parse;
			if (this->errmsg)
			{
				switch (ret)
				{
				case -1:
					*this->errmsg = "Illegal http version";
					break;
				case -2:
					*this->errmsg = "Illegal status code";
					break;
				case -3:
					*this->errmsg = "Illegal reason phrase";
					break;
				case -4:
					*this->errmsg = "Illegal header end";
					break;
				case -5:
					*this->errmsg = "Illegal field key";
					break;
				case -6:
					*this->errmsg = "Illegal field value";
					break;
				case -7:
					*this->errmsg = "Duplicate field";
					break;
				default:
					*this->errmsg = "Failed in http response parse";
					break;
				}
			}
			return;
		}
		
		if (!header_fields.count(header::ContentLength) && !header_fields.count(header::TransferEncoding))
		{
			this->ret = err_response_header_parse;
			if (this->errmsg)
				*this->errmsg = "Missing \"Content-Length\"/\"Transfer-Encoding\"";
			return;
		}
	}
}

void get::cancel()
{
	if (this->errmsg)
		*this->errmsg = "It was canceled";
}

get::~get()
{
}

}

}
