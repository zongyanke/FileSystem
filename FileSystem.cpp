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
    initFileSyatem();
    //到底是注册登陆之后才初始化系统还是先初始化系统
	// cout<<"请输入命令选择注册或者登陆：";
    // while(true)
	// 	command();
}
FileSystem::~FileSystem()
{
    delete file_pointer;
    delete[] multiuser;
}

void FileSystem::initFileSyatem()
{
    cout<<"正在初始化，请稍后"<<endl;
    superblock_offset=sizeof(SuperBlock);
    inodelist_offset=sizeof(inodelist);
    blocklist_offset=sizeof(blocklist);
    single_inode_offset=sizeof(Inode);
    total_inode_offset=single_inode_offset*INODE_COUTN;
    single_datablock_offset=BLOCK_SIZE;
    total_datablock_offset=single_datablock_offset*BLOCK_COUNT;
    printProgressBar();//到时候可能加一些多进程的东西进去
    if ((file_pointer = fopen(SYSTEM_NAME,"rb+"))==NULL)
    {   
        //初始化superblock
        superblock.block_size=BLOCK_SIZE;
        superblock.block_count=BLOCK_COUNT;
        superblock.block_used=1;//分配一个给root
        superblock.block_free=superblock.block_count-superblock.block_used;
        superblock.inode_count=INODE_COUTN;
        superblock.inode_used=1;//分配一个给root
        superblock.inode_free=superblock.inode_count-superblock.block_used;
        superblock.root_inode=0;//要不要从0开始算呢
        file_pointer = fopen(SYSTEM_NAME,"wb+");
        fwrite(&superblock,sizeof(SuperBlock),1,file_pointer); 
        
        //初始化inodelist
        inodelist[0]=true;//root
        for(int i=1;i<INODE_COUTN;++i)
            inodelist[i]=false;
        fwrite(&inodelist,sizeof(inodelist),1,file_pointer);

        //初始化blocklist
        blocklist[0]=-1;//代表已经到了文件末尾
        for(int i=1;i<BLOCK_COUNT;++i)
            blocklist[i]=0;
        fwrite(&blocklist,sizeof(blocklist),1,file_pointer);

        //初始化根目录的inode
        current_inode.block_id=0;
        current_inode.file_type=1;
        current_inode.link_count=1;
        current_inode.own_id=0;
        current_inode.group_id=0;
        strcpy(current_inode.authority,"drwxrw-r--");
        current_inode.file_size=sizeof(DirectoryEntry);
        current_inode.block_used_byfile=1;
        current_inode.block_id=0;
        fwrite(&current_inode,sizeof(current_inode),1,file_pointer);
        for(int i=1;i<INODE_COUTN;++i)
        {
            current_inode.block_id=-1;
            current_inode.file_type=-1;
            current_inode.link_count=-1;
            current_inode.own_id=-1;
            current_inode.group_id=-1;
            char authority[11]="----------";
            strcpy(current_inode.authority,authority);
            current_inode.file_size=sizeof(DirectoryEntry);
            current_inode.block_used_byfile=1;
            current_inode.block_id=0;
            fwrite(&current_inode,sizeof(Inode),1,file_pointer);
        }

        strcpy(directory_entry.directory_name,".");
        directory_entry.inode_identifier=0;
        fwrite(&directory_entry,sizeof(BLOCK_SIZE),1,file_pointer);
        char temp[BLOCK_SIZE];
        for (int i = 0; i < BLOCK_SIZE; i++)
            temp[i]='0';
        for (int i = 1; i < BLOCK_COUNT; i++)  
            fwrite(&temp,sizeof(temp),1,file_pointer);
    }
    else
    {
        rewind(file_pointer); 
        fread(&superblock,sizeof(SuperBlock),1,file_pointer);//写入成功
        fread(&inodelist,sizeof(inodelist),1,file_pointer);//写入成功
        fread(&blocklist,sizeof(blocklist),1,file_pointer);//写入成功
        fread(&current_inode,sizeof(Inode),1,file_pointer);//初始化的时候需要把根目录的inode找下来
    }
    current_path="/";
    cout<<"初始化成功"<<endl;
}
//unfinish
void FileSystem::initMultiuser()
{
    user_num=0;
    multiuser=new MultiUser[USER_COUNT_MAX];
    if ((file_pointer = fopen(MUTIUSER_ACCOUNT,"rb+"))==NULL)
    {   
        file_pointer = fopen(MUTIUSER_ACCOUNT,"wb+");
        file_pointer = fopen(MUTIUSER_ACCOUNT,"rb+");
    }
    rewind(file_pointer); //定位到文件头
    for(int i=0;i<USER_COUNT_MAX;++i)
    {   
        if(fread(&multiuser[i],sizeof(MultiUser),1,file_pointer)!=1)
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
    for(int k=0;k<CMD_COUNT;++k)
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
//输入密码或者用户名的时候要把前面的空格去掉，暂时还没有做，先把功能完善了再说
void FileSystem::userRegister()
{
    cout<<"注册提示：请注意用户名和密码的最大长度为10个字符。"<<endl;
    initMultiuser();
    MultiUser user;
    MultiUser temp_user;
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
    fseek(file_pointer, 0, SEEK_END);
    fwrite(&user,sizeof(MultiUser),1,file_pointer);
    fclose(file_pointer);
    cout<<"注册成功，请重新登陆"<<endl;
}

//unfinish
void FileSystem::userLogin()
{
    initMultiuser();
    MultiUser user;
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

void FileSystem::printProgressBar()
{
    int i = 0;
	char bar[102];
	const char *lable = "|/-\\";
	bar[0] = 0;
	while (i <= 100)
	{
		printf("[%-100s][%d%%][%c]\r", bar, i, lable[i%4]);
		fflush(stdout);
	    bar[i] = '#';
		i++;
		bar[i] = 0;
		usleep(100000);
	}
	printf("\n");
    return;
}