#include "参考.h"

/**
初始化常量，文件系统名和命令集合
finish
*/
FileSystem::FileSystem(char* name)
:blockSize(BLOCK_SIZE),
blockNum(BLOCK_NUM),
blockBitmap(new unsigned char[BLOCK_NUM+1]),
inodeBitmap(new unsigned char[BLOCK_NUM+1]),
isAlive(0),
fp(NULL),
curLink(NULL)
{
    userSize = sizeof(User);
    superBlockSize = sizeof(SuperBlock);
    blockBitmapSize = blockNum;
    inodeBitmapSize = blockNum;
    inodeSize = sizeof(Inode);
    fcbSize = sizeof(Fcb);
    itemSize = sizeof(unsigned int);

    sOffset = userSize;
    bbOffset = sOffset+sizeof(SuperBlock);
    ibOffset = bbOffset+blockBitmapSize;
    iOffset = ibOffset+inodeBitmapSize;
    bOffset = iOffset+sizeof(Inode)*blockNum;

    if(name == NULL || strcmp(name, "") == 0)
        strcpy(this->name, SYSTEM_NAME);
    else
        strcpy(this->name, name);

    SYS_CMD[0] = new char[5];
    strcpy(SYS_CMD[0], "sudo");
    SYS_CMD[1] = new char[5];
    strcpy(SYS_CMD[1], "help");
    SYS_CMD[2] = new char[3];
    strcpy(SYS_CMD[2], "ls");
    SYS_CMD[3] = new char[3];
    strcpy(SYS_CMD[3], "cd");
    SYS_CMD[4] = new char[6];
    strcpy(SYS_CMD[4], "mkdir");
    SYS_CMD[5] = new char[6];
    strcpy(SYS_CMD[5], "touch");
    SYS_CMD[6] = new char[4];
    strcpy(SYS_CMD[6], "cat");
    SYS_CMD[7] = new char[6];
    strcpy(SYS_CMD[7], "write");
    SYS_CMD[8] = new char[3];
    strcpy(SYS_CMD[8], "rm");
    SYS_CMD[9] = new char[3];
    strcpy(SYS_CMD[9], "mv");
    SYS_CMD[10] = new char[3];
    strcpy(SYS_CMD[10], "cp");
    SYS_CMD[11] = new char[6];
    strcpy(SYS_CMD[11], "chmod");
    SYS_CMD[12] = new char[7];
    strcpy(SYS_CMD[12], "logout");
    SYS_CMD[13] = new char[5];
    strcpy(SYS_CMD[13], "exit");
    SYS_CMD[14] = new char[8];
    strcpy(SYS_CMD[14], "sysinfo");
    SYS_CMD[15] = new char[6];
    strcpy(SYS_CMD[15], "clear");
    SYS_CMD[16] = new char[8];
    strcpy(SYS_CMD[16], "account");
}

/**
释放动态分配的控件
finish
*/
FileSystem::~FileSystem()
{
    if(blockBitmap != NULL)
    {
        delete blockBitmap;
        blockBitmap = NULL;
    }

    if(inodeBitmap != NULL)
    {
        delete inodeBitmap;
        inodeBitmap = NULL;
    }

    if(SYS_CMD[0] != NULL)
    {
        delete []SYS_CMD;
        SYS_CMD[0] = NULL;
    }

    if(curLink != NULL)
    {
        releaseFcbLink(curLink);
    }
}

/**
系统初始化,启动系统时调用
finish
*/
int FileSystem::init()
{
    if(isAlive)
        return -1;
    //signal(SIGINT,stopHandle);
    openFileSystem();
    login();
    command();
    return 0;
}

/**
创建文件系统
finish
*/
void FileSystem::createFileSystem()
{
    printf("create file system...\n");

    if ((fp = fopen(name,"wb+"))==NULL)
    {
        printf("open file %s error...\n",name);
        ::exit(-1);
    }

    //init user
    printf("username:");
    fgets(user.username, sizeof(user.username), stdin);
    if(user.username[strlen(user.username)-1] == '\n')
        user.username[strlen(user.username)-1] = '\0';
    system("stty -echo");
    printf("password:");
    fgets(user.password, sizeof(user.password), stdin);
    if(user.password[strlen(user.password)-1] == '\n')
        user.password[strlen(user.password)-1] = '\0';
    system("stty echo");
    printf("\n");
    printf("username:%s\npassword:%s\n", user.username, user.password);
    setUser(user);

    //init superBlock
    superBlock.blockSize = blockSize;
    superBlock.blockNum = blockNum;
    //分配一个给root
    superBlock.inodeNum = 1;
    superBlock.blockFree = blockNum-1;
    updateSuperBlock(superBlock);

    //init two bitmaps
    unsigned long i;
    //分配一个给root
    blockBitmap[0] = 1;
    inodeBitmap[0] = 1;
    for(i = 1; i < blockNum; i++)
    {
        blockBitmap[i] = 0;
        inodeBitmap[i] = 0;
    }
    updateBlockBitmap(blockBitmap, 0, blockBitmapSize);
    updateInodeBitmap(inodeBitmap, 0, inodeBitmapSize);

    //init inode and block
    long len = 0;
    len += (inodeSize+ blockSize) * blockNum;
    for (i = 0; i < len; i++)
    {
        fputc(0, fp);
    }

    //init root dir
    //set root inode info
    curInode.id = 0;
    strcpy(curInode.name, "/");
    curInode.isDir = 1;
    curInode.parent = inodeSize;
    curInode.length = 0;
    curInode.type = 0;
    time(&(curInode.time));
    for(i = 0; i < 12; i++)
        curInode.addr[i] = 0;
    curInode.blockId = 0;
    //write root inode
    updateInode(curInode);
    fflush(fp);
    //get curLink info
    getFcbLink(curLink, curInode);
    //get current path
    curPath = "/";
    printf("create file system %s finish.\n", this->name);

}

/**
打开文件系统，若不存在，则创建
finish
*/
void FileSystem::openFileSystem()
{
    if((fp = fopen(this->name,"rb"))==NULL)
    {
        createFileSystem();
    }
    else
    {
        printf("open file system...\n");
        if ((fp=fopen(this->name,"rb+"))==NULL)
        {
            printf("open file %s error...\n", name);
            ::exit(1);
        }
        rewind(fp);
        //read header
        getUser(&user);
        getSuperBlock(&superBlock);
        getBlockBitmap(blockBitmap);
        getInodeBitmap(inodeBitmap);
        //get current inode
        getInode(&curInode, 0);
        //get curLink info
        getFcbLink(curLink, curInode);
        //get current path
        curPath = "/";
        printf("open file system %s success\n", this->name);
    }
}

/**
显示帮助信息
finish
*/
void FileSystem::help()
{
    printf("command: \n\
    help    ---  show help menu \n\
    sysinfo ---  show system base information \n\
    logout  ---  exit user \n\
    account ---  modify username and password \n\
    clear   ---  clear the screen \n\
    ls      ---  list the digest of the directory's children \n\
    ls -l   ---  list the detail of the directory's children \n\
    cd      ---  change directory \n\
    mkdir   ---  make directory   \n\
    touch   ---  create a new file \n\
    cat     ---  read a file \n\
    write   ---  write something to a file \n\
    rm      ---  delete a directory or a file \n\
    cp      ---  cp a directory file to another directory or file (not finish)\n\
    mv      ---  rename a file or directory \n\
    chmod   ---  change the authorizatino of a directory or a file \n\
    exit    ---  exit this system\n");
}

/**
列出目录下所有文件(目录)名
finish
*/
void FileSystem::ls()
{
    //printf("ls %s\n", curInode.name);
    FcbLink link = curLink->next;
    while(link != NULL)
    {
        showFileDigest(link);
        link = link->next;
    }
}

/**
列出目录下所有文件(目录)的详细信息
finish
*/
void FileSystem::ls_l()
{
    //printf("ls -l %s\n", curInode.name);
    FcbLink link = curLink->next;
    PInode pInode = new Inode();
    while(link != NULL)
    {
        pInode->id = link->fcb.id;
        getInode(pInode, pInode->id);
        showFileDetail(pInode);
        link = link->next;
    }
}

/**
进入目录,暂时只支持进入子目录和返回父目录
finish
*/
int FileSystem::cd(char* name)
{
    //printf("cd from %s to child %s\n", curInode.name, name);
    unsigned int id;
    //回到父目录
    if(strcmp(name, "..") == 0)
    {
        id = curInode.parent;
        if(curInode.id > 0)
        {
            getInode(&curInode, id);
            getFcbLink(curLink, curInode);
            int pos = curPath.rfind('/', curPath.length()-2);
            curPath.erase(pos+1, curPath.length()-1-pos);
        }
        return 0;
    }
    id = findChildInode(curLink, name);
    if(id > 0)
    {
        getInode(&curInode, id);
        getFcbLink(curLink, curInode);
        curPath.append(name);
        curPath.append(1, '/');
        return 0;
    }
    else
    {
        printf("no such file or directory:%s\n", name);
        return -1;
    }
}

/**
创建文件或目录
flag:0-file,1-dir
finish
*/
int FileSystem::createFile(char * name, unsigned char isDir)
{
    if(name == NULL || strcmp(name, "") == 0 || findChildInode(curLink, name) > 0)
    {
        printf("invalid file name:the name is empty,or the file has existed\n");
        return -1;
    }
    //printf("create file %s (isDir=%d) in %s\n", name, isDir, curInode.name);
    unsigned int index;
    unsigned int dirBlockId = getAvailableFileItem(curInode, &index);
    //找到一个可用文件项
    if(dirBlockId > 0 || curInode.id == 0)
    {
        unsigned int blockId = getAvailableBlockId();
        //找到一个空闲数据块
        if(blockId > 0)
        {
            //更新superBlock
            superBlock.blockFree--;
            superBlock.inodeNum++;
            //更新blockBitmap
            blockBitmap[blockId] = 1;
            updateBlockBitmap(blockBitmap, blockId);

            unsigned int id = getAvailableInodeId();
            //创建i节点
            PInode pInode = new Inode();
            pInode->id = id;
            strcpy(pInode->name, name);
            pInode->isDir = isDir;
            pInode->parent = curInode.id;
            pInode->length = 0;
            pInode->type = 1;
            time(&(pInode->time));
            int i;
            pInode->addr[0] = blockId;
            for(i = 1; i < 12; i++)
                pInode->addr[i] = 0;
            pInode->blockId = dirBlockId;
            //写入i节点
            updateInode(*pInode);
            //更新inodeBitmap
            inodeBitmap[id] = 1;
            updateInodeBitmap(inodeBitmap, id);
            //将文件id(i节点索引)写入目录文件
            //printf("%d register in dir %d [%d]\n", id, dirBlockId, index);
            updateItem(dirBlockId, index, id);
            //printf("%d [%d] = %d\n", dirBlockId, index, id);
            //更新目录i节点
            curInode.length++;
            time(&(curInode.time));
            updateInode(curInode);
            //更新curLink
            appendFcbLinkNode(curLink, *pInode);
            delete pInode;
            //printf("create file %s success:id=%d\n", pInode->name, pInode->id);
            return 0;
        }
        else
        {
            printf("storage space is not enough, %d\n", blockId);
            return -1;
        }
    }
    else
    {
        printf("the directory can't append file item, %d\n", dirBlockId);
        return -1;
    }

}

/**
读取文件
finish
*/
int FileSystem::read(char *name)
{
    //printf("read file %s in %s\n", name, curInode.name);
    unsigned int id = findChildInode(curLink, name);
    if(id > 0)
    {
        //读取i节点
        PInode pInode = new Inode();
        getInode(pInode, id);

        if(pInode->isDir == 0)
        {
            unsigned long len = pInode->length;
            //remember to delete it
            char* buff = new char[blockSize+1];
            int i;
            unsigned int blockId;

            //遍历10个直接索引，读取数据块
            for(i = 0; i < 10; i++)
            {
                blockId = pInode->addr[i];
                if(blockId > 0)
                {
                    if(len > blockSize)
                    {
                        len -= getData(blockId, buff, blockSize, 0);
                        printf("%s", buff);
                    }
                    else
                    {
                        len -= getData(blockId, buff, len, 0);
                        printf("%s\n", buff);
                        //read finish
                        delete buff;
                        return 0;
                    }
                }
                else
                {
                    //read finish
                    printf("\n");
                    delete buff;
                    return 0;
                }
            }
            if(len <= 0)
            {
                //read finish
                printf("\n");
                delete buff;
                return 0;
            }

            //遍历１个一级索引
            unsigned int addrBlockId = pInode->addr[10];
            int totalItem = blockSize/itemSize;
            if(addrBlockId > 0)
            {
                //遍历totalItem个直接索引
                for(i = 0; i < totalItem; i++)
                {
                    blockId = getItem(addrBlockId, i);
                    if(blockId > 0)
                    {
                        if(len > blockSize)
                        {
                            len -= getData(blockId, buff, blockSize, 0);
                            printf("%s", buff);
                        }
                        else
                        {
                            len -= getData(blockId, buff, len, 0);
                            printf("%s\n", buff);
                            //read finish
                            delete buff;
                            return 0;
                        }
                    }
                    else
                    {
                        //read finish
                        printf("\n");
                        delete buff;
                        return 0;
                    }
                }
            }
            else
            {
                //read finish
                printf("\n");
                delete buff;
                return 0;
            }
            if(len <= 0)
            {
                //read finish
                printf("\n");
                delete buff;
                return 0;
            }

            //遍历一个二级索引
            unsigned int addrBlockId2 = pInode->addr[11];
            if(addrBlockId2 > 0)
            {
                //遍历totalItem个一级索引
                unsigned j;
                for(j = 0; j < totalItem; j++)
                {
                    addrBlockId = getItem(addrBlockId2, j);
                    if(addrBlockId > 0)
                    {
                        //遍历totalItem个直接索引
                        for(i = 0; i < totalItem; i++)
                        {
                            blockId = getItem(addrBlockId, i);
                            if(blockId > 0)
                            {
                                if(len > blockSize)
                                {
                                    len -= getData(blockId, buff, blockSize, 0);
                                    printf("%s", buff);
                                }
                                else
                                {
                                    len -= getData(blockId, buff, len, 0);
                                    printf("%s\n", buff);
                                    //read finish
                                    delete buff;
                                    return 0;
                                }
                            }
                            else
                            {
                                //read finish
                                printf("\n");
                                delete buff;
                                return 0;
                            }
                        }
                    }
                    else
                    {
                        //read finish
                        printf("\n");
                        delete buff;
                        return 0;
                    }
                }
            }
            else
            {
                //read finish
                printf("\n");
                delete buff;
                return 0;
            }
            return 0;
        }
        else
        {
            printf("%s is a directory\n", name);
            return -1;
        }

    }
    else
    {
        printf("no such file or directory\n");
        return -1;
    }
    return 0;
}

/**
写入文件
未完成大文件一级、二级索引时写入
finish
*/
int FileSystem::write(char *name)
{
    //printf("write file %s in  %s\n", name, curInode.name);
    unsigned int id = findChildInode(curLink, name);
    unsigned long len = 0;
    unsigned int num;
    char ch;
    if(id > 0)
    {
        //读取i节点
        PInode pInode = new Inode();
        getInode(pInode, id);
        if(pInode->isDir == 0)
        {
            if(pInode->type == 0)
            {
                printf("file %s is Read-Only file\n", name);
                return -1;
            }
            printf("write %s: use flag \"</>\" to end\n", name);
            //remember to delete it
            char* buff = new char[blockSize+1];
            //写入10个直接索引对应数据块
            int i;
            unsigned int blockId;
            for(i = 0; i < 10; i++)
            {
                blockId = pInode->addr[i];
                if(blockId > 0)
                {
                    num = waitForInput(buff, blockSize);
                    writeData(blockId, buff, num, 0);
                    len += num;
                    if(num < blockSize)
                    {
                        pInode->length = len;
                        time(&(pInode->time));
                        updateInode(*pInode);
                        delete buff;
                        return 0;
                    }
                    else
                    {
                        printf("You have input %ld Byte dat, continue?[Y/N]", len);
                        ch = getchar();
                        if(ch != 'Y' && ch != 'y')
                        {
                            pInode->length = len;
                            time(&(pInode->time));
                            updateInode(*pInode);
                            delete buff;
                            return 0;
                        }

                    }
                }
                else
                {
                    blockId = getAvailableBlockId();
                    if(blockId > 0)
                    {
                        superBlock.blockFree--;
                        updateSuperBlock(superBlock);
                        blockBitmap[blockId] = 1;
                        updateBlockBitmap(blockBitmap, blockId);
                        pInode->addr[i] = blockId;
                        time(&(pInode->time));
                        updateInode(*pInode);
                        num = waitForInput(buff, blockSize);
                        writeData(blockId, buff, num, 0);
                        len += num;
                        if(num < blockSize)
                        {
                            pInode->length = len;
                            time(&(pInode->time));
                            updateInode(*pInode);
                            delete buff;
                            return 0;
                        }
                        else
                        {
                            printf("You have input %ld Byte dat, continue?[Y/N]", len);
                            ch = getchar();
                            if(ch != 'Y' && ch != 'y')
                            {
                                pInode->length = len;
                                time(&(pInode->time));
                                updateInode(*pInode);
                                delete buff;
                                return 0;
                            }

                        }
                    }
                    else
                    {
                        pInode->length = len;
                        time(&(pInode->time));
                        updateInode(*pInode);
                        delete buff;
                        return 0;
                    }
                }
            }
            //写入1个一级索引
            unsigned addrBlockId = pInode->addr[10];
            int itemTotal = blockSize/itemSize;
            //写入itemTotal个直接索引对应的数据块
            if(addrBlockId > 0)
            {
                 for(i = 0; i < itemTotal; i++)
                {
                    blockId = getItem(addrBlockId, i);
                    if(blockId > 0)
                    {
                        num = waitForInput(buff, blockSize);
                        writeData(blockId, buff, num, 0);
                        len += num;
                        if(num < blockSize)
                        {
                            pInode->length = len;
                            time(&(pInode->time));
                            updateInode(*pInode);
                            delete buff;
                            return 0;
                        }
                        else
                        {
                            printf("You have input %ld Byte dat, continue?[Y/N]", len);
                            ch = getchar();
                            if(ch != 'Y' && ch != 'y')
                            {
                                pInode->length = len;
                                time(&(pInode->time));
                                updateInode(*pInode);
                                delete buff;
                                return 0;
                            }

                        }
                    }
                    else
                    {
                        blockId = getAvailableBlockId();
                        if(blockId > 0)
                        {
                            superBlock.blockFree--;
                            updateSuperBlock(superBlock);
                            blockBitmap[blockId] = 1;
                            updateBlockBitmap(blockBitmap, blockId);
                            updateItem(addrBlockId, i, blockId);
                            time(&(pInode->time));
                            updateInode(*pInode);
                            num = waitForInput(buff, blockSize);
                            writeData(blockId, buff, num, 0);
                            len += num;
                            if(num < blockSize)
                            {
                                pInode->length = len;
                                time(&(pInode->time));
                                updateInode(*pInode);
                                delete buff;
                                return 0;
                            }
                            else
                            {
                                printf("You have input %ld Byte dat, continue?[Y/N]", len);
                                ch = getchar();
                                if(ch != 'Y' && ch != 'y')
                                {
                                    pInode->length = len;
                                    time(&(pInode->time));
                                    updateInode(*pInode);
                                    delete buff;
                                    return 0;
                                }

                            }
                        }
                        else
                        {
                            pInode->length = len;
                            time(&(pInode->time));
                            updateInode(*pInode);
                            delete buff;
                            return 0;
                        }
                    }
                }

            }
            else
            {
                addrBlockId = getAvailableBlockId();
                if(addrBlockId > 0)
                {
                    superBlock.blockFree--;
                    updateSuperBlock(superBlock);
                    blockBitmap[addrBlockId] = 1;
                    updateBlockBitmap(blockBitmap, addrBlockId);
                    pInode->addr[10] = addrBlockId;
                    time(&(pInode->time));
                    updateInode(*pInode);
                    //
                    for(i = 0; i < itemTotal; i++)
                    {
                        blockId = getItem(addrBlockId, i);
                        if(blockId > 0)
                        {
                            num = waitForInput(buff, blockSize);
                            writeData(blockId, buff, num, 0);
                            len += num;
                            if(num < blockSize)
                            {
                                pInode->length = len;
                                time(&(pInode->time));
                                updateInode(*pInode);
                                delete buff;
                                return 0;
                            }
                            else
                            {
                                printf("You have input %ld Byte dat, continue?[Y/N]", len);
                                ch = getchar();
                                if(ch != 'Y' && ch != 'y')
                                {
                                    pInode->length = len;
                                    time(&(pInode->time));
                                    updateInode(*pInode);
                                    delete buff;
                                    return 0;
                                }

                            }
                        }
                        else
                        {
                            blockId = getAvailableBlockId();
                            if(blockId > 0)
                            {
                                superBlock.blockFree--;
                                updateSuperBlock(superBlock);
                                blockBitmap[blockId] = 1;
                                updateBlockBitmap(blockBitmap, blockId);
                                updateItem(addrBlockId, i, blockId);
                                time(&(pInode->time));
                                updateInode(*pInode);
                                num = waitForInput(buff, blockSize);
                                writeData(blockId, buff, num, 0);
                                len += num;
                                if(num < blockSize)
                                {
                                    pInode->length = len;
                                    time(&(pInode->time));
                                    updateInode(*pInode);
                                    delete buff;
                                    return 0;
                                }
                                else
                                {
                                    printf("You have input %ld Byte dat, continue?[Y/N]", len);
                                    ch = getchar();
                                    if(ch != 'Y' && ch != 'y')
                                    {
                                        pInode->length = len;
                                        time(&(pInode->time));
                                        updateInode(*pInode);
                                        delete buff;
                                        return 0;
                                    }

                                }
                            }
                            else
                            {
                                pInode->length = len;
                                time(&(pInode->time));
                                updateInode(*pInode);
                                delete buff;
                                return 0;
                            }
                        }
                    }
                }
                else
                {
                    pInode->length = len;
                    time(&(pInode->time));
                    updateInode(*pInode);
                    delete buff;
                    return 0;
                }
            }
            //写入1个二级索引
            unsigned int addrBlockId2 = pInode->addr[11];
            int j;
            if(addrBlockId2 > 0)
            {
                //遍历itemTotal个一级索引
                for(j = 0; j < itemTotal; j++)
                {
                    addrBlockId = getItem(addrBlockId2, j);

                    if(addrBlockId > 0)
                    {
                        //遍历itemTotal个直接索引
                        for(i = 0; i < itemTotal; i++)
                        {
                            blockId = getItem(addrBlockId, i);
                            if(blockId > 0)
                            {
                                num = waitForInput(buff, blockSize);
                                writeData(blockId, buff, num, 0);
                                len += num;
                                if(num < blockSize)
                                {
                                    pInode->length = len;
                                    time(&(pInode->time));
                                    updateInode(*pInode);
                                    delete buff;
                                    return 0;
                                }
                                else
                                {
                                    printf("You have input %ld Byte dat, continue?[Y/N]", len);
                                    ch = getchar();
                                    if(ch != 'Y' && ch != 'y')
                                    {
                                        pInode->length = len;
                                        time(&(pInode->time));
                                        updateInode(*pInode);
                                        delete buff;
                                        return 0;
                                    }

                                }
                            }
                            else
                            {
                                blockId = getAvailableBlockId();
                                if(blockId > 0)
                                {
                                    superBlock.blockFree--;
                                    updateSuperBlock(superBlock);
                                    blockBitmap[blockId] = 1;
                                    updateBlockBitmap(blockBitmap, blockId);
                                    updateItem(addrBlockId, i, blockId);
                                    time(&(pInode->time));
                                    updateInode(*pInode);
                                    num = waitForInput(buff, blockSize);
                                    writeData(blockId, buff, num, 0);
                                    len += num;
                                    if(num < blockSize)
                                    {
                                        pInode->length = len;
                                        time(&(pInode->time));
                                        updateInode(*pInode);
                                        delete buff;
                                        return 0;
                                    }
                                    else
                                    {
                                        printf("You have input %ld Byte dat, continue?[Y/N]", len);
                                        ch = getchar();
                                        if(ch != 'Y' && ch != 'y')
                                        {
                                            pInode->length = len;
                                            time(&(pInode->time));
                                            updateInode(*pInode);
                                            delete buff;
                                            return 0;
                                        }

                                    }
                                }
                                else
                                {
                                    pInode->length = len;
                                    time(&(pInode->time));
                                    updateInode(*pInode);
                                    delete buff;
                                    return 0;
                                }
                            }
                        }

                    }
                    else
                    {
                        addrBlockId = getAvailableBlockId();
                        if(addrBlockId > 0)
                        {
                            superBlock.blockFree--;
                            updateSuperBlock(superBlock);
                            blockBitmap[addrBlockId] = 1;
                            updateBlockBitmap(blockBitmap, addrBlockId);
                            updateItem(addrBlockId2, j, addrBlockId);
                            time(&(pInode->time));
                            updateInode(*pInode);
                            //
                            for(i = 0; i < itemTotal; i++)
                            {
                                blockId = getItem(addrBlockId, i);
                                if(blockId > 0)
                                {
                                    num = waitForInput(buff, blockSize);
                                    writeData(blockId, buff, num, 0);
                                    len += num;
                                    if(num < blockSize)
                                    {
                                        pInode->length = len;
                                        time(&(pInode->time));
                                        updateInode(*pInode);
                                        delete buff;
                                        return 0;
                                    }
                                    else
                                    {
                                        printf("You have input %ld Byte dat, continue?[Y/N]", len);
                                        ch = getchar();
                                        if(ch != 'Y' && ch != 'y')
                                        {
                                            pInode->length = len;
                                            time(&(pInode->time));
                                            updateInode(*pInode);
                                            delete buff;
                                            return 0;
                                        }

                                    }
                                }
                                else
                                {
                                    blockId = getAvailableBlockId();
                                    if(blockId > 0)
                                    {
                                        superBlock.blockFree--;
                                        updateSuperBlock(superBlock);
                                        blockBitmap[blockId] = 1;
                                        updateBlockBitmap(blockBitmap, blockId);
                                        updateItem(addrBlockId, i, blockId);
                                        time(&(pInode->time));
                                        updateInode(*pInode);
                                        num = waitForInput(buff, blockSize);
                                        writeData(blockId, buff, num, 0);
                                        len += num;
                                        if(num < blockSize)
                                        {
                                            pInode->length = len;
                                            time(&(pInode->time));
                                            updateInode(*pInode);
                                            delete buff;
                                            return 0;
                                        }
                                        else
                                        {
                                            printf("You have input %ld Byte dat, continue?[Y/N]", len);
                                            ch = getchar();
                                            if(ch != 'Y' && ch != 'y')
                                            {
                                                pInode->length = len;
                                                time(&(pInode->time));
                                                updateInode(*pInode);
                                                delete buff;
                                                return 0;
                                            }

                                        }
                                    }
                                    else
                                    {
                                        pInode->length = len;
                                        time(&(pInode->time));
                                        updateInode(*pInode);
                                        delete buff;
                                        return 0;
                                    }
                                }
                            }
                        }
                        else
                        {
                            pInode->length = len;
                            time(&(pInode->time));
                            updateInode(*pInode);
                            delete buff;
                            return 0;
                        }
                    }
                }
            }
            else
            {
                addrBlockId2 = getAvailableBlockId();
                if(addrBlockId2 > 0)
                {
                    superBlock.blockFree--;
                    updateSuperBlock(superBlock);
                    blockBitmap[addrBlockId2] = 1;
                    updateBlockBitmap(blockBitmap, addrBlockId2);
                    pInode->addr[11] = addrBlockId2;
                    time(&(pInode->time));
                    updateInode(*pInode);
                    //遍历itemTotal一级索引
                    for(j = 0; j < itemTotal; j++)
                    {
                        addrBlockId = getItem(addrBlockId2, j);

                        if(addrBlockId > 0)
                        {
                            //遍历itemTotal个直接索引
                            for(i = 0; i < itemTotal; i++)
                            {
                                blockId = getItem(addrBlockId, i);
                                if(blockId > 0)
                                {
                                    num = waitForInput(buff, blockSize);
                                    writeData(blockId, buff, num, 0);
                                    len += num;
                                    if(num < blockSize)
                                    {
                                        pInode->length = len;
                                        time(&(pInode->time));
                                        updateInode(*pInode);
                                        delete buff;
                                        return 0;
                                    }
                                    else
                                    {
                                        printf("You have input %ld Byte dat, continue?[Y/N]", len);
                                        ch = getchar();
                                        if(ch != 'Y' && ch != 'y')
                                        {
                                            pInode->length = len;
                                            time(&(pInode->time));
                                            updateInode(*pInode);
                                            delete buff;
                                            return 0;
                                        }

                                    }
                                }
                                else
                                {
                                    blockId = getAvailableBlockId();
                                    if(blockId > 0)
                                    {
                                        superBlock.blockFree--;
                                        updateSuperBlock(superBlock);
                                        blockBitmap[blockId] = 1;
                                        updateBlockBitmap(blockBitmap, blockId);
                                        updateItem(addrBlockId, i, blockId);
                                        time(&(pInode->time));
                                        updateInode(*pInode);
                                        num = waitForInput(buff, blockSize);
                                        writeData(blockId, buff, num, 0);
                                        len += num;
                                        if(num < blockSize)
                                        {
                                            pInode->length = len;
                                            time(&(pInode->time));
                                            updateInode(*pInode);
                                            delete buff;
                                            return 0;
                                        }
                                        else
                                        {
                                            printf("You have input %ld Byte dat, continue?[Y/N]", len);
                                            ch = getchar();
                                            if(ch != 'Y' && ch != 'y')
                                            {
                                                pInode->length = len;
                                                time(&(pInode->time));
                                                updateInode(*pInode);
                                                delete buff;
                                                return 0;
                                            }

                                        }
                                    }
                                    else
                                    {
                                        pInode->length = len;
                                        time(&(pInode->time));
                                        updateInode(*pInode);
                                        delete buff;
                                        return 0;
                                    }
                                }
                            }

                        }
                        else
                        {
                            addrBlockId = getAvailableBlockId();
                            if(addrBlockId > 0)
                            {
                                superBlock.blockFree--;
                                updateSuperBlock(superBlock);
                                blockBitmap[addrBlockId] = 1;
                                updateBlockBitmap(blockBitmap, addrBlockId);
                                updateItem(addrBlockId2, j, addrBlockId);
                                time(&(pInode->time));
                                updateInode(*pInode);
                                //
                                for(i = 0; i < itemTotal; i++)
                                {
                                    blockId = getItem(addrBlockId, i);
                                    if(blockId > 0)
                                    {
                                        num = waitForInput(buff, blockSize);
                                        writeData(blockId, buff, num, 0);
                                        len += num;
                                        if(num < blockSize)
                                        {
                                            pInode->length = len;
                                            time(&(pInode->time));
                                            updateInode(*pInode);
                                            delete buff;
                                            return 0;
                                        }
                                        else
                                        {
                                            printf("You have input %ld Byte dat, continue?[Y/N]", len);
                                            ch = getchar();
                                            if(ch != 'Y' && ch != 'y')
                                            {
                                                pInode->length = len;
                                                time(&(pInode->time));
                                                updateInode(*pInode);
                                                delete buff;
                                                return 0;
                                            }

                                        }
                                    }
                                    else
                                    {
                                        blockId = getAvailableBlockId();
                                        if(blockId > 0)
                                        {
                                            superBlock.blockFree--;
                                            updateSuperBlock(superBlock);
                                            blockBitmap[blockId] = 1;
                                            updateBlockBitmap(blockBitmap, blockId);
                                            updateItem(addrBlockId, i, blockId);
                                            time(&(pInode->time));
                                            updateInode(*pInode);
                                            num = waitForInput(buff, blockSize);
                                            writeData(blockId, buff, num, 0);
                                            len += num;
                                            if(num < blockSize)
                                            {
                                                pInode->length = len;
                                                time(&(pInode->time));
                                                updateInode(*pInode);
                                                delete buff;
                                                return 0;
                                            }
                                            else
                                            {
                                                printf("You have input %ld Byte dat, continue?[Y/N]", len);
                                                ch = getchar();
                                                if(ch != 'Y' && ch != 'y')
                                                {
                                                    pInode->length = len;
                                                    time(&(pInode->time));
                                                    updateInode(*pInode);
                                                    delete buff;
                                                    return 0;
                                                }

                                            }
                                        }
                                        else
                                        {
                                            pInode->length = len;
                                            time(&(pInode->time));
                                            updateInode(*pInode);
                                            delete buff;
                                            return 0;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                pInode->length = len;
                                time(&(pInode->time));
                                updateInode(*pInode);
                                delete buff;
                                return 0;
                            }
                        }
                    }
                }
                else
                {
                    pInode->length = len;
                    time(&(pInode->time));
                    updateInode(*pInode);
                    delete buff;
                    return 0;
                }
            }
            pInode->length = len;
            time(&(pInode->time));
            updateInode(*pInode);
            delete buff;
            return 0;
        }
        else
        {
            printf("%s is a directory\n", name);
            return -1;
        }
    }
    else
    {
        printf("no such file or directory\n");
        return -1;
    }
}

/**
删除文件
finish
*/
int FileSystem::del(char* name)
{
    //printf("delete file %s in  %s\n", name, curInode.name);
    unsigned int id = findChildInode(curLink, name);
    if(id > 0)
    {
        //读取i节点
        PInode pInode = new Inode();
        getInode(pInode, id);
        //删除文件
        if(pInode->isDir == 0)
        {
            if(pInode->type == 0)
            {
                printf("file %s is Read-Only file\n", name);
                return -1;
            }
            //释放文件内容数据块
            int i;
            unsigned int blockId;
            //遍历10个直接索引，释放数据块
            for(i = 0; i < 10; i++)
            {
                blockId = pInode->addr[i];
                //printf("direct addr:%d\n", blockId);
                if(blockId > 0)
                {
                    //更新superBlock
                    superBlock.blockFree++;
                    updateSuperBlock(superBlock);
                    //释放block
                    releaseBlock(blockId);
                    //更新blockBitmap
                    blockBitmap[blockId] = 0;
                    updateBlockBitmap(blockBitmap, blockId);
                }
            }
            //遍历１个一级索引
            unsigned int addrBlockId = pInode->addr[10];
            int totalItem = blockSize/itemSize;
            if(addrBlockId > 0)
            {
                //遍历totalItem个直接索引
                for(i = 0; i < totalItem; i++)
                {
                    blockId = getItem(addrBlockId, i);
                    if(blockId > 0)
                    {
                        //更新superBlock
                        superBlock.blockFree++;
                        updateSuperBlock(superBlock);
                        //释放block
                        releaseBlock(blockId);
                        //更新blockBitmap
                        blockBitmap[blockId] = 0;
                        updateBlockBitmap(blockBitmap, blockId);
                    }
                }
                //释放一级索引块
                //更新superBlock
                superBlock.blockFree++;
                updateSuperBlock(superBlock);
                //释放block
                releaseBlock(addrBlockId);
                //更新blockBitmap
                blockBitmap[addrBlockId] = 0;
                updateBlockBitmap(blockBitmap, addrBlockId);
            }
            //遍历一个二级索引
            unsigned int addrBlockId2 = pInode->addr[11];
            if(addrBlockId2 > 0)
            {
                //遍历totalItem个一级索引
                unsigned j;
                for(j = 0; j < totalItem; j++)
                {
                    addrBlockId = getItem(addrBlockId2, j);
                    if(addrBlockId > 0)
                    {
                        //遍历totalItem个直接索引
                        for(i = 0; i < totalItem; i++)
                        {
                            blockId = getItem(addrBlockId, i);
                            if(blockId > 0)
                            {
                                //更新superBlock
                                superBlock.blockFree++;
                                updateSuperBlock(superBlock);
                                //释放block
                                releaseBlock(blockId);
                                //更新blockBitmap
                                blockBitmap[blockId] = 0;
                                updateBlockBitmap(blockBitmap, blockId);
                            }
                        }
                        //释放一级索引块
                        //更新superBlock
                        superBlock.blockFree++;
                        updateSuperBlock(superBlock);
                        //释放block
                        releaseBlock(addrBlockId);
                        //更新blockBitmap
                        blockBitmap[addrBlockId] = 0;
                        updateBlockBitmap(blockBitmap, addrBlockId);
                    }
                }
                //释放二级索引块
                //更新superBlock
                superBlock.blockFree++;
                updateSuperBlock(superBlock);
                //释放block
                releaseBlock(addrBlockId2);
                //更新blockBitmap
                blockBitmap[addrBlockId2] = 0;
                updateBlockBitmap(blockBitmap, addrBlockId2);
            }

            //释放i节点
            releaseInode(pInode->id);
            //更新superBlock
            superBlock.inodeNum--;
            updateSuperBlock(superBlock);
            //更新inodeBitmap
            inodeBitmap[pInode->id] = 0;
            updateInodeBitmap(inodeBitmap, pInode->id);
            //释放目录文件项
            releaseItem(pInode->blockId, pInode->id);
            //printf("delete file %s\n", pInode->name);
        }
        //删除目录
        else
        {
            //删除子文件...
            cd(name);
            FcbLink link = curLink->next;
            while(link != NULL)
            {
                del(link->fcb.name);
                link = link->next;
            }
            cd("..");
            if(pInode->type == 0)
            {
                printf("file %s is Read-Only file\n", name);
                return -1;
            }
            //删除目录文件本身
            //释放文件内容数据块
            int i;
            unsigned int blockId;
            //遍历11个直接索引，释放数据块
            for(i = 0; i < 11; i++)
            {
                blockId = pInode->addr[i];
                if(blockId > 0)
                {
                    //更新superBlock
                    superBlock.blockFree++;
                    updateSuperBlock(superBlock);
                    //释放block
                    releaseBlock(blockId);
                    //更新blockBitmap
                    blockBitmap[blockId] = 0;
                    updateBlockBitmap(blockBitmap, blockId);
                }
            }
            //遍历１个一级索引
            unsigned int addrBlockId = pInode->addr[11];
            int totalItem = blockSize/itemSize;
            if(addrBlockId > 0)
            {
                //遍历totalItem个直接索引
                for(i = 0; i < totalItem; i++)
                {
                    blockId = getItem(addrBlockId, i);
                    if(blockId > 0)
                    {
                        //更新superBlock
                        superBlock.blockFree++;
                        updateSuperBlock(superBlock);
                        //释放block
                        releaseBlock(blockId);
                        //更新blockBitmap
                        blockBitmap[blockId] = 0;
                        updateBlockBitmap(blockBitmap, blockId);
                    }
                }
                //释放一级索引块
                //更新superBlock
                superBlock.blockFree++;
                updateSuperBlock(superBlock);
                //释放block
                releaseBlock(addrBlockId);
                //更新blockBitmap
                blockBitmap[addrBlockId] = 0;
                updateBlockBitmap(blockBitmap, addrBlockId);
            }
            //释放i节点
            releaseInode(pInode->id);
            //更新superBlock
            superBlock.inodeNum--;
            updateSuperBlock(superBlock);
            //更新inodeBitmap
            inodeBitmap[pInode->id] = 0;
            updateInodeBitmap(inodeBitmap, pInode->id);
            //释放目录文件项
            releaseItem(pInode->blockId, pInode->id);
            //printf("delete dir %s\n", pInode->name);
        }
        //更新目录i节点
        curInode.length--;
        time(&(curInode.time));
        updateInode(curInode);
        //更新curLink
        removeFcbLinkNode(curLink, *pInode);
        delete pInode;
        return 0;
    }
    else
    {
        printf("no such file or directory\n");
        return -1;
    }

}

/**
重命名文件
finish
*/
int FileSystem::mv(char* name, char* newName)
{
    //printf("rename file %s in %s to %s\n", name, curInode.name, newName);
    unsigned int id = findChildInode(curLink, name);
    if(id > 0)
    {

        Inode inode;
        getInode(&inode, id);
        if(inode.type == 0)
        {
            printf("file %s is Read-Only file\n", name);
            return -1;
        }
        strcpy(inode.name, newName);
        time(&(inode.time));
        updateInode(inode);
        //update curLink
        FcbLink link = curLink->next;
        while(link != NULL)
        {
            if(strcmp(link->fcb.name, name) == 0)
            {
                strcpy(link->fcb.name, newName);
                return 0;
            }
            link = link->next;
        }
    }
    else
    {
        printf("no such file or directory:%s\n", name);
        return -1;
    }
}

/**
复制文件
unsurpport because input and output control is difficult
*/
int FileSystem::cp(char* src, char* dst)
{
    printf("unsurpport command\n");
    return 0;
}


/**
修改文件权限:只读/可写
finish
*/
int FileSystem::chmod(char* name, unsigned char type)
{
    //printf("chmod file %s (isDir=%d) in %s to value %d\n", name, isDir, curInode.name, type);
    unsigned int id = findChildInode(curLink, name);
    if(id > 0)
    {
        Inode inode;
        getInode(&inode, id);
        //目录
        if(inode.isDir == 1)
        {
            cd(name);
            FcbLink link = curLink->next;
            while(link != NULL)
            {
                chmod(link->fcb.name, type);
                link = link->next;
            }
            cd("..");
        }

        inode.type = type;
        time(&(inode.time));
        updateInode(inode);
        return 0;
    }
    else
    {
        printf("no such file or directory:%s\n", name);
        return -1;
    }
}

/**
登录文件系统验证
finish
*/
void FileSystem::login()
{
    char username[10];
    char password[10];
    while(1)
    {
        printf("username:");
        fgets(username, sizeof(username), stdin);
        if(username[strlen(username)-1] == '\n')
            username[strlen(username)-1] = '\0';
        system("stty -echo");
        printf("password:");
        fgets(password, sizeof(password), stdin);
        if(password[strlen(password)-1] == '\n')
            password[strlen(password)-1] = '\0';
        system("stty echo");
        printf("\n");
        if(strcmp(username,user.username)==0 && strcmp(password,user.password)==0)
            break;
        else
        {
            printf("username or password is not correct,please try again.\n");
        }
    }
}

/**
退出账户
finish
*/
void FileSystem::logout()
{
    //保存数据
    updateResource();
    printf("%s has logout\n", user.username);
    //重新打开系统并进行登录验证，由于命令循环未退出，故会继续接受命令
    openFileSystem();
    login();
}

/**
退出
finish
*/
void FileSystem::exit()
{
    //保存数据并彻底退出
    printf("Bye!\n");
    stopHandle(0);
}

/**
查看文件系统信息
finish
*/
void FileSystem::systemInfo()
{
    printf("Sum of block number:%d\n",superBlock.blockNum);
    printf("Each block size:%d\n",superBlock.blockSize);
    printf("Free of block number:%d\n",superBlock.blockFree);
    printf("Sum of inode number:%d\n",superBlock.inodeNum);
}

/**
清屏
finish
*/
void FileSystem::clr()
{
    system("clear");
}

/**
修改账户信息:用户名和密码
*/
int FileSystem::account()
{
    char username[10];
    char password[10];

    system("stty -echo");
    printf("old password:");
    fgets(password, 10, stdin);
    if(password[strlen(password)-1] == '\n')
        password[strlen(password)-1] = '\0';
    system("stty echo");
    printf("\n");
    if(strcmp(password,user.password)==0)
    {
        while(1)
        {
            printf("new username:");
            fgets(username, 10, stdin);
            if(username[strlen(username)-1] == '\n')
                username[strlen(username)-1] = '\0';
            system("stty -echo");
            printf("new password:");
            fgets(password, 10, stdin);
            if(password[strlen(password)-1] == '\n')
                password[strlen(password)-1] = '\0';
            system("stty echo");
            printf("\n");
            if(strlen(username) < 2 || strlen(password) < 4)
            {
                printf("your username or password is too long or too short,please try again\n");
            }
            else
            {
                break;
            }
        }
        strcpy(user.username, username);
        strcpy(user.password, password);
        setUser(user);
        printf("modify account info success.\n");
        return 0;
    }
    else
    {
        printf("password is not correct, you are not permitted to modify account info.\n");
        return -1;
    }
}

/**
运行文件系统，处理用户命令
finish
*/
void FileSystem::command()
{
    char input[128];
    do
    {
        showPath();
        fgets(input, 128, stdin);
        switch(analyse(input))
        {
            case HELP:
                help();
                break;
            case LS://执行ls或ls-l
                if(strcmp(cmd[1], "-l") == 0)
                    ls_l();
                else
                    ls();
                break;
            case CD:
                cd(cmd[1]);
                break;
            case MKDIR:
                createFile(cmd[1],1);
                break;
            case TOUCH:
                createFile(cmd[1],0);
                break;
            case CAT:
                read(cmd[1]);
                break;
            case WRITE:
                write(cmd[1]);
                break;
            case RM:
                if(strcmp(cmd[1], "-r") == 0)
                    del(cmd[2]);
                else
                    del(cmd[1]);
                break;
            case MV:
                mv(cmd[1], cmd[2]);
                break;
            case CP:
                if(strcmp(cmd[1], "-r") == 0)
                    cp(cmd[2], cmd[3]);
                else
                    cp(cmd[1], cmd[2]);
                break;
            case CHMOD:
                if(strcmp(cmd[1], "-r") == 0)
                    chmod(cmd[3], cmd[2][0]-'0');
                else
                    chmod(cmd[2], cmd[1][0]-'0');
                break;
            case LOGOUT:
                logout();
                break;
            case EXIT:
                exit();
                break;
            case SYSINFO:
                systemInfo();
                break;
            case CLEAR:
                clr();
                break;
            case ACCOUNT:
                account();
                break;
            default:
                printf("unsurpport command\n");
                break;
        }
    }while(1);
}



/**
private methods
*/

//user区域操作
/**
取出user数据
finish
*/
void FileSystem::getUser(User* pUser)
{
    if(fp == NULL || pUser == NULL)
    {
        return;
    }
    rewind(fp);
    fread(pUser, userSize, 1, fp);
}

/**
更新user数据
finish
*/
void FileSystem::setUser(User user)
{
    if(fp == NULL)
    {
        return;
    }
    rewind(fp);
    fwrite(&user, userSize, 1, fp);
}

//superBlock区域操作
/**
取出superBlock数据
finish
*/
void FileSystem::getSuperBlock(SuperBlock* pSuper)
{
    if(fp == NULL || pSuper == NULL)
    {
        return;
    }
    fseek(fp, sOffset, SEEK_SET);
    fread(pSuper, superBlockSize, 1, fp);
}

/**
更新superBlock数据
finish
*/
void FileSystem::updateSuperBlock(SuperBlock super)
{
    if(fp == NULL)
    {
        return;
    }
    fseek(fp, sOffset, SEEK_SET);
    fwrite(&super, superBlockSize, 1, fp);
}

//blockImage区域操作
/**
寻找一个空闲的数据块,０号已被分配给根目录，返回值大于0表示找到
finish
*/
unsigned int FileSystem::getAvailableBlockId()
{
    if(superBlock.blockFree <= 0)
    {
        printf("no free block\n");
        return 0;
    }

    int i;
    for(i = 0; i < blockNum; i++)
    {
        if(blockBitmap[i] == 0)
        {
            //printf("find block %d\n", i);
            releaseBlock(i);
            return i;
        }
    }
    printf("no free block\n");
    return 0;
}


/**
取出blockBitmap
finish
*/
void FileSystem::getBlockBitmap(unsigned char* bitmap)
{
    if(fp == NULL || bitmap == NULL)
    {
        return;
    }
    fseek(fp, bbOffset, SEEK_SET);
    fread(bitmap, blockBitmapSize, 1, fp);
}

/**
更新blockBitmap
finish
*/
void FileSystem::updateBlockBitmap(unsigned char* bitmap, unsigned int index)
{
    this->updateBlockBitmap(bitmap, index, 1);
    //printf("update blockBitmap index=%d, value=%d\n", index, bitmap[index]);
}

/**
更新blockBitmap
finish
*/
void FileSystem::updateBlockBitmap(unsigned char* bitmap, unsigned int start, unsigned int count)
{
    if(fp == NULL)
    {
        return;
    }
    fseek(fp, bbOffset+start, SEEK_SET);
    fwrite(bitmap+start, count, 1, fp);
}

//inodeImage区域操作
/**
寻找一个空闲的i节点块,０号已被分配给根目录，返回值大于0表示找到
finish
*/
unsigned int FileSystem::getAvailableInodeId()
{
    unsigned int i;
    for(i = 0; i < blockNum; i++)
    {
        if(inodeBitmap[i] == 0)
        {
            //printf("find inode %d\n", i);
            return i;
        }
    }
    return 0;
}

/**
取出inodeBitmap
finish
*/
void FileSystem::getInodeBitmap(unsigned char* bitmap)
{
    if(fp == NULL || bitmap == NULL)
    {
        return;
    }
    fseek(fp, ibOffset, SEEK_SET);
    fread(bitmap, inodeBitmapSize, 1, fp);
}

/**
更新inodeBitmap
finish
*/
void FileSystem::updateInodeBitmap(unsigned char* bitmap, unsigned int index)
{
    this->updateInodeBitmap(bitmap, index, 1);
}

/**
更新inodeBitmap
finish
*/
void FileSystem::updateInodeBitmap(unsigned char* bitmap, unsigned int start, unsigned int count)
{
    if(fp == NULL)
    {
        return;
    }
    fseek(fp, ibOffset+start, SEEK_SET);
    fwrite(bitmap+start, count, 1, fp);
}

//inode block区域操作
/**
取出一条inode
finish
*/
void FileSystem::getInode(PInode pInode, unsigned int id)
{
    if(fp == NULL || pInode == NULL)
    {
        return;
    }
    fseek(fp, iOffset+inodeSize*id, SEEK_SET);
    fread(pInode, inodeSize, 1, fp);
}

/**
更新一条inode
finish
*/
void FileSystem::updateInode(Inode inode)
{
    if(fp == NULL )
    {
        return;
    }
    fseek(fp, iOffset+inodeSize*inode.id, SEEK_SET);
    fwrite(&inode, inodeSize, 1, fp);
}

/**
释放一条inode
finish
*/
void FileSystem::releaseInode(unsigned int id)
{
    if(fp == NULL )
    {
        return;
    }
    fseek(fp, iOffset+inodeSize*id, SEEK_SET);
    int i;
    for(i = 0; i < inodeSize; i++)
    {
        fputc(0, fp);
    }
}

//data block区域操作
//地址或文件项
/**
寻找一个当前目录文件中可用的文件项，返回数据块地址和文件项索引,大于0表示找到
finish
*/
unsigned int FileSystem::getAvailableFileItem(Inode& inode, unsigned int* availableIndex)
{
    //遍历建立子文件或子目录节点
    int i;
    unsigned int index;
    unsigned int blockId;
    unsigned int fileItem;

    Inode fileInode;
    const int itemTotal = blockSize/itemSize;
    //遍历11个直接索引
    for(i = 0; i < 11; i++)
    {
        blockId = inode.addr[i];
        //直接索引数据块已分配，直接查找
        if(blockId > 0 || inode.id == 0)
        {
            //遍历直接索引下itemTotal个fileItem项(文件i节点索引),查找
            for(index = 0; index < itemTotal; index++)
            {
                fileItem = getItem(blockId, index);
                //printf("%d [%d] = %d\n", blockId, index, fileItem);
                //读取i子文件或子目录i节点
                if(fileItem  == 0)
                {
                    *availableIndex = index;
                    return blockId;
                }
            }
        }
        //直接索引数据块未分配，分配成功后立即返回，
        else
        {
            blockId = getAvailableBlockId();
            //分配成功
            if(blockId > 0)
            {
                //更新superBlock
                superBlock.blockFree--;
                updateSuperBlock(superBlock);
                //更新blockBitmap
                blockBitmap[blockId] = 1;
                updateBlockBitmap(blockBitmap, blockId);
                //更新i节点
                inode.addr[i] = blockId;
                updateInode(inode);
                //直接返回第0个
                *availableIndex = 0;
                return blockId;
            }
            //分配失败
            else
            {
                return 0;
            }
        }
    }
    //遍历一级索引1个
    unsigned int addrBlockId = inode.addr[11];
    //一级索引数据块已分配
    if(addrBlockId > 0)
    {
        //遍历一级索引下itemTotal个直接索引
        for(i = 0; i < itemTotal; i++)
        {
            //直接索引数据块已分配，直接查找
            if(blockId > 0)
            {
                //遍历直接索引下itemTotal个fileItem项(文件i节点索引),查找
                for(index = 0; index < itemTotal; index++)
                {
                    fileItem = getItem(blockId, index);
                    //读取i子文件或子目录i节点
                    if(fileItem  == 0)
                    {
                        *availableIndex = index;
                        return blockId;
                    }
                }
            }
            //直接索引数据块未分配，分配成功后立即返回，
            else
            {
                blockId = getAvailableBlockId();
                //分配成功
                if(blockId > 0)
                {
                    //更新superBlock
                    superBlock.blockFree--;
                    updateSuperBlock(superBlock);
                    //更新blockBitmap
                    blockBitmap[blockId] = 1;
                    updateBlockBitmap(blockBitmap, blockId);
                    //更新i节点
                    inode.addr[i] = blockId;
                    updateInode(inode);
                    //直接返回第0个
                    *availableIndex = 0;
                    return blockId;
                }
            //分配失败
            else
            {
                return 0;
            }
        }
        }
    }
    //一级索引数据块未分配
    else
    {
        addrBlockId = getAvailableBlockId();
        //一级索引数据块分配成功
        if(addrBlockId > 0)
        {
            //更新superBlock
            superBlock.blockFree--;
            updateSuperBlock(superBlock);
            //更新blockBitmap
            blockBitmap[addrBlockId] = 1;
            updateBlockBitmap(blockBitmap, addrBlockId);
            //更新i节点
            inode.addr[11] = addrBlockId;
            updateInode(inode);

            //一级索引数据块第一项直接索引分配数据块
            blockId = getAvailableBlockId();
            //分配成功
            if(blockId > 0)
            {
                //更新superBlock
                superBlock.blockFree--;
                updateSuperBlock(superBlock);
                //更新blockBitmap
                blockBitmap[blockId] = 1;
                updateBlockBitmap(blockBitmap, blockId);
                //更新一级索引第一项直接索引数据块
                updateItem(addrBlockId, 0, blockId);
                //直接返回第0个
                *availableIndex = 0;
                return blockId;
            }
        }
        //一级索引数据块分配失败
        else
        {
            return 0;
        }
    }
}

/**
取出一个地址项或文件项,针对目录的操作
finish
*/
unsigned int FileSystem::getItem(unsigned int blockId, unsigned int index)
{
    unsigned int value = 0;
    if(fp == NULL)
    {
        return value;
    }
    fseek(fp, bOffset+blockSize*blockId+itemSize*index, SEEK_SET);
    fread(&value, itemSize, 1, fp);
    return value;
}

/**
更新一个地址项或文件项
finish
*/
void FileSystem::updateItem(unsigned int blockId, unsigned int index, unsigned int value)
{
    if(fp == NULL)
    {
        return;
    }
    fseek(fp, bOffset+blockSize*blockId+itemSize*index, SEEK_SET);
    fwrite(&value, itemSize, 1, fp);
}

/**
释放一个地址项或文件项
finish
*/
void FileSystem::releaseItem(unsigned int blockId, unsigned int id)
{
    int itemTotal = blockSize/itemSize;
    int i;
    unsigned int itemId;
    fseek(fp, bOffset+blockSize*blockId, SEEK_SET);
    for(i = 0; i < itemTotal; i++)
    {
        fread(&itemId, itemSize, 1, fp);
        if(itemId == id)
        {
            //printf("release item id=%d in %d\n", id, blockId);
            fseek(fp, -itemSize, SEEK_CUR);
            itemId = 0;
            fwrite(&itemId, itemSize, 1, fp);
            return;
        }
    }
}


//文件内容
/**
取出一个数据块的全部或部分,针对文件的操作
finish
*/
int FileSystem::getData(unsigned int blockId, char* buff, unsigned int size, unsigned int offset)
{
    int len = 0;
    if(fp == NULL || buff == NULL || offset >= blockSize)
    {
        return len;
    }
    fseek(fp, bOffset+blockSize*blockId, SEEK_SET);
    if(size > blockSize-offset)
        size = blockSize-offset;
    len = fread(buff, size, 1, fp);
    return len;
}

/**
写入一个数据块的全部或部分,针对文件的操作
finish
*/
int FileSystem::writeData(unsigned int blockId, char* buff, unsigned int size, unsigned int offset)
{
    int len = 0;
    if(fp == NULL || buff == NULL || offset >= blockSize)
    {
        return len;
    }
    fseek(fp, bOffset+blockSize*blockId, SEEK_SET);
    if(size > blockSize-offset)
        size = blockSize-offset;
    len = fwrite(buff, size, 1, fp);
    return len;
}

/**
释放一个数据块
finish
*/
void FileSystem::releaseBlock(unsigned int blockId)
{
    if(fp == NULL)
    {
        return;
    }
    fseek(fp, bOffset+blockSize*blockId, SEEK_SET);
    int i;
    for (i = 0; i < blockSize; i++)
    {
        fputc(0, fp);
    }
    //printf("release block %d\n", blockId);
}


//定位操作
/**
寻找目录下的文件或子目录,返回i节点索引,大于０表示找到
finish
*/
unsigned int FileSystem::findChildInode(FcbLink curLink, char* name)
{
    if(curLink == NULL || name == NULL)
    {
        return 0;
    }
    FcbLink link = curLink->next;
    while(link != NULL)
    {
        if(strcmp(link->fcb.name, name) == 0)
        {
            return link->fcb.id;
        }
        link = link->next;
    }
    return 0;
}

//目录信息链操作
/**
构建一个FcbLinkNode
finish
*/
void FileSystem::getFcbLinkNode(FcbLink pNode, Inode inode)
{
    if(pNode == NULL)
    {
        return;
    }
    pNode->fcb.id = inode.id;
    strcpy(pNode->fcb.name, inode.name);
    pNode->fcb.isDir = inode.isDir;
    pNode->fcb.blockId = inode.blockId;
    pNode->next = NULL;
}


/**
建立目录的信息链
finish
*/
void FileSystem::getFcbLink(FcbLink& curLink, Inode inode)
{
    if(curLink != NULL)
    {
        releaseFcbLink(curLink);
    }
    //目录本身节点
    //printf("start read dir self inode\n");
    curLink = new FcbLinkNode();
    getFcbLinkNode(curLink, inode);
    //printf("end read dir self inode\n");
    if(inode.length <= 0)
        return;

    //遍历建立子文件或子目录节点
    int i;
    unsigned int index;
    unsigned int blockId;
    unsigned int fileItem;
    Inode fileInode;
    FcbLink pNode;
    FcbLink link = curLink;
    unsigned long len = inode.length;//子文件或子目录数
    const int itemTotal = blockSize/itemSize;
    //遍历11个直接索引
    for(i = 0; i < 11; i++)
    {
        blockId = inode.addr[i];
        if(blockId > 0 || curInode.id == 0)
        {
            //遍历直接索引下itemTotal个fileItem项(文件i节点索引)
            for(index = 0; index < itemTotal; index++)
            {
                fileItem = getItem(blockId, index);
                //读取i子文件或子目录i节点
                if(fileItem > 0)
                {
                    getInode(&fileInode, fileItem);
                    pNode = new FcbLinkNode();
                    getFcbLinkNode(pNode, fileInode);
                    link->next = pNode;
                    link = pNode;
                    len--;
                    //printf("read dir item inode: id=%d, name=%s, index=%d\n", fileItem, fileInode.name, index);
                    if(len <= 0)
                    {
                        return;
                    }

                }
            }
        }
    }
    //遍历一级索引1个
    unsigned int addrBlockId = inode.addr[11];
    if(addrBlockId > 0)
    {
        //遍历一级索引下itemTotal个直接索引
        for(i = 0; i < itemTotal; i++)
        {
            blockId = inode.addr[i];
            if(blockId > 0)
            {
                //遍历直接索引下itemTotal个fileItem项(文件i节点索引)
                for(index = 0; index < itemTotal; index++)
                {
                    fileItem = getItem(blockId, index);
                    //读取i子文件或子目录i节点
                    if(fileItem > 0)
                    {
                        getInode(&fileInode, fileItem);
                        pNode = new FcbLinkNode();
                        getFcbLinkNode(pNode, fileInode);
                        link->next = pNode;
                        link = pNode;
                        len--;
                        //printf("read dir item inode: id=%d, name=%s, index=%d\n", fileItem, fileInode.name, index);
                        if(len <= 0)
                            return;
                    }
                }
            }
        }
    }
}

/**
目录信息链增加一个节点,不能增加根节点
finish
*/
void FileSystem::appendFcbLinkNode(FcbLink curLink, Inode inode)
{
    if(curLink == NULL || inode.id <= 0)
    {
        return;
    }
    FcbLink link = curLink;
    while(link->next != NULL)
    {
        link = link->next;
    }
    FcbLink pNode = new FcbLinkNode();
    getFcbLinkNode(pNode, inode);
    link->next = pNode;
}

/**
目录信息链删除一个节点，不能删除根节点
finish
*/
void FileSystem::removeFcbLinkNode(FcbLink curLink, Inode inode)
{
    if(curLink == NULL || inode.id <= 0)
    {
        return;
    }
    FcbLink link = curLink->next;
    FcbLink last = curLink;
    while(link != NULL)
    {
        if(link->fcb.id == inode.id)
        {
            last->next = link->next;
            delete link;
            break;
        }
        last = link;
        link = link->next;
    }
}

/**
目录信息链删除一个节点，不能删除根节点
finish
*/
void FileSystem::removeFcbLinkNode(FcbLink curLink, char* name)
{
    if(curLink == NULL || name == NULL)
    {
        return;
    }
    FcbLink link = curLink->next;
    FcbLink last = curLink;
    while(link != NULL)
    {
        if(strcmp(link->fcb.name, name) == 0)
        {
            last->next = link->next;
            delete link;
            break;
        }
        last = link;
        link = link->next;
    }
}

/**
清空并释放目录信息链
finish
*/
void FileSystem::releaseFcbLink(FcbLink& curLink)
{
    FcbLink link = curLink;
    FcbLink tmp;
    while(link != NULL)
    {
        tmp = link->next;
        delete link;
        link = tmp;
    }
    curLink = NULL;
}

//private system methods
/**
分析命令
finish
*/
int FileSystem::analyse(char* str)
{
    int i;
    for(i = 0; i < 5; i++)
        cmd[i][0] = '\0';
    sscanf(str, "%s %s %s %s %s",cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);

    for(i = 1; i < 17; i++)
    {
        if(strcmp(cmd[0], SYS_CMD[i]) == 0)
        {
            return i;
        }
    }
    return 0;
}

/**
停止控制输入捕获,更新并关闭文件,最后直接退出
finish
*/
void FileSystem::stopHandle(int sig)
{
    //printf("\nUpdating resource, please wait...\n");
    updateResource();
    ::exit(0);
}

/**
更新文件系统头部信息,关闭文件
finish
*/
void FileSystem::updateResource()
{
    rewind(fp);
    fwrite(&user, userSize, 1, fp);
    fwrite(&superBlock, superBlockSize, 1, fp);
    fwrite(blockBitmap, blockBitmapSize, 1, fp);
    fwrite(inodeBitmap, inodeBitmapSize, 1, fp);
    fclose(fp);
}

/**
输出当前路径作为命令提示符号
finish
*/
void FileSystem::showPath()
{
    printf("%s@localhost %s>",user.username, curPath.data());
}

//util
/**
显示文件摘要
finish
*/
void FileSystem::showFileDigest(FcbLink pNode)
{
    if(pNode == NULL)
        return;
    printf("%s",pNode->fcb.name);
    if(pNode->fcb.isDir==1)
    {
        printf("/");
    }
    printf("\n");
}

/**
显示文件详情
finish
*/
void FileSystem::showFileDetail(PInode pInode)
{
    if(pInode == NULL)
        return;
    //format output
    if(pInode->isDir == 1)
    {
        printf("%c", 'd');
    }
    else
    {
        printf("%c", '-');
    }

    printf("%c", 'r');
    if(pInode->type == 1)
    {
        printf("%c", 'w');
    }
    else
    {
        printf("%c", '-');
    }
    printf(" %10d", pInode->length);
    printf(" %.12s", 4 + ctime(&(pInode->time)));
    printf(" %s", pInode->name);
    printf("\n");
}

/**
等待数据输入
*/
unsigned int FileSystem::waitForInput(char* buff, unsigned int limit)
{
    unsigned int len = 0;
    char ch[3];
    ch[0] = 0;
    ch[1] = 0;
    while(len < limit)
    {
        ch[2] = getchar();
        if(ch[0] == '<' && ch[1] == '/' && ch[2] == '>')
        {
            len -= 2;
            buff[len] = '\0';
            return len;
        }
        else
        {
            ch[0] = ch[1];
            ch[1] = ch[2];
            buff[len] = ch[2];
            len++;
        }
    }
    buff[len] = '\0';
    return len;
}