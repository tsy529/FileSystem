#include "fileSystem.h"

void initVariable() {
	strcpy(curname, "root");
	path[0] = 0;
	num = 1;
}
//����һ��i�ڵ�
int ialloc() 
{
	//�п���i��㣬��С������䣬���ر��-1
	if (superblock.fiptr > 0) {
		int temp = superblock.fistack[FREE_INODE_MAX_NUM - superblock.fiptr]; //i�ڵ㵱ǰ����
		superblock.fistack[FREE_INODE_MAX_NUM - superblock.fiptr] = -1;
		superblock.fiptr--;
		return temp;
	}
	return -1;
}
//�黹i�ڵ�
void ifree(int index) 
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
//��i�ڵ���Ϣ
void readInode(int index, INODE& inode) 
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
//дi�ڵ�
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
	disk << setw(FILE_MODE_SIZE + 1) << inode.mode; //tsy:��Ҳ��֪������ΪʲôҪ��ԭ������+1�����Ӿ��ǲ���ʹ��debugҲû�ҳ�Ϊɶ���������յ���time����Խ��
	//disk << setw(12) << inode.mode;
	disk << setw(TIME_SIZE + 1) << inode.ctime;
	//disk << setw(10) << inode.ctime;
	disk.close();
}
//��������̿�
int balloc() 
{
	int temp = superblock.fbstack[10 - superblock.fbptr];
	if (1 == superblock.fbptr) //ջ��
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
			if (0 == id) break; //0��ʾ�����һ��
		}
		disk.seekg(BLK_SIZE_TOTAL * temp);
		for (int j = BLK_GROUP_NUM - num; j < BLK_GROUP_NUM; j++) {
			disk >> id;
			superblock.fbstack[j] = id;
		}
		superblock.fbptr = num;

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
//�������ͷŵ��̿�
void bfree(int index) 
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
//�������飻�����ļ����е�һ����ǳ����졣���СӦ�ô�initial����֪��Ӧδռ��
void readsupblk() 
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
//д������
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
//��Ŀ¼
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
//дĿ¼
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

//�����ļ�
void crtfile(char* filename, char* content)
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //��i�����Ϣ
	if (authority(inode)) //�ж�Ȩ��
	{
		if (BLK_SIZE - inode.fsize < DIR_SIZE) //Ŀ¼���Ѵﵽ���14��
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  ��ǰĿ¼�����������ļ�ʧ�ܣ�";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			int i, index2;
			if (ISsame(filename, inode, i, index2)) //
			{
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  �ļ��Ѵ��ڣ�����ʧ�ܣ�";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
			else {
				cout << "  �������ļ����ݣ�";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
				cin >> content;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				int size = strlen(content) + 1;
				cout << "  �������ļ���С��1~2048B)��";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
				cin >> size;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				if (size > FILE_CONTENT_MAX_SIZE) {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  ���������ļ����ݹ���������ʧ�ܣ�";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
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
								SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
								cout << "  �����̿鲻���������ļ�ʧ�ܣ�";
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
							cout << "  �ļ�"<<filename<<" �ѳɹ�������";
						}
					}
					else {
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
						cout << "  �ڵ��Ѿ����꣬���������ļ�ʧ�ܣ�";
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
					}
				}
			}
		}
	}
	else {
		cout << "  û��Ȩ��";
	}
	writesupblk(); //д������
	writeInode(inode, path[num - 1]); //дi���
}
//ƥ���С����
void crtfileAutoloc(char* filename, char* content)
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //��i�����Ϣ
	if (authority(inode)) //�ж�Ȩ��
	{
		if (BLK_SIZE - inode.fsize < DIR_SIZE) //Ŀ¼���Ѵﵽ���14��
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  ��ǰĿ¼�����������ļ�ʧ�ܣ�";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			int i, index2;
			if (ISsame(filename, inode, i, index2)) //
			{
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  �ļ��Ѵ��ڣ�����ʧ�ܣ�";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
			else {
				cout << "  �������ļ����ݣ�";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
				cin >> content;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				int size = strlen(content) + 1;
				if (size > FILE_CONTENT_MAX_SIZE) {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  ���������ļ����ݹ���������ʧ�ܣ�";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
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
								SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
								cout << "  �����̿鲻���������ļ�ʧ�ܣ�";
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
							cout << "  �ļ�" << filename << " �ѳɹ�������";
						}
					}
					else {
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
						cout << "  �ڵ��Ѿ����꣬���������ļ�ʧ�ܣ�";
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
					}
				}
			}
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  ��ǰ��½�û�û��Ȩ��";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	writesupblk(); //д������
	writeInode(inode, path[num - 1]); //дi���
}
//�Զ�����
void crtfileAuto(char* filename, char* content)
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //��i�����Ϣ
	if (authority(inode)) //�ж�Ȩ��
	{
		if (BLK_SIZE - inode.fsize < DIR_SIZE) //Ŀ¼���Ѵﵽ���14��
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  ��ǰĿ¼�����������ļ�ʧ�ܣ�";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			int i, index2;
			if (ISsame(filename, inode, i, index2)) //
			{
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  �ļ�:"<< filename<<" �Ѵ��ڣ�����ʧ�ܣ�";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
			else {
				int size = strlen(content) + 1;
				if (size > FILE_CONTENT_MAX_SIZE) {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  ���������ļ����ݹ���������ʧ�ܣ�";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
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
								SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
								cout << "  �����̿鲻���������ļ�ʧ�ܣ�";
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
							cout << "  �ļ�" << filename << " �ѳɹ�������";
						}
					}
					else {
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
						cout << "  �ڵ��Ѿ����꣬���������ļ�ʧ�ܣ�";
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
					}
				}
			}
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  ��ǰ��½�û�û��Ȩ��";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	writesupblk(); //д������
	writeInode(inode, path[num - 1]); //дi���
}

//׷���ļ�����
void writefile(char* filename, char* content)
{
	INODE inode, inode2;
	readInode(path[num - 1], inode); //��i���
	if (authority(inode)) //�ж�Ȩ��
	{
		int i, index2;
		if (!ISsame(filename, inode, i, index2)) //�鿴�ļ����Ƿ����
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  �ļ���������";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			readInode(index2, inode2);
			if (inode2.mode[0] == 'f') //���Ƿ����ļ�
			{
				cout << "    �������ļ����ݣ�";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
				cin >> content;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				int size = strlen(content);
				if ((size + inode2.fsize) > FILE_CONTENT_MAX_SIZE) {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  ׷���ļ����ݹ���";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
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
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
							cout << "  �����̿鲻��";
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
						cout << "  �ļ�:"<<filename<<" ׷�����ݳɹ���";
					}
				}
			}
			else {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  �ļ�������dir��������file";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
		}

	}
	else {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  ��ǰ�û�û�в���Ȩ��";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	writeInode(inode, path[num - 1]);
}

//ɾ���ļ�
void rmfile(char* filename) 
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
					cout << "  �ļ�:" << filename << " �ѳɹ�ɾ����";
				}
				else {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  ɾ��Ŀ¼Ӧ��ʹ��rmdir���";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				}
			}
			else {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  û�в���Ȩ�ޣ�";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  Ŀ¼�в�����Ŀ���ļ�: " << filename << " ��" << endl;
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  û�в���Ȩ�ޣ�";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	writesupblk();
	writeInode(inode, path[num - 1]);
}
//�����ļ�
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
			cout << "  �޷��������·��:" << string << " �ҵ��ļ�:" << fname << " ��";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  �޷��������·��:" << string << "�ҵ��ļ�:" << fname << " ��";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	strcpy(curname, tcurname); //		��ԭ·��
	num = tnum;
	for (int ii = 0; ii < tnum; ii++) {
		path[ii] = tpath[ii];
	}
	if (getit) {
		crtfileAuto(fname, content); //�������ļ�
		cout << "  �ļ�: " << fname << " ���Ƴɹ�" << curname << endl;
	}
}
//��//��ʾ�ļ�����
void show(char* filename) //��ʾ�ļ�����
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	int i, index2;
	if (ISsame(filename, inode, i, index2)) {
		readInode(index2, inode2);
		if (inode2.mode[0] == 'f') {
			cout << "  �ļ�����Ϊ��\n";
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
			cout << "  ��ʾʧ�ܣ�";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  Ŀ¼�в������ļ�����" << filename << "����";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
}

void shownew(char* filename) {
	File file;
	file = returnFile(filename);
	cout << file.content;
}
//������������
File returnFile(char* filename) {
	INODE inode, inode2;
	File FileSet;
	char totalContent[2049] = {'\0'};
	readInode(path[num - 1], inode);
	int i, index2;
	if (ISsame(filename, inode, i, index2)) {
		readInode(index2, inode2);
		if (inode2.mode[0] == 'f') {
			cout << "  �ļ�����Ϊ��\n";
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
			cout << "  �����ļ����ͣ�";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  Ŀ¼�в������ļ�����" << filename << "����";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	FileSet.content = totalContent;
	FileSet.size = inode2.fsize;
	FileSet.ContentSize = strlen(totalContent);
	return FileSet;
}
//�ж�Ŀ¼���Ƿ����
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
//������Ŀ¼
void mkdir(char* dirname, char* username) 
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	if (authority(inode)) {
		if (BLK_SIZE - inode.fsize < DIR_SIZE) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  Ŀ¼�������޷�������Ŀ¼��";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			int i, index2;
			if (ISsame(dirname, inode, i, index2)) {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  ��Ŀ¼�Ѵ��ڣ�����ʧ�ܣ�";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
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
						strcpy(inode2.ower, username);
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
						disk << setw(USER_NAME_SIZE) << username;
						disk << setw(USER_GROUP_SIZE) << agroup;
						disk << setw(FILE_MODE_SIZE + 1) << "dir";
						_strtime(tmpbuf);
						disk << setw(TIME_SIZE + 1) << tmpbuf;
						disk.close();
						cout << "  Ŀ¼�ѳɹ�������";
					}
					else {
						ifree(iid);
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
						cout << "  �޿����̿飬������Ŀ¼ʧ�ܣ�";
						SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
					}
				}
				else {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
					cout << "  �޿��нڵ㣬������Ŀ¼ʧ�ܣ�";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
				}
			}
		}
	}
	else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  ��ǰ��½�û�û�д���Ȩ�ޣ�";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}
	writeInode(inode, path[num - 1]);
	writesupblk();
}
//ɾ��Ŀ¼
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
							cout << "  ��Ŀ¼�ǿգ���ɾ��Ŀ¼�µ������ļ����Ƿ������(y/n):";
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
							//cout<<"Ŀ¼�ɹ�ɾ����";
						}
						else {
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
							cout << "  \nĿ¼ɾ��ʧ�ܣ�";
							SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
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
						_strtime_s(tmpbuf);
						strcpy_s(inode.ctime, tmpbuf);


					}

					cout << dirname << "Ŀ¼ɾ����ϣ�\n";

				}
				else {
					cout << "  �ļ�Ӧ��rmfile����ɾ��";
				}
			}
			else {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
				cout << "  ��ǰ��½�û�û�в���Ȩ�ޣ�";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			}
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  Ŀ¼�в����ڸ���Ŀ¼��";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
	else {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  ��ǰ��½�û�û�в���Ȩ�ޣ�";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	}

	writesupblk();
	writeInode(inode, index);
	t--;
}

//����Ŀ¼��Ĵ�С
int getlssize(INODE inode)
{
	int size = 0;
	if (inode.mode[0] == 'd') {
		if (inode.fsize != 0) {
			//Ŀ¼��������Ŀ¼
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
				else {  //���ļ�
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
//��ʾĿ¼
void ls() //��ʾĿ¼
{
	INODE inode, inode2;
	readInode(path[num - 1], inode);
	char name[FILE_NAME_SIZE];
	int index;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	cout << "===============================�� �� �� ��=======================================\n";
	cout << setw(15) << "�ļ���" << setw(6) << "��С" << setw(8) << "������" << " ";
	cout << setw(6) << "�û���" << setw(12) << "�ļ�����" << setw(10) << "�޸�ʱ��" << endl<<endl;
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
	cout << "===============================�� ʾ �� ��=======================================\n";

}
//��תĿ¼
void cd(char* string) 
{
	if (!strcmp(string, ".")) {
		cout << "  ���л�����ǰĿ¼��";
		return;
	}
	if (!strcmp(string, "/")) //��ת����Ŀ¼
	{
		strcpy(curname, "root");
		path[0] = 0;
		num = 1;
		cout << "  ���л�����Ŀ¼��";
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
			cout << "  ���л�����Ŀ¼��" << curname << endl;
			return;
		}
		cout << "  ��ǰ���Ǹ�Ŀ¼��";
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
				cout << "  ���л�����Ŀ¼��";
				for (int i = 0; i < num; i++) {
					cout << path[i];
				}
				return;
			}
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  ���ܸ���·���ҵ����Ŀ¼����Ϊ " << string << " �����ļ���";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  ����Ŀ¼�����ڣ����ܸ���·��"<< string<< "�ҵ����Ŀ¼";
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
				cout << "  ���л������Ŀ¼";
				return;
			}
		}
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  ���ܸ���·���ҵ����Ŀ¼"; //�Ҳ���Ŀ�꣬��ԭ
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		strcpy(curname, tcurname);
		num = tnum;
		for (int ii = 0; ii < tnum; ii++) {
			path[ii] = tpath[ii];
		}
	}
}
//Ȩ�޹���
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
//����·���ҵ�ָ���ļ���Ŀ¼
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
//��ȡ�û�����
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
//�ο�ע��
int guestSignup() 
{
	char auser2[8];
	char auser_new[8];
	char apwd1_new[8];

	cout << "   ��ʱUser:";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
	cin >> auser_new;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	fstream user;
	user.open("user.txt", ios::in | ios::out);
	int usernum = getUserNum(); //��ǰ�û���������������Ա���ο�
	for (int n = 0; n < usernum; n++) {
		user.seekg(8 + 24 * n);
		user >> auser2;
		if (strcmp(auser_new, auser2) == 0) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  �û����Ѵ���" << endl;
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
	user << setw(8) << usernum; //��ʼ��ʱ��ȷ���û�����
	user.seekg(8 + 24 * (usernum - 1));
	user << setw(8) << auser_new;
	user << setw(8) << apwd1_new;
	user << setw(8) << "guest";
	user.close();
	cout << "  �ο�ģʽ������welcome��" << endl;
	return 1;
}
//�û�ע��
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
	int usernum = getUserNum(); //��ǰ�û���������������Ա���ο�
	for (int n = 0; n < usernum; n++) {
		user.seekg(8 + 24 * n);
		user >> auser2;
		if (strcmp(auser_new, auser2) == 0) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			cout << "  �û����Ѵ���" << endl;
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
	user << setw(8) << usernum; //��ʼ��ʱ��ȷ���û�����
	user.seekg(8 + 24 * (usernum - 1));
	user << setw(8) << auser_new;
	user << setw(8) << apwd1_new;
	user << setw(8) << "su";
	user.close();
	mkdir(auser_new, auser_new);
	cout << "\n   <-     ��ע�����û�" << auser_new << "    ->" << endl;
	return 1;
}
//��¼
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
//�����
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
			cout << "  ��ǰĿ¼Ϊ��" << curname;
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
			cout << "  �����û�������####�����ο�ģʽ" << endl;
			cout << "       User:";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
			cin >> in_up;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
			if (in_up == "####") {
				while (!guestSignup()) {
					cout << "  ��������ע����ʱUser\n�����˺ţ�������->login\n";
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
				cout << "  wrong! pls try again��\n";
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
			cout << "  FOS ��ԭһ�£�";
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
			cout << commond << "FOS cannot fand this order ! please try again��";
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
	}
}
//��ȡ·������
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
		readInode(path[i], inode); //i�ڵ���Ϣ���ظ�inode
		fstream disk;
		disk.open("disk.txt", ios::in | ios::out);
		for (int j = 0; j < (inode.fsize / DIR_SIZE); j++) //dir��СΪ36
		{
			disk.seekg((BLK_SIZE_TOTAL)*inode.addr[0] + DIR_SIZE * j); //\n�ʶ��2
			disk >> name;
			disk >> nextindex;
			if (nextindex == path[i + 1]) //�Ⱦ�������һ�����˼��path�����д��ʲô������
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
//��ʼ��
void format() 
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
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
		cout << "  ��ǰ���̲�����!\n";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		exit(1);
	}
	for (i = 0; i < DISK_MAX_NUM; i++) {
		disk << setw(BLK_SIZE) << ' '; //��100*512���ո�д��disk
		disk << '\n';
	}
	disk.seekp(0);
	//�ı����λ��f.seekg(0, ios::beg); һ����ƫ����offset(long��������offset���λ�ã�����ֵ��  ios::beg -- �ļ�ͷ    ios::end -- �ļ�β    ios::cur -- ��ǰλ��
	disk << setw(INODE_NUM_SIZE) << -1; //��һ��i�ڵ����Ŀ¼ʹ��
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
	disk << setw(BLK_NUM_SIZE) << 10; //���ڿ����̿���
	superblock.fbptr = 10;
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
void orderhelp()
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
	cout << "\n               <--     Order List      -->\n";
	cout << "---------------------------------------------------------\n";
	cout << "      1.mkdir--------------����Ŀ¼\n";
	cout << "      2.rmdir--------------ɾ��Ŀ¼\n";
	cout << "      3.cd-----------------�ı䵱ǰĿ¼\n";
	cout << "      4.ls-----------------��ʾ��ǰĿ¼�е���Ŀ¼���ļ�\n";
	cout << "      5.pwd----------------��ʾ��ǰĿ¼\n";
	cout << "      6.crt----------------�����ļ�\n";
	cout << "      7.crta---------------����(auto)�ļ�\n";
	cout << "      8.rmfile-------------ɾ���ļ�\n";
	cout << "      9.write--------------׷���ļ�\n";
	cout << "      10.copy--------------�����ļ�\n";
	cout << "      11.show--------------��ʾ�ļ�����\n";
	cout << "      12.reset-------------ϵͳ����\n";
	cout << "      13.logout------------ע����¼\n";
	cout << "      14.useradd-----------��������Ա\n";
	cout << "      16.order-------------��ʾ���������\n";
	cout << "      16.help--------------��ʾ��������\n";
	cout << "      17.exit--------------�˳�ϵͳ\n";
	cout << "-------------------------------------------------------\n";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
}

//help����
void Help() 
{

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
	cout << "    <--     Help documentation      -->\n";
	cout << "  1.mkdir��mkdir+�ո�+Ŀ¼������\"mkdir d1\",�ڵ�ǰĿ¼������Ŀ¼d1\n\n";
	cout << "  2.rmdir��rmdir+�ո�+Ŀ¼������\"rmdir d1\",ɾ��Ŀ¼d1\n\n";
	cout << "  3.cd�÷�:3.1��cd+�ո�+Ŀ¼������ \"cd d1\" ������ת����ǰĿ¼����Ŀ¼d1\n";
	cout << "           3.2��cd+�ո�+���㣬��\"cd ..\" ������ת����Ŀ¼\n";
	cout << "           3.3��cd+�ո�+б�ܣ���\"cd /\" ������ת����Ŀ¼\n";
	cout << "           3.4��cd+�ո�+·������ \"cd root/d1/d2\" ������ת��d2\n";
	cout << "           3.5��cd+�ո�+��ţ��� \"cd .\" ��ת����Ŀ¼\n\n";
	cout << "  4.ls��ֱ��ʹ��,��\"ls\"��ʾ��ǰĿ¼�е���Ŀ¼���ļ� \n\n";
	cout << "  5.pwd��ֱ��ʹ��,��\"pwd\"��ʾ��ǰĿ¼ \n\n";
	cout << "  6.crt��crt+�ո�+�ļ�������\"crt f1\",�ڵ�ǰĿ¼�½����ļ�f1���ֶ�����ļ���С\n\n";
	cout << "  7.crta��crta+�ո�+�ļ�������\"crta f1\",�ڵ�ǰĿ¼�½����ļ�f1,�Զ�ƥ�������С\n\n";
	cout << "  8.rmfile��rmfile+�ո�+�ļ�������\"rmfile f1\",ɾ����ǰĿ¼�µ��ļ�f1\n\n";
	cout << "  9.write: write+�ո�+�ļ�������\"write f1\",���ļ�f1���ݺ�����������\n\n";
	cout << "  10.copy: copy+�ո�+·������\"copy root\\d1\\f1\",�ڵ�ǰĿ¼�½������ļ�f1��ͬ���ļ�\n\n";
	cout << "  11.show��show+�ո�+�ļ�������\"show f1\",��ʾ�ļ�f1������\n\n";
	cout << "  12.logout: ֱ��ʹ�ã��˳���ǰ�˻�\n\n";
	cout << "  13.reset��ֱ��ʹ�ã������ǽ�ϵͳ���õ���ʼ״̬\n\n";
	cout << "  14.useradd��ֱ��ʹ�ã������Ǵ����¹���Ա\n\n";
	cout << "  15.order: ֱ��ʹ�ã������ǲ鿴ϵͳ����\n\n";
	cout << "  16.help: ֱ��ʹ�ã������ǲ鿴ÿ��������ϸʹ�÷���\n\n";
	cout << "  17.exit: ֱ��ʹ�ã��������˳�ϵͳ\n";
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
	cout << "  �����û�������####�����ο�ģʽ" << endl;
	cout << "       User:";
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
	cin >> in_up;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	if (in_up == "####") {
		while (!guestSignup()) {
			cout << "  ��������ע����ʱUser\n�����˺ţ�������->login\n";
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
		cout << "  �û��������벻��ȷ!\n  ���������룡ͬʱ����������롰exitos���˳�����\n";
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
	cout << "  �����ͨ��-order -help ָ����ָ����Ϣ��ʹ�÷���->" << endl;
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

