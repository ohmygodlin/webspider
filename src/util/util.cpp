#include "util.h"

char* Util::sdup(char* src)
{
    return sdup(src, strlen(src));
}

char* Util::sdup(char* src, uint len)
{
    char* dst = new char[len + 1];
    strncpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

char* Util::joint(char* s1, char* s2)
{
    if (!s1 || !s2)
        return NULL;
    char* dst = new char[strlen(s1) + strlen(s2) + 1];
    strcpy(dst, s1);
    strcat(dst, s2);
    return dst;
}

/* Only one wild character is supported.
*/
bool Util::cmp_wild(char* pattern, char* str)
{
    const char WILD = '*';
    char* p_wild = strchr(pattern, WILD);
    int len = strlen(pattern);
    int len_str = strlen(str);
    if (p_wild == NULL)
    {
        if (pattern[0] == '/')
            return (len_str < len) ? false : (strncmp(pattern, str, len) == 0);
        return (strcmp(pattern, str) == 0);
    }
    if (len_str < len - 1)
        return false;
    int len_left = p_wild - pattern;
    int len_right = len - len_left - 1;
    p_wild ++;
    char* p_str = str + len_str - len_right;
    return (strncmp(pattern, str, len_left)==0) && (strncmp(p_wild, p_str, len_right)==0);
}
