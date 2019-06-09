/* Global structure.
 * Create Date: 2013/06/04
*/
#include<sys/types.h>
#include<sys/epoll.h>
#include<list>
#include<map>
using namespace std;
#include<adns.h>
#include "util/hashtable.h"
#include "entity.h"

const uint MAX_HOST_BIT_SiZE = 14;
const uint MAX_EPOLL_FD = 4 * 1024;  // Page size
const uint MAX_BUFF_SIZE = 1024 * 1024;

extern char* g_prog_name;

class Global
{
public:
    /* Interface */
    static void init();
    static void main_loop();
    static void destroy();
    static void add_adns_request(Host* host);
    static void add_url(Url* u);
    static bool ctl_fd(int op, FdNode* fdnode);

    /* Data Structure */
    //bool operator()(char* s1, char* s2) const;
    static HashTable* g_url_table;         // Url hash table recording pages ever seen.
    static Host* g_host_array;     // Host array that have been DNSed.
    static adns_state g_adns_state;      // State of adns
    static list<Url*> g_url_list;   // Url queue, to be accessed.
    static int g_epoll_fd;          // epoll fd created by epoll_create
    static struct sockaddr_in* g_proxy;   // proxy address and port
    static char* g_large_buff;       // Used for receive http response.
//    static map<int, FdNode*> g_fd_map;  // fd map.
    
protected:
    static void proc_url_list();
    static void proc_adns_state();
    static bool proc_epoll_fd();
};
