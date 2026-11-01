// 文件：fs.h
// 作用：极简 VFS + RamFS 文件接口，供 shell、ELF 加载器等读写内存文件。
// 要点：扁平命名空间无目录，每 inode 有 4KB 数据页（超 4KB 用第二页 data2）；fd 0/1/2 固定为 stdin/stdout/stderr，3 起为文件且 MAX_FDS=16；文件名上限 FILE_NAME=28。
// Minimal VFS + RamFS — flat namespace, no directories, one-page files.
// Files are stored in a linked list of inodes, each with a 4KB data buffer.
// File descriptor table: fd 0=stdin, 1=stdout, 2=stderr, 3+=files.

#pragma once
#include "types.h"

#define MAX_FDS     16
#define FILE_NAME   28
#define PAGE_SIZE_  4096

// ── Inode (file metadata + data) ────────────────────
typedef struct inode {
    int   id;
    char  name[FILE_NAME];
    char *data;             // first page of file content
    char *data2;            // second page (for files > 4KB)
    uint32_t size;          // bytes written
    uint32_t capacity;      // allocated size (PAGE_SIZE_ * (1 or 2))
    struct inode *next;
} inode_t;

// ── Open file entry ──────────────────────────────────
typedef struct {
    inode_t *inode;
    uint32_t offset;        // current read/write position
    int in_use;
} fd_t;

// ── API ──────────────────────────────────────────────
void fs_init(void);
int  fs_open(const char *name, int create_if_missing);
int  fs_file_size(const char *name);   // returns -1 if not found
int  fs_read(int fd, char *buf, uint32_t len);
int  fs_write(int fd, const char *buf, uint32_t len);
int  fs_close(int fd);
int  fs_seek(int fd, int offset, int whence);   // whence: 0=SET 1=CUR 2=END
void fs_list(void (*put)(const char *));
