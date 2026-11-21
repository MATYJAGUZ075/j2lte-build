#include "apple_dt_serializer.h"

size_t j2_dt_align4(size_t v) {
    return (v + 3) & ~((size_t)3);
}

size_t j2_dt_node_size(const j2_dt_node_header_t *node, size_t props_size, size_t children_size) {
    (void)node;
    return sizeof(j2_dt_node_header_t) + j2_dt_align4(props_size) + children_size;
}
