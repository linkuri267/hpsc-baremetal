#include <stdint.h>

struct dma;

struct sfs *sfs_mount(uint8_t *base, struct dma *dmac);
void sfs_unmount(struct sfs *fs);

/* sfs_load: load a blob from file system into memory
 *
 * @addr: if not null, will be set to the load addr found in the image
 * @ep: if not null, will be set to address of entry point found in the image
*/
int sfs_load(struct sfs *fs, const char *fname,
               uint32_t **addr, uint32_t **ep);
