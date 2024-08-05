# linuxUserManager

A tool to synchronize user uid and gid on linux.

## Usage

```bash
git clone https://github.com/liu-congcong/linuxUserManager.git
cd linuxUserManager
gcc -o linuxUserManager linuxUserManager.c -lcrypt -lpthread
```

```bash
linuxUserManager -h
User manager for Linux v0.3 (https://github.com/liu-congcong/linuxUserManager)
Usage:
Add an user:
    ./linuxUserManager -add -user <user> -password <password> -group <group> [-uid 1000-60000]
List all users:
    ./linuxUserManager -list
Modify an user:
    ./linuxUserManager -modify -user <user> [-uid 1000-60000] [-gid 1000-60000]
Synchronize user data:
    ./linuxUserManager -sync [-thread <int>] -f <file>
```

## Change logs

* 0.1: Initial version.

* 0.2: Initialize .bash_logout, .bashrc and .profile.

* 0.3: Multi-threads support.
