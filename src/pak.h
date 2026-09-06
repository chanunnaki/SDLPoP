#ifndef PAK_H
#define PAK_H

#include "common.h"

void init_pak(void);
image_type* load_image_from_pak(const char* folder, int res_id);
void* load_data_from_pak(const char* folder, int res_id, const char* ext, int* out_size);

#endif
