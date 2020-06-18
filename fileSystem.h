#pragma once
#include<iostream>
#include<cstdlib>
#include<cstring>
#include<ctime>
#include<fstream>
#include<iomanip>
using namespace std;

#define DISK_MAX_NUM 100	//总磁盘块数100
#define BLK_SIZE 512	//磁盘块大小512B
#define BLK_SIZE_TOTAL 514	//磁盘块大小加上'\n'
#define BLK_NUM_SIZE 3	//磁盘编号长度3
#define	BLK_GROUP_NUM 10	//空闲磁盘块分组，每组个数10块
#define INODE_SIZE 64	//i节点大小64B
#define INODE_NUM_SIZE 3	//i节点编号长度3
#define INODE_BLK_NUM 10	//i节点占磁盘块数10块
#define DIR_SIZE 36			//每个目录的大小为36B
#define FREE_BLK_MAX_NUM (DISK_MAX_NUM - 2 - INODE_BLK_NUM)		//最大空闲磁盘块数(100-2-10)=88块
#define FREE_INODE_MAX_NUM ((INODE_BLK_NUM * BLK_SIZE) / INODE_SIZE)	//最大空闲i节点个数 (10*512)/64=80个
#define DIRECT_ADR_NUM 4	//直接寻址地址位个数
#define USER_NUM 8			//用户个数长度 8B
#define USER_NAME_SIZE 6	//用户名 6B
#define USER_PWD_SIZE 6		//用户密码长度 6B
#define USER_GROUP_SIZE 6	//用户组名 6B su/guest
#define FILE_MODE_SIZE 11	//文件类别 11B
#define TIME_SIZE 9		//存储时间 9B
#define FILE_NAME_SIZE 14	//文件或目录名称大小
#define FILE_CONTENT_MAX_SIZE 2048	//文件内容最大长度


struct SUPERBLOCK		//超级块
{
	int fistack[FREE_INODE_MAX_NUM];	//空闲节点号栈
	int fiptr;			//空闲节点栈指针
	int fbstack[BLK_GROUP_NUM];	//空闲块号栈
	int fbptr;			//空闲块号栈指针
	int inum;			//空闲节点数
	int bnum;			//空闲块数
};

struct INODE			//节点，64B
{
	int fsize;			//文件大小，4B
	int fbnum;			//文件盘块数,4B
	int addr[DIRECT_ADR_NUM];		//4个直接盘块号,16B
	int addr1;			//一个一次间址,4B
	int addr2;			//一个二次间址,4B
	char ower[USER_NAME_SIZE];		//文件所有者,6B
	char grouper[USER_GROUP_SIZE];	//文件所属组,6B
	char mode[FILE_MODE_SIZE];		//文件类别,11B
	char ctime[TIME_SIZE];		//最后修改时间,9B
};

struct DIR              //36B
{
	char fname[FILE_NAME_SIZE];		//文件或目录名,14B
	int index;			//节点号,4B
	char parfname[FILE_NAME_SIZE];	//父目录名,14B
	int parindex;		//父目录节点号,4B
};

void Order();  //命令函数
void initial();  //初始化
void getPath();  //获取路径
void Help();  //帮助
int ialloc();		//申请i节点
void ifree(int index);//回收i节点
int balloc();	//申请磁盘块
void bfree(int index);	//回收磁盘块
void readInode(int index, INODE& inode);  // 读i节点
void writeInode(INODE inode, int index);  //写i节点
void readsupblk();		//读超级块
void writesupblk();		//写超级块
void rmdir(char* dirname, int index);  //删除目录
void readdir(INODE inode, int index, DIR& dir);	//读目录
void writedir(INODE inode, DIR dir, int index);		//写目录
void mkfile(char* dirname, char* content);  //创建文件
void rmfile(char* filename);  //删除文件
void showcontent(char* filename);	//显示文件内容
void write();//写文件
void copy(char* string);		//复制文件
int login();  //登录
int getUserNum();//获取用户数
bool authority(INODE inode);  //权限管理
bool ISsame(char* dirname, INODE inode, int& i, int& index2);	//判断目录项是否已创建
bool find(char* string);	//根据路径找到指定文件或目录
void mkdir(char* dirname);	//创建子目录
void ls();	//显示目录
void cd(char* string);	//跳转目录


int path[20];
int num = 1;	//当前目录层数
int t = 0;
char auser[8];
char apwd[8];
char agroup[8];
char curname[9];
SUPERBLOCK superblock;