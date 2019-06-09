#include<string.h>
#include<sys/types.h>

class Util
{
public:
    static char* sdup(char* src);
    static char* sdup(char* src, uint len);
    static char* joint(char* s1, char* s2);
    static bool cmp_wild(char* pattern, char* str);
};
