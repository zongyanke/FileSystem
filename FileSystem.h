#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>//文件读写
using namespace std;

#define CMD_NUM         22
#define MAX_USER_NUM    100
#define MUTIUSER_ACCOUNT "account"

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
    char username[11];
    char password[11];
    char group_id[11];//或许可以迟一点加上去
}Multiuser;


class FileSystem
{
private:
    FILE* file_pointer; //记得删除文件指针
    Multiuser* multiuser;
    int user_num;
    int cmd_type;
    string cmd[CMD_NUM];
    string split_command[5];//最多支持四个参数
    string current_path;
public:
    FileSystem();
    ~FileSystem();
    void initMultiuser();
    void splitCommand(string&);
    void discardBlank(string&);
    void command();

    //command
    void userRegister();
    void userLogin();







    //
    void printCurrentPath();
};

#endif 