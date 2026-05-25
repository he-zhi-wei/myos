#include "fs.h"
#include "pmm.h"
#include "keyboard.h"

static inode_t *root = 0;         // linked list of all files
static int next_id = 1;
static fd_t fd_table[MAX_FDS];

// ── Internal helpers ─────────────────────────────────

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}

static inode_t *find_inode(const char *name) {
    for (inode_t *ino = root; ino; ino = ino->next)
        if (streq(ino->name, name)) return ino;
    return 0;
}

static int alloc_fd(void) {
    for (int i = 3; i < MAX_FDS; i++)
        if (!fd_table[i].in_use) { fd_table[i].in_use = 1; return i; }
    return -1;
}

static inode_t *alloc_inode(const char *name) {
    inode_t *ino = (inode_t *)pmm_alloc_page();
    if (!ino) return 0;
    // zero it out
    uint8_t *p = (uint8_t *)ino;
    for (uint32_t i = 0; i < sizeof(inode_t); i++) p[i] = 0;

    for (int i = 0; i < FILE_NAME - 1 && name[i]; i++)
        ino->name[i] = name[i];

    ino->data     = (char *)pmm_alloc_page();
    ino->data2    = (char *)pmm_alloc_page();
    if (!ino->data || !ino->data2) {
        if (ino->data) pmm_free_page(ino->data);
        if (ino->data2) pmm_free_page(ino->data2);
        pmm_free_page(ino);
        return 0;
    }
    ino->capacity = PAGE_SIZE_ * 2;
    ino->id       = next_id++;

    // prepend to linked list
    ino->next = root;
    root = ino;
    return ino;
}

// ── Public API ───────────────────────────────────────

int fs_file_size(const char *name) {
    inode_t *ino = find_inode(name);
    return ino ? (int)ino->size : -1;
}

void fs_init(void) {
    for (int i = 0; i < MAX_FDS; i++) {
        fd_table[i].in_use = 0;
        fd_table[i].inode  = 0;
        fd_table[i].offset = 0;
    }
    // stdin (0), stdout (1), stderr (2) are always "open"
    fd_table[0].in_use = 1;
    fd_table[1].in_use = 1;
    fd_table[2].in_use = 1;
}

int fs_open(const char *name, int create) {
    inode_t *ino = find_inode(name);
    if (!ino) {
        if (!create) return -1;
        ino = alloc_inode(name);
        if (!ino) return -1;
    }
    int fd = alloc_fd();
    if (fd < 0) return -1;
    fd_table[fd].inode  = ino;
    fd_table[fd].offset = 0;
    return fd;
}

int fs_read(int fd, char *buf, uint32_t len) {
    if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].in_use) return -1;

    // stdin: read from keyboard
    if (fd == 0) {
        int ch = kb_getc();          // blocking
        buf[0] = (char)ch;
        if (ch == '\n') return 1;
        return 1;
    }

    inode_t *ino = fd_table[fd].inode;
    if (!ino) return -1;
    uint32_t off = fd_table[fd].offset;
    if (off >= ino->size) return 0;   // EOF
    uint32_t n = len;
    if (off + n > ino->size) n = ino->size - off;

    for (uint32_t i = 0; i < n; i++) {
        char *page = ((off + i) < PAGE_SIZE_) ? ino->data : ino->data2;
        uint32_t pg_off = (off + i) & (PAGE_SIZE_ - 1);
        buf[i] = page[pg_off];
    }
    fd_table[fd].offset += n;
    return n;
}

int fs_write(int fd, const char *buf, uint32_t len) {
    if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].in_use) return -1;

    // stdout / stderr: write to VGA + serial
    if (fd == 1 || fd == 2) {
        extern void vga_putc(char c);
        extern void serial_putc(char c);
        for (uint32_t i = 0; i < len; i++) {
            vga_putc(buf[i]);
            serial_putc(buf[i]);
        }
        return len;
    }

    inode_t *ino = fd_table[fd].inode;
    if (!ino) return -1;
    uint32_t off = fd_table[fd].offset;
    uint32_t written = 0;

    while (written < len && off < ino->capacity) {
        char *page = (off < PAGE_SIZE_) ? ino->data : ino->data2;
        uint32_t pg_off = off & (PAGE_SIZE_ - 1);
        uint32_t n = len - written;
        if (pg_off + n > PAGE_SIZE_) n = PAGE_SIZE_ - pg_off;

        for (uint32_t i = 0; i < n; i++)
            page[pg_off + i] = buf[written + i];

        written += n;
        off += n;
    }
    fd_table[fd].offset = off;
    if (fd_table[fd].offset > ino->size)
        ino->size = fd_table[fd].offset;
    return written;
}

int fs_close(int fd) {
    if (fd < 0 || fd >= MAX_FDS || fd <= 2) return -1;  // can't close stdin/out/err
    if (!fd_table[fd].in_use) return -1;
    fd_table[fd].in_use = 0;
    fd_table[fd].inode  = 0;
    fd_table[fd].offset = 0;
    return 0;
}

int fs_seek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= MAX_FDS || !fd_table[fd].in_use) return -1;
    if (!fd_table[fd].inode) return -1;

    inode_t *ino = fd_table[fd].inode;
    uint32_t base;
    switch (whence) {
    case 0: base = 0; break;                     // SEEK_SET
    case 1: base = fd_table[fd].offset; break;   // SEEK_CUR
    case 2: base = ino->size; break;             // SEEK_END
    default: return -1;
    }
    int new_off = (int)base + offset;
    if (new_off < 0) new_off = 0;
    if ((uint32_t)new_off > ino->size) new_off = (int)ino->size;
    fd_table[fd].offset = (uint32_t)new_off;
    return new_off;
}

void fs_list(void (*put)(const char *)) {
    for (inode_t *ino = root; ino; ino = ino->next) {
        put(ino->name);
        put(" (");
        char sz[16];
        uint32_t n = ino->size;
        int i = 14; sz[15] = 0;
        if (n == 0) sz[--i] = '0';
        while (n > 0 && i > 0) { sz[--i] = '0' + n%10; n /= 10; }
        put(&sz[i]);
        put(" bytes)\n");
    }
}
