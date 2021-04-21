#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wasi/libc.h>
#include <wasi/api.h>
#define STB_SPRINTF_IMPLEMENTATION

#include "stb_sprintf.h"
#define TRACE 0
#define DEBUG 0

#define WASM_EXPORT __attribute__((__visibility__("default")))
#define MAX_FDS 4096
#define MAX_NODES 1024
#define MAX_PATH 8192
#define MIN_PRESTAT_FDS 3
#define INVALID_INODE ((__wasi_inode_t)~0)

extern void memfs_log(const void* buf, size_t buf_size);
extern __wasi_errno_t host_write(__wasi_fd_t fd, const __wasi_ciovec_t *iovs,
                                 size_t iovs_len, size_t *nwritten);
extern __wasi_errno_t host_read(__wasi_fd_t fd, const __wasi_iovec_t *iovs,
                                 size_t iovs_len, size_t *nread);
extern void copy_out(void* their_dest, const void* my_src, size_t size);
extern void copy_in(void* my_dest, const void* their_src, size_t size);

static void __attribute__((format(printf, 1, 2))) logf(const char *fmt, ...) {
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  size_t buf_len = stbsp_vsnprintf(buf, sizeof(buf), fmt, args);
  memfs_log(buf, buf_len);
  va_end(args);
}

void Assert(bool result, const char* cond) {
  if (!result) {
    logf("Assertion failed: %s", cond);
    abort();
  }
}

#define ASSERT(cond) Assert(cond, #cond)

#if TRACE
#define tracef(...) logf(__VA_ARGS__)
#else
#define tracef(...) (void)0
#endif

#if DEBUG
#define debugf(...) logf(__VA_ARGS__)
#else
#define debugf(...) (void)0
#endif

#define TRACE_ERRNO(errno) tracef("!!  " #errno), errno

void TraceFDStat(__wasi_fdstat_t* stat) {
  tracef("!!  {filetype:%u, flags:%u, rights_base:%" PRIx64
         ", rights_inherit:%" PRIx64 "}",
         stat->fs_filetype, stat->fs_flags, stat->fs_rights_base,
         stat->fs_rights_inheriting);
}

void TraceFileStat(__wasi_filestat_t* stat) {
  tracef("!!  {dev:%" PRIu64 ", ino:%" PRIu64
         ", filetype:%u, nlink:%u, size:%" PRIu64 ", atim:%" PRIu64 ", "
         "mtime:%" PRIu64 ", ctime:%" PRIu64 "}",
         stat->dev, stat->ino, stat->filetype, stat->nlink,
         stat->size, stat->atim, stat->mtim, stat->ctim);
}

typedef struct FDesc FDesc;
typedef struct Node Node;
typedef struct FileContents FileContents;
typedef struct DirectoryContents DirectoryContents;

struct FileContents {
  void* data;
  __wasi_filesize_t size;
};

struct DirectoryContents {
  __wasi_dirent_t* dirents;
  size_t size;
};

struct Node {
  union {
    __wasi_inode_t parent;
    __wasi_inode_t next_free;
  };
  char *name;      // Null-terminated.
  size_t name_len; // Length w/o \0.
  __wasi_filestat_t stat;
  uint32_t fdesc_count;
  union {
    FileContents file;
    DirectoryContents dir;
  };
};

struct FDesc {
  __wasi_fdstat_t stat;
  __wasi_inode_t inode;
  __wasi_filesize_t offset;
  bool is_prestat;
};

static const __wasi_device_t kStdinDevice = 0;
static const __wasi_device_t kStdoutDevice = 1;
static const __wasi_device_t kStderrDevice = 2;
static const __wasi_device_t kMemDevice = 3;

static Node g_nodes[MAX_NODES];
static FDesc g_fdescs[MAX_FDS];
static __wasi_inode_t g_next_inode;
static char g_path_buf[MAX_PATH];
static Node* g_root_node;

static void InitInodes(void) {
  g_next_inode = 0;
  for (__wasi_inode_t i = 0; i < MAX_NODES; ++i) {
    g_nodes[i].next_free = i + 1;
  }
  g_nodes[MAX_NODES - 1].next_free = INVALID_INODE;
}

void DebugPrintNodeContents(struct Node* node) {
#if DEBUG
  __wasi_filesize_t addr = 0;
  __wasi_filesize_t next_line = 16;
  __wasi_filesize_t size = node->file.size;

  char buffer[100];

  uint8_t *data = node->file.data;
  for (; addr < size; addr = next_line) {
    next_line = addr + 16;
    if (next_line > size) {
      next_line = size;
    }

    char *p = buffer;
    size_t buf_len = sizeof(buffer);
    size_t len = stbsp_snprintf(buffer, buf_len, "%08x:", addr);
    p += len;
    buf_len -= len;

    for (; addr < next_line; addr += 2) {
      len = stbsp_snprintf(p, buf_len, " %02x", data[addr]);
      p += len;
      buf_len -= len;
      if (addr + 1 < next_line) {
        len = stbsp_snprintf(p, buf_len, "%02x", data[addr + 1]);
        p += len;
        buf_len -= len;
      }
    }

    memfs_log(buffer, p - buffer);
  }
#endif
}

static Node* GetNode(__wasi_inode_t node) {
  ASSERT(node < MAX_NODES);
  return &g_nodes[node];
}

static __wasi_inode_t GetInode(Node* node) {
  __wasi_inode_t inode = node - g_nodes;
  ASSERT(inode < MAX_NODES);
  return inode;
}

static char* GetDirentName(__wasi_dirent_t* dirent) {
  return (char *)dirent + sizeof(*dirent);
}

static __wasi_dirent_t *GetNextDirent(Node* dirnode, __wasi_dirent_t *dirent) {
  debugf("!!  GetNextDirent(dirnode:%p, dirent:%p) d_next:%" PRIu64 " size:%zu",
         dirnode, dirent, dirent->d_next, dirnode->dir.size);
  if (dirent->d_next < dirnode->dir.size) {
    return (__wasi_dirent_t *)((char *)dirnode->dir.dirents + dirent->d_next);
  }
  return NULL;
}

static bool IsRegularFileNode(Node *node) {
  return node->stat.filetype == __WASI_FILETYPE_REGULAR_FILE;
}

static bool IsDirectoryNode(Node *node) {
  return node->stat.filetype == __WASI_FILETYPE_DIRECTORY;
}

typedef struct {
  Node* node;
  __wasi_inode_t inode;
} NewNodeResult;

static NewNodeResult NewEmptyNode(void) {
  __wasi_inode_t inode = g_next_inode;
  ASSERT(inode != INVALID_INODE);
  Node* node = &g_nodes[inode];
  g_next_inode = node->next_free;
  return (NewNodeResult){.node = node, .inode = inode};
}

static NewNodeResult NewNode(Node *parent, const char *name, size_t name_len,
                             __wasi_filestat_t stat) {
  NewNodeResult result = NewEmptyNode();
  result.node->parent = GetInode(parent ? parent : result.node);
  result.node->name_len = name_len;
  result.node->name = malloc(name_len + 1);
  memcpy(result.node->name, name, name_len);
  result.node->name[name_len] = 0;
  result.node->stat = stat;
  result.node->stat.ino = result.inode;
  return result;
}

static void DeleteNode(Node* node) {
  tracef("!!  DeleteNode(node:%p)", node);
  ASSERT(node->stat.nlink == 0);
  __wasi_inode_t inode = GetInode(node);
  node->next_free = g_next_inode;
  g_next_inode = inode;
}

static void UnlinkNode(Node* node) {
  tracef("!!  UnlinkNode(node:%p) st_nlink=%u->%u", node, node->stat.nlink,
         node->stat.nlink - 1);
  ASSERT(node->stat.nlink > 0);
  if (--node->stat.nlink == 0 && node->fdesc_count == 0) {
    DeleteNode(node);
  }
}

static void AddDirent(Node *dirnode, const char *name, size_t name_len,
                      __wasi_inode_t inode) {
  size_t dirent_size = sizeof(__wasi_dirent_t) + name_len + 1;
  size_t old_size = dirnode->dir.size;
  size_t new_size = old_size + dirent_size;
  __wasi_dirent_t* new_dirents = realloc(dirnode->dir.dirents, new_size);
  __wasi_dirent_t* dirent = (__wasi_dirent_t*)((char*)new_dirents + old_size);
  dirent->d_next = new_size;
  dirent->d_ino = inode;
  dirent->d_namlen = name_len;
  dirent->d_type = GetNode(inode)->stat.filetype;
  char* dirent_name = GetDirentName(dirent);
  memcpy(dirent_name, name, name_len);
  dirent_name[name_len] = 0;

  dirnode->dir.dirents = new_dirents;
  dirnode->dir.size = new_size;

  debugf("!!  AddDirent(dirnode:%p, \"%.*s\", %" PRIu64
         ") new_size:%zu new_dirents:%p dirent:%p",
         dirnode, (int)name_len, name, inode, new_size, new_dirents, dirent);
}

#if 0
static __wasi_dirent_t *FindDirentByInode(Node *dirnode, __wasi_inode_t inode) {
  __wasi_dirent_t *dirent = dirnode->dir.dirents;
  while (dirent) {
    if (dirent->d_ino == inode) {
      return dirent;
    } else {
      dirent = GetNextDirent(dirnode, dirent);
    }
  }
  return NULL;
}
#endif

static __wasi_dirent_t *FindDirentByName(Node *dirnode, const char *name,
                                         size_t name_len) {
  debugf("!!  FindDirentByName(node:%p, \"%.*s\")", dirnode, (int)name_len,
         name);
  __wasi_dirent_t *dirent = dirnode->dir.dirents;
  while (dirent) {
    const char *dirent_name = GetDirentName(dirent);
    size_t len = dirent->d_namlen;
    debugf("!!    \"%.*s\" ==? \"%.*s\"", (int)name_len, name, (int)len,
           dirent_name);
    if (len == name_len && memcmp(name, dirent_name, len) == 0) {
      debugf("!!    yes");
      return dirent;
    } else {
      // No match.
      __wasi_dirent_t *new_dirent = GetNextDirent(dirnode, dirent);
      debugf("!!    no (name_len:%zu, len:%zu), dirent:%p=>%p", name_len, len,
             dirent, new_dirent);
      dirent = new_dirent;
    }
  }
  return NULL;
}

static __wasi_errno_t RemoveDirent(Node* dirnode, __wasi_dirent_t* dirent) {
  debugf("!!  RemoveDirent(node:%p, dirent:%p)", dirnode, dirent);
  const void* next_dirent = GetNextDirent(dirnode, dirent);
  size_t dirent_offset = (char*)dirent - (char*)dirnode->dir.dirents;
  size_t dirent_size = dirent->d_next - dirent_offset;
  size_t move_size = dirnode->dir.size - dirent->d_next;
  memmove(dirent, next_dirent, move_size);

  // Fix the dirents size early, so GetNextDirent works properly.
  size_t new_size = dirnode->dir.size - dirent_size;
  dirnode->dir.size = new_size;

  // We now need to fix up the d_next fields, since they still include the
  // space required for the removed dirent.
  while (dirent) {
    dirent->d_next -= dirent_size;
    dirent = GetNextDirent(dirnode, dirent);
  }

  dirnode->dir.dirents = realloc(dirnode->dir.dirents, new_size);
  return __WASI_ERRNO_SUCCESS;
}

static void EnsureFileSize(Node* node, __wasi_filesize_t new_size) {
  // TODO handle other file types
  ASSERT(IsRegularFileNode(node));
  __wasi_filesize_t old_size = node->file.size;
  if (new_size > old_size) {
    void *new_data = realloc(node->file.data, new_size);
    ASSERT(new_data);
    node->file.data = new_data;
    node->file.size = new_size;
    node->stat.size = new_size;
    memset((char*)new_data + old_size, 0, new_size - old_size);
  }
}

static void SetFileSize(Node* node, __wasi_filesize_t new_size) {
  ASSERT(IsRegularFileNode(node));
  __wasi_filesize_t old_size = node->file.size;
  if (new_size > old_size) {
    EnsureFileSize(node, new_size);
  } else {
    node->file.size = new_size;
    node->stat.size = new_size;
  }
}

static NewNodeResult NewFileNode(Node *parent, const char *name,
                                 size_t name_len, __wasi_filestat_t stat) {
  ASSERT(parent);
  NewNodeResult result = NewNode(parent, name, name_len, stat);
  AddDirent(parent, name, name_len, result.inode);
  return result;
}

static NewNodeResult NewDirectoryNode(Node *parent, const char *name,
                                      size_t name_len, __wasi_filestat_t stat) {
  NewNodeResult result = NewNode(parent, name, name_len, stat);
  AddDirent(result.node, ".", 1, result.inode);
  AddDirent(result.node, "..", 2, result.node->parent);
  if (parent != NULL) {
    AddDirent(parent, name, name_len, result.inode);
  }
  return result;
}

typedef struct {
  __wasi_errno_t error;
  FDesc* fdesc;
  __wasi_fd_t fd;
} NewFDResult;

static NewFDResult NewEmptyFD(void) {
  for (__wasi_fd_t i = 0; i < MAX_FDS; ++i) {
    FDesc* fdesc = &g_fdescs[i];
    if (fdesc->stat.fs_filetype == __WASI_FILETYPE_UNKNOWN) {
      return (NewFDResult){.error = __WASI_ERRNO_SUCCESS, .fdesc = fdesc, .fd = i};
    }
  }
  return (NewFDResult){.error = __WASI_ERRNO_MFILE, .fdesc = NULL, .fd = 0};
}

static bool IsValidFD(__wasi_fd_t fd) {
  return fd <= MAX_FDS &&
         g_fdescs[fd].stat.fs_filetype != __WASI_FILETYPE_UNKNOWN;
}

static FDesc *GetFDesc(__wasi_fd_t fd) {
  return IsValidFD(fd) ? &g_fdescs[fd] : NULL;
}

static NewFDResult NewFD(Node* node, __wasi_fdstat_t stat, bool is_prestat) {
  ASSERT(node);
  NewFDResult result = NewEmptyFD();
  ASSERT(result.error == __WASI_ERRNO_SUCCESS );
  result.fdesc->inode = GetInode(node);
  result.fdesc->stat = stat;
  result.fdesc->stat.fs_filetype = node->stat.filetype;
  result.fdesc->is_prestat = is_prestat;
  result.fdesc->offset = 0;
  node->fdesc_count++;
  return result;
}

static void ReleaseFD(FDesc* fdesc) {
  Node* node = GetNode(fdesc->inode);
  ASSERT(node->fdesc_count > 0);
  if (--node->fdesc_count == 0 && node->stat.nlink == 0) {
    DeleteNode(node);
  }
}

static __wasi_filestat_t GetCharDeviceStat(__wasi_device_t dev) {
  return (__wasi_filestat_t){.dev = dev,
                             .filetype = __WASI_FILETYPE_CHARACTER_DEVICE,
                             .nlink = 1,
                             .size = 0,
                             .atim = 0,
                             .mtim = 0,
                             .ctim = 0};
}

static __wasi_filestat_t GetDirectoryStat(void) {
  return (__wasi_filestat_t){.dev = kMemDevice,
                             .filetype = __WASI_FILETYPE_DIRECTORY,
                             .nlink = 1,
                             .size = 4096,
                             .atim = 0,
                             .mtim = 0,
                             .ctim = 0};
}

static __wasi_filestat_t GetFileStat(void) {
  return (__wasi_filestat_t){.dev = kMemDevice,
                             .filetype = __WASI_FILETYPE_REGULAR_FILE,
                             .nlink = 1,
                             .size = 0,
                             .atim = 0,
                             .mtim = 0,
                             .ctim = 0};
}

static __wasi_rights_t GetDefaultFileRights(void) {
  return UINT64_C(0x000000001FFFFFFF);
}

static __wasi_fdstat_t GetFileFDStat(__wasi_fdflags_t flags) {
  return (__wasi_fdstat_t){.fs_flags = flags,
                           .fs_rights_base = GetDefaultFileRights(),
                           .fs_rights_inheriting = GetDefaultFileRights()};
}

static __wasi_rights_t GetDefaultDirectoryRights(void) {
  return UINT64_C(0x000000001FFFFFFF);
}

static __wasi_fdstat_t GetDirectoryFDStat(void) {
  return (__wasi_fdstat_t){.fs_flags = 0,
                           .fs_rights_base = GetDefaultDirectoryRights(),
                           .fs_rights_inheriting = GetDefaultDirectoryRights()};
}

static void CreateStdFds(void) {
  NewNodeResult in = NewNode(NULL, "stdin", 5, GetCharDeviceStat(kStdinDevice));
  NewFDResult in_fd = NewFD(in.node, GetFileFDStat(0), false);
  ASSERT(in_fd.fd == 0);

  NewNodeResult out =
      NewNode(NULL, "stdout", 6, GetCharDeviceStat(kStdoutDevice));
  NewFDResult out_fd =
      NewFD(out.node, GetFileFDStat(__WASI_FDFLAGS_APPEND ), false);
  ASSERT(out_fd.fd == 1);

  NewNodeResult err =
      NewNode(NULL, "stderr", 6, GetCharDeviceStat(kStderrDevice));
  NewFDResult err_fd =
      NewFD(err.node, GetFileFDStat(__WASI_FDFLAGS_APPEND ), false);
  ASSERT(err_fd.fd == 2);

  NewNodeResult root = NewDirectoryNode(NULL, "", 0, GetDirectoryStat());
  NewFDResult root_fd = NewFD(root.node, GetDirectoryFDStat(), true);
  ASSERT(root_fd.fd == 3);
  g_root_node = root.node;
}

WASM_EXPORT
void init(void) {
  InitInodes();
  CreateStdFds();
}

WASM_EXPORT __wasi_errno_t fd_allocate(__wasi_fd_t fd, __wasi_filesize_t offset,
                                       __wasi_filesize_t len) {
  tracef("!!fd_allocate(fd:%u, offset:%" PRIu64 ", len:%" PRIu64 ")", fd,
         offset, len);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF );
  }
  Node* node = GetNode(fdesc->inode);
  if (node->stat.dev != kMemDevice) {
    return TRACE_ERRNO(__WASI_ERRNO_NOTCAPABLE);
  }

  // TODO allocate memory?
  return TRACE_ERRNO(__WASI_ERRNO_SUCCESS);
}

WASM_EXPORT __wasi_errno_t fd_close(__wasi_fd_t fd) {
  tracef("!!fd_close(fd:%u)", fd);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  fdesc->stat.fs_filetype = __WASI_FILETYPE_UNKNOWN;
  ReleaseFD(fdesc);
  DebugPrintNodeContents(GetNode(fdesc->inode));
  return TRACE_ERRNO(__WASI_ERRNO_SUCCESS);
}

WASM_EXPORT __wasi_errno_t fd_fdstat_get(__wasi_fd_t fd, __wasi_fdstat_t *buf) {
  tracef("!!fd_fdstat_get(fd:%u, buf:%p)", fd, buf);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  TraceFDStat(&fdesc->stat);
  copy_out(buf, &fdesc->stat, sizeof(*buf));
  return __WASI_ERRNO_SUCCESS ;
}

WASM_EXPORT __wasi_errno_t fd_fdstat_set_flags(__wasi_fd_t fd,
                                               __wasi_fdflags_t flags) {
  tracef("!!fd_fdstat_set_flags(fd:%u, flags:%u)", fd, flags);
  return TRACE_ERRNO(__WASI_ERRNO_NOTCAPABLE);
}

WASM_EXPORT __wasi_errno_t fd_filestat_get(__wasi_fd_t fd,
                                           __wasi_filestat_t *buf) {
  tracef("!!fd_filestat_get(fd:%u, buf:%p)", fd, buf);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  Node* node = GetNode(fdesc->inode);
  TraceFileStat(&node->stat);
  copy_out(buf, &node->stat, sizeof(*buf));
  return __WASI_ERRNO_SUCCESS ;
}

WASM_EXPORT __wasi_errno_t fd_filestat_set_size(__wasi_fd_t fd,
                                                __wasi_filesize_t st_size) {
  tracef("!!fd_filestat_set_size(fd:%u, buf:%" PRIu64 ")", fd, st_size);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  SetFileSize(GetNode(fdesc->inode), st_size);
  return TRACE_ERRNO(__WASI_ERRNO_SUCCESS);
}

static size_t ReadIovec(Node *node, __wasi_iovec_t *iovs, size_t iovs_len,
                        __wasi_filesize_t offset) {
  size_t total_len = 0;
  for (size_t i = 0; i < iovs_len; ++i) {
    __wasi_iovec_t *iov = &iovs[i];
    debugf("!!  {buf:%p, buf_len:%zu}", iov->buf, iov->buf_len);
    size_t len = iov->buf_len;
    if (offset + len < node->file.size) {
      len = node->file.size - offset;
    }
    copy_out(iov->buf, (char *)node->file.data + offset + total_len, len);
    total_len += len;
  }
  return total_len;
}

WASM_EXPORT __wasi_errno_t fd_pread(__wasi_fd_t fd, const __wasi_iovec_t *iovs,
                                    size_t iovs_len, __wasi_filesize_t offset,
                                    size_t *nread) {
  tracef("!!fd_pread(fd:%u, iovs:%p, iovs_len:%zu, offset:%" PRIu64
         ", nread:%p)",
         fd, iovs, iovs_len, offset, nread);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  Node* node = GetNode(fdesc->inode);
  __wasi_iovec_t iovs_copy[iovs_len];
  copy_in(iovs_copy, iovs, sizeof(*iovs) * iovs_len);
  size_t total_len = ReadIovec(node, iovs_copy, iovs_len, offset);
  tracef("!!  nread=%zu", total_len);
  copy_out(nread, &total_len, sizeof(*nread));
  return __WASI_ERRNO_SUCCESS ;
}

WASM_EXPORT __wasi_errno_t fd_prestat_dir_name(__wasi_fd_t fd, char *path,
                                               size_t path_len) {
  tracef("!!fd_prestat_dir_name(fd:%u, path:%p, path_len:%zu)", fd, path,
         path_len);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL || !fdesc->is_prestat) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  ASSERT(fdesc->stat.fs_filetype == __WASI_FILETYPE_DIRECTORY);
  Node* node = GetNode(fdesc->inode);
  size_t len = node->name_len;
  if (len < path_len) {
    len = path_len;
  }
  tracef("!!  \"%.*s\"", (int)len, node->name);
  copy_out(path, &node->name, len);
  return __WASI_ERRNO_SUCCESS ;
}

WASM_EXPORT __wasi_errno_t fd_prestat_get(__wasi_fd_t fd,
                                          __wasi_prestat_t *buf) {
  tracef("!!fd_prestat_get(fd:%u, buf:%p)", fd, buf);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL || !fdesc->is_prestat) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  ASSERT(fdesc->stat.fs_filetype == __WASI_FILETYPE_DIRECTORY);
  __wasi_prestat_t prestat = {.tag = 0,
                              .u = {{GetNode(fdesc->inode)->name_len}}};
  tracef("!!  {tag:%u, pr_name_len:%zu}", prestat.tag,
         prestat.u.dir.pr_name_len);
  copy_out(buf, &prestat, sizeof(*buf));
  return __WASI_ERRNO_SUCCESS ;
}

WASM_EXPORT __wasi_errno_t fd_read(__wasi_fd_t fd, const __wasi_iovec_t *iovs,
                                   size_t iovs_len, size_t *nread) {
  tracef("!!fd_read(fd:%u, iovs:%p, iovs_len:%zu, nread:%p)", fd, iovs,
         iovs_len, nread);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  Node* node = GetNode(fdesc->inode);
  switch (node->stat.dev) {
    case kStdinDevice:
      return host_read(node->stat.dev, iovs, iovs_len, nread);

    case kMemDevice:
      // Handled below.
      break;

    default:
      ASSERT(false);
      return __WASI_ERRNO_NODEV;
  }
  __wasi_iovec_t iovs_copy[iovs_len];
  copy_in(iovs_copy, iovs, sizeof(*iovs) * iovs_len);
  size_t total_len = ReadIovec(node, iovs_copy, iovs_len, fdesc->offset);
  fdesc->offset += total_len;
  tracef("!!  nread=%zu", total_len);
  copy_out(nread, &total_len, sizeof(*nread));
  return __WASI_ERRNO_SUCCESS ;
}

WASM_EXPORT __wasi_errno_t fd_readdir(__wasi_fd_t fd, void *buf, size_t buf_len,
                                      __wasi_dircookie_t cookie,
                                      size_t *bufused) {
  tracef("!!fd_readdir(fd:%u, buf:%p, buf_len:%zu, dir_cookie:%" PRIu64
         " bufused:%p)",
         fd, buf, buf_len, cookie, bufused);
  return TRACE_ERRNO(__WASI_ERRNO_NOTCAPABLE);
}

WASM_EXPORT __wasi_errno_t fd_seek(__wasi_fd_t fd, __wasi_filedelta_t offset,
                                   __wasi_whence_t whence,
                                   __wasi_filesize_t *newoffset) {
  tracef("!!fd_seek(fd:%u, offset:%" PRIu64 ", buf_len:%u, newoffset:%p)", fd,
         offset, whence, newoffset);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  Node* node = GetNode(fdesc->inode);
  __wasi_filesize_t size = node->stat.size;
  switch (whence) {
    case __WASI_WHENCE_CUR: fdesc->offset += offset; break;
    case __WASI_WHENCE_END: fdesc->offset = size + offset; break;
    case __WASI_WHENCE_SET: fdesc->offset = offset; break;
    default:
      return TRACE_ERRNO(__WASI_ERRNO_INVAL);
  }
  if (fdesc->offset > size) {
    fdesc->offset = size;
  }
  tracef("!!  newoffset=%" PRIu64, fdesc->offset);
  copy_out(newoffset, &fdesc->offset, sizeof(*newoffset));
  return __WASI_ERRNO_SUCCESS ;
}

WASM_EXPORT __wasi_errno_t fd_write(__wasi_fd_t fd, const __wasi_ciovec_t *iovs,
                                    size_t iovs_len, size_t *nwritten) {
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  Node* node = GetNode(fdesc->inode);
  switch (node->stat.dev) {
    case kStdoutDevice:
    case kStderrDevice:
      return host_write(node->stat.dev, iovs, iovs_len, nwritten);

    case kMemDevice:
      // Handled below.
      break;

    default:
      ASSERT(false);
      return __WASI_ERRNO_NODEV;
  }

  tracef("!!fd_write(fd:%u, iovs:%p, iovs_len:%zu, nwritten:%p)", fd, iovs,
         iovs_len, nwritten);

  // TODO handle other file types
  ASSERT(IsRegularFileNode(node));
  // TODO handle append flag
  ASSERT(!(fdesc->stat.fs_flags & __WASI_FDFLAGS_APPEND));

  size_t total_len = 0;
  __wasi_iovec_t iovs_copy[iovs_len];
  copy_in(iovs_copy, iovs, sizeof(*iovs) * iovs_len);
  for (size_t i = 0; i < iovs_len; ++i) {
    total_len += iovs_copy[i].buf_len;
  }


  __wasi_filesize_t offset = fdesc->offset;
  __wasi_filesize_t end = offset + total_len;
  EnsureFileSize(node, end);

  for (size_t i = 0; i < iovs_len; ++i) {
    __wasi_iovec_t* iov = &iovs_copy[i];
    size_t len = iov->buf_len;
    debugf("!!  {buf:%p, buf_len:%zu}", iov->buf, iov->buf_len);
    ASSERT(offset + len <= end);
    copy_in((char *)node->file.data + offset, iov->buf, len);
    offset += len;
  }

  fdesc->offset = offset;
  copy_out(nwritten, &total_len, sizeof(*nwritten));
  tracef("!!  nwritten=%zu", total_len);
  return __WASI_ERRNO_SUCCESS ;
}

typedef struct {
  Node *node;              // NULL if path doesn't exist.
  Node *parent;            // Never NULL.
  __wasi_dirent_t *dirent; // NULL if path doesn't exist.
  const char *name;        // Shared w/ |path| passed to LookupPath.
  size_t name_len;         // Length w/o \0.
} LookupResult;

static LookupResult LookupPath(Node *dirnode, const char *path,
                               size_t path_len) {
  debugf("!!  LookupPath(%p, \"%.*s\")", dirnode, (int)path_len, path);
  ASSERT(dirnode && IsDirectoryNode(dirnode));

  // Find path component to look up [0, sep).
  size_t sep;
  for (sep = 0; sep < path_len && path[sep] != '/'; ++sep) {
  }

  bool is_last_component = sep == path_len;

  __wasi_dirent_t* dirent = FindDirentByName(dirnode, path, sep);
  if (dirent == NULL) {
    // Nothing in this directory with that name; if this is the last component,
    // set the parent in case the caller wants to create a new file. If this
    // wasn't the last component, don't set the parent, since the path wasn't
    // fully resolved.
    bool ends_with_slash = sep == path_len - 1;
    if (is_last_component || ends_with_slash) {
      return (LookupResult){.parent = dirnode, .name = path, .name_len = sep};
    } else {
      return (LookupResult){};
    }
  }

  // Match, search next component.
  Node *node = GetNode(dirent->d_ino);
  if (is_last_component) {
    // End of path.
    return (LookupResult){.node = node,
                          .parent = dirnode,
                          .dirent = dirent,
                          .name = path,
                          .name_len = sep};
  } else if (dirent->d_type == __WASI_FILETYPE_DIRECTORY) {
    // Look in next directory.
    ASSERT(sep < path_len);
    return LookupPath(node, path + sep + 1, path_len - sep - 1);
  } else {
    // Not a directory; fail.
    return (LookupResult){};
  }
}

WASM_EXPORT __wasi_errno_t path_create_directory(__wasi_fd_t fd,
                                                 const char *path,
                                                 size_t path_len) {
  copy_in(g_path_buf, path, path_len);
  tracef("!!path_create_directory(fd:%u, path:\"%.*s\")", fd, (int)path_len,
         g_path_buf);
  return TRACE_ERRNO(__WASI_ERRNO_NOTCAPABLE);
}

WASM_EXPORT __wasi_errno_t path_filestat_get(__wasi_fd_t fd,
                                             __wasi_lookupflags_t flags,
                                             const char *path, size_t path_len,
                                             __wasi_filestat_t *buf) {
  copy_in(g_path_buf, path, path_len);
  tracef("!!path_filestat_get(fd:%u, flags:%u, path:\"%.*s\", buf:%p)", fd,
         flags, (int)path_len, g_path_buf, buf);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  LookupResult lookup = LookupPath(GetNode(fdesc->inode), g_path_buf, path_len);
  if (!lookup.node) {
    return TRACE_ERRNO(__WASI_ERRNO_NOENT);
  }
  TraceFileStat(&lookup.node->stat);
  copy_out(buf, &lookup.node->stat, sizeof(*buf));
  return __WASI_ERRNO_SUCCESS ;
}

WASM_EXPORT __wasi_errno_t
path_open(__wasi_fd_t dirfd, __wasi_lookupflags_t dirflags, const char *path,
          size_t path_len, __wasi_oflags_t oflags,
          __wasi_rights_t fs_rights_base, __wasi_rights_t fs_rights_inheriting,
          __wasi_fdflags_t fs_flags, __wasi_fd_t *fd) {
  copy_in(g_path_buf, path, path_len);
  tracef("!!path_open(dirfd:%u, dirflags:%u, path:\"%.*s\", oflags:%u, "
         "fs_rights_base:%" PRIx64 ", fs_rights_inheriting:%" PRIx64
         ", fs_flags:%u, fd:%p)",
         dirfd, dirflags, (int)path_len, g_path_buf, oflags, fs_rights_base,
         fs_rights_inheriting, fs_flags, fd);
  FDesc* fdesc = GetFDesc(dirfd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  LookupResult lookup = LookupPath(GetNode(fdesc->inode), g_path_buf, path_len);
  if ((oflags & __WASI_OFLAGS_EXCL) && lookup.node) {
    return TRACE_ERRNO(__WASI_ERRNO_EXIST);
  }
  if (!(oflags & (__WASI_OFLAGS_CREAT| __WASI_OFLAGS_EXCL)) && !lookup.node) {
    return TRACE_ERRNO(__WASI_ERRNO_NOENT);
  }
  if ((oflags & __WASI_OFLAGS_DIRECTORY) && IsDirectoryNode(lookup.node)) {
    return TRACE_ERRNO(__WASI_ERRNO_NOTDIR);
  }

  if ((oflags & __WASI_OFLAGS_TRUNC) && lookup.node) {
    // TODO handle other file types
    ASSERT(IsRegularFileNode(lookup.node));
    lookup.node->file.size = 0;
  }

  if ((oflags & __WASI_OFLAGS_CREAT) && !lookup.node) {
    if (!lookup.parent) {
      return TRACE_ERRNO(__WASI_ERRNO_NOENT);
    }

    NewNodeResult new_node =
        NewFileNode(lookup.parent, lookup.name, lookup.name_len, GetFileStat());
    lookup.node = new_node.node;
  }

  __wasi_rights_t inherited = fdesc->stat.fs_rights_inheriting;
  __wasi_fdstat_t stat = {.fs_flags = fs_flags,
                          .fs_rights_base = inherited & fs_rights_base,
                          .fs_rights_inheriting =
                              inherited & fs_rights_inheriting};
  NewFDResult result = NewFD(lookup.node, stat, false);
  tracef("!!  fd=%u", result.fd);
  copy_out(fd, &result.fd, sizeof(*fd));
  return __WASI_ERRNO_SUCCESS ;
}

WASM_EXPORT __wasi_errno_t path_readlink(__wasi_fd_t fd, const char *path,
                                         size_t path_len, char *buf,
                                         size_t buf_len, size_t *bufused) {
  copy_in(g_path_buf, path, path_len);
  tracef("!!path_readlink(fd:%u, path:\"%.*s\", buf:%p, buf_len:%zu, "
         "bufused:%p)",
         fd, (int)path_len, g_path_buf, buf, buf_len, bufused);
  return TRACE_ERRNO(__WASI_ERRNO_NOTCAPABLE);
}

WASM_EXPORT __wasi_errno_t path_remove_directory(__wasi_fd_t fd,
                                                 const char *path,
                                                 size_t path_len) {
  copy_in(g_path_buf, path, path_len);
  tracef("!!path_remove_directory(fd:%u, path:\"%.*s\")", fd, (int)path_len,
         g_path_buf);
  return TRACE_ERRNO(__WASI_ERRNO_NOTCAPABLE);
}

WASM_EXPORT __wasi_errno_t path_rename(__wasi_fd_t old_fd, const char *old_path,
                                       size_t old_path_len, __wasi_fd_t new_fd,
                                       const char *new_path,
                                       size_t new_path_len) {
  ASSERT(old_path_len + new_path_len < MAX_PATH);
  char* old_path_copy = g_path_buf;
  char* new_path_copy = g_path_buf + old_path_len;
  copy_in(old_path_copy, old_path, old_path_len);
  copy_in(new_path_copy, new_path, new_path_len);

  tracef("!!path_rename(old_fd:%u, old_path:\"%.*s\", new_fd:%u, "
         "new_path:\"%.*s\")",
         old_fd, (int)old_path_len, old_path_copy, new_fd, (int)new_path_len,
         new_path_copy);
  FDesc* old_fdesc = GetFDesc(old_fd);
  FDesc* new_fdesc = GetFDesc(new_fd);
  if (old_fdesc == NULL || new_fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }

  LookupResult old_lookup =
      LookupPath(GetNode(old_fdesc->inode), old_path_copy, old_path_len);
  if (!old_lookup.node) {
    return TRACE_ERRNO(__WASI_ERRNO_NOENT);
  }
  LookupResult new_lookup =
      LookupPath(GetNode(new_fdesc->inode), new_path_copy, new_path_len);
  if (!new_lookup.parent) {
    return TRACE_ERRNO(__WASI_ERRNO_NOENT);
  }

  // TODO rename directory.
  ASSERT(IsRegularFileNode(old_lookup.node) &&
         (!new_lookup.node || IsRegularFileNode(new_lookup.node)));

  if (new_lookup.node) {
    // Remove the directory entry for this node. We increment the link count,
    // in case we're renaming over a hard-link to the same node.
    new_lookup.node->stat.nlink++;
    RemoveDirent(new_lookup.parent, new_lookup.dirent);

    // We may have changed old_lookup.dirent by calling RemoveDirent; look it
    // up again.
    old_lookup.dirent = FindDirentByName(old_lookup.parent, old_lookup.name,
                                         old_lookup.name_len);
  }

  RemoveDirent(old_lookup.parent, old_lookup.dirent);
  AddDirent(new_lookup.parent, new_lookup.name, new_lookup.name_len,
            GetInode(old_lookup.node));
  if (new_lookup.node) {
    // Decrement the link count that we incremented earlier.
    UnlinkNode(new_lookup.node);
  }
  return TRACE_ERRNO(__WASI_ERRNO_SUCCESS );
}

WASM_EXPORT __wasi_errno_t path_symlink(const char *old_path,
                                        size_t old_path_len, __wasi_fd_t fd,
                                        const char *new_path,
                                        size_t new_path_len) {
  ASSERT(old_path_len + new_path_len < MAX_PATH);
  char* old_path_copy = g_path_buf;
  char* new_path_copy = g_path_buf + old_path_len;
  copy_in(old_path_copy, old_path, old_path_len);
  copy_in(new_path_copy, new_path, new_path_len);
  tracef("!!path_symlink(old_path:\"%.*s\", new_path:\"%.*s\")",
         (int)old_path_len, old_path, (int)new_path_len, new_path);
  return TRACE_ERRNO(__WASI_ERRNO_NOTCAPABLE);
}

WASM_EXPORT __wasi_errno_t path_unlink_file(__wasi_fd_t fd, const char *path,
                                            size_t path_len) {
  copy_in(g_path_buf, path, path_len);
  tracef("!!path_unlink_file(fd:%u, path:\"%.*s\")", fd, (int)path_len,
         g_path_buf);
  FDesc* fdesc = GetFDesc(fd);
  if (fdesc == NULL) {
    return TRACE_ERRNO(__WASI_ERRNO_BADF);
  }
  LookupResult lookup = LookupPath(GetNode(fdesc->inode), g_path_buf, path_len);
  if (!lookup.node) {
    return TRACE_ERRNO(__WASI_ERRNO_NOENT);
  }
  RemoveDirent(lookup.parent, lookup.dirent);
  UnlinkNode(lookup.node);
  return TRACE_ERRNO(__WASI_ERRNO_SUCCESS );
}


// Helper functions for reading/writing files from JS.
typedef uint32_t wasi_inode32_t;
typedef uint32_t wasi_filesize32_t;

WASM_EXPORT void* GetPathBuf(void) {
  return g_path_buf;
}

WASM_EXPORT size_t GetPathBufLen(void) {
  return sizeof(g_path_buf);
}

WASM_EXPORT wasi_inode32_t FindNode(size_t path_len) {
  LookupResult lookup = LookupPath(g_root_node, g_path_buf, path_len);
  ASSERT(lookup.node);
  __wasi_inode_t inode = GetInode(lookup.node);
  tracef("!!  FindNode(path:\"%.*s\") => %" PRIu64, (int)path_len, g_path_buf,
         inode);
  return (wasi_inode32_t)inode;
}

WASM_EXPORT wasi_inode32_t AddDirectoryNode(size_t path_len) {
  LookupResult lookup = LookupPath(g_root_node, g_path_buf, path_len);
  ASSERT(lookup.parent);
  NewNodeResult new_node = NewDirectoryNode(
      lookup.parent, lookup.name, lookup.name_len, GetDirectoryStat());
  __wasi_inode_t inode = GetInode(new_node.node);
  tracef("!!  AddDirectoryNode(path:\"%.*s\") => %" PRIu64, (int)path_len,
         g_path_buf, inode);
  return (wasi_inode32_t)inode;
}

WASM_EXPORT wasi_inode32_t AddFileNode(size_t path_len,
                                       wasi_filesize32_t file_size) {
  LookupResult lookup = LookupPath(g_root_node, g_path_buf, path_len);
  ASSERT(lookup.parent);
  NewNodeResult new_node;
  if (!lookup.node) {
    new_node =
        NewFileNode(lookup.parent, lookup.name, lookup.name_len, GetFileStat());
  } else {
    new_node.node = lookup.node;
    new_node.inode = GetInode(lookup.node);
  }
  SetFileSize(new_node.node, file_size);
  tracef("!!  AddFileNode(path:\"%.*s\") => %" PRIu64, (int)path_len,
         g_path_buf, new_node.inode);
  return (wasi_inode32_t)new_node.inode;
}

WASM_EXPORT void* GetFileNodeAddress(wasi_inode32_t inode) {
  Node* node = GetNode(inode);
  ASSERT(node);
  ASSERT(IsRegularFileNode(node));
  tracef("!!  GetFileNodeAddress(inode:%u) => %p", inode, node->file.data);
  return node->file.data;
}

WASM_EXPORT wasi_filesize32_t GetFileNodeSize(wasi_inode32_t inode) {
  Node* node = GetNode(inode);
  ASSERT(node);
  tracef("!!  GetFileNodeSize(inode:%u) => %" PRIu64, inode, node->file.size);
  return (wasi_filesize32_t)node->file.size;
}