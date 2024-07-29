#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <grp.h>
#include <sys/types.h>
#include <assert.h>
#include <pwd.h>
#include <time.h>
#include <shadow.h>
#include <sys/stat.h>
#include "hash.h"
#include "changeUidGid.h"
#include "homeInitialization.h"
#include "threadPool.h"

#define USERNAME 32
#define HASHSIZE 65535
#define MINUID 1000
#define MAXUID 60000


typedef struct PW
{
    struct passwd pw;
    int flag;
    struct PW *next;
} PW;

typedef struct USER
{
    char user[USERNAME + 1];
    int uid;
    int gid;
    struct USER *next;
} USER;

int printHelp(char *self)
{
    puts("\nUser manager for Linux v0.3 (https://github.com/liu-congcong/linuxUserManager)");
    puts("Usage:");
    puts("Add an user:");
    printf("    %s -add -user <user> -password <password> -group <group> [-uid 1000-60000]\n", self);
    puts("List all users:");
    printf("    %s -list\n", self);
    puts("Modify an user:");
    printf("    %s -modify -user <user> [-uid 1000-60000] [-gid 1000-60000]\n", self);
    puts("Synchronize user data:");
    printf("    %s -sync [-threads <int>] -f <file>\n\n", self);
    return 0;
}

PW *loadPasswd(void)
{
    PW *head = NULL;
    PW *tail = NULL;
    struct passwd *pw = NULL;
    setpwent();
    while ((pw = getpwent()) != NULL)
    {
        PW *node = malloc(sizeof(PW));
        node->pw.pw_name = malloc(sizeof(char) * (strlen(pw->pw_name) + 1));
        strcpy(node->pw.pw_name, pw->pw_name);
        node->pw.pw_passwd = malloc(sizeof(char) * (strlen(pw->pw_passwd) + 1));
        strcpy(node->pw.pw_passwd, pw->pw_passwd);
        node->pw.pw_uid = pw->pw_uid;
        node->pw.pw_gid = pw->pw_gid;
        node->pw.pw_gecos = malloc(sizeof(char) * (strlen(pw->pw_gecos) + 1));
        strcpy(node->pw.pw_gecos, pw->pw_gecos);
        node->pw.pw_dir = malloc(sizeof(char) * (strlen(pw->pw_dir) + 1));
        strcpy(node->pw.pw_dir, pw->pw_dir);
        node->pw.pw_shell = malloc(sizeof(char) * (strlen(pw->pw_shell) + 1));
        strcpy(node->pw.pw_shell, pw->pw_shell);
        node->flag = 0;
        node->next = NULL;
        if (!head)
        {
            head = node;
        }
        else
        {
            tail->next = node;
        }
        tail = node;
    }
    endpwent();
    return head;
}

int freePW(PW *node)
{
    PW *temp = NULL;
    while (node)
    {
        temp = node;
        node = node->next;
        free(temp->pw.pw_name);
        free(temp->pw.pw_passwd);
        free(temp->pw.pw_gecos);
        free(temp->pw.pw_dir);
        free(temp->pw.pw_shell);
        free(temp);
    }
    return 0;
}

int listUsers(void)
{
    PW *head = loadPasswd();
    PW *node = head;
    puts("              user |   uid |   gid |                     home");
    puts("-------------------------------------------------------------");
    while (node)
    {
        if (!strncasecmp("/home/", node->pw.pw_dir, 6))
        {
            printf("%18s | %5d | %5d | %24s\n", node->pw.pw_name, node->pw.pw_uid, node->pw.pw_gid, node->pw.pw_dir);
        }
        node = node->next;
    }
    freePW(head);
    return 0;
}

bool isUserExisted(char *user)
{
    struct passwd *pw = getpwnam(user);
    return (pw != NULL);
}

bool isUidExisted(int uid)
{
    struct passwd *pw = getpwuid(uid);
    return (pw != NULL);
}

int group2gid(char *group)
{
    struct group *GRP = getgrnam(group);
    int gid = (GRP == NULL) ? -1 : GRP->gr_gid;
    return gid;
}

char *encryptPassword(char *user, char *password)
{
    /* Encrypt the password using SHA-512 */
    char salt[USERNAME + 4];
    snprintf(salt, USERNAME + 4, "$6$%s", user);
    char *encryptedPassword = crypt(password, salt);
    assert(encryptedPassword);
    return encryptedPassword;
}

int add2Passwd(char *user, int uid, int gid)
{
    char *home = malloc(sizeof(char) * (USERNAME + 7));
    assert(home);
    snprintf(home, USERNAME + 7, "/home/%s", user);
    struct passwd pw;
    pw.pw_name = user;
    pw.pw_passwd = "x";
    pw.pw_uid = uid;
    pw.pw_gid = gid;
    pw.pw_gecos = user;
    pw.pw_dir = home;
    pw.pw_shell = "/bin/bash";

    FILE *openFile = fopen("/etc/passwd", "a");
    assert(openFile);
    assert(!putpwent(&pw, openFile));
    fclose(openFile);
    free(home);
    return 0;
}

int add2Shadow(char *user, char *password)
{
    struct spwd spw;
    spw.sp_namp = user;
    spw.sp_pwdp = encryptPassword(user, password);
    spw.sp_lstchg = time(NULL) / (24 * 60 * 60);
    spw.sp_min = 0;
    spw.sp_max = 99999;
    spw.sp_warn = 7;
    spw.sp_inact = -1;
    spw.sp_expire = -1;
    spw.sp_flag = ~0;

    FILE *openFile = fopen("/etc/shadow", "a");
    assert(openFile);
    assert(!putspent(&spw, openFile));
    fclose(openFile);
    return 0;
}

int addUser(char *user, char *password, char *group, int uid)
{
    if (uid == -1)
    {
        for (int i = MINUID; i <= MAXUID; i++)
        {
            if (!isUidExisted(i))
            {
                uid = i;
                break;
            }
        }
    }
    int gid = group2gid(group);
    if (isUserExisted(user))
    {
        printf("user: %s exists.\n", user);
    }
    else if (gid == -1)
    {
        printf("group: %s not exists.\n", group);
    }
    else if (isUidExisted(uid))
    {
        printf("uid: %d exists.\n", uid);
    }
    else
    {
        add2Passwd(user, uid, gid);
        add2Shadow(user, password);
        initializeHome(user, uid, gid);
    }
    return 0;
}

int multiChangeUidGid(void *arg)
{
    PATH *pathNode = (PATH *)arg;
    changeUidGid(pathNode);
    return 0;
}

int modifyUser(PW *pwNodes, char *user, int uid, int gid, int threads)
{
    PW *head = pwNodes ? pwNodes : loadPasswd();
    PW *node = head;

    ThreadPool *threadPool = threadPoolCreate(threads);
    assert(threadPool);

    FILE *openFile = fopen("/etc/passwd_temp", "w");
    assert(openFile);
    while (node)
    {
        if ((!strcmp(node->pw.pw_name, user)) || node->flag)
        {
            node->pw.pw_uid = (uid == -1) ? node->pw.pw_uid : uid;
            node->pw.pw_gid = (gid == -1) ? node->pw.pw_gid : gid;
            printf("user: %s -> uid: %d, gid: %d\n", node->pw.pw_name, node->pw.pw_uid, node->pw.pw_gid);
            PATH *pathNode = malloc(sizeof(PATH));
            pathNode->path = malloc(sizeof(char) * 1024);
            strncpy(pathNode->path, node->pw.pw_dir, 1024);
            pathNode->uid = node->pw.pw_uid;
            pathNode->gid = node->pw.pw_gid;
            assert(!threadPoolPut(threadPool, multiChangeUidGid, pathNode));
        }
        putpwent(&(node->pw), openFile);
        node = node->next;
    }
    fclose(openFile);
    rename("/etc/passwd_temp", "/etc/passwd");
    if (!pwNodes)
    {
        freePW(head);
    }
    assert(!threadPoolFree(threadPool));
    return 0;
}

int insertUser(USER **userNodes, char *user, int uid, int gid)
{
    USER *userNode = malloc(sizeof(USER));
    assert(userNode);
    strncpy(userNode->user, user, USERNAME + 1);
    userNode->uid = uid;
    userNode->gid = gid;
    userNode->next = NULL;
    unsigned int hash = elfHash(user) % HASHSIZE;
    if (userNodes[hash])
    {
        USER *tempUserNode = userNodes[hash];
        while (tempUserNode->next)
        {
            tempUserNode = tempUserNode->next;
        }
        tempUserNode->next = userNode;
    }
    else
    {
        userNodes[hash] = userNode;
    }
    return 0;
}

int searchUser(USER **userNodes, char *user, int *uid, int *gid)
{
    int flag = 0;
    unsigned int hash = elfHash(user) % HASHSIZE;
    USER *userNode = userNodes[hash];
    while (userNode)
    {
        if (!strcmp(user, userNode->user))
        {
            *uid = userNode->uid;
            *gid = userNode->gid;
            flag = 1;
            break;
        }
        userNode = userNode->next;
    }
    return flag;
}

int loadUsers(char *file, USER **userNodes, int *uidNodes)
{
    char user[USERNAME + 1];
    int uid;
    int gid;
    FILE *openFile = fopen(file, "r");
    assert(openFile);
    while (fscanf(openFile, "%s %d %d", user, &uid, &gid) == 3)
    {
        uidNodes[uid] = 1;
        insertUser(userNodes, user, uid, gid);
    }
    fclose(openFile);
    return 0;
}

int freeUserHash(USER **node)
{
    USER *temp = NULL;
    for (int i = 0; i < HASHSIZE; i++)
    {
        while (node[i])
        {
            temp = node[i];
            node[i] = temp->next;
            free(temp);
        }
    }
    free(node);
}

int syncUsers(char *file, int threads)
{
    int uid;
    int gid;
    USER **userNodes = malloc(HASHSIZE * sizeof(USER *));
    memset(userNodes, 0, HASHSIZE * sizeof(USER *));
    int *uidNodes = malloc(MAXUID * sizeof(int));
    memset(uidNodes, 0, MAXUID * sizeof(int));
    loadUsers(file, userNodes, uidNodes); /* load users from file */
    PW *pwHead = loadPasswd();
    PW *pwNode = pwHead;
    while (pwNode)
    {
        if (!strncasecmp("/home/", pwNode->pw.pw_dir, 6))
        {
            if (searchUser(userNodes, pwNode->pw.pw_name, &uid, &gid))
            {
                if ((pwNode->pw.pw_uid != uid) || (pwNode->pw.pw_gid != gid))
                {
                    pwNode->flag = 1;
                    pwNode->pw.pw_uid = uid;
                    pwNode->pw.pw_gid = gid;
                }
            }
            else
            {
                printf("user: %s not exists.\n", pwNode->pw.pw_name);
            }
        }
        pwNode = pwNode->next;
    }
    modifyUser(pwHead, "", -1, -1, threads);
    freePW(pwHead);
    free(uidNodes);
    freeUserHash(userNodes);
    return 0;
}

int main(int argc, char *argv[])
{
    int euid = geteuid();
    if (argc == 1 || !strncasecmp("-help", argv[1], 2) || !strncasecmp("--help", argv[1], 3))
    {
        printHelp(argv[0]);
    }
    else if (!strncasecmp("-add", argv[1], 2) || !strncasecmp("--add", argv[1], 3))
    {
        if (euid)
        {
            puts("Root privileges are required to run the program.");
            exit(0);
        }
        int uid = -1;
        char *user = NULL;
        char *password = NULL;
        char *group = NULL;
        for (int i = 2; i < argc; i++)
        {
            if (!strncasecmp("-user", argv[i], 3) || !strncasecmp("--user", argv[i], 4))
            {
                assert(strlen(argv[i + 1]) <= USERNAME);
                user = argv[i + 1];
            }
            else if (!strncasecmp("-password", argv[i], 2) || !strncasecmp("--password", argv[i], 3))
            {
                password = argv[i + 1];
            }
            else if (!strncasecmp("-group", argv[i], 2) || !strncasecmp("--group", argv[i], 3))
            {
                group = argv[i + 1];
            }
            else if (!strncasecmp("-uid", argv[i], 3) || !strncasecmp("--uid", argv[i], 4))
            {
                uid = atoi(argv[i + 1]);
            }
        }
        if (!user || !password || !group)
        {
            printHelp(argv[0]);
        }
        else
        {
            addUser(user, password, group, uid);
        }
    }
    else if (!strncasecmp("-list", argv[1], 2) || !strncasecmp("--list", argv[1], 3))
    {
        listUsers();
    }
    else if (!strncasecmp("-modify", argv[1], 2) || !strncasecmp("--modify", argv[1], 3))
    {
        if (euid)
        {
            puts("Root privileges are required to run the program.");
            exit(0);
        }
        char *user = NULL;
        int uid = -1;
        int gid = -1;
        for (int i = 2; i < argc; i++)
        {
            if (!strncasecmp("-user", argv[i], 3) || !strncasecmp("--user", argv[i], 4))
            {
                user = argv[i + 1];
            }
            else if (!strncasecmp("-uid", argv[i], 3) || !strncasecmp("--uid", argv[i], 4))
            {
                uid = atoi(argv[i + 1]);
            }
            else if (!strncasecmp("-group", argv[i], 2) || !strncasecmp("--group", argv[i], 3))
            {
                gid = atoi(argv[i + 1]);
            }
        }
        if (user && ((uid != -1) || (gid != -1)))
        {
            modifyUser(NULL, user, uid, gid, 1);
        }
        else
        {
            printHelp(argv[0]);
        }
    }
    else if (!strncasecmp("-sync", argv[1], 2) || !strncasecmp("--sync", argv[1], 3))
    {
        if (euid)
        {
            puts("Root privileges are required to run the program.");
            exit(0);
        }
        char *file = NULL;
        int threads = 100;
        for (int i = 2; i < argc; i++)
        {
            if (!strncasecmp("-file", argv[i], 2) || !strncasecmp("--file", argv[i], 3))
            {
                file = argv[i + 1];
            }
            else if (!strncasecmp("-threads", argv[i], 2) || !strncasecmp("--threads", argv[i], 3))
            {
                threads = atoi(argv[i + 1]);
                assert(threads > 0);
            }
        }
        if (file)
        {
            syncUsers(file, threads);
        }
        else
        {
            printHelp(argv[0]);
        }
    }
    return 0;
}
