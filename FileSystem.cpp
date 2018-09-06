#include "FileSystem.h"

FileSystem::FileSystem()
{
    cmd[0]="SUDO";
    cmd[1]="HELP";
    cmd[2]="LS";
    cmd[3]="CD";
    cmd[4]="TOUCH";
    cmd[5]="MKDIR";
    cmd[6]="CAT";
    cmd[7]="WRITE";
    cmd[8]="MV";
    cmd[9]="CP";
    cmd[10]="CHMOD";
    cmd[11]="RM";
    cmd[12]="PWD";
    cmd[13]="FIND";
    cmd[14]="CLEAR";
    cmd[15]="LOGOUT";
    cmd[16]="ACCOUNT";
    cmd[17]="SYSINFO";
    cmd[18]="EXIT";
    cmd[19]="RENAME";
    cmd[20]="LOGIN";
    cmd[21]="REGISTER";
    openMultiuser();
    command();
}
FileSystem::~FileSystem()
{
    delete[] multiuser;
}

void FileSystem::openMultiuser()
{
    multiuser=new Multiuser[100];
    FILE* file_pointer; //记得删除文件指针
    if ((file_pointer = fopen(multiuser_account,"wb+"))==NULL)
        cout<<"创建多用户文件失败"<<endl;
    rewind(file_pointer); //定位到文件头
    //需要登陆注册
    //fwrite(&user, userSize, 1, fp);
}

void FileSystem::command()
{
    string local_command;
    while(local_command.empty())
    {
        getline(cin,local_command);
        //去除开头的空格
        while(local_command[0]==' ')
            local_command=local_command.substr(1);
    }
    splitCommand(local_command);
}


void FileSystem::splitCommand(string &str)
{
    int i=0;
	int j=0;
    int start;
	int end;
    while(i<str.length())
    {
		start=i;
        while(str[i]!=' ')
			++i;
		end=i-1;
		//split_command[j]=str.substr(start,end-start);
		++i;
		++j;
    }
	split_command[0][0]='j';
    cout<<"0 "<<split_command[0];  
}
