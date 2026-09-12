#ifndef PAK_H
#define PAK_H

#include "common.h"

typedef struct {
	char name[32];      // Display name in menu: "DOS", "SNES", "SNES Alt"
	char filename[32];  // File name: "res_dos.pak", "res_snes.pak", "res_snes_alt.pak"
	char id[16];        // INI id: "dos", "snes", "snes_alt"
} available_pak_type;

#define MAX_AVAILABLE_PAKS 8
extern available_pak_type available_paks[MAX_AVAILABLE_PAKS];
extern byte num_available_paks;
extern byte multiple_paks_available;
extern byte graphics_pack_index;

void scan_available_paks(void);
void init_pak(void);
bool switch_graphics_pack(byte new_index);
void reload_current_level_graphics(void);
bool is_pak_available(void);
image_type* load_image_from_pak(const char* folder, int res_id);
void* load_data_from_pak(const char* folder, int res_id, const char* ext, int* out_size);

#endif
