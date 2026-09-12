#include "pak.h"

#pragma pack(push, 1)
typedef struct {
	char folder[16];
	char ext[4];
	uint16_t res_id;
	uint16_t width;
	uint16_t height;
	uint8_t n_pal_colors;
	uint8_t flags;
	uint32_t data_offset;
	uint32_t data_size;
} pak_entry_type;

typedef struct {
	char magic[8]; // "POPSPAK1"
	uint32_t version; // 1
	uint32_t num_entries;
} pak_header_type;
#pragma pack(pop)

static byte* pak_buffer = NULL;
static size_t pak_buffer_size = 0;
static pak_header_type* pak_header = NULL;
static pak_entry_type* pak_entries = NULL;
static bool pak_initialized = false;

available_pak_type available_paks[MAX_AVAILABLE_PAKS];
byte num_available_paks = 0;
byte multiple_paks_available = 0;
byte graphics_pack_index = 0;
static bool paks_scanned = false;
extern byte* level_var_palettes;

void scan_available_paks(void) {
	if (paks_scanned) return;
	paks_scanned = true;

	num_available_paks = 0;

	static const struct {
		const char* id;
		const char* filename;
		const char* name;
	} candidate_paks[] = {
		{"dos",      "res_dos.pak",      "DOS"},
		{"snes",     "res_snes.pak",     "SNES"},
		{"snes_alt", "res_snes_alt.pak", "SNES Alt"},
	};

	for (size_t i = 0; i < sizeof(candidate_paks) / sizeof(candidate_paks[0]); ++i) {
		char rel_path[POP_MAX_PATH];
		snprintf_check(rel_path, sizeof(rel_path), "data/%s", candidate_paks[i].filename);
		const char* full_path = locate_file(rel_path);
		if (file_exists(full_path)) {
			if (num_available_paks < MAX_AVAILABLE_PAKS) {
				strncpy(available_paks[num_available_paks].id, candidate_paks[i].id, sizeof(available_paks[0].id) - 1);
				available_paks[num_available_paks].id[sizeof(available_paks[0].id) - 1] = '\0';
				strncpy(available_paks[num_available_paks].filename, candidate_paks[i].filename, sizeof(available_paks[0].filename) - 1);
				available_paks[num_available_paks].filename[sizeof(available_paks[0].filename) - 1] = '\0';
				strncpy(available_paks[num_available_paks].name, candidate_paks[i].name, sizeof(available_paks[0].name) - 1);
				available_paks[num_available_paks].name[sizeof(available_paks[0].name) - 1] = '\0';
				num_available_paks++;
			}
		}
	}

	if (num_available_paks == 0) {
		const char* def_path = locate_file("data/res_dos.pak");
		if (file_exists(def_path)) {
			strncpy(available_paks[0].id, "dos", sizeof(available_paks[0].id) - 1);
			strncpy(available_paks[0].filename, "res_dos.pak", sizeof(available_paks[0].filename) - 1);
			strncpy(available_paks[0].name, "DOS", sizeof(available_paks[0].name) - 1);
			num_available_paks = 1;
		}
	}

	multiple_paks_available = (num_available_paks > 1) ? 1 : 0;

	// Initial selection matching graphics_pack_name from ini/cfg
	if (graphics_pack_name[0] != '\0' && strcasecmp(graphics_pack_name, "default") != 0) {
		for (byte i = 0; i < num_available_paks; ++i) {
			if (strcasecmp(graphics_pack_name, available_paks[i].id) == 0 ||
			    ((strcasecmp(graphics_pack_name, "x68") == 0 || strcasecmp(graphics_pack_name, "x68000") == 0) &&
			     strcasecmp(available_paks[i].id, "snes_alt") == 0)) {
				graphics_pack_index = i;
				break;
			}
		}
	} else {
		graphics_pack_index = 0;
	}
	if (graphics_pack_index >= num_available_paks) {
		graphics_pack_index = 0;
	}
	if (num_available_paks > 0) {
		snprintf_check(graphics_pack_name, sizeof(graphics_pack_name), "%s", available_paks[graphics_pack_index].id);
	}
}

static bool load_pak_buffer(const char* pak_path) {
	FILE* fp = fopen(pak_path, "rb");
	if (fp == NULL) return false;

	fseek(fp, 0, SEEK_END);
	size_t new_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (new_size < sizeof(pak_header_type)) {
		fclose(fp);
		return false;
	}

	byte* new_buffer = (byte*) malloc(new_size);
	if (new_buffer == NULL) {
		fclose(fp);
		return false;
	}

	if (fread(new_buffer, 1, new_size, fp) != new_size) {
		free(new_buffer);
		fclose(fp);
		return false;
	}
	fclose(fp);

	pak_header_type* new_header = (pak_header_type*) new_buffer;
	if (memcmp(new_header->magic, "POPSPAK1", 8) != 0 || new_header->version != 1) {
		free(new_buffer);
		return false;
	}

	byte* old_buffer = pak_buffer;
	pak_buffer = new_buffer;
	pak_buffer_size = new_size;
	pak_header = new_header;
	pak_entries = (pak_entry_type*) (pak_buffer + sizeof(pak_header_type));
	if (old_buffer != NULL) {
		free(old_buffer);
	}
	return true;
}

void init_pak(void) {
	if (pak_initialized) return;
	pak_initialized = true;

	scan_available_paks();

	const char* target_filename = "res_dos.pak";
	if (num_available_paks > 0 && graphics_pack_index < num_available_paks) {
		target_filename = available_paks[graphics_pack_index].filename;
	}

	char target_rel_path[POP_MAX_PATH];
	snprintf_check(target_rel_path, sizeof(target_rel_path), "data/%s", target_filename);
	const char* pak_path = locate_file(target_rel_path);
	if (!file_exists(pak_path)) {
		pak_path = locate_file("data/res_dos.pak");
		if (!file_exists(pak_path)) {
			pak_path = locate_file("data/res.pak");
			if (!file_exists(pak_path)) {
				return;
			}
		}
	}

	load_pak_buffer(pak_path);
}

void reload_current_level_graphics(void) {
	// Temporarily redirect rendering to offscreen_surface to protect overlay_surface (menu)
	surface_type* saved_target = current_target_surface;
	current_target_surface = offscreen_surface;

	free_all_chtabs_from(0);

	// Reload guard palettes and level color variations from PRINCE.DAT
	if (guard_palettes != NULL) {
		free(guard_palettes);
		guard_palettes = NULL;
	}
	if (level_var_palettes != NULL) {
		free(level_var_palettes);
		level_var_palettes = NULL;
	}
	dat_type* dathandle = open_dat("PRINCE.DAT", 'G');
	guard_palettes = (byte*) load_from_opendats_alloc(10, "bin", NULL, NULL);
	level_var_palettes = (byte*) load_from_opendats_alloc(20, "bin", NULL, NULL);
	close_dat(dathandle);

	// Core sprites (sword, potion, kid)
	load_chtab_from_file(id_chtab_0_sword, 700, "PRINCE.DAT", 1<<2);
	load_chtab_from_file(id_chtab_1_flameswordpotion, 150, "PRINCE.DAT", 1<<3);
	load_kid_sprite();

	word saved_guard_color = curr_guard_color;
	word saved_next_level = next_level;
	if (current_level >= 1 && current_level <= 15) {
		load_lev_spr(current_level);
	}
	next_level = saved_next_level;
	curr_guard_color = saved_guard_color;
	if (curr_guard_color && chtab_addrs[id_chtab_5_guard] && guard_palettes != NULL) {
		set_chtab_palette(chtab_addrs[id_chtab_5_guard], &guard_palettes[0x30 * curr_guard_color - 0x30], 0x10);
	}

	need_full_redraw = 1;
	redraw_screen(0);

	// Restore original target surface (overlay_surface if inside menu)
	current_target_surface = saved_target;
}

bool switch_graphics_pack(byte new_index) {
	if (!pak_initialized) init_pak();
	if (new_index >= num_available_paks) return false;
	if (new_index == graphics_pack_index && pak_buffer != NULL) return true;

	char target_rel_path[POP_MAX_PATH];
	snprintf_check(target_rel_path, sizeof(target_rel_path), "data/%s", available_paks[new_index].filename);
	const char* pak_path = locate_file(target_rel_path);
	if (!file_exists(pak_path)) return false;

	if (!load_pak_buffer(pak_path)) return false;

	graphics_pack_index = new_index;
	snprintf_check(graphics_pack_name, sizeof(graphics_pack_name), "%s", available_paks[new_index].id);

	reload_current_level_graphics();
	return true;
}

bool is_pak_available(void) {
	if (!pak_initialized) init_pak();
	return pak_buffer != NULL && pak_entries != NULL;
}


static int compare_pak_key(const void* a, const void* b) {
	const pak_entry_type* entry_a = (const pak_entry_type*) a;
	const pak_entry_type* entry_b = (const pak_entry_type*) b;

	int c = strcasecmp(entry_a->folder, entry_b->folder);
	if (c != 0) return c;
	if (entry_a->res_id < entry_b->res_id) return -1;
	if (entry_a->res_id > entry_b->res_id) return 1;
	return strcasecmp(entry_a->ext, entry_b->ext);
}

static pak_entry_type* lookup_pak(const char* folder, int res_id, const char* ext) {
	if (pak_buffer == NULL || pak_entries == NULL || pak_header == NULL) return NULL;

	pak_entry_type key;
	memset(&key, 0, sizeof(key));
	snprintf(key.folder, sizeof(key.folder), "%s", folder);
	key.res_id = (uint16_t) res_id;
	snprintf(key.ext, sizeof(key.ext), "%s", ext);

	return (pak_entry_type*) bsearch(&key, pak_entries, pak_header->num_entries, sizeof(pak_entry_type), compare_pak_key);
}

image_type* load_image_from_pak(const char* folder, int res_id) {
	if (!pak_initialized) init_pak();
	pak_entry_type* entry = lookup_pak(folder, res_id, "png");
	if (entry == NULL) return NULL;

	image_type* image = SDL_CreateRGBSurface(0, entry->width, entry->height, 8, 0, 0, 0, 0);
	if (image == NULL) return NULL;

	byte* payload = pak_buffer + entry->data_offset;
	int pal_bytes_len = entry->n_pal_colors * 3;
	byte* pal_data = payload;
	byte* pixel_data = payload + pal_bytes_len;

	SDL_Color colors[256];
	for (int i = 0; i < entry->n_pal_colors; ++i) {
		colors[i].r = pal_data[i * 3 + 0];
		colors[i].g = pal_data[i * 3 + 1];
		colors[i].b = pal_data[i * 3 + 2];
		colors[i].a = 255;
	}
	SDL_SetPaletteColors(image->format->palette, colors, 0, entry->n_pal_colors);

	for (int y = 0; y < entry->height; ++y) {
		memcpy((byte*)image->pixels + y * image->pitch, pixel_data + y * entry->width, entry->width);
	}

	if (entry->flags & 1) {
		SDL_SetColorKey(image, SDL_TRUE, 0);
	}
	return image;
}

void* load_data_from_pak(const char* folder, int res_id, const char* ext, int* out_size) {
	if (!pak_initialized) init_pak();
	pak_entry_type* entry = lookup_pak(folder, res_id, ext);
	if (entry == NULL) return NULL;

	void* area = malloc(entry->data_size);
	if (area == NULL) return NULL;

	memcpy(area, pak_buffer + entry->data_offset, entry->data_size);
	if (out_size) *out_size = entry->data_size;
	return area;
}
