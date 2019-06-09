/* All entity class, including Host, Url, etc.
 * Create Date: 2013/06/04
*/
#ifndef ENTITY_H
#define ENTITY_H

#include<sys/epoll.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<list>
using namespace std;

const int DEFAULT_DEPTH = 3;

class Host;

class Url
{
public:
    Url(char* hostname, char* filename, ushort port, int depth, Host* host);
    ~Url();
    char* get_host_name();
    char* get_file_name();
    ushort get_port();
    Host* get_host();
    uint hash_code();
    uint host_hash_code();
    void print();
    bool parse_response(char* buff);
    // If the url has been processed, return true, otherwise, return false.
    bool process();
    static Url* gen_url(char* str, int depth = DEFAULT_DEPTH, Host* host = NULL); //return new Url if str is correct url format.
protected:
    char* m_host_name;
    char* m_file_name;
    ushort m_port;
    int m_depth;   // whether go deeper.
    Host* m_host;   // Pointer to g_host_array item, never destroy it!
};

enum FileType
{
    INVALID_FILE = -1,
    ROBOTS_FILE = 0,
    HTML_FILE
};

class FdNode
{
public:
    FdNode();
    ~FdNode();
    static FdNode* gen_fdnode(short type, void* obj);
    bool call_back();
    bool send_request();  // write request to fd.
    bool recv_response(); // read response from fd.
    int get_fd();
    void set_revents(u_int32_t revents);
    u_int32_t get_revents();
protected:
    int m_fd;
    u_int32_t m_revents;  // return status: POLLHUP for connecting
    short m_type;         // file type: currently support robots/html
    void* m_obj;          // Pointer to Host or Url
};

class Host
{
public:
    Host();
    // Only when the m_urls is empty could this Host be init.
    void init(Url* u);
    void clear();
    bool is_valid();
    char* get_host_name();
    void set_sin_addr(struct in_addr* addr_p);
    struct in_addr* get_sin_addr();
    void set_cname(char* str);
    char* get_cname();
    ushort get_port();
    void add_url(Url* u);
    void request_robots();
    bool parse_response(char* buff);
    bool process_urls();
    bool process(Url* u);
protected:
    char* m_host_name;            // Host name
    char* m_cname;           // cname for adns
    ushort m_port;           // Port number
    struct in_addr m_sin_addr;    // Internet address
    list<char*> m_forbiddens;    // Forbidden file list by robots.txt
    list<Url*> m_urls;           // File queue to be accessed when DNSed.
    bool m_valid;                // Whether this host is DNSed.
};

bool write_all(int fd, char* buff); // write all in buff to socket fd.
bool read_all(int fd, char* buff, uint max_len); // read all from socket fd into buff.
int parse_header(char* buff);

#endif //ENTITY_H
