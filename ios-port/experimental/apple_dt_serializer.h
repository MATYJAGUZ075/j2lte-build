#ifndef J2_APPLE_DT_SERIALIZER_H
#define J2_APPLE_DT_SERIALIZER_H

#include <stdint.h>
#include <stddef.h>

#define J2_DT_PROP_NAME_LEN 32
#define J2_DT_PROP_HEADER_SIZE (J2_DT_PROP_NAME_LEN + sizeof(uint32_t))

typedef struct {
    char name[J2_DT_PROP_NAME_LEN];
    uint32_t length;
    const uint8_t *value;
} j2_dt_prop_t;

typedef struct {
    uint32_t nProperties;
    uint32_t nChildren;
} j2_dt_node_t;

typedef struct j2_dt_tree_node {
    j2_dt_prop_t *props;
    size_t nprops;
    struct j2_dt_tree_node **children;
    size_t nchildren;
    char name[64];
} j2_dt_tree_node_t;

size_t j2_dt_align4(size_t v);
size_t j2_dt_prop_size(size_t value_len);
size_t j2_dt_node_header_size(void);
size_t j2_dt_tree_size(const j2_dt_tree_node_t *node);
int j2_dt_serialize_tree(uint8_t *buf, size_t bufsize, const j2_dt_tree_node_t *node, size_t *out_size);

int j2_dt_parse_header(const uint8_t *buf, size_t bufsize, j2_dt_node_t *out);
int j2_dt_prop_at(const uint8_t *buf, size_t bufsize, size_t prop_index, j2_dt_prop_t **out_prop, const uint8_t **out_val);
int j2_dt_validate_tree(const uint8_t *buf, size_t bufsize, int max_depth);
size_t j2_dt_node_size(const uint8_t *buf, size_t bufsize);
size_t j2_dt_subtree_size(const uint8_t *buf, size_t bufsize);
int j2_dt_parse_tree(const uint8_t *buf, size_t bufsize, j2_dt_tree_node_t *out, int max_depth);
int j2_dt_tree_from_buffer(const uint8_t *buf, size_t bufsize, j2_dt_tree_node_t **out, int max_depth);
void j2_dt_free_tree(j2_dt_tree_node_t *node);
int j2_dt_trees_equal(const j2_dt_tree_node_t *a, const j2_dt_tree_node_t *b);

#endif