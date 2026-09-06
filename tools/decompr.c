#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;
typedef signed char sbyte;
typedef unsigned short word;

static inline word swap_le16(word v) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return (v << 8) | (v >> 8);
#else
    return v;
#endif
}

static void decompress_rle_lr(byte* destination, const byte* source, int dest_length) {
    const byte* src_pos = source;
    byte* dest_pos = destination;
    short rem_length = dest_length;
    while (rem_length > 0) {
        sbyte count = *src_pos++;
        if (count >= 0) {
            ++count;
            do {
                *dest_pos++ = *src_pos++;
                --rem_length;
                --count;
            } while (count && rem_length > 0);
        } else {
            byte al = *src_pos++;
            count = -count;
            do {
                *dest_pos++ = al;
                --rem_length;
                --count;
            } while (count && rem_length > 0);
        }
    }
}

static void decompress_rle_ud(byte* destination, const byte* source, int dest_length, int width, int height) {
    short rem_height = height;
    const byte* src_pos = source;
    byte* dest_pos = destination;
    short rem_length = dest_length;
    --dest_length;
    --width;
    while (rem_length > 0) {
        sbyte count = *src_pos++;
        if (count >= 0) {
            ++count;
            do {
                *dest_pos = *src_pos++;
                dest_pos++;
                dest_pos += width;
                --rem_height;
                if (rem_height == 0) {
                    dest_pos -= dest_length;
                    rem_height = height;
                }
                --rem_length;
                --count;
            } while (count && rem_length > 0);
        } else {
            byte al = *src_pos++;
            count = -count;
            do {
                *dest_pos = al;
                dest_pos++;
                dest_pos += width;
                --rem_height;
                if (rem_height == 0) {
                    dest_pos -= dest_length;
                    rem_height = height;
                }
                --rem_length;
                --count;
            } while (count && rem_length > 0);
        }
    }
}

static byte* decompress_lzg_lr(byte* dest, const byte* source, int dest_length) {
    byte* window = (byte*) malloc(0x400);
    if (!window) return NULL;
    memset(window, 0, 0x400);
    byte* window_pos = window + 0x400 - 0x42;
    short remaining = dest_length;
    byte* window_end = window + 0x400;
    const byte* source_pos = source;
    byte* dest_pos = dest;
    word mask = 0;
    do {
        mask >>= 1;
        if ((mask & 0xFF00) == 0) {
            mask = *source_pos | 0xFF00;
            source_pos++;
        }
        if (mask & 1) {
            *window_pos = *dest_pos = *source_pos;
            window_pos++;
            dest_pos++;
            source_pos++;
            if (window_pos >= window_end) window_pos = window;
            --remaining;
        } else {
            word copy_info = *source_pos++;
            copy_info = (copy_info << 8) | *source_pos++;
            byte* copy_source = window + (copy_info & 0x3FF);
            byte copy_length = (copy_info >> 10) + 3;
            do {
                *window_pos = *dest_pos = *copy_source;
                window_pos++;
                dest_pos++;
                copy_source++;
                if (copy_source >= window_end) copy_source = window;
                if (window_pos >= window_end) window_pos = window;
                --remaining;
                --copy_length;
            } while (remaining && copy_length);
        }
    } while (remaining > 0);
    free(window);
    return dest;
}

static byte* decompress_lzg_ud(byte* dest, const byte* source, int dest_length, int stride, int height) {
    byte* window = (byte*) malloc(0x400);
    if (!window) return NULL;
    memset(window, 0, 0x400);
    byte* window_pos = window + 0x400 - 0x42;
    short remaining = height;
    byte* window_end = window + 0x400;
    const byte* source_pos = source;
    byte* dest_pos = dest;
    word mask = 0;
    short dest_end = dest_length - 1;
    do {
        mask >>= 1;
        if ((mask & 0xFF00) == 0) {
            mask = *source_pos | 0xFF00;
            source_pos++;
        }
        if (mask & 1) {
            *window_pos = *dest_pos = *source_pos;
            window_pos++;
            source_pos++;
            dest_pos += stride;
            --remaining;
            if (remaining == 0) {
                dest_pos -= dest_end;
                remaining = height;
            }
            if (window_pos >= window_end) window_pos = window;
            --dest_length;
        } else {
            word copy_info = *source_pos++;
            copy_info = (copy_info << 8) | *source_pos++;
            byte* copy_source = window + (copy_info & 0x3FF);
            byte copy_length = (copy_info >> 10) + 3;
            do {
                *window_pos = *dest_pos = *copy_source;
                window_pos++;
                copy_source++;
                dest_pos += stride;
                --remaining;
                if (remaining == 0) {
                    dest_pos -= dest_end;
                    remaining = height;
                }
                if (copy_source >= window_end) copy_source = window;
                if (window_pos >= window_end) window_pos = window;
                --dest_length;
                --copy_length;
            } while (dest_length && copy_length);
        }
    } while (dest_length > 0);
    free(window);
    return dest;
}

int decode_dat_image(const byte* src_data, int src_len, byte* out_8bpp, int* out_w, int* out_h) {
    if (src_len < 6) return -1;
    word height = swap_le16(*(const word*)(src_data + 0));
    word width  = swap_le16(*(const word*)(src_data + 2));
    word flags  = swap_le16(*(const word*)(src_data + 4));
    if (height == 0 || width == 0) return -1;
    
    int depth = ((flags >> 12) & 7) + 1;
    int cmeth = (flags >> 8) & 0x0F;
    int stride = (depth * width + 7) / 8;
    int dest_size = stride * height;
    
    byte* dest = (byte*) malloc(dest_size);
    if (!dest) return -2;
    memset(dest, 0, dest_size);
    
    const byte* payload = src_data + 6;
    switch (cmeth) {
        case 0:
            memcpy(dest, payload, dest_size);
            break;
        case 1:
            decompress_rle_lr(dest, payload, dest_size);
            break;
        case 2:
            decompress_rle_ud(dest, payload, dest_size, stride, height);
            break;
        case 3:
            decompress_lzg_lr(dest, payload, dest_size);
            break;
        case 4:
            decompress_lzg_ud(dest, payload, dest_size, stride, height);
            break;
        default:
            free(dest);
            return -3;
    }
    
    int pixels_per_byte = 8 / depth;
    int mask = (1 << depth) - 1;
    for (int y = 0; y < height; ++y) {
        byte* in_pos = dest + y * stride;
        byte* out_pos = out_8bpp + y * width;
        for (int x_pixel = 0, x_byte = 0; x_byte < stride; ++x_byte) {
            byte v = *in_pos;
            int shift = 8;
            for (int pixel_in_byte = 0; pixel_in_byte < pixels_per_byte && x_pixel < width; ++pixel_in_byte, ++x_pixel) {
                shift -= depth;
                *out_pos = (v >> shift) & mask;
                ++out_pos;
            }
            ++in_pos;
        }
    }
    
    free(dest);
    *out_w = width;
    *out_h = height;
    return 0;
}
