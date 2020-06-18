#pragma once
#include<iostream>
#include<cstdlib>
#include<cstring>
#include<ctime>
#include<fstream>
#include<iomanip>
using namespace std;

#define DISK_MAX_NUM 100	//�ܴ��̿���100
#define BLK_SIZE 512	//���̿��С512B
#define BLK_SIZE_TOTAL 514	//���̿��С����'\n'
#define BLK_NUM_SIZE 3	//���̱�ų���3
#define	BLK_GROUP_NUM 10	//���д��̿���飬ÿ�����10��
#define INODE_SIZE 64	//i�ڵ��С64B
#define INODE_NUM_SIZE 3	//i�ڵ��ų���3
#define INODE_BLK_NUM 10	//i�ڵ�ռ���̿���10��
#define DIR_SIZE 36			//ÿ��Ŀ¼�Ĵ�СΪ36B
#define FREE_BLK_MAX_NUM (DISK_MAX_NUM - 2 - INODE_BLK_NUM)		//�����д��̿���(100-2-10)=88��
#define FREE_INODE_MAX_NUM ((INODE_BLK_NUM * BLK_SIZE) / INODE_SIZE)	//������i�ڵ���� (10*512)/64=80��
#define DIRECT_ADR_NUM 4	//ֱ��Ѱַ��ַλ����
#define USER_NUM 8			//�û��������� 8B
#define USER_NAME_SIZE 6	//�û��� 6B
#define USER_PWD_SIZE 6		//�û����볤�� 6B
#define USER_GROUP_SIZE 6	//�û����� 6B su/guest
#define FILE_MODE_SIZE 11	//�ļ���� 11B
#define TIME_SIZE 9		//�洢ʱ�� 9B
#define FILE_NAME_SIZE 14	//�ļ���Ŀ¼���ƴ�С
#define FILE_CONTENT_MAX_SIZE 2048	//�ļ�������󳤶�


struct SUPERBLOCK		//������
{
	int fistack[FREE_INODE_MAX_NUM];	//���нڵ��ջ
	int fiptr;			//���нڵ�ջָ��
	int fbstack[BLK_GROUP_NUM];	//���п��ջ
	int fbptr;			//���п��ջָ��
	int inum;			//���нڵ���
	int bnum;			//���п���
};

struct INODE			//�ڵ㣬64B
{
	int fsize;			//�ļ���С��4B
	int fbnum;			//�ļ��̿���,4B
	int addr[DIRECT_ADR_NUM];		//4��ֱ���̿��,16B
	int addr1;			//һ��һ�μ�ַ,4B
	int addr2;			//һ�����μ�ַ,4B
	char ower[USER_NAME_SIZE];		//�ļ�������,6B
	char grouper[USER_GROUP_SIZE];	//�ļ�������,6B
	char mode[FILE_MODE_SIZE];		//�ļ����,11B
	char ctime[TIME_SIZE];		//����޸�ʱ��,9B
};

struct DIR              //36B
{
	char fname[FILE_NAME_SIZE];		//�ļ���Ŀ¼��,14B
	int index;			//�ڵ��,4B
	char parfname[FILE_NAME_SIZE];	//��Ŀ¼��,14B
	int parindex;		//��Ŀ¼�ڵ��,4B
};

void Order();  //�����
void initial();  //��ʼ��
void getPath();  //��ȡ·��
void Help();  //����
int ialloc();		//����i�ڵ�
void ifree(int index);//����i�ڵ�
int balloc();	//������̿�
void bfree(int index);	//���մ��̿�
void readInode(int index, INODE& inode);  // ��i�ڵ�
void writeInode(INODE inode, int index);  //дi�ڵ�
void readsupblk();		//��������
void writesupblk();		//д������
void rmdir(char* dirname, int index);  //ɾ��Ŀ¼
void readdir(INODE inode, int index, DIR& dir);	//��Ŀ¼
void writedir(INODE inode, DIR dir, int index);		//дĿ¼
void mkfile(char* dirname, char* content);  //�����ļ�
void rmfile(char* filename);  //ɾ���ļ�
void showcontent(char* filename);	//��ʾ�ļ�����
void write();//д�ļ�
void copy(char* string);		//�����ļ�
int login();  //��¼
int getUserNum();//��ȡ�û���
bool authority(INODE inode);  //Ȩ�޹���
bool ISsame(char* dirname, INODE inode, int& i, int& index2);	//�ж�Ŀ¼���Ƿ��Ѵ���
bool find(char* string);	//����·���ҵ�ָ���ļ���Ŀ¼
void mkdir(char* dirname);	//������Ŀ¼
void ls();	//��ʾĿ¼
void cd(char* string);	//��תĿ¼


int path[20];
int num = 1;	//��ǰĿ¼����
int t = 0;
char auser[8];
char apwd[8];
char agroup[8];
char curname[9];
SUPERBLOCK superblock;