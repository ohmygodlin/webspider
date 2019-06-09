#include<string.h>
#include<errno.h>
#include<netdb.h>
#include "global.h"

/*
bool Global::operator()(char* s1, char* s2) const
{
    return strcmp(s1, s2) < 0;
}
*/

// If defined proxy, no need to get ip for each host.
// TODO Need to make the proxy configurable
char* g_prog_name = NULL;
const char PROXY_HOST[] = "www-proxy.us.oracle.com";
const ushort PROXY_PORT = 80;

// Static variable
HashTable* Global::g_url_table;
Host* Global::g_host_array;
adns_state Global::g_adns_state;
list<Url*> Global::g_url_list;
int Global::g_epoll_fd;
struct sockaddr_in* Global::g_proxy = NULL;
char* Global::g_large_buff = NULL;

void Global::init()
{
    g_proxy = new struct sockaddr_in;
    memset((char*)g_proxy, 0, sizeof(struct sockaddr_in));
    struct hostent* hp = gethostbyname(PROXY_HOST);
    endhostent();
    if (hp == NULL)
    {
        herror("gethostbyname");
        exit(h_errno);
    }
    g_proxy->sin_family = hp->h_addrtype;
    memcpy((char*)&g_proxy->sin_addr, hp->h_addr, hp->h_length);
    g_proxy->sin_port = htons(PROXY_PORT);
printf("Proxy ip: %s\n", inet_ntoa(g_proxy->sin_addr));
    
    g_epoll_fd = epoll_create(MAX_EPOLL_FD);
    if (g_epoll_fd == -1)
    {
        perror("epoll_create fail\n");
        exit(errno);
    }
    g_url_table = new HashTable();
    g_host_array = new Host[1<<MAX_HOST_BIT_SiZE];
    if (g_proxy == NULL)
    {
        adns_initflags flags;
        flags = adns_initflags(adns_if_nosigpipe | adns_if_noerrprint);
        adns_init(&g_adns_state, flags, NULL);
    }
    g_large_buff = new char[MAX_BUFF_SIZE];
}

void Global::main_loop()
{
//    while(true)
    for(int i = 0; i < 10; i ++)
    {
        proc_url_list();
        if (g_proxy == NULL)
            proc_adns_state();
        
        // Process g_epoll_fd
        if (!proc_epoll_fd())
            break;
    }
}

void Global::proc_url_list()
{
    for (list<Url*>::iterator it = g_url_list.begin(); it != g_url_list.end(); )
    {
        Url* u = *it;
        if (u->process())
            g_url_list.erase(it ++);
        else
            it ++;
    }
}

void Global::proc_adns_state()
{
    while(true)
    {
        Host* host;
        adns_query quer = NULL;
        adns_answer* ans;
        int res = adns_check(Global::g_adns_state,
          &quer, &ans, (void**)&host);
        if (res == 0)
        {
printf("ansstatus: %d\n", ans->status);
            if (ans->status == adns_s_ok)
            {
                // Successfully get ip address, then get robots.txt
                host->set_sin_addr(&ans->rrs.addr->addr.inet.sin_addr);
                host->request_robots();
            }
            else if (ans->status == adns_s_prohibitedcname)
            {
//                if (host->get_cname() == NULL)
//                {
printf("cname: %s\n", ans->cname);
                    host->set_cname(ans->cname);
                    adns_query quer = NULL;
                    adns_submit(Global::g_adns_state, host->get_cname(), 
                     (adns_rrtype)adns_r_addr, (adns_queryflags) 0, host, &quer);
//                }
//                else
//                {
                    // dns chains too long => dns error.
//                    printf("DNS chains too long for host: %s\n", host->get_host_name());
//                    host->clear();
//                }
            }
            else
            {
                // Other error.
                printf("Can not be adns for host: %s, error code: %d\n", host->get_host_name(), ans->status);
                host->clear();
            }
            free(ans);
        }
        else
        {
            break;
        }
    }
}

bool Global::proc_epoll_fd()
{
    static struct epoll_event event_array[MAX_EPOLL_FD];
    // Timeout: 1 sec!
    int fd_count = epoll_wait(g_epoll_fd, event_array, MAX_EPOLL_FD, 1000);
printf("epoll_wait fd_count: %d\n", fd_count);
    if (fd_count < 0)
    {
        perror("epoll_wait fail");
        return false;
    }
    if (fd_count > 0)
    {
        for (int i = 0; i < fd_count; i ++)
        {
            FdNode* fdnode = (FdNode*)event_array[i].data.ptr;
            fdnode->set_revents(event_array[i].events);
            fdnode->call_back();
        }
    }
    return true;
}

bool Global::ctl_fd(int op, FdNode* fdnode)
{
    struct epoll_event event;
    int fd = fdnode->get_fd();
    event.events = fdnode->get_revents();
    event.data.ptr = fdnode;
    if(epoll_ctl(g_epoll_fd, op, fd, &event) < 0)
    {
        if (op == EPOLL_CTL_ADD)
        {
            if (errno == ENOMEM)
            {
                // TODO add fdnode to local list, waiting for memory.
printf("Too many fd, op: %d, fd: %d\n", op, fd);
            }
            else if (errno == EEXIST)
            {
                return ctl_fd(EPOLL_CTL_MOD, fdnode);
            }
            else
            {
                perror("epoll_ctl fail");
                return false;
            }
        }
        else if (op == EPOLL_CTL_MOD)
        {
            if (errno == ENOENT)
            {
                return ctl_fd(EPOLL_CTL_ADD, fdnode);
            }
            else
            {
                perror("epoll_ctl fail");
                return false;
            }
        }
        else
        {
            perror("epoll_ctl fail");
            return false;
        }
    }
    else
    {
        printf("Sucessfull epoll_ctl for fd: %d, op: %d, event: %d\n", fd, op, event.events);
    }
    return true;
}

void Global::destroy()
{
    if (g_large_buff)
    {
        delete[] g_large_buff;
        g_large_buff = NULL;
    }
    while(!g_url_list.empty())
    {
        delete g_url_list.front();
        g_url_list.pop_front();
    }
    if (g_proxy == NULL)
        adns_finish(g_adns_state);
    delete[] g_host_array;
    delete g_url_table;
    close(g_epoll_fd);
}

void Global::add_adns_request(Host* host)
{
    // TODO request dns. In the future version, we should restrict the account of dns request.
    adns_query quer = NULL;
    adns_submit(Global::g_adns_state, host->get_host_name(), 
      (adns_rrtype)adns_r_addr, (adns_queryflags) 0, host, &quer);
}

void Global::add_url(Url* u)
{
    if (g_url_table->test_put(u->hash_code()))
    {
        g_url_list.push_back(u);
    }
}
