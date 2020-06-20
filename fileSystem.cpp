#include "fileSystem.h"

void initVariable() {
	strcpy(curname, "root");
	path[0] = 0;
	num = 1;
}
//申请一个i节点
int ialloc() 
{
	//有空闲i结点，从小到大分配，返回编号-1
	if (superblock.fiptr > 0) {
		int temp = superblock.fistack[FREE_INODE_MAX_NUM - superblock.fiptr]; //i节点当前可用
		superblock.fistack[FREE_INODE_MAX_NUM - superblock.fiptr] = -1;
		superblock.fiptr--;
		return temp;
	}
	return -1;
}
//归还i节点
void ifree(int index) 
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out); //打开文件
	disk.seekp(BLK_SIZE_TOTAL + INODE_SIZE * index); // 514是超级块的大小，加上64*index是i节点块的偏移，定位
	disk << setw(INODE_SIZE) << ' '; //清空i节点的内容
	disk.close();
	for (int i = FREE_INODE_MAX_NUM - superblock.fiptr; i < FREE_INODE_MAX_NUM; i++) //找到新释放的i节点号在空闲节点号栈的位置并插入
	{
		if (superblock.fistack[i] < index) //保证索引号按从小到大的顺序排列
		{
			superblock.fistack[i - 1] = superblock.fistack[i];
		}
		else {
			superblock.fistack[i - 1] = index;
			break;
		}
	}
	superblock.fiptr++;
}
//读i节点信息
void readInode(int index, INODE& inode) 
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out); //打开文件
	disk.seekg(BLK_SIZE_TOTAL + INODE_SIZE * index); //超级块、每个i节点64B
	disk >> inode.fsize;
	disk >> inode.fbnum;
	for (int i = 0; i < DIRECT_ADR_NUM; i++)
		disk >> inode.addr[i]; //读出4个直接盘块号
	disk >> inode.addr1;
	disk >> inode.addr2;
	disk >> inode.ower;
	disk >> inode.grouper;
	disk >> inode.mode;
	disk >> inode.ctime;
	disk.close();
}
//写i节点
void writeInode(INODE inode, int index) 
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	disk.seekp(BLK_SIZE_TOTAL + INODE_SIZE * index);
	disk << setw(6) << inode.fsize;
	disk << setw(6) << inode.fbnum;
	for (int i = 0; i < DIRECT_ADR_NUM; i++)
		disk << setw(BLK_NUM_SIZE) << inode.addr[i];
	disk << setw(BLK_NUM_SIZE) << inode.addr1;
	disk << setw(BLK_NUM_SIZE) << inode.addr2;
	disk << setw(USER_NAME_SIZE) << inode.ower;
	disk << setw(USER_GROUP_SIZE) << inode.grouper;
	disk << setw(FILE_MODE_SIZE + 1) << inode.mode; //tsy:我也不知道这里为什么要在原长度上+1，不加就是不好使，debug也没找出为啥，不加最终导致time数组越界
	//disk << setw(12) << inode.mode;
	disk << setw(TIME_SIZE + 1) << inode.ctime;
	//disk << setw(10) << inode.ctime;
	disk.close();
}
//申请空闲盘块
int balloc() 
{
	int temp = superblock.fbstack[10 - superblock.fbptr];
	if (1 == superblock.fbptr) //栈满
	{
		if (0 == temp) {
			return -1;
		}
		superblock.fbstack[BLK_GROUP_NUM - superblock.fbptr] = -1;
		superblock.fbptr = 0;

		int id, num = 0;
		fstream disk;
		disk.open("disk.txt", ios::in | ios::out);
		disk.seekg(BLK_SIZE_TOTAL * temp);
		for (int i = 0; i < BLK_GROUP_NUM; i++) {
			disk >> id;
			num++;
			if (0 == id) break; //0表示是最后一组
		}
		disk.seekg(BLK_SIZE_TOTAL * temp);
		for (int j = BLK_GROUP_NUM - num; j < BLK_GROUP_NUM; j++) {
			disk >> id;
			superblock.fbstack[j] = id;
		}
		superblock.fbptr = num;

		disk.seekp(BLK_SIZE_TOTAL * temp); //回收盘块号
		disk << setw(BLK_SIZE) << ' ';
		disk.close();
		return temp;
	}
	//若不是组长块，则直接分配
	superblock.fbstack[10 - superblock.fbptr] = -1;
	superblock.fbptr--;
	return temp;
}
//回收新释放的盘块
void bfree(int index) 
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	disk.seekp(BLK_SIZE_TOTAL * index);
	disk << setw(BLK_SIZE) << ' '; //清空待回收的盘块
	disk.close();
	if (10 == superblock.fbptr) //空闲盘块栈已满
	{
		disk.open("disk.txt", ios::in | ios::out);
		disk.seekp(BLK_SIZE_TOTAL * index);
		for (int i = 0; i < BLK_GROUP_NUM; i++) {
			disk << setw(BLK_NUM_SIZE) << superblock.fbstack[i];
			superblock.fbstack[i] = -1;
		}
		disk.close();
		superblock.fbptr = 0;
	}
	superblock.fbstack[10 - superblock.fbptr - 1] = index; //待回收盘号入栈
	superblock.fbptr++;
}
//读超级块；；；文件卷中第一块就是超级快。块大小应该从initial中推知，应未占满
void readsupblk() 
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	int i;
	for (i = 0; i < FREE_INODE_MAX_NUM; i++) {
		disk >> superblock.fistack[i]; //磁盘中内容分给空闲i节点栈；；磁盘中什么内容？
	}
	disk >> superblock.fiptr;
	for (i = 0; i < BLK_GROUP_NUM; i++) {
		disk >> superblock.fbstack[i]; //空闲块号栈
	}
	disk >> superblock.fbptr;
	disk >> superblock.inum;
	disk >> superblock.bnum;
	disk.close();
}
//写超级块
void writesupblk() 
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	int i;
	for (i = 0; i < FREE_INODE_MAX_NUM; i++) {
		disk << setw(INODE_NUM_SIZE) << superblock.fistack[i];
	}
	disk << setw(INODE_NUM_SIZE) << superblock.fiptr;
	for (i = 0; i < BLK_GROUP_NUM; i++) {
		disk << setw(BLK_NUM_SIZE) << superblock.fbstack[i];
	}
	disk << setw(BLK_NUM_SIZE) << superblock.fbptr;
	disk << setw(INODE_NUM_SIZE) << superblock.inum;
	disk << setw(BLK_NUM_SIZE) << superblock.bnum;
	disk.close();
}
//读目录
void readdir(INODE inode, int index, DIR& dir) 
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	disk.seekg(BLK_SIZE_TOTAL * inode.addr[0] + DIR_SIZE * index);
	disk >> dir.fname;
	disk >> dir.index;
	disk >> dir.parfname;
	disk >> dir.parindex;
}
//写目录
void writedir(INODE inode, DIR dir, int index) 
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	disk.seekg(BLK_SIZE_TOTAL * inode.addr[0] + DIR_SIZE * index);
	disk << setw(FILE_NAME_SIZE) << dir.fname;
	disk << setw(INODE_NUM_SIZE) << dir.index;
	disk << setw(FILE_NAME_SIZE) << dir.parfname;
	disk << setw(INODE_NUM_SIZE) << dir.parindex;
	disk.close();
}

//创建文件
void crtfile(char* filename, char* content)
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //读i结点信息
	if (authority(inode)) //判断权限
	{
		if (BLK_SIZE - inode.fsize < DIR_SIZE) //目录项已达到最多14个
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  当前目录已满，创建文件失败！";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			int i, index2;
			if (ISsame(filename, inode, i, index2)) //
			{
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  文件已存在，创建失败！";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
			else {
				cout << "  请输入文件内容：";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
				cin >> content;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				int size = strlen(content) + 1;
				cout << "  请输入文件大小（1~2048B)：";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
				cin >> size;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				if (size > FILE_CONTENT_MAX_SIZE) {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  所创建的文件内容过长，创建失败！";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				}
				else {
					int bnum = (size - 1) / BLK_SIZE + 1; //计算盘快数
					int bid[DIRECT_ADR_NUM];
					int iid = ialloc();
					if (iid != -1) {
						bool success = true;
						for (int i = 0; i < bnum; i++) {
							bid[i] = balloc();
							if (-1 == bid[i]) {
								SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
								cout << "  空闲盘块不够，创建文件失败！";
								SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
								success = false;
								ifree(iid);
								for (int j = i - 1; j >= 0; j--) {
									bfree(bid[j]);
								}
								break;
							}
						}
						if (success) {
							fstream disk;
							disk.open("disk.txt", ios::in | ios::out);
							disk.seekp(BLK_SIZE_TOTAL * inode.addr[0] + inode.fsize);
							disk << setw(FILE_NAME_SIZE) << filename; //文件名
							disk << setw(INODE_NUM_SIZE) << iid; //i结点号
							disk << setw(FILE_NAME_SIZE) << curname; //目录名
							disk << setw(BLK_NUM_SIZE) << path[num - 1]; //层数
							disk.close();
							inode.fsize += DIR_SIZE;
							char tmpbuf[TIME_SIZE];
							_strtime(tmpbuf);
							strcpy(inode.ctime, tmpbuf);
							inode2.fsize = size;
							inode2.fbnum = bnum;
							int i;
							for (i = 0; i < DIRECT_ADR_NUM; i++) //循环定义四个盘块信息
							{
								if (i < bnum) {
									inode2.addr[i] = bid[i];
								}
								else
									inode2.addr[i] = 0;
							}
							inode2.addr1 = 0;
							inode2.addr2 = 0;
							strcpy(inode2.ower, auser);
							strcpy(inode2.grouper, agroup);
							strcpy(inode2.mode, "file");
							_strtime(tmpbuf);
							strcpy(inode2.ctime, tmpbuf);
							writeInode(inode2, iid);
							char temp;
							disk.open("disk.txt", ios::in | ios::out);
							int cnum = 0;
							while (content[cnum] != '\0') //计算内容大小
							{
								cnum++;
							}
							for (int i = 0; i < bnum; i++) //将文件内容写入四个盘块
							{
								disk.seekp(BLK_SIZE_TOTAL * bid[i]);
								for (int j = 0; (j < BLK_SIZE) && (j < size); j++) {
									if ((j + i * BLK_SIZE) < cnum) {
										temp = content[j + i * BLK_SIZE];
										disk << temp;
									}
									else {
										disk << '*';
									}
								}
								size = size - BLK_SIZE;
							}
							disk.close();
							cout << "  文件"<<filename<<" 已成功创建！";
						}
					}
					else {
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
						cout << "  节点已经用完，创建数据文件失败！";
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
					}
				}
			}
		}
	}
	else {
		cout << "  没有权限";
	}
	writesupblk(); //写超级快
	writeInode(inode, path[num - 1]); //写i结点
}
//匹配大小创建
void crtfileAutoloc(char* filename, char* content)
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //读i结点信息
	if (authority(inode)) //判断权限
	{
		if (BLK_SIZE - inode.fsize < DIR_SIZE) //目录项已达到最多14个
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  当前目录已满，创建文件失败！";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			int i, index2;
			if (ISsame(filename, inode, i, index2)) //
			{
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  文件已存在，创建失败！";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
			else {
				cout << "  请输入文件内容：";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
				cin >> content;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				int size = strlen(content) + 1;
				if (size > FILE_CONTENT_MAX_SIZE) {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  所创建的文件内容过长，创建失败！";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				}
				else {
					int bnum = (size - 1) / BLK_SIZE + 1; //计算盘快数
					int bid[DIRECT_ADR_NUM];
					int iid = ialloc();
					if (iid != -1) {
						bool success = true;
						for (int i = 0; i < bnum; i++) {
							bid[i] = balloc();
							if (-1 == bid[i]) {
								SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
								cout << "  空闲盘块不够，创建文件失败！";
								SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
								success = false;
								ifree(iid);
								for (int j = i - 1; j >= 0; j--) {
									bfree(bid[j]);
								}
								break;
							}
						}
						if (success) {
							fstream disk;
							disk.open("disk.txt", ios::in | ios::out);
							disk.seekp(BLK_SIZE_TOTAL * inode.addr[0] + inode.fsize);
							disk << setw(FILE_NAME_SIZE) << filename; //文件名
							disk << setw(INODE_NUM_SIZE) << iid; //i结点号
							disk << setw(FILE_NAME_SIZE) << curname; //目录名
							disk << setw(BLK_NUM_SIZE) << path[num - 1]; //层数
							disk.close();
							inode.fsize += DIR_SIZE;
							char tmpbuf[TIME_SIZE];
							_strtime(tmpbuf);
							strcpy(inode.ctime, tmpbuf);
							inode2.fsize = size;
							inode2.fbnum = bnum;
							int i;
							for (i = 0; i < DIRECT_ADR_NUM; i++) //循环定义四个盘块信息
							{
								if (i < bnum) {
									inode2.addr[i] = bid[i];
								}
								else
									inode2.addr[i] = 0;
							}
							inode2.addr1 = 0;
							inode2.addr2 = 0;
							strcpy(inode2.ower, auser);
							strcpy(inode2.grouper, agroup);
							strcpy(inode2.mode, "file");
							_strtime(tmpbuf);
							strcpy(inode2.ctime, tmpbuf);
							writeInode(inode2, iid);
							char temp;
							disk.open("disk.txt", ios::in | ios::out);
							int cnum = 0;
							while (content[cnum] != '\0') //计算内容大小
							{
								cnum++;
							}
							for (int i = 0; i < bnum; i++) //将文件内容写入四个盘块
							{
								disk.seekp(BLK_SIZE_TOTAL * bid[i]);
								for (int j = 0; (j < BLK_SIZE) && (j < size); j++) {
									if ((j + i * BLK_SIZE) < cnum) {
										temp = content[j + i * BLK_SIZE];
										disk << temp;
									}
									else {
										disk << '*';
									}
								}
								size = size - BLK_SIZE;
							}
							disk.close();
							cout << "  文件" << filename << " 已成功创建！";
						}
					}
					else {
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
						cout << "  节点已经用完，创建数据文件失败！";
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
					}
				}
			}
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  当前登陆用户没有权限";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	writesupblk(); //写超级快
	writeInode(inode, path[num - 1]); //写i结点
}
//自动创建
void crtfileAuto(char* filename, char* content)
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //读i结点信息
	if (authority(inode)) //判断权限
	{
		if (BLK_SIZE - inode.fsize < DIR_SIZE) //目录项已达到最多14个
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  当前目录已满，创建文件失败！";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			int i, index2;
			if (ISsame(filename, inode, i, index2)) //
			{
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  文件:"<< filename<<" 已存在，创建失败！";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
			else {
				int size = strlen(content) + 1;
				if (size > FILE_CONTENT_MAX_SIZE) {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  所创建的文件内容过长，创建失败！";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				}
				else {
					int bnum = (size - 1) / BLK_SIZE + 1; //计算盘快数
					int bid[DIRECT_ADR_NUM];
					int iid = ialloc();
					if (iid != -1) {
						bool success = true;
						for (int i = 0; i < bnum; i++) {
							bid[i] = balloc();
							if (-1 == bid[i]) {
								SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
								cout << "  空闲盘块不够，创建文件失败！";
								SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
								success = false;
								ifree(iid);
								for (int j = i - 1; j >= 0; j--) {
									bfree(bid[j]);
								}
								break;
							}
						}
						if (success) {
							fstream disk;
							disk.open("disk.txt", ios::in | ios::out);
							disk.seekp(BLK_SIZE_TOTAL * inode.addr[0] + inode.fsize);
							disk << setw(FILE_NAME_SIZE) << filename; //文件名
							disk << setw(INODE_NUM_SIZE) << iid; //i结点号
							disk << setw(FILE_NAME_SIZE) << curname; //目录名
							disk << setw(BLK_NUM_SIZE) << path[num - 1]; //层数
							disk.close();
							inode.fsize += DIR_SIZE;
							char tmpbuf[TIME_SIZE];
							_strtime(tmpbuf);
							strcpy(inode.ctime, tmpbuf);
							inode2.fsize = size;
							inode2.fbnum = bnum;
							int i;
							for (i = 0; i < DIRECT_ADR_NUM; i++) //循环定义四个盘块信息
							{
								if (i < bnum) {
									inode2.addr[i] = bid[i];
								}
								else
									inode2.addr[i] = 0;
							}
							inode2.addr1 = 0;
							inode2.addr2 = 0;
							strcpy(inode2.ower, auser);
							strcpy(inode2.grouper, agroup);
							strcpy(inode2.mode, "file");
							_strtime(tmpbuf);
							strcpy(inode2.ctime, tmpbuf);
							writeInode(inode2, iid);
							char temp;
							disk.open("disk.txt", ios::in | ios::out);
							int cnum = 0;
							while (content[cnum] != '\0') //计算内容大小
							{
								cnum++;
							}
							for (int i = 0; i < bnum; i++) //将文件内容写入四个盘块
							{
								disk.seekp(BLK_SIZE_TOTAL * bid[i]);
								for (int j = 0; (j < BLK_SIZE) && (j < size); j++) {
									if ((j + i * BLK_SIZE) < cnum) {
										temp = content[j + i * BLK_SIZE];
										disk << temp;
									}
									else {
										disk << '*';
									}
								}
								size = size - BLK_SIZE;
							}
							disk.close();
							cout << "  文件" << filename << " 已成功创建！";
						}
					}
					else {
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
						cout << "  节点已经用完，创建数据文件失败！";
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
					}
				}
			}
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  当前登陆用户没有权限";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	writesupblk(); //写超级快
	writeInode(inode, path[num - 1]); //写i结点
}

//追加文件内容
void writefile(char* filename, char* content)
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //读i结点
	if (authority(inode)) //判断权限
	{
		int i, index2;
		if (!ISsame(filename, inode, i, index2)) //查看文件名是否存在
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  文件名不存在";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			readInode(index2, inode2);
			if (inode2.mode[0] == 'f') //看是否是文件
			{
				cout << "    请输入文件内容：";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
				cin >> content;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				int size = strlen(content);
				if ((size + inode2.fsize) > FILE_CONTENT_MAX_SIZE) {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  追加文件内容过长";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				}
				else {
					int bnum = (size + inode2.fsize - 1) / BLK_SIZE + 1; //计算盘快数
					int bid[DIRECT_ADR_NUM];
					for (i = 0; i < DIRECT_ADR_NUM; i++) {
						bid[i] = inode2.addr[i];
					}
					bool success = true;
					for (int i = inode2.fbnum; i < bnum; i++) {
						bid[i] = balloc();
						if (-1 == bid[i]) {
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
							cout << "  空闲盘块不够";
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
							success = false;
							for (int j = i - 1; j >= inode2.fbnum; j--) {
								bfree(bid[j]);
								return;
							}
							break;
						}
					}
					if (success) {
						fstream disk;
						char tmpbuf[TIME_SIZE];
						_strtime(tmpbuf);
						strcpy(inode.ctime, tmpbuf);
						int oldsize = inode2.fsize;
						inode2.fsize = size + inode2.fsize;
						int oldbnum = inode2.fbnum;
						inode2.fbnum = bnum;
						int i;
						for (i = 0; i < DIRECT_ADR_NUM; i++) {
							if (i < bnum) {
								inode2.addr[i] = bid[i];
							}
							else
								inode2.addr[i] = 0;
						}
						inode2.addr1 = 0;
						inode2.addr2 = 0;
						strcpy(inode2.ower, auser);
						strcpy(inode2.grouper, agroup);
						strcpy(inode2.mode, "file");
						_strtime(tmpbuf);
						strcpy(inode2.ctime, tmpbuf);
						writeInode(inode2, index2);
						char temp;
						disk.open("disk.txt", ios::in | ios::out);
						for (int i = oldbnum - 1; i < bnum; i++) {
							if (oldsize % BLK_SIZE != 0) {
								disk.seekp(BLK_SIZE_TOTAL * (bid[i]) + oldsize % BLK_SIZE);
								for (int j = 0; (j < BLK_SIZE - oldsize % BLK_SIZE) && (j < size); j++) {
									if (j < size) {
										temp = content[j];
										disk << temp;
									}
									else {
										disk << '*';
									}
								}
								size = size - BLK_SIZE + oldsize % BLK_SIZE;
							}
							for (int j = 0; (j < BLK_SIZE) && (j < size); j++) {
								if ((j + (i - oldbnum + 1) * BLK_SIZE) < size) {
									temp = content[j + (i - oldbnum + 1) * BLK_SIZE + BLK_SIZE - oldsize % BLK_SIZE];
									disk << temp;
								}
								else {
									disk << '*';
								}
							}
							size = size - BLK_SIZE;
						}
						disk.close();
						cout << "  文件:"<<filename<<" 追加内容成功！";
					}
				}
			}
			else {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  文件类型是dir，而不是file";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
		}

	}
	else {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  当前用户没有操作权限";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	writeInode(inode, path[num - 1]);
}

//删除文件
void rmfile(char* filename) 
{
	INODE inode, inode2;
	DIR dir;
	readInode(path[num - 1], inode); //当前结点写入结点对象
	if (authority(inode)) //判断权限
	{
		int i, index2 = 0;
		if (ISsame(filename, inode, i, index2)) {
			readInode(index2, inode2);
			if (authority(inode2)) {
				if (inode2.mode[0] == 'f') //删除文件
				{
					for (int ii = 0; ii < inode2.fbnum; ii++) {
						bfree(inode2.addr[ii]);
					}
					ifree(index2);
					fstream disk;
					disk.open("disk.txt", ios::in | ios::out);
					disk.seekp(BLK_SIZE_TOTAL * inode.addr[0] + DIR_SIZE * i);
					disk << setw(DIR_SIZE) << ' ';
					disk.close();
					for (int j = i + 1; j < (inode.fsize / DIR_SIZE); j++) //后面的前移一位，修改父目录信息
					{
						readdir(inode, j, dir);
						writedir(inode, dir, j - 1);
					}
					inode.fsize -= DIR_SIZE;
					char tmpbuf[TIME_SIZE];
					_strtime(tmpbuf);
					strcpy(inode.ctime, tmpbuf);
					cout << "  文件:" << filename << " 已成功删除！";
				}
				else {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  删除目录应该使用rmdir命令！";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				}
			}
			else {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  没有操作权限！";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  目录中不存在目标文件: " << filename << " ！" << endl;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  没有操作权限！";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	writesupblk();
	writeInode(inode, path[num - 1]);
}
//复制文件
void copy(char* string) 
{
	bool getit = false;
	char content[FILE_CONTENT_MAX_SIZE];
	char fname[FILE_NAME_SIZE];

	char tcurname[FILE_NAME_SIZE];
	char tpath[20];
	int tnum = num;
	strcpy(tcurname, curname); //
	cout << tcurname << endl;
	for (int i = 0; i < num; i++) {
		tpath[i] = path[i];
	}
	if (find(string)) {
		INODE inode;
		readInode(path[num - 1], inode);
		if (inode.mode[0] == 'f') {
			getit = true;
			strcpy(fname, curname);
			char temp = ' ';
			fstream disk;
			disk.open("disk.txt", ios::in | ios::out);
			int i, j;
			for (i = 0, j = 0; j < inode.fsize; i++, j++) {
				disk.seekg(BLK_SIZE_TOTAL * inode.addr[0] + i);
				disk >> temp;
				content[j] = temp;
				if (i % BLK_SIZE == 511) {
					i = i + 2;
				}
			}
			content[j + 1] = '\0';
			disk.close();
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  无法根据相关路径:" << string << " 找到文件:" << fname << " ！";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  无法根据相关路径:" << string << "找到文件:" << fname << " ！";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	strcpy(curname, tcurname); //		还原路径
	num = tnum;
	for (int ii = 0; ii < tnum; ii++) {
		path[ii] = tpath[ii];
	}
	if (getit) {
		crtfileAuto(fname, content); //创建新文件
		cout << "  文件: " << fname << " 复制成功" << curname << endl;
	}
}
//改//显示文件内容
void show(char* filename) //显示文件内容
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	int i, index2;
	if (ISsame(filename, inode, i, index2)) {
		readInode(index2, inode2);
		if (inode2.mode[0] == 'f') {
			cout << "  文件内容为：\n";
			char content[512];
			fstream disk;
			disk.open("disk.txt", ios::in | ios::out);
			for (int i = 0; i < inode2.fbnum; i++) {
				disk.seekg(inode2.addr[i] * BLK_SIZE_TOTAL);
				disk >> content;
				cout << content;
			}
			disk.close();
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  显示失败！";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  目录中不存在文件：“" << filename << "”！";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
}

void shownew(char* filename) {
	File file;
	file = returnFile(filename);
	cout << file.content;
}
//返回文章内容
File returnFile(char* filename) {
	INODE inode, inode2;
	File FileSet;
	char totalContent[2049] = {'\0'};
	readInode(path[num - 1], inode);
	int i, index2;
	if (ISsame(filename, inode, i, index2)) {
		readInode(index2, inode2);
		if (inode2.mode[0] == 'f') {
			cout << "  文件内容为：\n";
			char content[512];
			fstream disk;
			disk.open("disk.txt", ios::in | ios::out);
			for (int i = 0; i < inode2.fbnum; i++) {
				disk.seekg(inode2.addr[i] * BLK_SIZE_TOTAL);
				disk >> content;
				strcat(totalContent, content);
			}
			disk.close();
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  不是文件类型！";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  目录中不存在文件：“" << filename << "”！";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	FileSet.content = totalContent;
	FileSet.size = inode2.fsize;
	FileSet.ContentSize = strlen(totalContent);
	return FileSet;
}
//判断目录项是否存在
bool ISsame(char* dirname, INODE inode, int& i, int& index2) 
{
	bool have = false;
	char name[FILE_NAME_SIZE];
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	for (i = 0; i < (inode.fsize / DIR_SIZE); i++) {
		disk.seekg(BLK_SIZE_TOTAL * inode.addr[0] + DIR_SIZE * i);
		disk >> name;
		if (!strcmp(dirname, name)) {
			have = true;
			disk >> index2;
			break;
		}
	}
	disk.close();
	return have;
}
//创建子目录
void mkdir(char* dirname, char* username) 
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	if (authority(inode)) {
		if (BLK_SIZE - inode.fsize < DIR_SIZE) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  目录已满，无法创建子目录！";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			int i, index2;
			if (ISsame(dirname, inode, i, index2)) {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  该目录已存在，创建失败！";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
			else {
				int iid = ialloc(); //申请节点
				if (iid != -1) {
					int bid = balloc(); //申请盘块
					if (bid != -1) {
						fstream disk;
						disk.open("disk.txt", ios::in | ios::out);
						disk.seekp(BLK_SIZE_TOTAL * inode.addr[0] + inode.fsize);
						disk << setw(FILE_NAME_SIZE + 1) << dirname; //写目录名
						disk << setw(INODE_NUM_SIZE) << iid; //写节点
						disk << setw(FILE_NAME_SIZE + 1) << curname;
						disk << setw(INODE_NUM_SIZE) << path[num - 1];
						disk.close();
						inode.fsize += DIR_SIZE;
						char tmpbuf[TIME_SIZE];
						_strtime(tmpbuf);
						strcpy(inode.ctime, tmpbuf);
						inode2.fsize = 0;
						inode2.fbnum = 1;
						inode2.addr[0] = bid;
						for (int aaa = 1; aaa < DIRECT_ADR_NUM; aaa++)
							inode2.addr[aaa] = 0;
						inode2.addr1 = 0;
						inode2.addr2 = 0;
						strcpy(inode2.ower, username);
						strcpy(inode2.grouper, agroup);
						strcpy(inode2.mode, "dir");
						_strtime(tmpbuf);
						strcpy(inode2.ctime, tmpbuf);
						writeInode(inode2, iid); //写入新节点信息

						disk.open("disk.txt", ios::in | ios::out);
						disk.seekp(BLK_SIZE_TOTAL + INODE_SIZE * iid + 2 * (iid / 8));
						disk << setw(6) << 0;
						disk << setw(6) << 1;
						disk << setw(BLK_NUM_SIZE) << bid;
						disk << setw(BLK_NUM_SIZE) << 0;
						disk << setw(BLK_NUM_SIZE) << 0;
						disk << setw(BLK_NUM_SIZE) << 0;
						disk << setw(BLK_NUM_SIZE) << 0;
						disk << setw(BLK_NUM_SIZE) << 0;
						disk << setw(USER_NAME_SIZE) << username;
						disk << setw(USER_GROUP_SIZE) << agroup;
						disk << setw(FILE_MODE_SIZE + 1) << "dir";
						_strtime(tmpbuf);
						disk << setw(TIME_SIZE + 1) << tmpbuf;
						disk.close();
						cout << "  目录已成功创建！";
					}
					else {
						ifree(iid);
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
						cout << "  无空闲盘块，创建子目录失败！";
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
					}
				}
				else {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  无空闲节点，创建子目录失败！";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				}
			}
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  当前登陆用户没有创建权限！";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	writeInode(inode, path[num - 1]);
	writesupblk();
}
//删除目录
void rmdir(char* dirname, int index) //删除目录
{
	t++;
	INODE inode, inode2;
	DIR dir;
	readInode(index, inode);
	if (authority(inode)) //判断操作权限
	{
		int i, index2;
		if (ISsame(dirname, inode, i, index2)) //是否有该目录
		{
			readInode(index2, inode2);
			if (authority(inode2)) //判断权限
			{
				if (inode2.mode[0] == 'd') //是否是目录
				{
					if (inode2.fsize != 0) //是否有子目录或文件
					{
						char yes = 'y';
						if (t == 1) {
							cout << "  该目录非空，将删除目录下的所有文件，是否继续？(y/n):";
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
							cin >> yes;
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
						}
						if (('y' == yes) || ('Y' == yes)) {
							char name[FILE_NAME_SIZE];
							int index3;
							INODE inode3;
							for (int ii = 0; ii < (inode2.fsize / DIR_SIZE); ii++) {
								fstream disk;
								disk.open("disk.txt", ios::in | ios::out);
								disk.seekg(inode2.addr[0] * BLK_SIZE_TOTAL + DIR_SIZE * ii);
								disk >> name;
								disk >> index3;
								disk.close();

								readInode(index3, inode3);
								num++;
								if (inode3.mode[0] == 'd') {
									rmdir(name, index2);

								}
								else {
									rmfile(name);
								}
								num--;
							}
							bfree(inode2.addr[0]);
							ifree(index2);
							fstream disk;
							disk.open("disk.txt", ios::in | ios::out);
							disk.seekp(BLK_SIZE_TOTAL * inode.addr[0] + DIR_SIZE * i);
							disk << setw(DIR_SIZE) << ' ';
							disk.close();
							for (int j = i + 1; j < (inode.fsize / DIR_SIZE); j++) {
								readdir(inode, j, dir);
								writedir(inode, dir, j - 1);
							}
							inode.fsize -= DIR_SIZE;
							char tmpbuf[TIME_SIZE];
							_strtime_s(tmpbuf);
							strcpy_s(inode.ctime, tmpbuf);
							//cout<<"目录成功删除！";
						}
						else {
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
							cout << "  \n目录删除失败！";
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
						}
					}
					else //空
					{
						bfree(inode2.addr[0]);
						ifree(index2);
						fstream disk;
						disk.open("disk.txt", ios::in | ios::out);
						disk.seekp(BLK_SIZE_TOTAL * inode.addr[0] + DIR_SIZE * i);
						disk << setw(DIR_SIZE) << ' ';
						disk.close();
						for (int j = i + 1; j < (inode.fsize / DIR_SIZE); j++) {
							readdir(inode, j, dir);
							writedir(inode, dir, j - 1);
						}
						inode.fsize -= DIR_SIZE;
						char tmpbuf[TIME_SIZE];
						_strtime_s(tmpbuf);
						strcpy_s(inode.ctime, tmpbuf);


					}

					cout << dirname << "目录删除完毕！\n";

				}
				else {
					cout << "  文件应用rmfile命令删除";
				}
			}
			else {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  当前登陆用户没有操作权限！";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  目录中不存在该子目录！";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
	else {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  当前登陆用户没有操作权限！";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}

	writesupblk();
	writeInode(inode, index);
	t--;
}

//计算目录项的大小
int getlssize(INODE inode)
{
	int size = 0;
	if (inode.mode[0] == 'd') {
		if (inode.fsize != 0) {
			//目录项下面有目录
			char name[FILE_NAME_SIZE];
			int index1;
			INODE inode1;
			for (int ii = 0; ii < (inode.fsize / DIR_SIZE); ii++) {
				fstream disk;
				disk.open("disk.txt", ios::in | ios::out);
				disk.seekg(inode.addr[0] * BLK_SIZE_TOTAL + DIR_SIZE * ii);
				disk >> name;
				disk >> index1;
				disk.close();
				readInode(index1, inode1);

				if (inode1.mode[0] == 'd') {
					size += getlssize(inode1);

				}
				else {  //是文件
					size += inode1.fsize;
				}

			}
		}
		return size;
	}
	else {
		return inode.fsize;
	}


}
//显示目录
void ls() //显示目录
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	char name[FILE_NAME_SIZE];
	int index;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	cout << "===============================文 件 列 表=======================================\n";
	cout << setw(15) << "文件名" << setw(6) << "大小" << setw(8) << "所有者" << " ";
	cout << setw(6) << "用户组" << setw(12) << "文件类型" << setw(10) << "修改时间" << endl<<endl;
	for (int i = 0; i < (inode.fsize / DIR_SIZE); i++) {
		fstream disk;
		disk.open("disk.txt", ios::in | ios::out);
		disk.seekg(inode.addr[0] * BLK_SIZE_TOTAL + DIR_SIZE * i);
		disk >> name;
		disk >> index;
		disk.close();
		readInode(index, inode2);


		cout << setw(15) << name << setw(6) << getlssize(inode2) << setw(8) << inode2.ower << " ";
		cout << setw(6) << inode2.grouper << setw(12) << inode2.mode << setw(10) << inode2.ctime << endl;
	}
	cout << "===============================显 示 完 毕=======================================\n";

}
//跳转目录
void cd(char* string) 
{
	if (!strcmp(string, ".")) {
		cout << "  已切换到当前目录！";
		return;
	}
	if (!strcmp(string, "/")) //跳转到根目录
	{
		strcpy(curname, "root");
		path[0] = 0;
		num = 1;
		cout << "  已切换到根目录！";
		return;
	}
	if (!strcmp(string, "..")) //跳转到父目录,
	{
		if (strcmp(curname, "root")) {
			INODE inode;
			num--;
			readInode(path[num - 1], inode); //根据path中寸的前一级i结点编号，找到父结点
			char name[FILE_NAME_SIZE];
			fstream disk;
			disk.open("disk.txt", ios::in | ios::out);
			disk.seekg(inode.addr[0] * BLK_SIZE_TOTAL + 18); //因为父目录名在i结点结构体中的第18位
			disk >> name;
			disk.close();
			strcpy(curname, name);
			cout << "  已切换到父目录！" << curname << endl;
			return;
		}
		cout << "  当前已是根目录！";
		return;
	}
	char* per = strchr(string, static_cast<int>('/')); //返回string中首次出现/的位置，没有则返回空
	if (per == nullptr) {
		INODE inode, inode2;
		int i, index2;
		readInode(path[num - 1], inode);
		char name[FILE_NAME_SIZE];
		if (ISsame(string, inode, i, index2)) //index2存的节点号
		{
			readInode(index2, inode2);
			if (inode2.mode[0] == 'd') {
				fstream disk;
				disk.open("disk.txt", ios::in | ios::out);
				disk.seekg(BLK_SIZE_TOTAL * inode.addr[0] + DIR_SIZE * i);
				disk >> name;
				disk.close();
				strcpy(curname, name);
				path[num] = index2;
				num++;
				cout << "  已切换到子目录！";
				for (int i = 0; i < num; i++) {
					cout << path[i];
				}
				return;
			}
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  不能根据路径找到相关目录，因为 " << string << " 数据文件！";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  该子目录不存在，不能根据路径"<< string<< "找到相关目录";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
	else {
		char tcurname[FILE_NAME_SIZE];
		int tpath[20];
		int tnum = num;
		strcpy(tcurname, curname);
		for (int i = 0; i < num; i++) {
			tpath[i] = path[i];
		}
		if (find(string)) {
			INODE inode;
			readInode(path[num - 1], inode);
			if (inode.mode[0] == 'd') {
				cout << "  已切换到相关目录";
				return;
			}
		}
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  不能根据路径找到相关目录"; //找不到目标，还原
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		strcpy(curname, tcurname);
		num = tnum;
		for (int ii = 0; ii < tnum; ii++) {
			path[ii] = tpath[ii];
		}
	}
}
//权限管理
bool authority(INODE inode) 
{
	if (!strcmp(auser, "admin")) {
		return true;
	}
	if (!strcmp(agroup, "su") and !strcmp(auser, inode.ower)) {
		return true;
	}
	return false;
}
//根据路径找到指定文件或目录
bool find(char* string) 
{
	int ptr = 0;
	char name[FILE_NAME_SIZE] = " ";
	INODE inode;
	for (int i = 0; string[ptr] != '/'; ptr++, i++) {
		if (i == 15) return false;
		name[i] = string[ptr];
		//cout<<name[i]<<"-";
	}
	if (!strcmp(name, "root")) {
		strcpy(curname, "root");
		path[0] = 0;
		num = 1;
		for (;;) {
			readInode(path[num - 1], inode);
			ptr++;
			char name[FILE_NAME_SIZE] = " ";
			for (int i = 0; (string[ptr] != '/') && (string[ptr] != '\0'); ptr++, i++) {
				if (15 == i) return false;
				name[i] = string[ptr];
			}
			//int ii;
			int ii, index2;
			if (ISsame(name, inode, ii, index2)) {
				char tname[FILE_NAME_SIZE];
				fstream disk;
				disk.open("disk.txt", ios::in | ios::out);
				disk.seekg(BLK_SIZE_TOTAL * inode.addr[0] + DIR_SIZE * ii);
				disk >> tname;
				disk.close();
				strcpy(curname, tname);

				path[num] = index2;
				num++;
				if (string[ptr] == '\0') {
					return true;
				}
			}
			else {
				return false;
			}
		}
	}
	return false;
}
//获取用户数量
int getUserNum() 
{
	char temp[8];
	int usernum = 0;
	int i = 0;

	fstream user;
	user.open("user.txt", ios::in);
	user >> temp;
	user.close();

	while (temp[i]) {
		usernum = 10 * usernum + (temp[i++] - 0x30);
	}
	return usernum;
}
//游客注册
int guestSignup() 
{
	char auser2[8];
	char auser_new[8];
	char apwd1_new[8];

	cout << "   临时User:";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
	cin >> auser_new;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	fstream user;
	user.open("user.txt", ios::in | ios::out);
	int usernum = getUserNum(); //当前用户数量，包括管理员的游客
	for (int n = 0; n < usernum; n++) {
		user.seekg(8 + 24 * n);
		user >> auser2;
		if (strcmp(auser_new, auser2) == 0) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  用户名已存在" << endl;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			user.close();
			return 0;
		}
	}
	cout << "  set password:";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
	cin >> apwd1_new;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	usernum++;
	user.seekg(0);
	user << setw(8) << usernum; //初始化时先确定用户个数
	user.seekg(8 + 24 * (usernum - 1));
	user << setw(8) << auser_new;
	user << setw(8) << apwd1_new;
	user << setw(8) << "guest";
	user.close();
	cout << "  游客模式启动，welcome！" << endl;
	return 1;
}
//用户注册
int userSignup() {
	char auser2[8];
	char auser_new[8];
	char apwd1_new[8];

	cout << "  admin User:";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
	cin >> auser_new;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	fstream user;
	user.open("user.txt", ios::in | ios::out);
	int usernum = getUserNum(); //当前用户数量，包括管理员和游客
	for (int n = 0; n < usernum; n++) {
		user.seekg(8 + 24 * n);
		user >> auser2;
		if (strcmp(auser_new, auser2) == 0) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  用户名已存在" << endl;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			user.close();
			return 0;
		}
	}
	cout << "  set Psaaword:";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
	cin >> apwd1_new;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	usernum++;
	user.seekg(0);
	user << setw(8) << usernum; //初始化时先确定用户个数
	user.seekg(8 + 24 * (usernum - 1));
	user << setw(8) << auser_new;
	user << setw(8) << apwd1_new;
	user << setw(8) << "su";
	user.close();
	mkdir(auser_new, auser_new);
	cout << "\n   <-     已注册新用户" << auser_new << "    ->" << endl;
	return 1;
}
//登录
int login(char userword[]) 
{
	char auser2[8];
	char apwd2[8];
	/*
	for (int i = 0; i < strlen(userword); i++) {
		auser[i] = userword[i];
	}*/
	memset(auser, 0, sizeof(auser) / sizeof(char));
	strcpy(auser ,userword);
	cout << "   Password: ";
	SetConsoleTextAttribute(handle, 0);
	cin >> apwd;
	SetConsoleTextAttribute(handle, 10);
	bool have = false;
	fstream user;
	user.open("user.txt", ios::in);
	int usernum = getUserNum();

	for (int n = 0; n < usernum; n++) {
		user.seekg(8 + 24 * n);
		user >> auser2;
		user >> apwd2;
		int a = strcmp(auser, auser2);
		int b = strcmp(apwd, apwd2);
		if ((!a) && (!b)) {
			have = true;
			user >> agroup;
			if (agroup == "su")
			{
				//cout << " --------------- Welcome, administrator---------------- ";
			}
			break;
		}
	}
	user.close();
	if (have == true)
		return 1;
	return 0;
}
//命令函数
void Order() 
{
	char commond[10];
	bool have;
	for (;;) {
		have = false;
		cout << "  \n";
		getPath();
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
		cout << "$->  ";
		cin >> commond;
		if (!strcmp(commond, "cd")) {
			have = true;
			char string[100];
			cin >> string;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			cd(string);
		}
		if (!strcmp(commond, "write")) {
			have = true;
			char filename[FILE_NAME_SIZE];
			cin >> filename;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			char content[2048] = { '\0' };
			//write_new(filename, content);
			writefile(filename, content);

		}
		if (!strcmp(commond, "mkdir")) {
			have = true;
			char dirname[FILE_NAME_SIZE];
			cin >> dirname;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			mkdir(dirname);
		}
		if (!strcmp(commond, "rmdir")) {
			have = true;
			char dirname[FILE_NAME_SIZE];
			cin >> dirname;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			rmdir(dirname, path[num - 1]);
		}
		if (!strcmp(commond, "crt")) {
			have = true;
			char filename[FILE_NAME_SIZE];
			cin >> filename;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			char content[2048] = { '\0' };
			crtfile(filename, content);

		}
		if (!strcmp(commond, "crta")) {
			have = true;
			char filename[FILE_NAME_SIZE];
			cin >> filename;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			char content[2048] = { '\0' };
			crtfileAutoloc(filename, content);

		}
		if (!strcmp(commond, "copy")) {
			have = true;
			char string[100];
			cin >> string;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			copy(string);
		}
		if (!strcmp(commond, "rmfile")) {
			have = true;
			char filename[FILE_NAME_SIZE];
			cin >> filename;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			rmfile(filename);
		}
		if (!strcmp(commond, "show")) {
			have = true;
			char filename[FILE_NAME_SIZE];
			cin >> filename;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			shownew(filename);
			//show(filename);
		}
		if (!strcmp(commond, "pwd")) {
			have = true;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			cout << "  当前目录为：" << curname;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
		}
		if (!strcmp(commond, "ls")) {
			have = true;
			ls();
		}
		if (!strcmp(commond, "logout")) {
			have = true;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			cout << "  User " << auser << "log out\n";
			int times = 0;
			char in_up[8] = {'\0'};
			cout << "  若无用户，输入####进入游客模式" << endl;
			cout << "       User:";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
			cin >> in_up;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			if (in_up == "####") {
				while (!guestSignup()) {
					cout << "  重新输入注册临时User\n已有账号，则输入->login\n";
					cout << "   user->";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
					cin >> in_up;
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
					if (in_up == "login") break;
				}
				cout << "  <-LOGIN->" << endl;
				cout << "       User:";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
				cin >> in_up;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}

			while (!login(in_up))
			{
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  wrong! pls try again！\n";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				cout << "       User:";
				cin >> in_up;
			}
			cout << "  <-      Login success!    ->" << endl;
			cout << "  Welcome " << auser << endl;
		}
		if (!strcmp(commond, "useradd")) {
			have = true;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			userSignup();
		}
		if (!strcmp(commond, "reset")) {
			have = true;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			format();
			cout << "  FOS 还原一新！";
		}
		if (!strcmp(commond, "help")) {
			have = true;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			Help();
		}
		if (!strcmp(commond, "order")) {
			have = true;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			orderhelp();
		}

		if (!strcmp(commond, "exit")) {
			have = true;
			return;
		}
		if (have == false) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << commond << "FOS cannot fand this order ! please try again！";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
}
//获取路径函数
void getPath() 
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
	cout << "  root";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	INODE inode;
	int nextindex;
	char name[FILE_NAME_SIZE];

	for (int i = 0; i + 1 < num; i++)
	{
		readInode(path[i], inode); //i节点信息返回给inode
		fstream disk;
		disk.open("disk.txt", ios::in | ios::out);
		for (int j = 0; j < (inode.fsize / DIR_SIZE); j++) //dir大小为36
		{
			disk.seekg((BLK_SIZE_TOTAL)*inode.addr[0] + DIR_SIZE * j); //\n故多出2
			disk >> name;
			disk >> nextindex;
			if (nextindex == path[i + 1]) //等就跳到下一层的意思？path数组中存的什么？？？
			{
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
				cout << '/';
				cout << name;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				break;
			}
		}
		disk.close();
	}
}
//初始化
void format() 
{
	fstream user; //初始化；；读文件流到user中，利用user实现对文件的读写
	user.open("user.txt", ios::in | ios::out);
	int usernum = 1;
	user << setw(8) << usernum; //初始化时先确定用户个数；；重载<<，将8个空格和usernum写到文件中去
	user << setw(8) << "admin";
	user << setw(8) << "123";
	user << setw(8) << "su"; //超级管理员
	int i;
	for (i = 0; i < DISK_MAX_NUM; i++) {
		user << setw(BLK_SIZE) << ' '; //把512个空格写入user
		user << '\n';
	}
	user.close();
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	if (!disk.is_open()) //加了.is_open()检查文件是否打开成功
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  当前磁盘不可用!\n";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		exit(1);
	}
	for (i = 0; i < DISK_MAX_NUM; i++) {
		disk << setw(BLK_SIZE) << ' '; //把100*512个空格写入disk
		disk << '\n';
	}
	disk.seekp(0);
	//改变读入位置f.seekg(0, ios::beg); 一参数偏移量offset(long）二参数offset相对位置，三个值：  ios::beg -- 文件头    ios::end -- 文件尾    ios::cur -- 当前位置
	disk << setw(INODE_NUM_SIZE) << -1; //第一个i节点给根目录使用
	superblock.fistack[0] = -1;
	for (i = 1; i < FREE_INODE_MAX_NUM; i++) {
		disk << setw(INODE_NUM_SIZE) << i;
		superblock.fistack[i] = i;
	}
	disk << setw(INODE_NUM_SIZE) << FREE_INODE_MAX_NUM - 1;
	superblock.fiptr = FREE_INODE_MAX_NUM - 1;
	for (i = 0; i < 10; i++) {
		disk << setw(BLK_NUM_SIZE) << i + 12;
		superblock.fbstack[i] = i + 12;
	}
	disk << setw(BLK_NUM_SIZE) << 10; //组内空闲盘块数
	superblock.fbptr = 10;
	disk << setw(BLK_NUM_SIZE) << 80; //总空闲i节点数
	disk << setw(BLK_NUM_SIZE) << 89; //总空闲盘块数
	//初始化根目录i节点
	disk.seekp(BLK_SIZE_TOTAL); //来到盘块1
	disk << setw(6) << 0; //当前根目录大小0
	disk << setw(6) << 1; //占用1个磁盘块
	disk << setw(BLK_NUM_SIZE) << 11; //addr[0] = 块号为11
	disk << setw(BLK_NUM_SIZE) << 0; //addr[1]
	disk << setw(BLK_NUM_SIZE) << 0; //addr[2]
	disk << setw(BLK_NUM_SIZE) << 0; //addr[3]
	disk << setw(BLK_NUM_SIZE) << 0; //addr1
	disk << setw(BLK_NUM_SIZE) << 0; //addr2
	disk << setw(USER_NAME_SIZE) << "admin"; //创建者admin
	disk << setw(USER_GROUP_SIZE) << "su"; //创建者权限所属组为su
	disk << setw(FILE_MODE_SIZE) << "dir"; //类型为目录
	char tmpbuf[TIME_SIZE];
	_strtime(tmpbuf); //获取当前系统的时间
	disk << setw(TIME_SIZE) << tmpbuf;
	for (i = 21; i < DISK_MAX_NUM; i++) //给剩余空闲磁盘块分组
	{
		if (i % BLK_GROUP_NUM == 1) {
			disk.seekp(BLK_SIZE_TOTAL * i);
			for (int j = 0; j < 10; j++) {
				int temp = i + j + 1;
				if (temp < DISK_MAX_NUM) {
					disk << setw(BLK_NUM_SIZE) << temp;
				}
			}
		}
	}
	disk << setw(BLK_NUM_SIZE) << 0;
	disk.close();
}
void orderhelp()
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
	cout << "\n               <--     Order List      -->\n";
	cout << "---------------------------------------------------------\n";
	cout << "      1.mkdir--------------建立目录\n";
	cout << "      2.rmdir--------------删除目录\n";
	cout << "      3.cd-----------------改变当前目录\n";
	cout << "      4.ls-----------------显示当前目录中的子目录和文件\n";
	cout << "      5.pwd----------------显示当前目录\n";
	cout << "      6.crt----------------建立文件\n";
	cout << "      7.crta---------------建立(auto)文件\n";
	cout << "      8.rmfile-------------删除文件\n";
	cout << "      9.write--------------追加文件\n";
	cout << "      10.copy--------------复制文件\n";
	cout << "      11.show--------------显示文件内容\n";
	cout << "      12.reset-------------系统重置\n";
	cout << "      13.logout------------注销登录\n";
	cout << "      14.useradd-----------创建管理员\n";
	cout << "      16.order-------------显示命令表内容\n";
	cout << "      16.help--------------显示帮助内容\n";
	cout << "      17.exit--------------退出系统\n";
	cout << "-------------------------------------------------------\n";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
}

//help函数
void Help() 
{

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
	cout << "    <--     Help documentation      -->\n";
	cout << "  1.mkdir：mkdir+空格+目录名，如\"mkdir d1\",在当前目录建立子目录d1\n\n";
	cout << "  2.rmdir：rmdir+空格+目录名，如\"rmdir d1\",删除目录d1\n\n";
	cout << "  3.cd用法:3.1：cd+空格+目录名，如 \"cd d1\" ——跳转到当前目录的子目录d1\n";
	cout << "           3.2：cd+空格+两点，如\"cd ..\" ——跳转到父目录\n";
	cout << "           3.3：cd+空格+斜杠，如\"cd /\" ——跳转到根目录\n";
	cout << "           3.4：cd+空格+路径，如 \"cd root/d1/d2\" ——跳转到d2\n";
	cout << "           3.5：cd+空格+点号，如 \"cd .\" 跳转到根目录\n\n";
	cout << "  4.ls：直接使用,如\"ls\"显示当前目录中的子目录和文件 \n\n";
	cout << "  5.pwd：直接使用,如\"pwd\"显示当前目录 \n\n";
	cout << "  6.crt：crt+空格+文件名，如\"crt f1\",在当前目录下建立文件f1，手动添加文件大小\n\n";
	cout << "  7.crta：crta+空格+文件名，如\"crta f1\",在当前目录下建立文件f1,自动匹配输入大小\n\n";
	cout << "  8.rmfile：rmfile+空格+文件名，如\"rmfile f1\",删除当前目录下的文件f1\n\n";
	cout << "  9.write: write+空格+文件名，如\"write f1\",在文件f1内容后继续添加文字\n\n";
	cout << "  10.copy: copy+空格+路径，如\"copy root\\d1\\f1\",在当前目录下建立与文件f1相同的文件\n\n";
	cout << "  11.show：show+空格+文件名，如\"show f1\",显示文件f1的内容\n\n";
	cout << "  12.logout: 直接使用，退出当前账户\n\n";
	cout << "  13.reset：直接使用，功能是将系统重置到初始状态\n\n";
	cout << "  14.useradd：直接使用，功能是创建新管理员\n\n";
	cout << "  15.order: 直接使用，功能是查看系统命令\n\n";
	cout << "  16.help: 直接使用，功能是查看每个命令详细使用方法\n\n";
	cout << "  17.exit: 直接使用，功能是退出系统\n";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
}

void UI() {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
	cout << "  This is File Operating System  "<<endl;
	cout << "       No one's life would have been perfect, but no matter what,"
		<< endl << "     we must look forward, hopefully will be invincible." << endl;
	cout << endl << endl;
	int times = 0;
	char in_up[8] = {'\0'};
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	cout << "  若无用户，输入####进入游客模式" << endl;
	cout << "       User:";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
	cin >> in_up;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	if (in_up == "####") {
		while (!guestSignup()) {
			cout << "  重新输入注册临时User\n已有账号，则输入->login\n";
			cout << "   user->";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
			cin >> in_up;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			if (in_up == "login") break;
		}
		cout << "  <-    LOGIN    ->" << endl;
		cout << "       User:";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
		cin >> in_up;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}

	while (!login(in_up))
	{
		times++;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  用户名或密码不正确!\n  请重新输入！同时，你可以输入“exitos”退出程序\n";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		cout << "       User:";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
		cin >> in_up;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		if (in_up == "exitos") exit(0);
	}
	cout << "  <-      Login success!    ->" << endl;
	cout << "  Welcome " << auser << endl;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
	cout << "  你可以通过-order -help 指令获得指令信息与使用方法->" << endl;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
}

int main() {
	initVariable();
	UI();
	readsupblk();
	Order();
	writesupblk();
	return 0;
}

