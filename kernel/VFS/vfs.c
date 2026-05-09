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

static int name_match(const char *a, const char *b)
{
    int i = 0;
    while (a[i] && b[i] && a[i] == b[i])
        i++;

    return (a[i] == '\0' && b[i] == '\0');
}

vfs_node_t *vfs_lookup(vfs_node_t *root, const char *path)
{

    if (!root || !path)
        return 0;

    const char *p = skip_slash(path);
    vfs_node_t *current = root;

    while (*p)
    {
        char part[128];
        int i = 0;

        while (*p && *p != '/' && i < 127)
            part[i++] = *p++;

        part[i] = '\0';
        p = skip_slash(p);

        int found = 0;

        for (uint32_t j = 0; j < current->child_count; j++)
        {
            vfs_node_t *child = current->children[i];

            if (child && name_match(child->name, part))
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