#ifndef LDT_KEEPER_H
#define LDT_KEEPER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* manage i386 per-process segment descriptors
 * LDT = Local Descriptor Table
 */
typedef struct {
  void* fs_seg;
  char* prev_struct;
  int fd;
  unsigned int teb_sel;
} ldt_fs_t;

void Setup_FS_Segment(ldt_fs_t *ldt_fs);
ldt_fs_t* Setup_LDT_Keeper(void);
void Check_FS_Segment(ldt_fs_t *ldt_fs);
void Restore_LDT_Keeper(ldt_fs_t* ldt_fs);
#ifdef __cplusplus
}
#endif

#endif /* LDT_KEEPER_H */
