#include <stdint.h>
#include "C:\Users\PROLINE LAPTOP STORE\ForgeOS\include\vfs.h"

vfs_node_t *vfs_root = 0;

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node && node->ops && node->ops->read)
    {
        return node->ops->read(node, offset, size, buffer);
    }
    return 0;
}

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node && node->ops && node->ops->write)
    {
        return node->ops->write(node, offset, size, buffer);
    }
    return 0;
}
