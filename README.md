# MoFS
A simple File System implemented by MaxMorning.

## 层级
### 应用层
FTP CLI

### 原语层
mofs_creat mofs_read mofs_write mofs_lseek等

### 用户层
User.Open User.Create

**涉及到的类**  
OpenFile User

### 系统层
MemInode.Close DiskInode::DiskInodeFactory

**涉及到的类**  
MemInode DiskInode SuperBlock

### 设备层
BlockManager.ReadBlock  BlockManager.ReadInode

**涉及到的类**  
BlockManager Buffer

### 物理层
fopen fread fwrite fseek fclose(操作系统提供)

## 说明
### 空闲inode的管理
在superBlock中，s_ninode指示超级块直接管辖的空闲inode数量；s_nextInodeBlk指向下一个存储空闲inode的块。  
在存储空闲inode的块中，有101字的有效数据。其中第0个字为下一个存储空闲inode的块号；后100字为空闲inode序号。