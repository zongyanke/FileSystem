#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>//文件读写
#include <time.h> //记录文件修改时间
using namespace std;

#define CMD_COUNT         22
#define MAX_USER_COUNT    100
#define BLOCK_SIZE      1024
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
}MultiUser;

//超级块的数据结构，最后应该需要占用1024个字节
struct SuperBlock{
    unsigned int block_size; 	//4字节
	unsigned int block_count; 
	unsigned int block_used;	
	unsigned int block_free;
	unsigned int inode_count;
	unsigned int inode_used;
	unsigned int inode_free;
}; //28个字节

//inode当中文件链表，确定文件所占用的block_id
//指针实际上就是一串地址，所以在64位的机器上占用八个字节
struct InodeList{
    unsigned int block_id;
    InodeList *next_block_id;
};

//inode的数据结构
struct Inode{
	unsigned int inode_ifentifier;//inode的编号
	unsigned int file_type; //文件类型
	unsigned int link_count;//暂时没有加这个功能的计划
	unsigned int own_id; //文件所有者的id
	unsigned int group_id;//所属的组的id
	char authority[11];//文件权限，十个位
	unsigned int file_size; //文件大小
	unsigned int block_used_byfile; //文件使用了几个块
	time_t inode_modified_time;//8字节 inode上一次修改的时间
	time_t create_time;//文件创建时间
	time_t last_modified_time;//文件内容上一次修改的时间
	time_t last_open_time;//文件上一次打开的时间
    InodeList* block_id; //16 bytes
};

//每一个目录项
struct DirectoryEntry{
    char directory_name[100];//文件名长度最大支持99个字符
    unsigned int inode_identifier;//文件inode索引号
};

class FileSystem
{
private:
    FILE* file_pointer; //记得删除文件指针
    MultiUser* multiuser;
    int user_num;
    int cmd_type;
    string cmd[CMD_COUNT];
    string split_command[5];//最多支持四个参数
    string current_path;//当前目录
    Inode current_inode;//当前inode

    bool* inode_bitmap; //使用位图来表示inode使用情况
    bool* block_bitmap;
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