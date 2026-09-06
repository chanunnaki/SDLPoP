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

void init_pak(void) {
	if (pak_initialized) return;
	pak_initialized = true;

	const char* pak_path = locate_file("data/res.pak");
	if (!file_exists(pak_path)) {
		return;
	}

	FILE* fp = fopen(pak_path, "rb");
	if (fp == NULL) return;

	fseek(fp, 0, SEEK_END);
	pak_buffer_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (pak_buffer_size < sizeof(pak_header_type)) {
		fclose(fp);
		return;
	}

	pak_buffer = (byte*) malloc(pak_buffer_size);
	if (pak_buffer == NULL) {
		fclose(fp);
		return;
	}

	if (fread(pak_buffer, 1, pak_buffer_size, fp) != pak_buffer_size) {
		free(pak_buffer);
		pak_buffer = NULL;
		fclose(fp);
		return;
	}
	fclose(fp);

	pak_header = (pak_header_type*) pak_buffer;
	if (memcmp(pak_header->magic, "POPSPAK1", 8) != 0 || pak_header->version != 1) {
		free(pak_buffer);
		pak_buffer = NULL;
		return;
	}

	pak_entries = (pak_entry_type*) (pak_buffer + sizeof(pak_header_type));
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
