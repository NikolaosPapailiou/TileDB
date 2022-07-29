/**
 * @file tiledb/sm/c_api/experimental/tiledb_dimension_label.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2022 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * Experimental C-API for dimension labels.
 */

#ifndef TILEDB_DIMENSION_LABEL_EXPERIMENTAL_H
#define TILEDB_DIMENSION_LABEL_EXPERIMENTAL_H

#include "tiledb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
#define TILEDB_LABEL_ORDER_ENUM(id) TILEDB_##id
#include "tiledb_enum_experimental.h"
#undef TILEDB_LABEL_ORDER_ENUM
} tiledb_label_order_t;

typedef struct tiledb_dimension_label_schema_t tiledb_dimension_label_schema_t;

/**
 * Adds a dimension label to a array schema.
 *
 * @code{.c}
 * int64_t dim_domain[] = {1, 10};
 * int64_t tile_extent = 5;
 * double label_domain [] = {-10.0, 10.0};
 * double label_tile_extent = 4.0;
 * tiledb_dimension_label_schema_t* dim_label;
 * tiledb_dimension_label_schema_alloc(
 *     ctx,
 *     TILEDB_INCREASING_LABELS,
 *     TILEDB_INT64,
 *     dim_domain,
 *     &tile_extent,
 *     TILEDB_FLOAT64,
 *     label_domain,
 *     &label_tile_extent,
 *     &dim_label);
 * tiledb_array_schema_t* label_array_schema;
 * tiledb_array_schema_add_dimension_label(
 *     ctx,
 *     array_schema,
 *     0,
 *     dim_label_schema);
 * @endcode
 *
 */
TILEDB_EXPORT int32_t tiledb_array_schema_add_dimension_label(
    tiledb_ctx_t* ctx,
    tiledb_array_schema_t* array_schema,
    uint32_t dim_id,
    const char* name,
    tiledb_dimension_label_schema_t* dim_label_schema) TILEDB_NOEXCEPT;

/**
 * Checks whether the array schema has a dimension label of the given name.
 *
 * **Example:**
 *
 * @code{.c}
 * int32_t has_dim_label;
 * tiledb_array_schema_has_dimension_label(
 *     ctx, array_schema, "label_0", &has_dim_label);
 * @endcode
 *
 * @param ctx The TileDB context.
 * @param array_schema The array schema.
 * @param name The name of the dimension label to check for.
 * @param has_dim_label Set to `1` if the array schema has an attribute of the
 *      given name, else `0`.
 * @return `TILEDB_OK` for success and `TILEDB_ERR` for error.
 */
TILEDB_EXPORT int32_t tiledb_array_schema_has_dimension_label(
    tiledb_ctx_t* ctx,
    const tiledb_array_schema_t* array_schema,
    const char* name,
    int32_t* has_dim_label) TILEDB_NOEXCEPT;

/**
 * Creates a TileDB dimension label object.
 *
 * **Example:**
 *
 * @code{.c}
 * int64_t dim_domain[] = {1, 10};
 * int64_t tile_extent = 5;
 * double label_domain [] = {-10.0, 10.0};
 * double label_tile_extent = 4.0;
 * tiledb_dimension_label_schema_t* dim_label;
 * tiledb_dimension_label_schema_alloc(
 *     ctx,
 *     TILEDB_INCREASING_LABELS,
 *     TILEDB_INT64,
 *     dim_domain,
 *     &tile_extent,
 *     TILEDB_FLOAT64,
 *     label_domain,
 *     &label_tile_extent,
 *     &dim_label);
 * @endcode
 *
 * @param ctx The TileDB context.
 * @param label_type The label type.
 * @param index_type The datatype for the original dimension data. Must be the
 *     same as the dimension the dimension label is applied to.
 * @param index_domain The range the original dimension is defined on. Must be
 *     the same as the dimension the dimension label is applied to.
 * @param index_tile_extent The tile extent for the original dimension data on
 *     the dimension label.
 * @param label_type The datatype for the new label dimension data.
 * @param label_dim_domain The range the label data is defined on domain.
 * @param label_tile_extent The tile extent for the label data.
 * @return `TILEDB_OK` for success and `TILEDB_ERR` for error.
 */
TILEDB_EXPORT int32_t tiledb_dimension_label_schema_alloc(
    tiledb_ctx_t* ctx,
    tiledb_label_order_t label_order,
    tiledb_datatype_t index_type,
    const void* index_domain,
    const void* index_tile_extent,
    tiledb_datatype_t label_type,
    const void* label_domain,
    const void* label_tile_extent,
    tiledb_dimension_label_schema_t** dim_label_schema) TILEDB_NOEXCEPT;

/**
 * Destroys a TileDB dimension label schema, freeing associated memory.
 *
 * @code{.c}
 * tiledb_dimension_t* dim;
 * int64_t dim_domain[] = {1, 10};
 * int64_t tile_extent = 5;
 * tiledb_dimension_alloc(
 *     ctx, "dim_0", TILEDB_INT64, dim_domain, &tile_extent, &dim);
 * double label_domain [] = {-10.0, 10.0};
 * double label_tile_extent = 4.0;
 * tiledb_dimension_label_schema_t* dim_label_schema;
 * tiledb_dimension_label_schema_alloc(
 *     ctx,
 *     TILEDB_INCREASING_LABELS,
 *     dim,
 *     TILEDB_FLOAT64,
 *     label_domain,
 *     &label_tile_extent,
 *     &dim_label_schema);
 * tiledb_dimension_label_schema_free(&dim_label_schema)
 * @endcode
 *
 * @param dim_label_schema The dimension label schema to be destroyed.
 */
TILEDB_EXPORT void tiledb_dimension_label_schema_free(
    tiledb_dimension_label_schema_t** dim_label_schema) TILEDB_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // TILEDB_DIMENSION_LABEL_EXPERIMENTAL_H