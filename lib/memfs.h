#include <stdint.h>

struct dma;

struct memfs *memfs_mount(uintptr_t base, struct dma *dmac);
void memfs_unmount(struct memfs *fs);

/* memfs_load: load a blob from file system into memory
 *
 * @addr: if not null, will be set to the load addr found in the image
 * @ep: if not null, will be set to address of entry point found in the image
*/
int memfs_load(struct memfs *fs, const char *fname,
               uint32_t **addr, uint32_t **ep);
