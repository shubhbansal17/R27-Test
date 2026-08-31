```c
#include "en_dc.h"
#include <stdlib.h>

/*****************************************************************************
 * Defines
 ****************************************************************************/

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

/*****************************************************************************
 * Functions
 ****************************************************************************/

/* Encode */
encode_result frame_encode(void *dst_buf_ptr, size_t dst_buf_len,
                           const void *src_ptr, size_t src_len)
{
    encode_result result = {0u, ENCODE_OK};

    const uint8_t *src_read_ptr;
    const uint8_t *src_end_ptr;
    uint8_t *dst_buf_start_ptr;
    uint8_t *dst_buf_end_ptr;
    uint8_t *dst_write_ptr;
    uint8_t *dst_code_write_ptr;
    uint8_t search_len = 1u;

    if ((dst_buf_ptr == NULL) || (src_ptr == NULL)) {
        result.status = ENCODE_NULL_POINTER;
        return result;
    }

    dst_buf_start_ptr = (uint8_t *)dst_buf_ptr;
    dst_buf_end_ptr = dst_buf_start_ptr + dst_buf_len;
    dst_write_ptr = dst_buf_start_ptr;

    src_read_ptr = (const uint8_t *)src_ptr;
    src_end_ptr = src_read_ptr + src_len;

    if (dst_buf_len == 0u) {
        result.status = ENCODE_OUT_BUFFER_OVERFLOW;
        return result;
    }

    /* Reserve the first byte for the COBS code */
    dst_code_write_ptr = dst_write_ptr;
    dst_write_ptr++;

    while (src_read_ptr < src_end_ptr) {
        uint8_t src_byte = *src_read_ptr++;

        if (src_byte == 0u) {

            /* Finish the current block */
            *dst_code_write_ptr = search_len;

            /* Need a new code byte */
            if (dst_write_ptr >= dst_buf_end_ptr) {
                result.status |= ENCODE_OUT_BUFFER_OVERFLOW;
                break;
            }

            dst_code_write_ptr = dst_write_ptr++;
            search_len = 1u;

        } else {

            if (dst_write_ptr >= dst_buf_end_ptr) {
                result.status |= ENCODE_OUT_BUFFER_OVERFLOW;
                break;
            }

            *dst_write_ptr++ = src_byte;
            search_len++;

            /*
             * Maximum block reached: 254 non-zero bytes.
             * Only reserve another code byte if more input remains.
             */
            if ((search_len == 0xFFu) &&
                (src_read_ptr < src_end_ptr)) {

                *dst_code_write_ptr = search_len;

                if (dst_write_ptr >= dst_buf_end_ptr) {
                    result.status |= ENCODE_OUT_BUFFER_OVERFLOW;
                    break;
                }

                dst_code_write_ptr = dst_write_ptr++;
                search_len = 1u;
            }
        }
    }

    /* Finish the final block */
    if (dst_code_write_ptr < dst_buf_end_ptr) {
        *dst_code_write_ptr = search_len;
    }

    result.out_len = (size_t)(dst_write_ptr - dst_buf_start_ptr);

    return result;
}


/* Decode */
decode_result frame_decode(void *dst_buf_ptr, size_t dst_buf_len,
                           const void *src_ptr, size_t src_len)
{
    decode_result result = {0u, DECODE_OK};

    const uint8_t *src_read_ptr;
    const uint8_t *src_end_ptr;
    uint8_t *dst_buf_start_ptr;
    uint8_t *dst_buf_end_ptr;
    uint8_t *dst_write_ptr;

    if ((dst_buf_ptr == NULL) || (src_ptr == NULL)) {
        result.status = DECODE_NULL_POINTER;
        return result;
    }

    src_read_ptr = (const uint8_t *)src_ptr;
    src_end_ptr = src_read_ptr + src_len;

    dst_buf_start_ptr = (uint8_t *)dst_buf_ptr;
    dst_buf_end_ptr = dst_buf_start_ptr + dst_buf_len;
    dst_write_ptr = dst_buf_start_ptr;

    while (src_read_ptr < src_end_ptr) {
        uint8_t len_code = *src_read_ptr++;
        size_t copy_len;
        size_t remaining_input;
        size_t remaining_output;

        /* Zero is invalid in COBS encoded input */
        if (len_code == 0u) {
            result.status |= DECODE_ZERO_BYTE_IN_INPUT;
            break;
        }

        copy_len = (size_t)(len_code - 1u);

        remaining_input =
            (size_t)(src_end_ptr - src_read_ptr);

        if (copy_len > remaining_input) {
            result.status |= DECODE_INPUT_TOO_SHORT;
            break;
        }

        remaining_output =
            (size_t)(dst_buf_end_ptr - dst_write_ptr);

        if (copy_len > remaining_output) {
            result.status |= DECODE_OUT_BUFFER_OVERFLOW;
            break;
        }

        /* Copy data bytes */
        while (copy_len > 0u) {
            uint8_t src_byte = *src_read_ptr++;

            if (src_byte == 0u) {
                result.status |= DECODE_ZERO_BYTE_IN_INPUT;
                break;
            }

            *dst_write_ptr++ = src_byte;
            copy_len--;
        }

        if (result.status & DECODE_ZERO_BYTE_IN_INPUT) {
            break;
        }

        /*
         * Restore the zero removed during encoding.
         * Do not add one after the final block or after
         * a full 0xFF block.
         */
        if ((len_code != 0xFFu) &&
            (src_read_ptr < src_end_ptr)) {

            if (dst_write_ptr >= dst_buf_end_ptr) {
                result.status |= DECODE_OUT_BUFFER_OVERFLOW;
                break;
            }

            *dst_write_ptr++ = 0u;
        }
    }

    result.out_len = (size_t)(dst_write_ptr - dst_buf_start_ptr);

    return result;
}
```
