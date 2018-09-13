#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>//文件读写
#include <time.h> //记录文件修改时间
#include <unistd.h>
using namespace std;

#define CMD_COUNT         22
#define USER_COUNT_MAX    100
#define FILE_NAME_MAX	  124		//文件名最大长度25
#define BLOCK_SIZE        1024
#define BLOCK_COUNT		  102400 //块的数目，要不要把目录项这种放进计数里面
#define INODE_COUTN		  102400 //inode的数目，占的空间的确是有点大
#define MUTIUSER_ACCOUNT "account"
#define SYSTEM_NAME		"FileSystem"

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



struct MultiUser{
    char username[11];
    char password[11];
    char group_id[11];//或许可以迟一点加上去
};

//超级块的数据结构，最后应该需要占用1024个字节
struct SuperBlock{
    unsigned int block_size; 	//4字节
	unsigned int block_count;
	unsigned int block_used;	
	unsigned int block_free;
	unsigned int inode_count;
	unsigned int inode_used;
	unsigned int inode_free;
	unsigned int root_inode;//记录一下root的位置
}; //28个字节

//最后这个可能不要了
//inode当中文件链表，确定文件所占用的block_id
struct InodeList{
    unsigned int block_id;
    unsigned int next_block_id;//下一个block的id
};
//在初始化的时候要建立一个二维数组来记录各种文件，且可以放进目录项中存放起来
//在打开一个新的文件的时间要new一个一维数组来记录所有文件的inode

//inode的数据结构
struct Inode{
	unsigned int inode_ifentifier;//inode的编号
	unsigned int file_type; //文件类型,0是文件，1是目录
	unsigned int link_count;//暂时没有加这个功能的计划
	unsigned int own_id; //文件所有者的id ,root用户为0
	unsigned int group_id;//所属的组的id
	char authority[11];//文件权限，十个位
	unsigned int file_size; //文件大小
	unsigned int block_used_byfile; //文件使用了几个块
	time_t inode_modified_time;//8字节 inode上一次修改的时间       
	time_t create_time;//文件创建时间
	time_t last_modified_time;//文件内容上一次修改的时间
	time_t last_open_time;//文件上一次打开的时间
    unsigned int block_id; //16 bytes
};

//每一个目录项
//每一个data块可以容纳8个目录项
struct DirectoryEntry{
    char directory_name[FILE_NAME_MAX];//文件名长度最大支持255个字符
    unsigned int inode_identifier;//文件inode索引号
};

class FileSystem
{
private:
    FILE* file_pointer; //记得删除文件指针
    MultiUser* multiuser;
    int user_num;
    int cmd_type;
    string cmd[CMD_COUNT]; //命令的个数
    string split_command[5];//最多支持四个参数
    string current_path;//当前目录

	SuperBlock superblock;
	//这三个量要看看
	bool inodelist[INODE_COUTN];
    unsigned int blocklist[BLOCK_COUNT];
	Inode current_directory_inode;
	Inode current_data_inode;
	char current_data_block[BLOCK_SIZE];
	DirectoryEntry current_directory_block[BLOCK_SIZE/sizeof(DirectoryEntry)];
	DirectoryEntry directory_entry;
    bool* block_bitmap;

	//读取文件时候指针的移动
	unsigned int superblock_offset;
	unsigned int inodelist_offset;
	unsigned int blocklist_offset;
	unsigned int single_inode_offset;
	unsigned int total_inode_offset;
	unsigned int single_datablock_offset;
	unsigned int total_datablock_offset;
	unsigned int auto_offset;
public:
    FileSystem();
    ~FileSystem();
    void initMultiuser();
    void splitCommand(string&);
    void discardBlank(string&);
    void command();
	void initFileSyatem();

    //command
    void userRegister();
    void userLogin();
	void help();
	void ls();


    //
    void printCurrentPath();
	void printProgressBar();
	void readinode(unsigned int inode_id);
	void readBlock(unsigned int block_id,unsigned int type);
};

#endif