#pragma once
#include <stdint.h>

struct _pngenc_node;

/**
 * To be implemented by a concrete node implementation.
 *
 * This function provides data for the node to process. The node DOES NOT NEED
 * to consume all data at once!
 * The implementation is responsible for forwarding the write signal to the
 * next node once enough data has been processed.
 *
 * @return The number bytes consumed. A negative value indicates an error.
 */
typedef int64_t (*write_func)(struct _pngenc_node * node, const uint8_t * data,
                              uint32_t size);

/**
 * To be implemented by a concrete node implementation.
 *
 * This function signals the node to process any remaining data. DOES NOT NEED
 * to forward the signal!
 *
 * @return The number bytes consumed. A negative value indicates an error.
 */
typedef int64_t (*finish_func)(struct _pngenc_node * node);

/**
 * To be implemented by a concrete node implementation.
 *
 * This function signals the node reset into the starting state. DOES NOT NEED
 * to forward the signal!
 *
 * @return PNGENC_SUCCESS if data has been written or an according error status
 *         if not.
 */
typedef int64_t (*init_func)(struct _pngenc_node * node);

/**
 * A node in the png processing pipeline.
 * This is pretty much an interface with some useful helper functions defined
 * on it.
 */
typedef struct _pngenc_node {
    struct _pngenc_node * next;
    init_func init;
    write_func write;
    finish_func finish;
    uint64_t buf_size;
    uint64_t buf_pos;
    void * buf;
} pngenc_node;

/**
 * Called before the first node_write() is issued. Also forwards the
 * init signal to the next node in the pipeline.
 * @return PNGENC_SUCCESS if data has been written or an according error status
 *         if not.
 */
int node_init(pngenc_node * node);

/**
 * Consumes the data FULLY or returns an error on failure.
 * NOTE: This function wraps the write_func() of the node's implementation.
 *       This makes it easier for the caller.
 * @return PNGENC_SUCCESS if data has been written or an according error status
 *         if not.
 */
int node_write(pngenc_node * node, const uint8_t * data, uint64_t size);

/**
 * Signals the node to process and forward any remaining data. After this call
 * no more node_write() calls will be received. Also forwards the finish
 * signal to the next node in the pipeline.
 * @return PNGENC_SUCCESS if data has been written or an according error status
 *         if not.
 */
int node_finish(pngenc_node * node);
