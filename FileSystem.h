#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>//文件读写
using namespace std;

#define SUDO     0
#define HELP     1
#define LS       2
#define CD       3
#define TOUCH    4
#define MKDIR    5
#define CAT      6
#define WRITE    7
#define MV       8
#define CP       9
#define CHMOD    10
#define RM       11
#define PWD      12       //当前目录
#define FIND     13
#define CLEAR    14       //清屏
#define LOGOUT   15
#define ACCOUNT  16       //列出当前用户的信息
#define SYSINFO  17
#define EXIT     18
#define RENAME   19       //重命名
#define LOGIN    20
#define REGISTER 21


typedef struct{
    string username[10];
    string password[10];
    string system_name[10];
}Multiuser;


class FileSystem
{
private:
    Multiuser* multiuser;
    char multiuser_account[8] = "account";
    string cmd[22];
    string split_command[5];//最多支持四个参数
public:
    FileSystem();
    ~FileSystem();
    void openMultiuser();
    void splitCommand(string&);
    void command();
};

#endif 