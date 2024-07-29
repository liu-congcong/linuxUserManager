#ifndef __HOMEINITIALIZATION_H__
#define __HOMEINITIALIZATION_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include "changeUidGid.h"


static int initializeProfile(char *file)
{
    FILE *openFile = fopen(file, "w");
    assert(openFile);
    fputs("if [ -n \"$BASH_VERSION\" ]; then\n", openFile);
    fputs("    if [ -f \"$HOME/.bashrc\" ]; then\n", openFile);
    fputs("        . \"$HOME/.bashrc\"\n", openFile);
    fputs("    fi\n", openFile);
    fputs("fi\n", openFile);
    fputs("if [ -d \"$HOME/bin\" ] ; then\n", openFile);
    fputs("    PATH=\"$HOME/bin:$PATH\"\n", openFile);
    fputs("fi\n", openFile);
    fputs("if [ -d \"$HOME/.local/bin\" ] ; then\n", openFile);
    fputs("    PATH=\"$HOME/.local/bin:$PATH\"\n", openFile);
    fputs("fi\n", openFile);
    fclose(openFile);
    return 0;
}

static int initializeBashrc(char *file)
{
    FILE *openFile = fopen(file, "w");
    assert(openFile);
    fputs("case $- in\n", openFile);
    fputs("    *i*) ;;\n", openFile);
    fputs("      *) return;;\n", openFile);
    fputs("esac\n", openFile);
    fputs("HISTCONTROL=ignoreboth\n", openFile);
    fputs("shopt -s histappend\n", openFile);
    fputs("HISTSIZE=1000\n", openFile);
    fputs("HISTFILESIZE=2000\n", openFile);
    fputs("shopt -s checkwinsize\n", openFile);
    fputs("[ -x /usr/bin/lesspipe ] && eval \"$(SHELL=/bin/sh lesspipe)\"\n", openFile);
    fputs("if [ -z \"${debian_chroot:-}\" ] && [ -r /etc/debian_chroot ]; then\n", openFile);
    fputs("    debian_chroot=$(cat /etc/debian_chroot)\n", openFile);
    fputs("fi\n", openFile);
    fputs("case \"$TERM\" in\n", openFile);
    fputs("    xterm-color|*-256color) color_prompt=yes;;\n", openFile);
    fputs("esac\n", openFile);
    fputs("if [ -n \"$force_color_prompt\" ]; then\n", openFile);
    fputs("    if [ -x /usr/bin/tput ] && tput setaf 1 >&/dev/null; then\n", openFile);
    fputs("        color_prompt=yes\n", openFile);
    fputs("    else\n", openFile);
    fputs("        color_prompt=\n", openFile);
    fputs("    fi\n", openFile);
    fputs("fi\n", openFile);
    fputs("if [ \"$color_prompt\" = yes ]; then\n", openFile);
    fputs("    PS1='${debian_chroot:+($debian_chroot)}\\[\\033[01;32m\\]\\u@\\h\\[\\033[00m\\]:\\[\\033[01;34m\\]\\w\\[\\033[00m\\]\\$ '\n", openFile);
    fputs("else\n", openFile);
    fputs("    PS1='${debian_chroot:+($debian_chroot)}\\u@\\h:\\w\\$ '\n", openFile);
    fputs("fi\n", openFile);
    fputs("unset color_prompt force_color_prompt\n", openFile);
    fputs("case \"$TERM\" in\n", openFile);
    fputs("xterm*|rxvt*)\n", openFile);
    fputs("    PS1=\"\\[\\e]0;${debian_chroot:+($debian_chroot)}\\u@\\h: \\w\\a\\]$PS1\"\n", openFile);
    fputs("    ;;\n", openFile);
    fputs("*)\n", openFile);
    fputs("    ;;\n", openFile);
    fputs("esac\n", openFile);
    fputs("if [ -x /usr/bin/dircolors ]; then\n", openFile);
    fputs("    test -r ~/.dircolors && eval \"$(dircolors -b ~/.dircolors)\" || eval \"$(dircolors -b)\"\n", openFile);
    fputs("    alias ls='ls --color=auto'\n", openFile);
    fputs("    alias grep='grep --color=auto'\n", openFile);
    fputs("    alias fgrep='fgrep --color=auto'\n", openFile);
    fputs("    alias egrep='egrep --color=auto'\n", openFile);
    fputs("fi\n", openFile);
    fputs("alias ll='ls -alF'\n", openFile);
    fputs("alias la='ls -A'\n", openFile);
    fputs("alias l='ls -CF'\n", openFile);
    fputs("alias alert='notify-send --urgency=low -i \"$([ $? = 0 ] && echo terminal || echo error)\" \"$(history|tail -n1|sed -e '\\''s/^\\s*[0-9]\\+\\s*//;s/[;&|]\\s*alert$//'\\'')\"'\n", openFile);
    fputs("if [ -f ~/.bash_aliases ]; then\n", openFile);
    fputs("    . ~/.bash_aliases\n", openFile);
    fputs("fi\n", openFile);
    fputs("if ! shopt -oq posix; then\n", openFile);
    fputs("  if [ -f /usr/share/bash-completion/bash_completion ]; then\n", openFile);
    fputs("    . /usr/share/bash-completion/bash_completion\n", openFile);
    fputs("  elif [ -f /etc/bash_completion ]; then\n", openFile);
    fputs("    . /etc/bash_completion\n", openFile);
    fputs("  fi\n", openFile);
    fputs("fi\n", openFile);
    fclose(openFile);
    return 0;
}

static int initializeBashLogout(char *file)
{
    FILE *openFile = fopen(file, "w");
    assert(openFile);
    fputs("if [ \"$SHLVL\" = 1 ]; then\n", openFile);
    fputs("    [ -x /usr/bin/clear_console ] && /usr/bin/clear_console -q\n", openFile);
    fputs("fi\n", openFile);
    fclose(openFile);
    return 0;
}

int initializeHome(char *user, int uid, int gid)
{
    PATH *node = malloc(sizeof(PATH));
    assert(node);
    node->path = malloc(sizeof(char) * 1024);
    assert(node->path);

    snprintf(node->path, 1024, "/home/%s", user);
    mkdir(node->path, 0755);
    snprintf(node->path, 1024, "/home/%s/.profile", user);
    initializeProfile(node->path);
    chmod(node->path, 0644);
    snprintf(node->path, 1024, "/home/%s/.bashrc", user);
    initializeBashrc(node->path);
    chmod(node->path, 0644);
    snprintf(node->path, 1024, "/home/%s/.bash_logout", user);
    initializeBashLogout(node->path);
    chmod(node->path, 0644);
    snprintf(node->path, 1024, "/home/%s", user);
    node->uid = uid;
    node->gid = gid;
    changeUidGid(node);
    free(node->path);
    free(node);
    return 0;
}

#endif
