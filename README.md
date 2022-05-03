# MoFS
A simple File System implemented by MaxMorning.

## 层级
### 应用层
FTP CLI

**涉及到的类**  
User

### 原语层
creat read write seek等

**涉及到的类**  
File

### 系统层
ReadBlock WriteBlock

**涉及到的类**  
MemInode DiskInode SuperBlock BlockManager

### 物理层
fopen fread fwrite fseek fclose(操作系统提供)