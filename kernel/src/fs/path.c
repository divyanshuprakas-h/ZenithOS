#include "path.h"

int path_next_component(const char **path, char *component)
{
    const char *p = *path;
    int len = 0;

    while (*p == '/')
    {
        p++;
    }

    if (*p == '\0')
    {
        return 0;
    }

    while (*p != '/' && *p != '\0')
    {
        if (len >= VFS_MAX_COMPONENT_LEN - 1)
        {
            return -1;
        }
        component[len++] = *p++;
    }

    component[len] = '\0';

    *path = p;

    return 1;
}