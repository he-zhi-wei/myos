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
