#include<unistd.h>
#include<string.h>
#include<ctype.h>
#include<errno.h>
#include<fcntl.h>
#include "entity.h"
#include "util/util.h"
#include "global.h"

//////////////////////////Clase: Url//////////////////////////////////
Url::Url(char* hostname, char* filename, ushort port, int depth, Host* host)
{
    this->m_host_name = hostname;
    this->m_file_name = filename;
    this->m_port = port;
    this->m_depth = depth;
    this->m_host = host;
}

Url::~Url()
{
    if (this->m_host_name)
    {
        delete[] this->m_host_name;
        this->m_host_name = NULL;
    }
    if (this->m_file_name)
    {
        delete[] this->m_file_name;
        this->m_file_name = NULL;
    }
    if (this->m_host)
        this->m_host = NULL;
}

char* Url::get_host_name()
{
    return this->m_host_name;
}

char* Url::get_file_name()
{
    return this->m_file_name;
}

ushort Url::get_port()
{
    return this->m_port;
}

Host* Url::get_host()
{
    return this->m_host;
}

void Url::print()
{
    // For debug.
    printf("Host: %s, Port: %d, File: %s, Depth: %d\n", 
        this->m_host_name, this->m_port, this->m_file_name, this->m_depth);
}

bool Url::parse_response(char* buff)
{
    // TODO parser html
    int code = parse_header(buff);
    if (code == 2)
    {
        return true;
    }
    return false;
}

bool Url::process()
{
    // TODO process current URL.
    if (this->m_host == NULL)
    {
        this->m_host = Global::g_host_array + host_hash_code();
    }
    if (this->m_host->is_valid())
    {
        // Already be DNSed.
        if (strcmp(this->m_host->get_host_name(), this->m_host_name) == 0)
            this->m_host->process(this);
        else
            this->m_host->init(this);
    }
    else
    {
        // Not yet be DNSed.
        if (!this->m_host->get_host_name())
            this->m_host->init(this);
        else if(strcmp(this->m_host->get_host_name(), this->m_host_name) == 0)
            this->m_host->add_url(this);
        else
            return false;
    }
    return true;
}

uint Url::hash_code()
{
    char* full = Util::joint(this->m_host_name, this->m_file_name);
    uint code = HashTable::hash_code(full, HASHTABLE_BIT_SIZE, this->m_port);
// For debug.
printf("Url_hash_code: %d\n", code);
    delete[] full;
    return code;
}

uint Url::host_hash_code()
{
    // Different Port takes as different host. This should be minor case.
    return HashTable::hash_code(this->m_host_name, MAX_HOST_BIT_SiZE, this->m_port);
}

Url* Url::gen_url(char* str, int depth, Host* host)
{
    // Judge whether str is a valid url.
    char *p, *q;
    if ((p = strstr(str, "://")))
    {
        // ONLY support http protocal so far.
        if (strncmp(str, "http", 4) != 0)
            return NULL;
        p += 3;
    }
    else
        p = str;
    // beginning of hostname
    if (!*p)
        return NULL;
    q = p;
    while(*q && *q != '/' && *q != ':') 
    {
        // To Lower
        *q = tolower(*q);
        q ++;
    }
    if (q == p)
        return NULL;
    char *hostname = Util::sdup(p, q-p);
    
    uint port = 80;
    if (*q == ':')
    {
        // Port num
        port = 0;
        while(*(++q) && isdigit(*q))
        {
            port = port * 10 + *q - '0';
        }
    }
    
    if (*q != '/')
    {
        // file name default to be "/".
        return new Url(hostname, Util::sdup("/"), port, depth, host);
    }
    else
    {
        p = q;
        // TODO need to consider [;parameters][?query] and . / .. case.
        while(*(++q) && *q != '#')
        {
            *q = tolower(*q);
        }
        return new Url(hostname, Util::sdup(p, q-p), port, depth, host);
    }
}

//////////////////////////Clase: FdNode////////////////////////////////
FdNode::FdNode()
{
    this->m_type = INVALID_FILE;
    this->m_obj = NULL;
}

FdNode::~FdNode()
{
    // Need to delete Url obj.
    if (this->m_type == HTML_FILE && this->m_obj != NULL)
    {
        Url* u = (Url*)this->m_obj;
        delete u;
        this->m_obj == NULL;
    }
    this->m_type = INVALID_FILE;
}

FdNode* FdNode::gen_fdnode(short type, void* obj)
{
    Host* host = NULL;
    switch(type)
    {
    case ROBOTS_FILE:
        host = (Host*)obj;
        break;
    case HTML_FILE:
        host = ((Url*)obj)->get_host();
        break;
    default:
        return NULL;
    }
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        perror("socket failed.");
        exit(errno);
    }
    
    fcntl(fd, F_SETFL, O_NONBLOCK);
    struct sockaddr_in addr;
    struct sockaddr* addr_p;
    if (Global::g_proxy)
    {
        addr_p = (struct sockaddr*)Global::g_proxy;
    }
    else
    {
        addr.sin_family = AF_INET;
        memcpy(&addr.sin_addr, host->get_sin_addr(), sizeof(struct in_addr));
        addr.sin_port = htons(host->get_port());
        addr_p = (struct sockaddr*)&addr;
    }
    
    FdNode* fdnode = NULL;
    if (connect(fd, addr_p, sizeof(struct sockaddr)) == 0)
    {
        // Success connected.
        fdnode = new FdNode();
        fdnode->m_revents = EPOLLOUT;
    }
    else if (errno == EINPROGRESS)
    {
        // Pending to finish connection.
        fdnode = new FdNode();    
        fdnode->m_revents = EPOLLHUP;
    }
    else
    {
        // Failure connection.
        close(fd);
        perror("connect fail.");
        return NULL;
    }
    fdnode->m_fd = fd;
    fdnode->m_type = type;
    fdnode->m_obj = obj;
    return fdnode;
}

bool FdNode::call_back()
{
    bool ret = true;
    switch(this->m_revents)
    {
    case EPOLLHUP:
        // TODO Add fd to g_epoll_fd, waiting to write.
        this->m_revents = EPOLLOUT;
        Global::ctl_fd(EPOLL_CTL_ADD, this);
printf("Wait a minute, fd: %d\n", this->m_fd);
        break;
    case EPOLLOUT:
        // TODO Send out request and add fd to g_epoll_fd, waiting to read.
        ret = send_request();
        if (ret)
        {
            this->m_revents = EPOLLIN;
            Global::ctl_fd(EPOLL_CTL_MOD, this);
        }
        break;
    case EPOLLIN:
        // TODO Read all and remove fd from g_epoll_fd, close fd
        recv_response();
        Global::ctl_fd(EPOLL_CTL_DEL, this);
        close(this->m_fd);
        delete this;
        break;
    }
    return ret;
}

/* Http request: GET /filename HTTP/1.1\r\nHost:hostname\r\n\r\n\r\n
Host must be given or will get a "bad request" response,
and the last \r\n\r\n are also required!
*/
bool FdNode::send_request()
{
    char* file_name = NULL;
    Host* host = NULL;
    ushort port;
    if (this->m_type == ROBOTS_FILE)
    {
        file_name = "/robots.txt";
        host = (Host*)this->m_obj;
        port = host->get_port();
    }
    else if (this->m_type == HTML_FILE)
    {
        Url* u = (Url*)this->m_obj;
        file_name = u->get_file_name();
        host = u->get_host();
        port = u->get_port();
    }
    else
        return false;
    char* host_name = host->get_host_name();
    char* buff = new char[2 * strlen(host_name) + strlen(file_name) + 256];
    strcpy(buff, "GET ");
    if (Global::g_proxy)
    {
        sprintf(buff+strlen(buff), "http://%s:%u", host_name, port);
    }
    sprintf(buff+strlen(buff), "%s HTTP/1.1\r\nHost: %s\r\n\r\n\r\n", file_name, host_name);
printf("HTTP header: %s\n", buff);
    bool ret = write_all(this->m_fd, buff);
    delete[] buff;
    return ret;
}

bool FdNode::recv_response()
{
    char* buff = Global::g_large_buff;
    memset(buff, 0, MAX_BUFF_SIZE);
    bool ret = read_all(this->m_fd, buff, MAX_BUFF_SIZE);
    if (!ret)
        return ret;
    FILE* f = fopen("1.txt", "a");
    fputs(buff, f);
    fclose(f);
    if (this->m_type == ROBOTS_FILE)
    {
        Host* host = (Host*)this->m_obj;
        ret = host->parse_response(buff);
        //Process m_urls.
        host->process_urls();
    }
    else if (this->m_type == HTML_FILE)
    {
        Url* u = (Url*)this->m_obj;
        ret = u->parse_response(buff);
    }
    return ret;
}

int FdNode::get_fd()
{
    return this->m_fd;
}

void FdNode::set_revents(u_int32_t revents)
{
    this->m_revents = revents;
}

u_int32_t FdNode::get_revents()
{
    return this->m_revents;
}

//////////////////////////Clase: Host//////////////////////////////////
Host::Host()
{
    this->m_host_name = NULL;
    this->m_cname = NULL;
    this->m_valid = false;
}

void Host::clear()
{
    process_urls();
    
    if (this->m_host_name)
    {
        delete[] this->m_host_name;
        this->m_host_name = NULL;
    }
    
    if (this->m_cname)
    {
        delete[] this->m_cname;
        this->m_cname = NULL;
    }
    while(!this->m_forbiddens.empty())
    {
        delete[] this->m_forbiddens.front();
        this->m_forbiddens.pop_front();
    }
    this->m_valid = false;
}

void Host::init(Url* u)
{
    clear();
    this->m_host_name = Util::sdup(u->get_host_name());
    this->m_port = u->get_port();
    this->m_urls.push_back(u);
    if (Global::g_proxy)
    {
        request_robots();
    }
    else
    {
        if (inet_aton(this->m_host_name, &this->m_sin_addr))
            request_robots();
        else
            Global::add_adns_request(this);
    }
}

void Host::request_robots()
{
    if (Global::g_proxy == NULL)
        printf("Ip for %s is %s\n", this->m_host_name, inet_ntoa(this->m_sin_addr));
    // TODO to fetch robots.txt
    FdNode* fdnode = FdNode::gen_fdnode(ROBOTS_FILE, (void*)this);
    if (fdnode == NULL)
    {
        clear();
        return;
    }
    
    if (!fdnode->call_back())
        clear();
}

bool Host::parse_response(char* buff)
{
    int code = parse_header(buff);
    // No matter whether is a robots.txt
    this->m_valid = true;
    if (code == 2)
    {
        char* p = strstr(buff, "User-agent: ");
        if (p)
            p = strtok(p, " ");
        bool match = false;
        while(p)
        {
            while (*p && (*p == '\r' || *p == '\n')) p ++;
            if (!*p) break;
            char* q = strtok(NULL, "\r");
            if (strcmp(p, "User-agent:") == 0)
            {
                // Only need to match once.
                if (!match)
                {
                    if (q && Util::cmp_wild(q, g_prog_name))
                        match = true;
                }
                else
                    break;
            }
            else if (strcmp(p, "Disallow:") == 0)
            {
                if (match && q && *q)
                {
printf("Disallow: %s\n", q);
                    char* forbid = Util::sdup(q);
                    this->m_forbiddens.push_back(forbid);
                }
            }
            else
            {
                // Other attribute, ignore.
            }
            p = strtok(NULL, " ");
        }
        return true;
    }
    return false;
}

bool Host::process_urls()
{
    while (!this->m_urls.empty())
    {
        if (this->m_valid)
            this->process(this->m_urls.front());
        this->m_urls.pop_front();
    }
}

bool Host::process(Url* u)
{   
    // Test whether it is forbidden.
    bool match = false;
    char* filename = u->get_file_name();
    for(list<char*>::iterator it = this->m_forbiddens.begin(); it != this->m_forbiddens.end(); ++it)
    {
        if (Util::cmp_wild(*it, filename))
        {
printf("Matched: %s, %s\n", *it, filename);
            match = true;
        }
    }
    if (match)
    {
        // this url is forbidden.
        delete u;
    }
    else
    {
        // Send request.
        FdNode* fdnode = FdNode::gen_fdnode(HTML_FILE, (void*)u);
        if (fdnode == NULL)
            delete u;
        else if (!fdnode->call_back())
            delete u;
    }
    return true;
}

char* Host::get_host_name()
{
    return this->m_host_name;
}

void Host::set_sin_addr(struct in_addr* addr_p)
{
    memcpy(&this->m_sin_addr, addr_p, sizeof(struct in_addr));
}

struct in_addr* Host::get_sin_addr()
{
    return &this->m_sin_addr;
}

void Host::set_cname(char* str)
{
    if (this->m_cname)
        delete[] this->m_cname;
    this->m_cname = Util::sdup(str);
}

char* Host::get_cname()
{
    return this->m_cname;
}

ushort Host::get_port()
{
    return this->m_port;
}

bool Host::is_valid()
{
    return this->m_valid;
}

void Host::add_url(Url* u)
{
    this->m_urls.push_back(u);
}

//////////////////////////Functions//////////////////////////////////
bool write_all(int fd, char* buff)
{
    int to_write = strlen(buff);
    int written = 0;
    char* p = buff;
printf("Writing %s to %d\n", buff, fd);
    while(to_write > 0)
    {
        written = write(fd, p, to_write);
        if (written <= 0)
        {
            if (errno == EINTR)
            {
                // interrupt, continue.
                written = 0;
                continue;
            }
            else
            {
                // Other error, have to quit.
                perror("write fail");
                return false;
            }
        }
        to_write -= written;
        p += written;
    }
printf("Write all successfully\n");
    return true;
}

bool read_all(int fd, char* buff, uint max_len)
{
    int to_read = max_len;
    int has_read = 0;
    char* p = buff;
    bool last_read = true;
    while(to_read > 0)
    {
        has_read = read(fd, p, to_read);
printf("Each time read: %d\n", has_read);
        if (has_read < 0)
        {
printf("Error occurs: %d\n", errno);
            if (errno == EINTR)
            {
                has_read = 0;
                continue;
            }
            else if (errno == EAGAIN)
            {
                // Read would always return EAGAIN if no more data, so just wait one time.
                // TODO We could use content-length to determine end of the http response.
                if (last_read)
                {
                    has_read = 0;
                    last_read = false;
                    sleep(1);
                    continue;
                }
                else
                {
                    break;
                }
            }
            else
            {
                // Other error, have to quit.
                perror("read fail");
                return false;
            }
        }
        else if (has_read == 0)
        {
            // Finish reading
            break;
        }
        last_read = true;
        to_read -= has_read;
        if (to_read <= 0)
        {
            printf("Too many response, ignore it...\n");
            return false;
        }
        p += has_read;
    }
printf("Read all successfully\n");
    return true;
}

/*
HTTP/1.1 301 Moved Permanently
Date: Thu, 27 Jun 2013 08:51:13 GMT
Server: DNSPod URL V2.0
Expires: Thu, 27 Jun 2013 09:01:13 GMT
Location: http://itindex.net
Cache-Control: max-age=600
Content-Length: 0
Proxy-Connection: Keep-Alive
*/
int parse_header(char* buff)
{
    // Here Only handle 2XX, and 301
    int ret = buff[9] - '0';
    if (ret == 3)
    {
        char* p = strstr(buff, "Location: ");
        if (p)
        {
            p += strlen("Location: ");
            char* q = p;
            while(*q && *q != '\r')
                q ++;
            char* redirect = Util::sdup(p, q-p);
            Url* u = Url::gen_url(redirect);
            if (u)
            {
                u->print();
                Global::add_url(u);
            }
            delete[] redirect;
        }
    }
    return ret;
}
