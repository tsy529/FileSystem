#include "fileSystem.h"

int ialloc() //����һ��i�ڵ�
{
	//�п���i��㣬��С������䣬���ر�ţ�
	//-1
	if (superblock.fiptr > 0) {
		int temp = superblock.fistack[FREE_INODE_MAX_NUM - superblock.fiptr]; //i�ڵ㵱ǰ����
		superblock.fistack[FREE_INODE_MAX_NUM - superblock.fiptr] = -1;
		superblock.fiptr--;
		return temp;
	}
	return -1;
}

void ifree(int index) //�黹i�ڵ�
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out); //���ļ�
	disk.seekp(BLK_SIZE_TOTAL + INODE_SIZE * index); // 514�ǳ�����Ĵ�С������64*index��i�ڵ���ƫ�ƣ���λ
	disk << setw(INODE_SIZE) << ' '; //���i�ڵ������
	disk.close();
	for (int i = FREE_INODE_MAX_NUM - superblock.fiptr; i < FREE_INODE_MAX_NUM; i++) //�ҵ����ͷŵ�i�ڵ���ڿ��нڵ��ջ��λ�ò�����
	{
		if (superblock.fistack[i] < index) //��֤�����Ű���С�����˳������
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

void readInode(int index, INODE& inode) //��i�ڵ���Ϣ
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out); //���ļ�
	disk.seekg(BLK_SIZE_TOTAL + INODE_SIZE * index); //�����顢ÿ��i�ڵ�64B
	disk >> inode.fsize;
	disk >> inode.fbnum;
	for (int i = 0; i < DIRECT_ADR_NUM; i++)
		disk >> inode.addr[i]; //����4��ֱ���̿��
	disk >> inode.addr1;
	disk >> inode.addr2;
	disk >> inode.ower;
	disk >> inode.grouper;
	disk >> inode.mode;
	disk >> inode.ctime;
	disk.close();
}

void writeInode(INODE inode, int index) //дi�ڵ�
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
	disk << setw(FILE_MODE_SIZE + 1) << inode.mode; //tsy:��Ҳ��֪������ΪʲôҪ��ԭ������+1�����Ӿ��ǲ���ʹ��debugҲû�ҳ�Ϊɶ���������յ���time����Խ��
	//disk << setw(12) << inode.mode;
	disk << setw(TIME_SIZE + 1) << inode.ctime;
	//disk << setw(10) << inode.ctime;
	disk.close();
}

int balloc() //��������̿�
{
	int temp = superblock.fbstack[10 - superblock.fbptr];
	if (1 == superblock.fbptr) //ջ��
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
				if (0 == id) break; //0��ʾ�����һ��
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
		disk.seekp(BLK_SIZE_TOTAL * temp); //�����̿��
		disk << setw(BLK_SIZE) << ' ';
		disk.close();
		return temp;
	}
	//�������鳤�飬��ֱ�ӷ���
	superblock.fbstack[10 - superblock.fbptr] = -1;
	superblock.fbptr--;
	return temp;
}

void bfree(int index) //�������ͷŵ��̿�
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	disk.seekp(BLK_SIZE_TOTAL * index);
	disk << setw(BLK_SIZE) << ' '; //��մ����յ��̿�
	disk.close();
	if (10 == superblock.fbptr) //�����̿�ջ����
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
	superblock.fbstack[10 - superblock.fbptr - 1] = index; //�������̺���ջ
	superblock.fbptr++;
}

void readsupblk() //�������飻�����ļ����е�һ����ǳ����졣���СӦ�ô�initial����֪��Ӧδռ��
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	int i;
	for (i = 0; i < FREE_INODE_MAX_NUM; i++) {
		disk >> superblock.fistack[i]; //���������ݷָ�����i�ڵ�ջ����������ʲô���ݣ�
	}
	disk >> superblock.fiptr;
	for (i = 0; i < BLK_GROUP_NUM; i++) {
		disk >> superblock.fbstack[i]; //���п��ջ
	}
	disk >> superblock.fbptr;
	disk >> superblock.inum;
	disk >> superblock.bnum;
	disk.close();
}

void writesupblk() //д������
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

void readdir(INODE inode, int index, DIR& dir) //��Ŀ¼
{
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	disk.seekg(BLK_SIZE_TOTAL * inode.addr[0] + DIR_SIZE * index);
	disk >> dir.fname;
	disk >> dir.index;
	disk >> dir.parfname;
	disk >> dir.parindex;
}

void writedir(INODE inode, DIR dir, int index) //дĿ¼
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

void mkfile(char* filename, char* content) //�����ļ�
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //��i�����Ϣ
	if (authority(inode)) //�ж�Ȩ��
	{
		if (BLK_SIZE - inode.fsize < DIR_SIZE) //Ŀ¼���Ѵﵽ���14��
		{
			cout << "��ǰĿ¼�����������ļ�ʧ�ܣ�";
		}
		else {
			int i, index2;
			if (ISsame(filename, inode, i, index2)) //
			{
				cout << "�ļ��Ѵ��ڣ�����ʧ�ܣ�";
			}
			else {
				int size = strlen(content) + 1;
				cout << "�������ļ���С��1~2048b)��";
				cin >> size;
				if (size > FILE_CONTENT_MAX_SIZE) {
					cout << "���������ļ����ݹ���������ʧ�ܣ�";
				}
				else {
					int bnum = (size - 1) / BLK_SIZE + 1; //�����̿���
					int bid[DIRECT_ADR_NUM];
					int iid = ialloc();
					if (iid != -1) {
						bool success = true;
						for (int i = 0; i < bnum; i++) {
							bid[i] = balloc();
							if (-1 == bid[i]) {
								cout << "�����̿鲻���������ļ�ʧ�ܣ�";
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
							disk << setw(FILE_NAME_SIZE) << filename; //�ļ���
							disk << setw(INODE_NUM_SIZE) << iid; //i����
							disk << setw(FILE_NAME_SIZE) << curname; //Ŀ¼��
							disk << setw(BLK_NUM_SIZE) << path[num - 1]; //����
							disk.close();
							inode.fsize += DIR_SIZE;
							char tmpbuf[TIME_SIZE];
							_strtime(tmpbuf);
							strcpy(inode.ctime, tmpbuf);
							inode2.fsize = size;
							inode2.fbnum = bnum;
							int i;
							for (i = 0; i < DIRECT_ADR_NUM; i++) //ѭ�������ĸ��̿���Ϣ
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
							while (content[cnum] != '\0') //�������ݴ�С
							{
								cnum++;
							}
							for (int i = 0; i < bnum; i++) //���ļ�����д���ĸ��̿�
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
							cout << "�ļ��ѳɹ�������";
						}
					}
					else {
						cout << "�ڵ��Ѿ����꣬���������ļ�ʧ�ܣ�";
					}
				}
			}
		}
	}
	else {
		cout << "û��Ȩ��";
	}
	writesupblk(); //д������
	writeInode(inode, path[num - 1]); //дi���
}

void write(char* filename, char* content) //׷���ļ�����
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //��i���
	if (authority(inode)) //�ж�Ȩ��
	{
		int i, index2;
		if (!ISsame(filename, inode, i, index2)) //�鿴�ļ����Ƿ����
		{
			cout << "�ļ�������";
		}
		else {
			readInode(index2, inode2);
			if (inode2.mode[0] == 'f') //���Ƿ����ļ�
			{
				int size = strlen(content);
				if ((size + inode2.fsize) > FILE_CONTENT_MAX_SIZE) {
					cout << "�ļ����ݹ���";
				}
				else {
					int bnum = (size + inode2.fsize - 1) / BLK_SIZE + 1; //�����̿���
					int bid[DIRECT_ADR_NUM];
					for (i = 0; i < DIRECT_ADR_NUM; i++) {
						bid[i] = inode2.addr[i];
					}
					bool success = true;
					for (int i = inode2.fbnum; i < bnum; i++) {
						bid[i] = balloc();
						if (-1 == bid[i]) {
							cout << "�����̿鲻��";
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
						cout << "�ļ������ɹ���";
					}
				}
			}
			else {
				cout << "�ļ��������ļ���";
			}
		}

	}
	else {
		cout << "û�в���Ȩ��";
	}
	writeInode(inode, path[num - 1]);
}

void rmfile(char* filename) //ɾ���ļ�
{
	INODE inode, inode2;
	DIR dir;
	readInode(path[num - 1], inode); //��ǰ���д�������
	if (authority(inode)) //�ж�Ȩ��
	{
		int i, index2 = 0;
		if (ISsame(filename, inode, i, index2)) {
			readInode(index2, inode2);
			if (authority(inode2)) {
				if (inode2.mode[0] == 'f') //ɾ���ļ�
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
					for (int j = i + 1; j < (inode.fsize / DIR_SIZE); j++) //�����ǰ��һλ���޸ĸ�Ŀ¼��Ϣ
					{
						readdir(inode, j, dir);
						writedir(inode, dir, j - 1);
					}
					inode.fsize -= DIR_SIZE;
					char tmpbuf[TIME_SIZE];
					_strtime(tmpbuf);
					strcpy(inode.ctime, tmpbuf);
					cout << "�ļ��ѳɹ�ɾ����";
				}
				else {
					cout << "ɾ��Ŀ¼Ӧ��rmdir���";
				}
			}
			else {
				cout << "û�в���Ȩ�ޣ�";
			}
		}
		else {
			cout << "Ŀ¼�в����ڸ��ļ���";
		}
	}
	else {
		cout << "û�в���Ȩ�ޣ�";
	}
	writesupblk();
	writeInode(inode, path[num - 1]);
}

void copy(char* string) //�����ļ�
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
			cout << "�޷��������·���ҵ��ļ���";
		}
	}
	else {
		cout << "�޷��������·���ҵ��ļ���";
	}
	strcpy(curname, tcurname); //		��ԭ·��
	num = tnum;
	for (int ii = 0; ii < tnum; ii++) {
		path[ii] = tpath[ii];
	}
	if (getit) {
		mkfile(fname, content); //�������ļ�
		cout << "���Ƴɹ�" << curname << endl;
	}
}

void showcontent(char* filename) //��ʾ�ļ�����
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	int i, index2;
	if (ISsame(filename, inode, i, index2)) {
		readInode(index2, inode2);
		if (inode2.mode[0] == 'f') {
			cout << "�ļ�����Ϊ��\n";
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
			cout << "��ʾʧ�ܣ�";
		}
	}
	else {
		cout << "Ŀ¼�в����ڸ��ļ���";
	}
}

bool ISsame(char* dirname, INODE inode, int& i, int& index2) //�ж�Ŀ¼���Ƿ����
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

void mkdir(char* dirname) //������Ŀ¼
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	if (authority(inode)) {
		if (BLK_SIZE - inode.fsize < DIR_SIZE) {
			cout << "Ŀ¼�������޷�������Ŀ¼��";
		}
		else {
			int i, index2;
			if (ISsame(dirname, inode, i, index2)) {
				cout << "��Ŀ¼�Ѵ��ڣ�����ʧ�ܣ�";
			}
			else {
				int iid = ialloc(); //����ڵ�
				if (iid != -1) {
					int bid = balloc(); //�����̿�
					if (bid != -1) {
						fstream disk;
						disk.open("disk.txt", ios::in | ios::out);
						disk.seekp(BLK_SIZE_TOTAL * inode.addr[0] + inode.fsize);
						disk << setw(FILE_NAME_SIZE + 1) << dirname; //дĿ¼��
						disk << setw(INODE_NUM_SIZE) << iid; //д�ڵ�
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
						writeInode(inode2, iid); //д���½ڵ���Ϣ

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
						cout << "Ŀ¼�ѳɹ�������";
					}
					else {
						ifree(iid);
						cout << "�޿����̿飬������Ŀ¼ʧ�ܣ�";
					}
				}
				else {
					cout << "�޿��нڵ㣬������Ŀ¼ʧ�ܣ�";
				}
			}
		}
	}
	else {
		cout << "û�д���Ȩ�ޣ�";
	}
	writeInode(inode, path[num - 1]);
	writesupblk();
}

void rmdir(char* dirname, int index) //ɾ��Ŀ¼
{
	t++;
	INODE inode, inode2;
	DIR dir;
	readInode(index, inode);
	if (authority(inode)) //�жϲ���Ȩ��
	{
		int i, index2;
		if (ISsame(dirname, inode, i, index2)) //�Ƿ��и�Ŀ¼
		{
			readInode(index2, inode2);
			if (authority(inode2)) //�ж�Ȩ��
			{
				if (inode2.mode[0] == 'd') //�Ƿ���Ŀ¼
				{
					if (inode2.fsize != 0) //�Ƿ�����Ŀ¼���ļ�
					{
						char yes = 'y';
						if (t == 1) {
							cout << "��Ŀ¼�ǿգ���ɾ��Ŀ¼�µ������ļ����Ƿ������(y/n):";
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
							//cout<<"Ŀ¼�ɹ�ɾ����";
						}
						else {
							cout << "Ŀ¼ɾ��ʧ�ܣ�";
						}
					}
					else //��
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
						//cout<<"Ŀ¼�ɹ�ɾ����";
					}

					cout << "Ŀ¼�ɹ�ɾ����";

				}
				else {
					cout << "�ļ�Ӧ��rmfile����ɾ��";
				}
			}
			else {
				cout << "û�в���Ȩ�ޣ�";
			}
		}
		else {
			cout << "Ŀ¼�в����ڸ���Ŀ¼��";
		}
	}
	else {
		cout << "û�в���Ȩ�ޣ�";
	}

	writesupblk();
	writeInode(inode, index);
	t--;
}

void ls() //��ʾĿ¼
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	char name[FILE_NAME_SIZE];
	int index;
	cout << setw(15) << "�ļ���" << setw(6) << "��С" << setw(8) << "������" << " ";
	cout << setw(6) << "�û���" << setw(12) << "�ļ�����" << setw(10) << "�޸�ʱ��" << endl;
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
	cout << "��ʾ��ϣ�";
}

void cd(char* string) //��תĿ¼
{
	if (!strcmp(string, ".")) {
		cout << "���л�����ǰĿ¼��";
		return;
	}
	if (!strcmp(string, "/")) //��ת����Ŀ¼
	{
		strcpy(curname, "root");
		path[0] = 0;
		num = 1;
		cout << "���л�����Ŀ¼��";
		return;
	}
	if (!strcmp(string, "..")) //��ת����Ŀ¼,
	{
		if (strcmp(curname, "root")) {
			INODE inode;
			num--;
			readInode(path[num - 1], inode); //����path�д��ǰһ��i����ţ��ҵ������
			char name[FILE_NAME_SIZE];
			fstream disk;
			disk.open("disk.txt", ios::in | ios::out);
			disk.seekg(inode.addr[0] * BLK_SIZE_TOTAL + 18); //��Ϊ��Ŀ¼����i���ṹ���еĵ�18λ
			disk >> name;
			disk.close();
			strcpy(curname, name);
			cout << "���л�����Ŀ¼��" << curname << endl;
			return;
		}
		cout << "��ǰ���Ǹ�Ŀ¼��";
		return;
	}
	char* per = strchr(string, static_cast<int>('/')); //����string���״γ���/��λ�ã�û���򷵻ؿ�
	if (per == nullptr) {
		INODE inode, inode2;
		int i, index2;
		readInode(path[num - 1], inode);
		char name[FILE_NAME_SIZE];
		if (ISsame(string, inode, i, index2)) //index2��Ľڵ��
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
				cout << "���л�����Ŀ¼��";
				for (int i = 0; i < num; i++) {
					cout << path[i];
				}
				return;
			}
			cout << "���ܸ���·���ҵ����Ŀ¼����Ϊ " << string << " �����ļ���";
		}
		else {
			cout << "����Ŀ¼�����ڣ����ܸ���·���ҵ����Ŀ¼";
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
				cout << "���л������Ŀ¼";
				return;
			}
		}
		cout << "���ܸ���·���ҵ����Ŀ¼"; //�Ҳ���Ŀ�꣬��ԭ
		strcpy(curname, tcurname);
		num = tnum;
		for (int ii = 0; ii < tnum; ii++) {
			path[ii] = tpath[ii];
		}
	}
}

bool authority(INODE inode) //Ȩ�޹���
{
	if (agroup[0] == 's') {
		return true;
	}
	return false;
}

bool find(char* string) //����·���ҵ�ָ���ļ���Ŀ¼
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

int getUserNum() //��ȡ�û�����
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

int signup() //ע��
{
	char auser2[8];
	char auser_new[8];
	char apwd1_new[8];

	cout << "���������ο��û���:";
	cin >> auser_new;
	fstream user;
	user.open("user.txt", ios::in | ios::out);
	int usernum = getUserNum(); //��ǰ�û���������������Ա���ο�
	for (int n = 0; n < usernum; n++) {
		user.seekg(8 + 24 * n);
		user >> auser2;
		if (strcmp(auser_new, auser2) == 0) {
			cout << "�û����Ѵ���" << endl;
			user.close();
			return 0;
		}
	}
	cout << "������������:";
	cin >> apwd1_new;
	usernum++;
	user.seekg(0);
	user << setw(8) << usernum; //��ʼ��ʱ��ȷ���û�����
	user.seekg(8 + 24 * (usernum - 1));
	user << setw(8) << auser_new;
	user << setw(8) << apwd1_new;
	user << setw(8) << "guest";
	user.close();
	cout << "*************ע��ɹ�*************" << endl;
	return 1;
}

int login() //��¼
{
	char auser2[8];
	char apwd2[8];
	cout << "�������û���:";
	cin >> auser;
	cout << "����������:";
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

void Order() //�����
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
			cout << "�������ļ����ݣ�";
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
			cout << "�������ļ����ݣ�";
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
			cout << "��ǰĿ¼Ϊ��" << curname;
		}
		if (!strcmp(commond, "ls")) {
			have = true;
			ls();
		}
		if (!strcmp(commond, "logout")) {
			have = true;
			cout << "�˻�" << auser << "��ע��\n";
			int times = 0;
			int in_up;
			cout << "���¼ϵͳ��ע�᣺\n1����¼\n2���ο�ע��" << endl;
			cin >> in_up;
			if (in_up == 2) {
				while (!signup()) {
					cout << "���¼ϵͳ��ע�᣺\n1������\n2���ο�ע��" << endl;
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
			cout << "ϵͳ������ɣ�";
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

void getPath() //��ȡ·������
{
	cout << "root";
	INODE inode;
	int nextindex;
	char name[FILE_NAME_SIZE];

	for (int i = 0; i + 1 < num; i++)
	{
		readInode(path[i], inode); //i�ڵ���Ϣ���ظ�inode
		fstream disk;
		disk.open("disk.txt", ios::in | ios::out);
		for (int j = 0; j < (inode.fsize / DIR_SIZE); j++) //dir��СΪ36
		{
			disk.seekg((BLK_SIZE_TOTAL) * inode.addr[0] + DIR_SIZE * j); //\n�ʶ��2
			disk >> name;
			disk >> nextindex;
			if (nextindex == path[i + 1]) //�Ⱦ�������һ�����˼��path�����д��ʲô������
			{
				cout << '/';
				cout << name;
				break;
			}
		}
		disk.close();
	}
}

void initial() //��ʼ��
{
	fstream user; //��ʼ���������ļ�����user�У�����userʵ�ֶ��ļ��Ķ�д
	user.open("user.txt", ios::in | ios::out);
	int usernum = 1;
	user << setw(8) << usernum; //��ʼ��ʱ��ȷ���û�������������<<����8���ո��usernumд���ļ���ȥ
	user << setw(8) << "admin";
	user << setw(8) << "123";
	user << setw(8) << "su"; //��������Ա
	int i;
	for (i = 0; i < DISK_MAX_NUM; i++) {
		user << setw(BLK_SIZE) << ' '; //��512���ո�д��user
		user << '\n';
	}
	user.close();
	fstream disk;
	disk.open("disk.txt", ios::in | ios::out);
	if (!disk.is_open()) //����.is_open()����ļ��Ƿ�򿪳ɹ�
	{
		cout << "Can't use the disk!\n";
		exit(1);
	}
	for (i = 0; i < DISK_MAX_NUM; i++) {
		disk << setw(BLK_SIZE) << ' '; //��100*512���ո�д��disk
		disk << '\n';
	}
	disk.seekp(0);
	//�ı����λ��f.seekg(0, ios::beg); һ����ƫ����offset(long��������offset���λ�ã�����ֵ��  ios::beg -- �ļ�ͷ    ios::end -- �ļ�β    ios::cur -- ��ǰλ��
	disk << setw(INODE_NUM_SIZE) << -1; //��һ��i�ڵ����Ŀ¼ʹ��
	for (i = 1; i < FREE_INODE_MAX_NUM; i++) {
		disk << setw(INODE_NUM_SIZE) << i;
	}
	disk << setw(INODE_NUM_SIZE) << FREE_INODE_MAX_NUM - 1;
	for (i = 0; i < 10; i++) {
		disk << setw(BLK_NUM_SIZE) << i + 12;
	}
	disk << setw(BLK_NUM_SIZE) << 10; //���ڿ����̿���
	disk << setw(BLK_NUM_SIZE) << 80; //�ܿ���i�ڵ���
	disk << setw(BLK_NUM_SIZE) << 89; //�ܿ����̿���
	//��ʼ����Ŀ¼i�ڵ�
	disk.seekp(BLK_SIZE_TOTAL); //�����̿�1
	disk << setw(6) << 0; //��ǰ��Ŀ¼��С0
	disk << setw(6) << 1; //ռ��1�����̿�
	disk << setw(BLK_NUM_SIZE) << 11; //addr[0] = ���Ϊ11
	disk << setw(BLK_NUM_SIZE) << 0; //addr[1]
	disk << setw(BLK_NUM_SIZE) << 0; //addr[2]
	disk << setw(BLK_NUM_SIZE) << 0; //addr[3]
	disk << setw(BLK_NUM_SIZE) << 0; //addr1
	disk << setw(BLK_NUM_SIZE) << 0; //addr2
	disk << setw(USER_NAME_SIZE) << "admin"; //������admin
	disk << setw(USER_GROUP_SIZE) << "su"; //������Ȩ��������Ϊsu
	disk << setw(FILE_MODE_SIZE) << "dir"; //����ΪĿ¼
	char tmpbuf[TIME_SIZE];
	_strtime(tmpbuf); //��ȡ��ǰϵͳ��ʱ��
	disk << setw(TIME_SIZE) << tmpbuf;
	for (i = 21; i < DISK_MAX_NUM; i++) //��ʣ����д��̿����
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

void Help() //help����
{

	cout << "1.mkdir��mkdir+�ո�+Ŀ¼������\"mkdir d1\",�ڵ�ǰĿ¼������Ŀ¼d1\n\n";
	cout << "2.rmdir��rmdir+�ո�+Ŀ¼������\"rmdir d1\",ɾ��Ŀ¼d1\n\n";
	cout << "3.cd�÷�:3.1��cd+�ո�+Ŀ¼������ \"cd d1\" ������ת����ǰĿ¼����Ŀ¼d1\n";
	cout << "         3.2��cd+�ո�+���㣬��\"cd ..\" ������ת����Ŀ¼\n";
	cout << "         3.3��cd+�ո�+б�ܣ���\"cd /\" ������ת����Ŀ¼\n";
	cout << "         3.4��cd+�ո�+·������ \"cd root/d1/d2\" ������ת��d2\n";
	cout << "         3.5��cd+�ո�+��ţ��� \"cd .\" ��ת����Ŀ¼\n\n";
	cout << "4.ls��ֱ��ʹ��,��\"ls\"��ʾ��ǰĿ¼�е���Ŀ¼���ļ� \n\n";
	cout << "5.pwd��ֱ��ʹ��,��\"pwd\"��ʾ��ǰĿ¼ \n\n";
	cout << "6.mkfile��mkfile+�ո�+�ļ�������\"mkfile f1\",�ڵ�ǰĿ¼�½����ļ�f1\n\n";
	cout << "7.rmfile��rmfile+�ո�+�ļ�������\"rmfile f1\",ɾ����ǰĿ¼�µ��ļ�f1\n\n";
	cout << "8.write: write+�ո�+�ļ�������\"write f1\",���ļ�f1���ݺ�����������\n\n";
	cout << "9.copy: copy+�ո�+·������\"copy root\\d1\\f1\",�ڵ�ǰĿ¼�½������ļ�f1��ͬ���ļ�\n\n";
	cout << "10.showcontent��showcontent+�ո�+�ļ�������\"showcontent f1\",��ʾ�ļ�f1������\n\n";
	cout << "11.logout: ֱ��ʹ�ã��˳���ǰ�˻�\n\n";
	cout << "12.reset��ֱ��ʹ�ã������ǽ�ϵͳ���õ���ʼ״̬\n\n";
	cout << "13.help: ֱ��ʹ�ã������ǲ鿴ÿ��������ϸʹ�÷���\n\n";
	cout << "14.exit: ֱ��ʹ�ã��������˳�ϵͳ\n";
}

int main() {
	int in_up;
	cout << "�Ƿ�Format:(y/n)?" << endl;
	char formate;
	cin >> formate;
	if ((formate == 'y') || (formate == 'Y')) {
		initial();
	}
	strcpy(curname, "root");
	path[0] = 0;
	num = 1;
	int times = 0;
	cout << "���¼ϵͳ��ע�᣺\n1����¼\n2���ο�ע��" << endl;
	cin >> in_up;
	if (in_up == 2) {
		while (!signup()) {
			cout << "���¼ϵͳ��ע�᣺\n1����¼\n2���ο�ע��" << endl;
			cin >> in_up;
			if (in_up == 1) break;
		}
	}
	while (!login()) {
		times++;
		if (times >= 3) {
			cout << "���������û����Ϳ��������¼ʧ��!\n";
			exit(0);
		}
		cout << "�û������������!!!\n";
	}
	cout << "��ϲ�㣬��¼�ɹ�!\n";
	cout << "********************File System " << auser << "***************" << endl;
	cout << "---------------------Order List---------------------\n";
	cout << "1.mkdir-----------����Ŀ¼\n";
	cout << "2.rmdir-----------ɾ��Ŀ¼\n";
	cout << "3.cd--------------�ı䵱ǰĿ¼\n";
	cout << "4.ls--------------��ʾ��ǰĿ¼�е���Ŀ¼���ļ�\n";
	cout << "5.pwd----------------��ʾ��ǰĿ¼\n";
	cout << "6.mkfile--------------�����ļ�\n";
	cout << "7.rmfile--------------ɾ���ļ�\n";
	cout << "8.write--------------׷���ļ�\n";
	cout << "9.copy--------------�����ļ�\n";
	cout << "10.showcontent-------------��ʾ�ļ�����\n";
	cout << "11.reset-----------ϵͳ����\n";
	cout << "12.logout---------ע����¼\n";
	cout << "13.help-----------��ʾ��������\n";
	cout << "14.exit-----------�˳�ϵͳ\n";
	cout << "----------------------------------------------------\n";
	readsupblk();
	Order();
	writesupblk();
	return 0;
}
