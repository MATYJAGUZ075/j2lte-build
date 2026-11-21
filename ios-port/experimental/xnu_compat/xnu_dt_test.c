#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "xnu_compat/shims.h"
#include "apple_dt_serializer.h"

/* XNU original header path provided by CI include */
#include <pexpert/device_tree.h>

static int failures = 0;
#define CHECK(cond,msg) do { if(!(cond)){printf("FAIL: %s\n",msg);failures++;} else {printf("PASS: %s\n",msg);} } while(0)

int main(void) {
    /* Build tree */
    j2_dt_tree_node_t root; memset(&root,0,sizeof(root));
    strcpy(root.name,"root");
    j2_dt_prop_t rp[1]; memset(rp,0,sizeof(rp));
    strcpy(rp[0].name,"name"); rp[0].length=4; rp[0].value=(const uint8_t*)"root";
    root.props=rp; root.nprops=1;

    j2_dt_tree_node_t child; memset(&child,0,sizeof(child));
    strcpy(child.name,"child");
    j2_dt_prop_t cp[1]; memset(cp,0,sizeof(cp));
    strcpy(cp[0].name,"name"); cp[0].length=5; cp[0].value=(const uint8_t*)"child";
    child.props=cp; child.nprops=1;
    j2_dt_tree_node_t *ch[1]={&child};
    root.children=ch; root.nchildren=1;

    size_t sz=0;
    j2_dt_serialize_tree(NULL,0,&root,&sz);
    uint8_t *buf=malloc(sz);
    size_t outsz=0;
    j2_dt_serialize_tree(buf,sz,&root,&outsz);

    DTInit(buf);
    DTEntry found=NULL;
    CHECK(DTLookupEntry(NULL, "/child", &found)==kSuccess, "DTLookupEntry child");
    if (found) {
        void *val=NULL; unsigned int vsize=0;
        CHECK(DTGetProperty(found, "name", &val, &vsize)==kSuccess, "DTGetProperty name");
        CHECK(vsize==5, "prop size");
        CHECK(val && memcmp(val,"child",5)==0, "prop value");
        /* nonexistent */
        CHECK(DTGetProperty(found, "nope", &val, &vsize)!=kSuccess, "DTGetProperty missing");
    }
    DTEntry rootE=NULL;
    CHECK(DTLookupEntry(NULL, "/", &rootE)==kSuccess, "DTLookupEntry root");

    free(buf);
    printf("%d FAILURES\n", failures);
    return failures?1:0;
}
