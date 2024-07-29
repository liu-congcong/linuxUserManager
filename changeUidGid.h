#ifndef __CHANGEUIDGID_H__
#define __CHANGEUIDGID_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>


typedef struct PATH
{
    char *path;
    int uid;
    int gid;
} PATH;

int changeUidGid(PATH *node)
{
    lchown(node->path, node->uid, node->gid);
    DIR *dir = opendir(node->path);
    if (dir)
    {
        struct dirent *entry = NULL;
        struct stat statBuffer;
        PATH *subNode = malloc(sizeof(PATH));
        subNode->path = malloc(sizeof(char) * 1024 * 1024);
        assert(subNode->path);
        subNode->uid = node->uid;
        subNode->gid = node->gid;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            {
                snprintf(subNode->path, 1024 * 1024, "%s/%s", node->path, entry->d_name);
                if (lstat(subNode->path, &statBuffer) != -1)
                {
                    if (S_ISDIR(statBuffer.st_mode))
                    {
                        changeUidGid(subNode);
                    }
                    else
                    {
                        lchown(subNode->path, subNode->uid, subNode->gid);
                    }
                }
            }
        }
        free(subNode->path);
        free(subNode);
    }
    closedir(dir);
    return 0;
}

#endif