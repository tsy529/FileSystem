#include "fileSystem.h"

int ialloc() //申请一个i节点
{
	//有空闲i结点，从小到大分配，返回编号，
	//-1
	if (superblock.fiptr > 0) {
		int temp = superblock.fistack[FREE_INODE_MAX_NUM - superblock.fiptr]; //i节点当前可用
		superblock.fistack[FREE_INODE_MAX_NUM - superblock.fiptr] = -1;
		superblock.fiptr--;
		return temp;
	}
	return -1;
}

void ifree(int index) //归还i节点
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

void readInode(int index, INODE& inode) //读i节点信息
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

void writeInode(INODE inode, int index) //写i节点
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

int balloc() //申请空闲盘块
{
	int temp = superblock.fbstack[10 - superblock.fbptr];
	if (1 == superblock.fbptr) //栈满
	{
		if (0 == temp) {
			return -1;
		}
		superblock.fbstack[BLK_GROUP_NUM - superblock.fbptr] = -1;
		superblock.fbptr = 0;
		for (int i = 0; i < BLK_GROUP_NUM; i++) {
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
			disk.close();
		}
		fstream disk;
		disk.open("disk.txt", ios::in | ios::out);
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

void bfree(int index) //回收新释放的盘块
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

void readsupblk() //读超级块；；；文件卷中第一块就是超级快。块大小应该从initial中推知，应未占满
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

void writesupblk() //写超级块
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

void readdir(INODE inode, int index, DIR& dir) //读目录
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	disk.seekg(BLK_SIZE_TOTAL * inode.addr[0] + DIR_SIZE * index);
	disk >> dir.fname;
	disk >> dir.index;
	disk >> dir.parfname;
	disk >> dir.parindex;
}

void writedir(INODE inode, DIR dir, int index) //写目录
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

void mkfile(char* filename, char* content) //创建文件
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //读i结点信息
	if (authority(inode)) //判断权限
	{
		if (BLK_SIZE - inode.fsize < DIR_SIZE) //目录项已达到最多14个
		{
			cout << "当前目录已满，创建文件失败！";
		}
		else {
			int i, index2;
			if (ISsame(filename, inode, i, index2)) //
			{
				cout << "文件已存在，创建失败！";
			}
			else {
				int size = strlen(content) + 1;
				cout << "请输入文件大小（1~2048b)：";
				cin >> size;
				if (size > FILE_CONTENT_MAX_SIZE) {
					cout << "所创建的文件内容过长，创建失败！";
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
								cout << "空闲盘块不够，创建文件失败！";
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
							cout << "文件已成功创建！";
						}
					}
					else {
						cout << "节点已经用完，创建数据文件失败！";
					}
				}
			}
		}
	}
	else {
		cout << "没有权限";
	}
	writesupblk(); //写超级快
	writeInode(inode, path[num - 1]); //写i结点
}

void write(char* filename, char* content) //追加文件内容
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //读i结点
	if (authority(inode)) //判断权限
	{
		int i, index2;
		if (!ISsame(filename, inode, i, index2)) //查看文件名是否存在
		{
			cout << "文件不存在";
		}
		else {
			readInode(index2, inode2);
			if (inode2.mode[0] == 'f') //看是否是文件
			{
				int size = strlen(content);
				if ((size + inode2.fsize) > FILE_CONTENT_MAX_SIZE) {
					cout << "文件内容过长";
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
							cout << "空闲盘块不够";
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
						strcpy(inode2.mode, "file2");
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
						cout << "文件创建成功！";
					}
				}
			}
			else {
				cout << "文件类型是文件夹";
			}
		}

	}
	else {
		cout << "没有操作权限";
	}
	writeInode(inode, path[num - 1]);
}

void rmfile(char* filename) //删除文件
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
					cout << "文件已成功删除！";
				}
				else {
					cout << "删除目录应用rmdir命令！";
				}
			}
			else {
				cout << "没有操作权限！";
			}
		}
		else {
			cout << "目录中不存在该文件！";
		}
	}
	else {
		cout << "没有操作权限！";
	}
	writesupblk();
	writeInode(inode, path[num - 1]);
}

void copy(char* string) //复制文件
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
			cout << "无法根据相关路径找到文件！";
		}
	}
	else {
		cout << "无法根据相关路径找到文件！";
	}
	strcpy(curname, tcurname); //		还原路径
	num = tnum;
	for (int ii = 0; ii < tnum; ii++) {
		path[ii] = tpath[ii];
	}
	if (getit) {
		mkfile(fname, content); //创建新文件
		cout << "复制成功" << curname << endl;
	}
}

void showcontent(char* filename) //显示文件内容
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	int i, index2;
	if (ISsame(filename, inode, i, index2)) {
		readInode(index2, inode2);
		if (inode2.mode[0] == 'f') {
			cout << "文件内容为：\n";
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
			cout << "显示失败！";
		}
	}
	else {
		cout << "目录中不存在该文件！";
	}
}

bool ISsame(char* dirname, INODE inode, int& i, int& index2) //判断目录项是否存在
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

void mkdir(char* dirname) //创建子目录
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	if (authority(inode)) {
		if (BLK_SIZE - inode.fsize < DIR_SIZE) {
			cout << "目录已满，无法创建子目录！";
		}
		else {
			int i, index2;
			if (ISsame(dirname, inode, i, index2)) {
				cout << "该目录已存在，创建失败！";
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
						strcpy(inode2.ower, auser);
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
						disk << setw(USER_NAME_SIZE) << auser;
						disk << setw(USER_GROUP_SIZE) << agroup;
						disk << setw(FILE_MODE_SIZE + 1) << "dir";
						_strtime(tmpbuf);
						disk << setw(TIME_SIZE + 1) << tmpbuf;
						disk.close();
						cout << "目录已成功创建！";
					}
					else {
						ifree(iid);
						cout << "无空闲盘块，创建子目录失败！";
					}
				}
				else {
					cout << "无空闲节点，创建子目录失败！";
				}
			}
		}
	}
	else {
		cout << "没有创建权限！";
	}
	writeInode(inode, path[num - 1]);
	writesupblk();
}

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
							cout << "该目录非空，将删除目录下的所有文件，是否继续？(y/n):";
							cin >> yes;
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
							_strtime(tmpbuf);
							strcpy(inode.ctime, tmpbuf);
							//cout<<"目录成功删除！";
						}
						else {
							cout << "目录删除失败！";
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
						_strtime(tmpbuf);
						strcpy(inode.ctime, tmpbuf);
						//cout<<"目录成功删除！";
					}

					cout << "目录成功删除！";

				}
				else {
					cout << "文件应用rmfile命令删除";
				}
			}
			else {
				cout << "没有操作权限！";
			}
		}
		else {
			cout << "目录中不存在该子目录！";
		}
	}
	else {
		cout << "没有操作权限！";
	}

	writesupblk();
	writeInode(inode, index);
	t--;
}

void ls() //显示目录
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	char name[FILE_NAME_SIZE];
	int index;
	cout << setw(15) << "文件名" << setw(6) << "大小" << setw(8) << "所有者" << " ";
	cout << setw(6) << "用户组" << setw(12) << "文件类型" << setw(10) << "修改时间" << endl;
	for (int i = 0; i < (inode.fsize / DIR_SIZE); i++) {
		fstream disk;
		disk.open("disk.txt", ios::in | ios::out);
		disk.seekg(inode.addr[0] * BLK_SIZE_TOTAL + DIR_SIZE * i);
		disk >> name;
		disk >> index;
		disk.close();
		readInode(index, inode2);

		cout << setw(15) << name << setw(6) << inode2.fsize << setw(8) << inode2.ower << " ";
		cout << setw(6) << inode2.grouper << setw(12) << inode2.mode << setw(10) << inode2.ctime << endl;
	}
	cout << "显示完毕！";
}

void cd(char* string) //跳转目录
{
	if (!strcmp(string, ".")) {
		cout << "已切换到当前目录！";
		return;
	}
	if (!strcmp(string, "/")) //跳转到根目录
	{
		strcpy(curname, "root");
		path[0] = 0;
		num = 1;
		cout << "已切换到根目录！";
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
			cout << "已切换到父目录！" << curname << endl;
			return;
		}
		cout << "当前已是根目录！";
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
				cout << "已切换到子目录！";
				for (int i = 0; i < num; i++) {
					cout << path[i];
				}
				return;
			}
			cout << "不能根据路径找到相关目录，因为 " << string << " 数据文件！";
		}
		else {
			cout << "该子目录不存在，不能根据路径找到相关目录";
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
				cout << "已切换到相关目录";
				return;
			}
		}
		cout << "不能根据路径找到相关目录"; //找不到目标，还原
		strcpy(curname, tcurname);
		num = tnum;
		for (int ii = 0; ii < tnum; ii++) {
			path[ii] = tpath[ii];
		}
	}
}

bool authority(INODE inode) //权限管理
{
	if (agroup[0] == 's') {
		return true;
	}
	return false;
}

bool find(char* string) //根据路径找到指定文件或目录
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

int getUserNum() //获取用户数量
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

int signup() //注册
{
	char auser2[8];
	char auser_new[8];
	char apwd1_new[8];

	cout << "请输入新游客用户名:";
	cin >> auser_new;
	fstream user;
	user.open("user.txt", ios::in | ios::out);
	int usernum = getUserNum(); //当前用户数量，包括管理员的游客
	for (int n = 0; n < usernum; n++) {
		user.seekg(8 + 24 * n);
		user >> auser2;
		if (strcmp(auser_new, auser2) == 0) {
			cout << "用户名已存在" << endl;
			user.close();
			return 0;
		}
	}
	cout << "请输入新密码:";
	cin >> apwd1_new;
	usernum++;
	user.seekg(0);
	user << setw(8) << usernum; //初始化时先确定用户个数
	user.seekg(8 + 24 * (usernum - 1));
	user << setw(8) << auser_new;
	user << setw(8) << apwd1_new;
	user << setw(8) << "guest";
	user.close();
	cout << "*************注册成功*************" << endl;
	return 1;
}

int login() //登录
{
	char auser2[8];
	char apwd2[8];
	cout << "请输入用户名:";
	cin >> auser;
	cout << "请输入密码:";
	cin >> apwd;
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
			cout << agroup;
			break;
		}
	}
	user.close();
	if (have == true)
		return 1;
	return 0;
}

void Order() //命令函数
{
	char commond[10];
	bool have;
	for (;;) {
		have = false;
		cout << "\n";
		getPath(); //
		cout << "$";
		cin >> commond;
		if (!strcmp(commond, "cd")) {
			have = true;
			char string[100];
			cin >> string;
			cd(string);
		}
		if (!strcmp(commond, "write")) {
			have = true;
			char filename[FILE_NAME_SIZE];
			cin >> filename;
			char content[2048];
			cout << "请输入文件内容：";
			cin >> content;
			write(filename, content);

		}
		if (!strcmp(commond, "mkdir")) {
			have = true;
			char dirname[FILE_NAME_SIZE];
			cin >> dirname;
			mkdir(dirname);
		}
		if (!strcmp(commond, "rmdir")) {
			have = true;
			char dirname[FILE_NAME_SIZE];
			cin >> dirname;
			rmdir(dirname, path[num - 1]);
		}
		if (!strcmp(commond, "mkfile")) {
			have = true;
			char filename[FILE_NAME_SIZE];
			cin >> filename;
			char content[2048];
			cout << "请输入文件内容：";
			cin >> content;
			mkfile(filename, content);

		}
		if (!strcmp(commond, "copy")) {
			have = true;
			char string[100];
			cin >> string;
			copy(string);
		}
		if (!strcmp(commond, "rmfile")) {
			have = true;
			char filename[FILE_NAME_SIZE];
			cin >> filename;
			rmfile(filename);
		}
		if (!strcmp(commond, "showcontent")) {
			have = true;
			char filename[FILE_NAME_SIZE];
			cin >> filename;
			showcontent(filename);
		}
		if (!strcmp(commond, "pwd")) {
			have = true;
			cout << "当前目录为：" << curname;
		}
		if (!strcmp(commond, "ls")) {
			have = true;
			ls();
		}
		if (!strcmp(commond, "logout")) {
			have = true;
			cout << "账户" << auser << "已注销\n";
			int times = 0;
			int in_up;
			cout << "请登录系统或注册：\n1，登录\n2，游客注册" << endl;
			cin >> in_up;
			if (in_up == 2) {
				while (!signup()) {
					cout << "请登录系统或注册：\n1，登入\n2，游客注册" << endl;
					cin >> in_up;
					if (in_up == 1) break;
				}
			}
			while (!login())
				cout << "wrong!!!\n";
			cout << "login success!" << endl;
			cout << "*******************Welcome " << auser << "*******************";
		}
		if (!strcmp(commond, "reset")) {
			have = true;
			initial();
			cout << "系统重置完成！";
		}
		if (!strcmp(commond, "help")) {
			have = true;
			Help();
		}
		if (!strcmp(commond, "exit")) {
			have = true;
			return;
		}
		if (have == false) {
			cout << commond << " is not a legal command!!!";
		}
	}
}

void getPath() //获取路径函数
{
	cout << "root";
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
			disk.seekg((BLK_SIZE_TOTAL) * inode.addr[0] + DIR_SIZE * j); //\n故多出2
			disk >> name;
			disk >> nextindex;
			if (nextindex == path[i + 1]) //等就跳到下一层的意思？path数组中存的什么？？？
			{
				cout << '/';
				cout << name;
				break;
			}
		}
		disk.close();
	}
}

void initial() //初始化
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
		cout << "Can't use the disk!\n";
		exit(1);
	}
	for (i = 0; i < DISK_MAX_NUM; i++) {
		disk << setw(BLK_SIZE) << ' '; //把100*512个空格写入disk
		disk << '\n';
	}
	disk.seekp(0);
	//改变读入位置f.seekg(0, ios::beg); 一参数偏移量offset(long）二参数offset相对位置，三个值：  ios::beg -- 文件头    ios::end -- 文件尾    ios::cur -- 当前位置
	disk << setw(INODE_NUM_SIZE) << -1; //第一个i节点给根目录使用
	for (i = 1; i < FREE_INODE_MAX_NUM; i++) {
		disk << setw(INODE_NUM_SIZE) << i;
	}
	disk << setw(INODE_NUM_SIZE) << FREE_INODE_MAX_NUM - 1;
	for (i = 0; i < 10; i++) {
		disk << setw(BLK_NUM_SIZE) << i + 12;
	}
	disk << setw(BLK_NUM_SIZE) << 10; //组内空闲盘块数
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

void Help() //help函数
{

	cout << "1.mkdir：mkdir+空格+目录名，如\"mkdir d1\",在当前目录建立子目录d1\n\n";
	cout << "2.rmdir：rmdir+空格+目录名，如\"rmdir d1\",删除目录d1\n\n";
	cout << "3.cd用法:3.1：cd+空格+目录名，如 \"cd d1\" ——跳转到当前目录的子目录d1\n";
	cout << "         3.2：cd+空格+两点，如\"cd ..\" ——跳转到父目录\n";
	cout << "         3.3：cd+空格+斜杠，如\"cd /\" ——跳转到根目录\n";
	cout << "         3.4：cd+空格+路径，如 \"cd root/d1/d2\" ——跳转到d2\n";
	cout << "         3.5：cd+空格+点号，如 \"cd .\" 跳转到根目录\n\n";
	cout << "4.ls：直接使用,如\"ls\"显示当前目录中的子目录和文件 \n\n";
	cout << "5.pwd：直接使用,如\"pwd\"显示当前目录 \n\n";
	cout << "6.mkfile：mkfile+空格+文件名，如\"mkfile f1\",在当前目录下建立文件f1\n\n";
	cout << "7.rmfile：rmfile+空格+文件名，如\"rmfile f1\",删除当前目录下的文件f1\n\n";
	cout << "8.write: write+空格+文件名，如\"write f1\",在文件f1内容后继续添加文字\n\n";
	cout << "9.copy: copy+空格+路径，如\"copy root\\d1\\f1\",在当前目录下建立与文件f1相同的文件\n\n";
	cout << "10.showcontent：showcontent+空格+文件名，如\"showcontent f1\",显示文件f1的内容\n\n";
	cout << "11.logout: 直接使用，退出当前账户\n\n";
	cout << "12.reset：直接使用，功能是将系统重置到初始状态\n\n";
	cout << "13.help: 直接使用，功能是查看每个命令详细使用方法\n\n";
	cout << "14.exit: 直接使用，功能是退出系统\n";
}

int main() {
	int in_up;
	cout << "是否Format:(y/n)?" << endl;
	char formate;
	cin >> formate;
	if ((formate == 'y') || (formate == 'Y')) {
		initial();
	}
	strcpy(curname, "root");
	path[0] = 0;
	num = 1;
	int times = 0;
	cout << "请登录系统或注册：\n1，登录\n2，游客注册" << endl;
	cin >> in_up;
	if (in_up == 2) {
		while (!signup()) {
			cout << "请登录系统或注册：\n1，登录\n2，游客注册" << endl;
			cin >> in_up;
			if (in_up == 1) break;
		}
	}
	while (!login()) {
		times++;
		if (times >= 3) {
			cout << "多次输入的用户名和口令不符，登录失败!\n";
			exit(0);
		}
		cout << "用户名或密码错误!!!\n";
	}
	cout << "恭喜你，登录成功!\n";
	cout << "********************File System " << auser << "***************" << endl;
	cout << "---------------------Order List---------------------\n";
	cout << "1.mkdir-----------建立目录\n";
	cout << "2.rmdir-----------删除目录\n";
	cout << "3.cd--------------改变当前目录\n";
	cout << "4.ls--------------显示当前目录中的子目录和文件\n";
	cout << "5.pwd----------------显示当前目录\n";
	cout << "6.mkfile--------------建立文件\n";
	cout << "7.rmfile--------------删除文件\n";
	cout << "8.write--------------追加文件\n";
	cout << "9.copy--------------复制文件\n";
	cout << "10.showcontent-------------显示文件内容\n";
	cout << "11.reset-----------系统重置\n";
	cout << "12.logout---------注销登录\n";
	cout << "13.help-----------显示帮助内容\n";
	cout << "14.exit-----------退出系统\n";
	cout << "----------------------------------------------------\n";
	readsupblk();
	Order();
	writesupblk();
	return 0;
}
