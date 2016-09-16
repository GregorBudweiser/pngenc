#include "node.h"
#include "pngenc.h"
#include "utils.h"
#include <assert.h>

int node_write(pngenc_node * node, const uint8_t * data, uint64_t size) {
    uint64_t total_bytes_written = 0;
    while(total_bytes_written < size) {
        // Write MAX_INT at max
        int64_t bytes_written = node->write(node, data + total_bytes_written,
                                            (uint32_t)min_u64(size - total_bytes_written,
                                                              0xFFFFFFFFULL));
        if(bytes_written < 0)
            return (int)bytes_written;
        total_bytes_written += (uint64_t)bytes_written;
    }
    return PNGENC_SUCCESS;
}

int node_init(pngenc_node * node) {
    assert(node);

    // chain init first
    if(node->next)
        RETURN_ON_ERROR(node_init(node->next));

    if(node->init)
        RETURN_ON_ERROR(node->init(node));

    return PNGENC_SUCCESS;
}

int node_finish(pngenc_node * node) {
    assert(node);

    if(node->finish)
        RETURN_ON_ERROR(node->finish(node));

    if(node->next)
        RETURN_ON_ERROR(node_finish(node->next));

    return PNGENC_SUCCESS;
}
