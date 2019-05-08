#include <stdint.h>

struct dma;

struct memfs *memfs_mount(volatile uint32_t *base, struct dma *dmac);
void memfs_unmount(struct memfs *fs);

// addr: will be set to the load addr found in the image
int memfs_load(struct memfs *fs, const char *fname, uint32_t **addr);
