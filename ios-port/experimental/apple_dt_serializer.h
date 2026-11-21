#ifndef J2_APPLE_DT_SERIALIZER_H
#define J2_APPLE_DT_SERIALIZER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t nProperties;
    uint32_t nChildren;
} j2_dt_node_header_t;

typedef struct {
    char name[32];
    uint32_t length;
} j2_dt_prop_header_t;

size_t j2_dt_align4(size_t v);
size_t j2_dt_node_size(const j2_dt_node_header_t *node, size_t props_size, size_t children_size);

#endif
