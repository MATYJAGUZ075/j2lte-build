#include "apple_dt_serializer.h"
#include <string.h>

size_t j2_dt_align4(size_t v) { return (v + 3) & ~((size_t)3); }

size_t j2_dt_prop_size(size_t value_len) {
    return sizeof(j2_dt_prop_t) + j2_dt_align4(value_len);
}

size_t j2_dt_node_header_size(void) { return sizeof(j2_dt_node_t); }

size_t j2_dt_tree_size(const j2_dt_tree_node_t *node) {
    if (!node) return 0;
    size_t props_size = 0;
    for (size_t i = 0; i < node->nprops; i++)
        props_size += j2_dt_prop_size(node->props[i].length);
    size_t children_size = 0;
    for (size_t i = 0; i < node->nchildren; i++)
        children_size += j2_dt_tree_size(node->children[i]);
    return sizeof(j2_dt_node_t) + props_size + children_size;
}

int j2_dt_serialize_tree(uint8_t *buf, size_t bufsize, const j2_dt_tree_node_t *node, size_t *out_size) {
    if (!node) return 0;
    size_t total = j2_dt_tree_size(node);
    if (out_size) *out_size = total;
    if (!buf) return 1;
    if (bufsize < total) return 0;
    /* header */
    j2_dt_node_t *hdr = (j2_dt_node_t*)buf;
    hdr->nProperties = (uint32_t)node->nprops;
    hdr->nChildren = (uint32_t)node->nchildren;
    size_t off = sizeof(j2_dt_node_t);
    /* props (values zeroed; caller fills externally if needed) */
    for (size_t i = 0; i < node->nprops; i++) {
        memcpy(buf + off, &node->props[i], sizeof(j2_dt_prop_t) - sizeof(const uint8_t*));
        off += sizeof(j2_dt_prop_t) - sizeof(const uint8_t*);
        if (node->props[i].value && node->props[i].length > 0)
            memcpy(buf + off, node->props[i].value, node->props[i].length);
        off += j2_dt_align4(node->props[i].length);
    }
    /* children contiguous */
    for (size_t i = 0; i < node->nchildren; i++) {
        size_t child_size = 0;
        if (!j2_dt_serialize_tree(buf + off, bufsize - off, node->children[i], &child_size)) return 0;
        off += child_size;
    }
    return 1;
}

int j2_dt_parse_header(const uint8_t *buf, size_t bufsize, j2_dt_node_t *out) {
    if (!buf || bufsize < sizeof(j2_dt_node_t) || !out) return 0;
    memcpy(out, buf, sizeof(j2_dt_node_t));
    if (out->nProperties > 10000 || out->nChildren > 10000) return 0;
    return 1;
}

int j2_dt_prop_at(const uint8_t *buf, size_t bufsize, size_t prop_index, j2_dt_prop_t **out_prop, const uint8_t **out_val) {
    if (!buf || !out_prop || !out_val) return 0;
    j2_dt_node_t node;
    if (!j2_dt_parse_header(buf, bufsize, &node)) return 0;
    if (prop_index >= node.nProperties) return 0;
    size_t off = sizeof(j2_dt_node_t);
    for (size_t i = 0; i < prop_index; i++) {
        if (off + sizeof(j2_dt_prop_t) > bufsize) return 0;
        j2_dt_prop_t *p = (j2_dt_prop_t*)(buf + off);
        off += sizeof(j2_dt_prop_t);
        size_t valsize = j2_dt_align4(p->length);
        if (off + valsize > bufsize) return 0;
        off += valsize;
    }
    if (off + sizeof(j2_dt_prop_t) > bufsize) return 0;
    j2_dt_prop_t *p = (j2_dt_prop_t*)(buf + off);
    if (p->length > bufsize - off - sizeof(j2_dt_prop_t)) return 0;
    *out_prop = p;
    *out_val = buf + off + sizeof(j2_dt_prop_t);
    return 1;
}
int j2_dt_validate_tree(const uint8_t *buf, size_t bufsize, int max_depth) {
    if (!buf || bufsize < sizeof(j2_dt_node_t) || max_depth <= 0) return 0;
    j2_dt_node_t node;
    if (!j2_dt_parse_header(buf, bufsize, &node)) return 0;
    size_t off = sizeof(j2_dt_node_t);
    for (size_t i = 0; i < node.nProperties; i++) {
        if (off + sizeof(j2_dt_prop_t) > bufsize) return 0;
        j2_dt_prop_t *p = (j2_dt_prop_t*)(buf + off);
        int term = 0;
        for (int k = 0; k < J2_DT_PROP_NAME_LEN; k++) if (p->name[k] == 0) { term = 1; break; }
        if (!term) return 0;
        if (p->length > bufsize - off - sizeof(j2_dt_prop_t)) return 0;
        off += sizeof(j2_dt_prop_t);
        off += j2_dt_align4(p->length);
        if (off > bufsize) return 0;
    }
    return 1;
}


size_t j2_dt_node_size(const uint8_t *buf, size_t bufsize) {
    if (!buf || bufsize < sizeof(j2_dt_node_t)) return 0;
    j2_dt_node_t node;
    if (!j2_dt_parse_header(buf, bufsize, &node)) return 0;
    size_t off = sizeof(j2_dt_node_t);
    for (size_t i = 0; i < node.nProperties; i++) {
        if (off + sizeof(j2_dt_prop_t) > bufsize) return 0;
        j2_dt_prop_t *p = (j2_dt_prop_t*)(buf + off);
        off += sizeof(j2_dt_prop_t);
        off += j2_dt_align4(p->length);
        if (off > bufsize) return 0;
    }
    /* children: size not computed here (would need recursion); return props+header only */
    return off;
}

size_t j2_dt_subtree_size(const uint8_t *buf, size_t bufsize) {
    if (!buf || bufsize < sizeof(j2_dt_node_t)) return 0;
    j2_dt_node_t node;
    if (!j2_dt_parse_header(buf, bufsize, &node)) return 0;
    size_t off = sizeof(j2_dt_node_t);
    for (size_t i = 0; i < node.nProperties; i++) {
        if (off + sizeof(j2_dt_prop_t) > bufsize) return 0;
        j2_dt_prop_t *p = (j2_dt_prop_t*)(buf + off);
        off += sizeof(j2_dt_prop_t);
        off += j2_dt_align4(p->length);
        if (off > bufsize) return 0;
    }
    /* children contiguous */
    for (size_t i = 0; i < node.nChildren; i++) {
        if (off >= bufsize) return 0;
        size_t child_size = j2_dt_subtree_size(buf + off, bufsize - off);
        if (child_size == 0) return 0;
        off += child_size;
        if (off > bufsize) return 0;
    }
    return off;
}

int j2_dt_parse_tree(const uint8_t *buf, size_t bufsize, j2_dt_tree_node_t *out, int max_depth) {
    if (!buf || !out || bufsize < sizeof(j2_dt_node_t) || max_depth <= 0) return 0;
    j2_dt_node_t node;
    if (!j2_dt_parse_header(buf, bufsize, &node)) return 0;
    out->nprops = node.nProperties;
    out->nchildren = node.nChildren;
    /* props */
    size_t off = sizeof(j2_dt_node_t);
    if (out->nprops > 0) {
        out->props = (j2_dt_prop_t*)(buf + off); /* points into buffer; caller must not free */
    } else {
        out->props = NULL;
    }
    for (size_t i = 0; i < out->nprops; i++) {
        if (off + sizeof(j2_dt_prop_t) > bufsize) return 0;
        j2_dt_prop_t *p = (j2_dt_prop_t*)(buf + off);
        /* name terminated */
        int term = 0;
        for (int k = 0; k < J2_DT_PROP_NAME_LEN; k++) if (p->name[k] == 0) { term = 1; break; }
        if (!term) return 0;
        off += sizeof(j2_dt_prop_t);
        off += j2_dt_align4(p->length);
        if (off > bufsize) return 0;
    }
    /* children contiguous */
    if (out->nchildren > 0) {
        /* allocate array of pointers (caller-owned; for host tests use static storage) */
        out->children = NULL; /* parsing into caller-provided storage not supported here */
    }
    /* validate children recursively */
    for (size_t i = 0; i < out->nchildren; i++) {
        if (off >= bufsize) return 0;
        size_t child_size = j2_dt_subtree_size(buf + off, bufsize - off);
        if (child_size == 0) return 0;
        /* recursively validate */
        if (!j2_dt_validate_tree(buf + off, bufsize - off, max_depth - 1)) return 0;
        off += child_size;
        if (off > bufsize) return 0;
    }
    return 1;
}

#include <stdlib.h>

int j2_dt_tree_from_buffer(const uint8_t *buf, size_t bufsize, j2_dt_tree_node_t **out, int max_depth) {
    if (!buf || !out || bufsize < sizeof(j2_dt_node_t) || max_depth <= 0) return 0;
    j2_dt_node_t node;
    if (!j2_dt_parse_header(buf, bufsize, &node)) return 0;
    j2_dt_tree_node_t *n = calloc(1, sizeof(*n));
    if (!n) return 0;
    n->nprops = node.nProperties;
    n->nchildren = node.nChildren;
    if (n->nprops > 0) {
        n->props = calloc(n->nprops, sizeof(j2_dt_prop_t));
        if (!n->props) { free(n); return 0; }
    }
    /* copy props */
    size_t off = sizeof(j2_dt_node_t);
    for (size_t i = 0; i < n->nprops; i++) {
        j2_dt_prop_t *p = NULL; const uint8_t *val = NULL;
        if (!j2_dt_prop_at(buf, bufsize, i, &p, &val)) { j2_dt_free_tree(n); return 0; }
        memcpy(&n->props[i], p, sizeof(j2_dt_prop_t) - sizeof(const uint8_t*));
        n->props[i].value = NULL;
        if (p->length > 0) {
            n->props[i].value = malloc(p->length);
            if (!n->props[i].value) { j2_dt_free_tree(n); return 0; }
            const uint8_t *val = NULL;
            j2_dt_prop_t *pp = NULL;
            if (!j2_dt_prop_at(buf, bufsize, i, &pp, &val)) { j2_dt_free_tree(n); return 0; }
            memcpy((void*)n->props[i].value, val, p->length);
        }
        if (strcmp(p->name, "name") == 0 && p->length < 64 && val) {
            memcpy(n->name, val, p->length);
            n->name[p->length] = 0;
        }
        off += sizeof(j2_dt_prop_t) + j2_dt_align4(p->length);
    }
    /* children */
    if (n->nchildren > 0) {
        n->children = calloc(n->nchildren, sizeof(j2_dt_tree_node_t*));
        if (!n->children) { j2_dt_free_tree(n); return 0; }
    }
    size_t child_off = off;
    for (size_t i = 0; i < n->nchildren; i++) {
        if (child_off >= bufsize) { j2_dt_free_tree(n); return 0; }
        size_t child_size = j2_dt_subtree_size(buf + child_off, bufsize - child_off);
        if (child_size == 0) { j2_dt_free_tree(n); return 0; }
        if (!j2_dt_validate_tree(buf + child_off, bufsize - child_off, max_depth - 1)) { j2_dt_free_tree(n); return 0; }
        j2_dt_tree_node_t *child = NULL;
        if (!j2_dt_tree_from_buffer(buf + child_off, child_size, &child, max_depth - 1)) { j2_dt_free_tree(n); return 0; }
        n->children[i] = child;
        child_off += child_size;
        if (child_off > bufsize) { j2_dt_free_tree(n); return 0; }
    }
    *out = n;
    return 1;
}

int j2_dt_trees_equal(const j2_dt_tree_node_t *a, const j2_dt_tree_node_t *b) {
    if (a == b) return 1;
    if (!a || !b) return 0;
    if (a->nprops != b->nprops) return 0;
    if (a->nchildren != b->nchildren) return 0;
    for (size_t i = 0; i < a->nprops; i++) {
        if (strcmp(a->props[i].name, b->props[i].name) != 0) return 0;
        if (a->props[i].length != b->props[i].length) return 0;
        if (a->props[i].length > 0 && (a->props[i].value == NULL || b->props[i].value == NULL)) return 0;
        if (a->props[i].length > 0 && memcmp(a->props[i].value, b->props[i].value, a->props[i].length) != 0) return 0;
    }
    for (size_t i = 0; i < a->nchildren; i++) {
        if (!j2_dt_trees_equal(a->children[i], b->children[i])) return 0;
    }
    return 1;
}
void j2_dt_free_tree(j2_dt_tree_node_t *node) {
    if (!node) return;
    if (node->props) {
        for (size_t i = 0; i < node->nprops; i++) free((void*)node->props[i].value);
        free(node->props);
    }
    if (node->children) {
        for (size_t i = 0; i < node->nchildren; i++) j2_dt_free_tree(node->children[i]);
        free(node->children);
    }
    free(node);
}
