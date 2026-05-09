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

static const char *skip_slash(const char *p)
{
    while (*p == '/')
        p++;
    return p;
}

//-----------------------a=>what exist ---b=>what asked for

static int match_seg(const char *name, const char *start, uint32_t len)
{
    uint32_t i = 0;
    while (name[i] && i < len)
    {
        if (name[i] != start[i])
            return 0;
        i++;
    }
    return (name[i] == '\0' && i == len);
}

vfs_node_t *vfs_lookup(vfs_node_t *root, const char *path)
{

    if (!root || !path)
        return 0;

    const char *p = path;
    vfs_node_t *current = root;

    while (*p)
    {
        while (*p == '/')
            p++;

        if (!*p)
            break;

        const char *start = p;
        uint32_t len = 0;

        while (*p && *p != '/')
        {
            p++;
            len++;
        }

        int found = 0;

        for (uint32_t j = 0; j < current->child_count; j++)
        {
            vfs_node_t *child = current->children[j];

            if (child && match_seg(child->name, start, len))
            {
                current = child;
                found = 1;
                break;
            }
        }
        if (!found)
            return 0;
    }

    return current;
}