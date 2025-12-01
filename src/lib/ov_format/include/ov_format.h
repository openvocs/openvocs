/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. https://openvocs.org

        ------------------------------------------------------------------------
*//**

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

        A format is located on top of some data source or sink.
        A source or sink can be anything either providing or receiving data,
        e.g. a file, a socket or a memory block.
        The format determines the way the data is written to / read from the
        sink / source.

        See src/tools/ov_wav_to_pcm for a rather simple example on how to
        facilitate formats.

        Formats can be stacked, e.g.

        file
          |
        pcap
          |
        ethernet
          |
        ipv4
          |
        udp
          |
        rtp

        General procedure would be to create some 'bare' source like a file,
        and then stacking another format on top, like `pcap`, then stacking 
        another format on top like `ethernet` etc.

        Bare formats can be created using

        * ov_format_open() to create a format from a file
        * ov_format_from_memory to create a format from a chunk of memory
        * ov_format_buffered() to create a format from a chunk of memory, but buffered

        Stacking is done using `ov_format_as`:

        ```

        ov_format *file_fmt = ov_format_open("my.pcap", OV_READ);

        assert(0 != file_fmt);
        void *options = 0;

        ov_format *pcap = ov_format_as(file_fmt, 'pcap', options, 0);
        assert(0 != pcap);

        ...

        pcapc = ov_format_close(pcap);

        ```

        A format must be closed when not longer used using `ov_format_close()`. 

        Reading / Writing is done using

        `ov_format_payload_read_chunk / ov_format_payload_read_chunk_nocopy` and
        `ov_format_payload_write_chunk`

        ------------------------------------------------------------------------
*/
#ifndef OV_FORMAT_H
#define OV_FORMAT_H

#include <stdint.h>
#include <stdlib.h>

#include <ov_base/ov_buffer.h>

/*----------------------------------------------------------------------------*/

typedef enum {

  OV_INVALID = -1,
  OV_READ = 1,
  OV_WRITE = 2

} ov_format_mode;

/*----------------------------------------------------------------------------*/

#define OV_FORMAT_TYPE_MAX_STR_LEN ((size_t)20)

typedef struct ov_format_struct ov_format;

struct ov_format_struct {

  char const *type;
  ov_format_mode mode;
};

ov_format_mode ov_format_get_mode(ov_format *self);

/*----------------------------------------------------------------------------*/

/**
 * Handlers a format can provide.
 * If a handler is 0, it is not called / its functionality is not provided.
 * All formats appearing in here are the lower_layer format,
 * all data in here is the custom data attached to the actual format.
 */
typedef struct {

  /**
   * Get next chunk from format.
   * The buffer object returned is not a full-blown 'ov_buffer',
   * just used to return both the data pointer and it's length.
   * If it's capacity is greater than 0, the data pointer is freed.
   * If it's capacity is 0, the data pointer is not freed.
   */
  ov_buffer (*next_chunk)(ov_format *f, size_t requested_bytes, void *data);

  ssize_t (*write_chunk)(ov_format *f, ov_buffer const *chunk, void *data);

  ssize_t (*overwrite)(ov_format *f, size_t offset, ov_buffer const *chunk,
                       void *data);

  /**
   * Some formats are not streamable - like wav, which requires the entire
   * length written into its headers.
   * This function will be called whenever wants to receive the written
   * result, in order to be able to 'ready' it, in the case of wave, by
   * updating the length in the headers appropriately...
   * Called on ov_format_close or ov_format_get_memory e.g.
   */
  bool (*ready_format)(ov_format *f, void *data);

  void *(*create_data)(ov_format *f, void *options);

  /**
   * Just there to free potentially present custom data - don't do
   * anything else in here, for reading the written result, use 'ready_format'
   */
  void *(*free_data)(void *);

  /**
   * If a format 'fmt' is requested, for each format in the stack
   * it will be checked whether type equals 'fmt' OR this function
   * is present - then this function is called with 'fmt' and if it returns
   * a format pointer, this format pointer will be returned
   */
  ov_format *(*responsible_for)(ov_format const *f, char const *type_name);

} ov_format_handler;

/*****************************************************************************
                                 Memory Format
 ****************************************************************************/

/**
 * Create a format on top of a memory chunk.
 *
 * READ - mode
 *
 * Nothing special about that - returns data until the end of the supplied chunk
 * is reached.
 *
 * WRITE - mode
 *
 * 2 possible operation modes:
 *
 * * Fixed size memory: `memory` is not 0
 *   A Fixed size memory chunk is supplied which can be written to until there
 *   is no more space left.
 *
 *
 * * Auto-extend: `memory` is 0
 *   No memory chunk is supplied.
 *   The format will automatically manage an internal mem chunk that grows
 *   as data is written to it.
 *   `length` denotes the initial size of this memory chunk
 *
 * If memory == 0, the format will allocate / extend internal memory
 *
 * @param memory memory address to write to. If 0, turn on auto-extend
 * @param length of mem chunk in bytes. Must never be 0.
 * @return pointer to the new format or 0 in case of error
 */
ov_format *ov_format_from_memory(uint8_t *memory, size_t length,
                                 ov_format_mode mode);

/*----------------------------------------------------------------------------*/

/**
 * Returns the memory area the format operates upon.
 * Only for memory formats!
 *
 * The returned buffer content is freed once `fmt` is closed !
 * buffer.start will be 0 in case of error
 */
ov_buffer ov_format_get_memory(ov_format const *fmt);

/*----------------------------------------------------------------------------*/

/**
 * If format is a memory format, attaches a pointer that will be updated
 * to always point to the last written byte.
 *
 * Memory formats with auto-expansion will destroy their mem when closed.
 * Don't use such a tracker on mem-formats with auto expansion after
 * having closed them!
 *
 */
bool ov_format_attach_end_ptr_tracker(ov_format *fmt, uint8_t **tracker);

/*****************************************************************************
                                  File Format
 ****************************************************************************/

/**
 * Create a format on top of a physical file
 */
ov_format *ov_format_open(char const *path, ov_format_mode mode);

/*****************************************************************************
                                Format disposal
 ****************************************************************************/

/**
 * Closes format `f` and all underlying formats.
 */
ov_format *ov_format_close(ov_format *f);

/*----------------------------------------------------------------------------*/

/**
 * Closes format `f` but not its underlying formats.
 */
ov_format *ov_format_close_non_recursive(ov_format *f);

/*****************************************************************************
                                Format stacking
 ****************************************************************************/

ov_format *ov_format_wrap(ov_format *f, char const *type,
                          ov_format_handler *handler, void *options);

ov_format const *ov_format_get(ov_format const *f, char const *format_desc);

/*****************************************************************************
                     Public - general functions on formats
 ****************************************************************************/

/**
 * @return true if there is more data available from format `f`
 */
bool ov_format_has_more_data(ov_format const *f);

/*----------------------------------------------------------------------------*/

/**
 * Works only on formats opened with OV_READ
 * Whenever possible, us ov_format_payload_read_chunk_nocopy !
 * @param requested_bytes A pure request, number of returned bytes might differ.
 * Usually, 0 will indicate to receive all bytes vailable
 * @return Buffer containing payload data or 0 if frame could not be decoded /
 * no more data available. To distinguish no more data / chunk could not be
 * decoded, use `ov_format_has_more_data`
 */
ov_buffer *ov_format_payload_read_chunk(ov_format *f, size_t requested_bytes);

/**
 * The buffer->start points firectly into the format buffer, the data is not
 * copied. therefore it must not be modified!
 * @return buffer containing payload data (pointer to ...). If no more data is
 * available, buffer.start will be 0
 */
ov_buffer ov_format_payload_read_chunk_nocopy(ov_format *f,
                                              size_t requested_bytes);

/**
 * Format must be writable.
 * @return number of bytes actually written or negative number if error
 */
ssize_t ov_format_payload_write_chunk(ov_format *f, ov_buffer const *chunk);

/**
 * Write chunk to a random place in the underlying format.
 * Format must be writable and support random access (i.e.
 * a format on top of a socket might not be randomly accessible)
 *
 * Only overwrites, that is, the offset and offset + chunk->length must lie
 * within already written data.
 *
 * Does not write partial data!
 *
 * @return number of overwritten bytes, or negative in case of error
 */
ssize_t ov_format_payload_overwrite(ov_format *f, size_t offset,
                                    ov_buffer const *chunk);

/**
 * Returns the custom data the actual format attached to f.
 * Usually only needed for implementing new formats
 */
void *ov_format_get_custom_data(ov_format const *f);

/*****************************************************************************
                                Buffered Format
 ****************************************************************************/

ov_format *ov_format_buffered(uint8_t *data, size_t length);

/*----------------------------------------------------------------------------*/

bool ov_format_buffered_update(ov_format *f, uint8_t *new_data, size_t length);

/*----------------------------------------------------------------------------*/

#endif
