/********************************************************
 *
 *
 *      Stub functions for Wine module
 *
 *
 ********************************************************/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "windef.h"
#include "winbase.h"
#include "debugtools.h"
#include "heap.h"
#include "ext.h"

#if 0
//REMOVE SIMPLIFY
static void *
mymalloc (unsigned int size)
{
  printf ("malloc %d\n", size);
  return malloc (size);
}

#undef malloc
#define malloc mymalloc
#endif

int
dbg_header_err (const char *dbg_channel, const char *func)
{
  return 0;
}

int
dbg_header_warn (const char *dbg_channel, const char *func)
{
  return 0;
}

int
dbg_header_fixme (const char *dbg_channel, const char *func)
{
  return 0;
}

int
dbg_header_trace (const char *dbg_channel, const char *func)
{
  return 0;
}

int
dbg_vprintf (const char *format, va_list args)
{
  return 0;
}

int
__vprintf (const char *format, ...)
{
#ifdef DETAILED_OUT
  va_list va;
  va_start (va, format);
  vprintf (format, va);
  va_end (va);
#endif
  return 0;
}

HANDLE WINAPI
GetProcessHeap (void)
{
  return 1;
}

LPVOID WINAPI
HeapAlloc (HANDLE heap, DWORD flags, DWORD size)
{
  //static int i = 5;
  void *m = (flags & 0x8) ? calloc (size, 1) : malloc (size);
  //printf("HeapAlloc %p  %d  (%d)\n", m, size, flags);
  //if (--i == 0)
  //    abort();
  return m;
}

WIN_BOOL WINAPI
HeapFree (HANDLE heap, DWORD flags, LPVOID mem)
{
  if (mem)
    free (mem);
  //printf("HeapFree  %p\n", mem);
  //if (!mem)
  //    abort();
  return 1;
}

static int last_error;

DWORD WINAPI
GetLastError (void)
{
  return last_error;
}

VOID WINAPI
SetLastError (DWORD error)
{
  last_error = error;
}

WIN_BOOL WINAPI
ReadFile (HANDLE handle, LPVOID mem, DWORD size, LPDWORD result,
    LPOVERLAPPED flags)
{
  *result = read (handle, mem, size);
  return *result;
}

INT WINAPI
lstrcmpiA (LPCSTR c1, LPCSTR c2)
{
  return strcasecmp (c1, c2);
}

LPSTR WINAPI
lstrcpynA (LPSTR dest, LPCSTR src, INT num)
{
  return strncpy (dest, src, num);
}

INT WINAPI
lstrlenA (LPCSTR s)
{
  return strlen (s);
}

INT WINAPI
lstrlenW (LPCWSTR s)
{
  int l;
  if (!s)
    return 0;
  l = 0;
  while (s[l])
    l++;
  return l;
}

LPSTR WINAPI
lstrcpynWtoA (LPSTR dest, LPCWSTR src, INT count)
{
  LPSTR result = dest;
  int moved = 0;
  if ((dest == 0) || (src == 0))
    return 0;
  while (moved < count) {
    *dest = *src;
    moved++;
    if (*src == 0)
      break;
    src++;
    dest++;
  }
  return result;
}

/* i stands here for ignore case! */
int
wcsnicmp (const unsigned short *s1, const unsigned short *s2, int n)
{
  /*
     if(s1==0)
     return;
     if(s2==0)
     return;
   */
  while (n > 0) {
    if (((*s1 | *s2) & 0xff00) || toupper ((char) *s1) != toupper ((char) *s2)) {

      if (*s1 < *s2)
        return -1;
      else if (*s1 > *s2)
        return 1;
      else if (*s1 == 0)
        return 0;
    }
    s1++;
    s2++;
    n--;
  }
  return 0;
}

WIN_BOOL WINAPI
IsBadReadPtr (LPCVOID data, UINT size)
{
  if (size == 0)
    return 0;
  if (data == NULL)
    return 1;
  return 0;
}

LPSTR
HEAP_strdupA (HANDLE heap, DWORD flags, LPCSTR string)
{
//    return strdup(string);
  char *answ = malloc (strlen (string) + 1);
  strcpy (answ, string);
  return answ;
}

LPWSTR
HEAP_strdupAtoW (HANDLE heap, DWORD flags, LPCSTR string)
{
  int size, i;
  short *answer;
  if (string == 0)
    return 0;
  size = strlen (string);
  answer = malloc (2 * (size + 1));
  for (i = 0; i <= size; i++)
    answer[i] = (short) string[i];
  return (LPWSTR) answer;
}

LPSTR
HEAP_strdupWtoA (HANDLE heap, DWORD flags, LPCWSTR string)
{
  int size, i;
  char *answer;
  if (string == 0)
    return 0;
  size = 0;
  while (string[size])
    size++;
  answer = malloc (size + 2);
  for (i = 0; i <= size; i++)
    answer[i] = (char) string[i];
  return answer;
}

/***********************************************************************
 *           FILE_dommap
 */

//#define MAP_PRIVATE
//#define MAP_SHARED
#undef MAP_ANON
LPVOID
FILE_dommap (int unix_handle, LPVOID start,
    DWORD size_high, DWORD size_low,
    DWORD offset_high, DWORD offset_low, int prot, int flags)
{
  int fd = -1;
  int pos;
  LPVOID ret;
  ssize_t read_ret;

  if (size_high || offset_high)
    printf ("offsets larger than 4Gb not supported\n");

  if (unix_handle == -1) {
#ifdef MAP_ANON
//      printf("Anonymous\n");
    flags |= MAP_ANON;
#else
    static int fdzero = -1;

    if (fdzero == -1) {
      if ((fdzero = open ("/dev/zero", O_RDONLY)) == -1) {
        perror ("Cannot open /dev/zero for READ. Check permissions! error: ");
        abort ();
      }
    }
    fd = fdzero;
#endif /* MAP_ANON */
    /* Linux EINVAL's on us if we don't pass MAP_PRIVATE to an anon mmap */
#ifdef MAP_SHARED
    flags &= ~MAP_SHARED;
#endif
#ifdef MAP_PRIVATE
    flags |= MAP_PRIVATE;
#endif
  } else
    fd = unix_handle;
//    printf("fd %x, start %x, size %x, pos %x, prot %x\n",fd,start,size_low, offset_low, prot);
//    if ((ret = mmap( start, size_low, prot,
//                     flags, fd, offset_low )) != (LPVOID)-1)
  if ((ret = mmap (start, size_low, prot,
              MAP_PRIVATE | MAP_FIXED, fd, offset_low)) != (LPVOID) - 1) {
//          printf("address %08x\n", *(int*)ret);
//      printf("%x\n", ret);
    return ret;
  }
//    printf("mmap %d\n", errno);

  /* mmap() failed; if this is because the file offset is not    */
  /* page-aligned (EINVAL), or because the underlying filesystem */
  /* does not support mmap() (ENOEXEC), we do it by hand.        */

  if (unix_handle == -1)
    return ret;
  if ((errno != ENOEXEC) && (errno != EINVAL))
    return ret;
  if (prot & PROT_WRITE) {
    /* We cannot fake shared write mappings */
#ifdef MAP_SHARED
    if (flags & MAP_SHARED)
      return ret;
#endif
#ifdef MAP_PRIVATE
    if (!(flags & MAP_PRIVATE))
      return ret;
#endif
  }
/*    printf( "FILE_mmap: mmap failed (%d), faking it\n", errno );*/
  /* Reserve the memory with an anonymous mmap */
  ret = FILE_dommap (-1, start, size_high, size_low, 0, 0,
      PROT_READ | PROT_WRITE, flags);
  if (ret == (LPVOID) - 1)
//    {
//      perror(
    return ret;
  /* Now read in the file */
  if ((pos = lseek (fd, offset_low, SEEK_SET)) == -1) {
    FILE_munmap (ret, size_high, size_low);
//      printf("lseek\n");
    return (LPVOID) - 1;
  }
  if ((read_ret = read (fd, ret, size_low)) != size_low) {
    FILE_munmap (ret, size_high, size_low);
//      printf("read\n");
    return (LPVOID) - 1;
  }
  /* Restore the file pointer */
  if (lseek (fd, pos, SEEK_SET) == -1) {
    FILE_munmap (ret, size_high, size_low);
//      printf("lseek back\n");
    return (LPVOID) - 1;
  }
  /* Set the right protection */
  if (mprotect (ret, size_low, prot) == -1) {
    FILE_munmap (ret, size_high, size_low);
//      printf("mprotect\n");
    return (LPVOID) - 1;
  }
//    printf("address %08x\n", *(int*)ret);
  return ret;
}


/***********************************************************************
 *           FILE_munmap
 */
int
FILE_munmap (LPVOID start, DWORD size_high, DWORD size_low)
{
  if (size_high)
    printf ("offsets larger than 4Gb not supported\n");
  return munmap (start, size_low);
}

//static int mapping_size=0;

struct file_mapping_s;
typedef struct file_mapping_s
{
  int mapping_size;
  char *name;
  LPVOID handle;
  struct file_mapping_s *next;
  struct file_mapping_s *prev;
} file_mapping;
static file_mapping *fm = 0;



#define	PAGE_NOACCESS		0x01
#define	PAGE_READONLY		0x02
#define	PAGE_READWRITE		0x04
#define	PAGE_WRITECOPY		0x08
#define	PAGE_EXECUTE		0x10
#define	PAGE_EXECUTE_READ	0x20
#define	PAGE_EXECUTE_READWRITE	0x40
#define	PAGE_EXECUTE_WRITECOPY	0x80
#define	PAGE_GUARD		0x100
#define	PAGE_NOCACHE		0x200

HANDLE WINAPI
CreateFileMappingA (HANDLE handle, LPSECURITY_ATTRIBUTES lpAttr,
    DWORD flProtect, DWORD dwMaxHigh, DWORD dwMaxLow, LPCSTR name)
{
  int hFile = (int) handle;
  unsigned int len = dwMaxLow;
  LPVOID answer;
  int anon = 0;
  int mmap_access = 0;
  if (hFile < 0) {
    anon = 1;
    hFile = open ("/dev/zero", O_RDWR);
    if (hFile < 0) {
      perror
          ("Cannot open /dev/zero for READ+WRITE. Check permissions! error: ");
      return 0;
    }
  }
  if (!anon) {
    off_t ret = lseek (hFile, 0, SEEK_END);
    if (ret != -1)
      len = ret;
    lseek (hFile, 0, SEEK_SET);
  }

  if (flProtect & PAGE_READONLY)
    mmap_access |= PROT_READ;
  else
    mmap_access |= PROT_READ | PROT_WRITE;

  answer = mmap (NULL, len, mmap_access, MAP_PRIVATE, hFile, 0);
  if (anon)
    close (hFile);
  if (answer != (LPVOID) - 1) {
    if (fm == 0) {
      fm = malloc (sizeof (file_mapping));
      fm->prev = NULL;
    } else {
      fm->next = malloc (sizeof (file_mapping));
      fm->next->prev = fm;
      fm = fm->next;
    }
    fm->next = NULL;
    fm->handle = answer;
    if (name) {
      fm->name = malloc (strlen (name) + 1);
      strcpy (fm->name, name);
    } else
      fm->name = NULL;
    fm->mapping_size = len;

    if (anon)
      close (hFile);
    return (HANDLE) answer;
  }
  return (HANDLE) 0;
}

WIN_BOOL WINAPI
UnmapViewOfFile (LPVOID handle)
{
  file_mapping *p;
  int result;
  if (fm == 0)
    return 0;
  for (p = fm; p; p = p->next) {
    if (p->handle == handle) {
      result = munmap ((void *) handle, p->mapping_size);
      if (p->next)
        p->next->prev = p->prev;
      if (p->prev)
        p->prev->next = p->next;
      if (p->name)
        free (p->name);
      if (p == fm)
        fm = p->prev;
      free (p);
      return result;
    }
  }
  return 0;
}

//static int va_size=0;
struct virt_alloc_s;
typedef struct virt_alloc_s
{
  int mapping_size;
  char *address;
  struct virt_alloc_s *next;
  struct virt_alloc_s *prev;
  int state;
} virt_alloc;
static virt_alloc *vm = 0;
#define MEM_COMMIT              0x00001000
#define MEM_RESERVE             0x00002000

LPVOID WINAPI
VirtualAlloc (LPVOID address, DWORD size, DWORD type, DWORD protection)
{
  void *answer;
  int fd;
  long pgsz;

  if ((type & (MEM_RESERVE | MEM_COMMIT)) == 0)
    return NULL;

  fd = open ("/dev/zero", O_RDWR);
  if (fd < 0) {
    perror ("Cannot open /dev/zero for READ+WRITE. Check permissions! error: ");
    return NULL;
  }

  if (type & MEM_RESERVE && (void *) ((unsigned long) address & 0xffff)) {
    size += (unsigned long) address & 0xffff;
    address = (LPVOID) ((unsigned long) address & ~0xffff);
  }
  pgsz = sysconf (_SC_PAGESIZE);
  if (type & MEM_COMMIT && (void *) ((unsigned long) address % pgsz)) {
    size += (unsigned long) address % pgsz;
    address -= (unsigned long) address % pgsz;
  }

  if (type & MEM_RESERVE && size < 0x10000)
    size = 0x10000;
  if (size % pgsz)
    size += pgsz - size % pgsz;

  if (address != 0) {
    //check whether we can allow to allocate this
    virt_alloc *str = vm;
    while (str) {
      if ((void *) ((unsigned long) address) >=
          (void *) ((unsigned long) str->address + str->mapping_size)) {
        str = str->prev;
        continue;
      }
      if ((void *) ((unsigned long) address + size) <=
          (void *) ((unsigned long) str->address)) {
        str = str->prev;
        continue;
      }
      if (str->state == 0) {
//#warning FIXME
        if ((address >= (void *) (unsigned long) str->address)
            && ((void *) ((unsigned long) address + size) <=
                (void *) ((unsigned long) str->address + str->mapping_size))
            && (type & MEM_COMMIT)) {
          close (fd);
          return address;       //returning previously reserved memory
        }
      }
      close (fd);
      return NULL;
    }
  }

  answer = mmap (address, size, PROT_READ | PROT_WRITE | PROT_EXEC,
      MAP_PRIVATE, fd, 0);
//    answer=FILE_dommap(-1, address, 0, size, 0, 0,
//      PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE);
  close (fd);
  if (answer != (void *) -1 && address && answer != address) {
    /* It is dangerous to try mmap() with MAP_FIXED since it does not
       always detect conflicts or non-allocation and chaos ensues after
       a successful call but an overlapping or non-allocated region.  */
    munmap (answer, size);
    answer = (void *) -1;
    errno = EINVAL;
  }
  if (answer == (void *) -1) {
    return NULL;
  } else {
    virt_alloc *new_vm = malloc (sizeof (virt_alloc));
    new_vm->mapping_size = size;
    new_vm->address = answer;
    new_vm->prev = vm;
    if (type == MEM_RESERVE)
      new_vm->state = 0;
    else
      new_vm->state = 1;
    if (vm)
      vm->next = new_vm;
    vm = new_vm;
    vm->next = 0;
    //if(va_size!=0)
    //    printf("Multiple VirtualAlloc!\n");
    //printf("answer=0x%08x\n", answer);
    return answer;
  }
}

WIN_BOOL WINAPI
VirtualFree (LPVOID address, SIZE_T dwSize, DWORD dwFreeType)   //not sure
{
  virt_alloc *str = vm;

  while (str) {
    if (address != str->address) {
      str = str->prev;
      continue;
    }
    //printf("VirtualFree(0x%08X, %d - %d)\n", str->address, dwSize, str->mapping_size);
    munmap (str->address, str->mapping_size);
    if (str->next)
      str->next->prev = str->prev;
    if (str->prev)
      str->prev->next = str->next;
    if (vm == str)
      vm = str->prev;
    free (str);
    return 0;
  }
  return -1;
}

INT WINAPI
WideCharToMultiByte (UINT codepage, DWORD flags, LPCWSTR src,
    INT srclen, LPSTR dest, INT destlen, LPCSTR defch, WIN_BOOL * used_defch)
{
  int i;
  if (src == 0)
    return 0;
  if ((srclen == -1) && (dest == 0))
    return 0;
  if (srclen == -1) {
    srclen = 0;
    while (src[srclen++]);
  }
//    for(i=0; i<srclen; i++)
//      printf("%c", src[i]);
//    printf("\n");
  if (dest == 0) {
    for (i = 0; i < srclen; i++) {
      src++;
      if (*src == 0)
        return i + 1;
    }
    return srclen + 1;
  }
  if (used_defch)
    *used_defch = 0;
  for (i = 0; i < __min (srclen, destlen); i++) {
    *dest = (char) *src;
    dest++;
    src++;
    if (*src == 0)
      return i + 1;
  }
  return __min (srclen, destlen);
}

INT WINAPI
MultiByteToWideChar (UINT codepage, DWORD flags, LPCSTR src, INT srclen,
    LPWSTR dest, INT destlen)
{
  return 0;
}

HANDLE WINAPI
OpenFileMappingA (DWORD access, WIN_BOOL prot, LPCSTR name)
{
  file_mapping *p;
  if (fm == 0)
    return (HANDLE) 0;
  if (name == 0)
    return (HANDLE) 0;
  for (p = fm; p; p = p->prev) {
    if (p->name == 0)
      continue;
    if (strcmp (p->name, name) == 0)
      return (HANDLE) p->handle;
  }
  return 0;
}
