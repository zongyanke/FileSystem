#include "FileSystem.h"

FileSystem::FileSystem()
{
    cmd[0]="sudo";
    cmd[1]="help";
    cmd[2]="ls";
    cmd[3]="cd";
    cmd[4]="touch";
    cmd[5]="mkdir";
    cmd[6]="cat";
    cmd[7]="write";
    cmd[8]="mv";
    cmd[9]="cp";
    cmd[10]="chmod";
    cmd[11]="rm";
    cmd[12]="pwd";
    cmd[13]="find";
    cmd[14]="clear";
    cmd[15]="logout";
    cmd[16]="account";
    cmd[17]="sysinfo";
    cmd[18]="exit";
    cmd[19]="rename";
    cmd[20]="login";
    cmd[21]="register";
	cout<<"请输入命令选择注册或者登陆：";
    while(true)
		command();
}
FileSystem::~FileSystem()
{
    delete file_pointer;

    delete[] multiuser;
}

//unfinish
void FileSystem::initMultiuser()
{
    user_num=0;
    multiuser=new Multiuser[MAX_USER_NUM];
    if ((file_pointer = fopen(MUTIUSER_ACCOUNT,"rb+"))==NULL)
    {   
        file_pointer = fopen(MUTIUSER_ACCOUNT,"wb+");
        file_pointer = fopen(MUTIUSER_ACCOUNT,"rb+");
    }
    rewind(file_pointer); //定位到文件头
    for(int i=0;i<MAX_USER_NUM;++i)
    {   
        if(fread(&multiuser[i],sizeof(Multiuser),1,file_pointer)!=1)
            break;
        ++user_num;
        cout<<"用户数"<<user_num<<endl;
    }
}

//unfinish
void FileSystem::command()
{
    string local_command;
    while(local_command.empty())
    {
        getline(cin,local_command);
        discardBlank(local_command);
    }
    splitCommand(local_command);
    switch(cmd_type)
    {
        case SUDO:
            break;
        case HELP:
            break;
        case LS:
            break;
        case CD:
            break;
        case TOUCH:
            break;
        case MKDIR:
            break;
        case CAT:
            break;
        case WRITE:
            break;
        case MV:
            break;
        case CP:
            break;
        case CHMOD:
            break;
        case RM:
            break;
        case PWD:
            break;
        case FIND:
            break;
        case CLEAR:
            break;
        case LOGOUT:
            break;
        case ACCOUNT:
            break;
        case SYSINFO:
            break;
        case EXIT:
            break;
        case RENAME:
            break;
        case LOGIN:
            userLogin();
            break;
        case REGISTER:
            userRegister();
            break;
        default:
            break;
    }
	printCurrentPath();
}

//finish
//把命令拆分，取出各个参数，并且去除空格
void FileSystem::splitCommand(string &str)
{
    int i=0;
    int j=0;
    int start;
	int end;
    while(i<str.length())
    {
        while(str[i]==' ')
            ++i;
        start=i;
        while(str[i]!=' ' && i<str.length())
			++i;
		end=i;
		split_command[j]=str.substr(start,end-start);
        ++i;
        ++j;
    }
    for(int k=0;k<CMD_NUM;++k)
        if(split_command[0]==cmd[k])
            cmd_type=k;
}

//finish
void FileSystem::discardBlank(string& str)
{
    while(str[0]==' ')
        str=str.substr(1);
}

//unfinish
//还差打开或者创建对应的文件系统
void FileSystem::userRegister()
{
    cout<<"注册提示：请注意用户名和密码的最大长度为10个字符。"<<endl;
    initMultiuser();
    Multiuser user;
    Multiuser temp_user;
    cout<<"请输入用户名：";
    while(true)
    {
        fgets(user.username,sizeof(user.username),stdin);
        if(user.username[strlen(user.username)-1]=='\n')
            user.username[strlen(user.username)-1]='\0';
        if(strlen(user.username)==0)
        {
            cout<<"\n用户名不能为空，请重新输入: ";
            continue;
        }
        else
            break;
    }
    while(user_num!=0)
    {
        bool flag=true;
        for(int i=0;i<user_num;++i)
            if(strcmp(multiuser[i].username,user.username)==0)
            {
                flag=false;
                cout<<"存在同名用户，请重新输入用户名：";
                fgets(user.username,sizeof(user.username),stdin);
                if(user.username[strlen(user.username)-1]=='\n')
                    user.username[strlen(user.username)-1]='\0';
                break;
            }
        if(flag)
            break;

    }
    cout<<"请输入密码：";
    while(true)
    {
        system("stty -echo");
        fgets(user.password,sizeof(user.password),stdin);
        if(user.password[strlen(user.password)-1]=='\n')
        user.password[strlen(user.password)-1]='\0';
        if(strlen(user.password)==0)
        {
            cout<<"\n密码不能为空，请重新输入: ";
            continue;
        }
        else
            break;
        system("stty echo");
    }
    //记得要考虑密码不能为空还有要二次确认密码
    cout<<"\n请第二次输入密码确认: ";
    while(true)
    {
        system("stty -echo");
        fgets(temp_user.password,sizeof(temp_user.password),stdin);
        if(temp_user.password[strlen(temp_user.password)-1]=='\n')
            temp_user.password[strlen(temp_user.password)-1]='\0';
        if(strcmp(temp_user.password,user.password)!=0)
        {
            cout<<"\n密码不一致，请重新输入: ";
            continue;
        }
        else
            break;
        system("stty echo");
    }
    fgets(user.system_name,sizeof(user.system_name),stdin);
    if(user.system_name[strlen(user.system_name)-1]=='\n')
        user.system_name[strlen(user.system_name)-1]='\0';
    fseek(file_pointer, 0, SEEK_END);
    fwrite(&user,sizeof(Multiuser),1,file_pointer);
    fclose(file_pointer);
    cout<<"注册成功，请重新登陆"<<endl;
}

//unfinish
void FileSystem::userLogin()
{
    initMultiuser();
    Multiuser user;
    int user_id;
    cout<<"请输入用户名：";
    fgets(user.username,sizeof(user.username),stdin);
    if(user.username[strlen(user.username)-1]=='\n')
        user.username[strlen(user.username)-1]='\0';
    bool flag=false;
    if(user_num!=0)
    {
        for(int i=0;i<user_num;++i)
            if(strcmp(multiuser[i].username,user.username)==0)
                flag=true;
    }
    if(user_num==0||flag==false)
    {
        cout<<"用户不存在，请先注册。"<<endl;
        userRegister();
        cout<<"请输入用户名：";
        fgets(user.username,sizeof(user.username),stdin);
        if(user.username[strlen(user.username)-1]=='\n')
            user.username[strlen(user.username)-1]='\0';
    }
    for(int i=0;i<user_num;++i)
            if(strcmp(multiuser[i].username,user.username)==0)
                user_id=i;
    cout<<"请输入密码：";
    while(true)
    {
        system("stty -echo");
        fgets(user.password,sizeof(user.password),stdin);
        if(user.password[strlen(user.password)-1]=='\n')
        	user.password[strlen(user.password)-1]='\0';
        system("stty echo");
        cout<<endl;
		cout<<"密码"<<user.password<<endl;
        if(strcmp(multiuser[user_id].password,user.password)==0)
        	break;
		else
			cout<<"密码错误，请重新输入密码：";
    }
    cout<<"登陆成功"<<endl;
	current_path.append(user.username);
	current_path.append(":~");
    fclose(file_pointer);
}




void FileSystem::printCurrentPath()
{
	cout<<current_path<<"$ ";
}