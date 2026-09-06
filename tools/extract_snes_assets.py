#!/usr/bin/env python3
import os
import struct
import ctypes
from PIL import Image

# Load decompression library
lib_path = os.path.abspath(os.path.join(os.path.dirname(__file__), 'libdecompr.dylib'))
lib = ctypes.CDLL(lib_path)
lib.decode_dat_image.argtypes = [
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.POINTER(ctypes.c_int),
    ctypes.POINTER(ctypes.c_int)
]
lib.decode_dat_image.restype = ctypes.c_int

DAT_MAP = {
    'VDUNGEON.DAT': 'VDUNGEON',
    'VPALACE.DAT': 'VPALACE',
    'KID.DAT': 'KID',
    'GUARD.DAT': 'GUARD',
    'GUARD1.DAT': 'GUARD1',
    'GUARD2.DAT': 'GUARD2',
    'FAT.DAT': 'FAT',
    'SKEL.DAT': 'SKEL',
    'SHADOW.DAT': 'SHADOW',
    'VIZIER.DAT': 'VIZIER',
    'PV.DAT': 'PV',
    'TITLE.DAT': 'TITLE',
    'PRINCE.DAT': 'PRINCE',
}

def extract_dat(dat_path, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    with open(dat_path, 'rb') as f:
        header = f.read(6)
        if len(header) < 6:
            return
        table_offset, table_size = struct.unpack('<IH', header)
        f.seek(table_offset)
        table_data = f.read(table_size)
        
    res_count = struct.unpack('<H', table_data[:2])[0]
    entries = []
    for i in range(res_count):
        entry_bytes = table_data[2 + i*8 : 2 + (i+1)*8]
        if len(entry_bytes) < 8:
            break
        res_id, offset, size = struct.unpack('<HIH', entry_bytes)
        entries.append((res_id, offset, size))
        
    # Read palettes first (res_id < 1000, size == 100)
    palettes = {}
    with open(dat_path, 'rb') as f:
        for res_id, offset, size in entries:
            if size == 100 and res_id < 1000:
                f.seek(offset + 1)
                pal_data = f.read(size)
                palettes[res_id] = pal_data
                pal_out = os.path.join(out_dir, f'res{res_id}.pal')
                with open(pal_out, 'wb') as p_out:
                    p_out.write(pal_data)
                print(f'  [PAL] {os.path.basename(dat_path)}: res{res_id}.pal')

    # If no palette in DAT, look for existing .pal files in out_dir
    if not palettes:
        pal_files = [f for f in os.listdir(out_dir) if f.endswith('.pal')]
        for pf in pal_files:
            try:
                pid = int(pf[3:-4])
                with open(os.path.join(out_dir, pf), 'rb') as p_in:
                    palettes[pid] = p_in.read()
            except ValueError:
                pass

    # Build RGB palette map from dat_shpl_type
    def make_pal_rgb(pal_data):
        rgb = []
        if pal_data and len(pal_data) >= 100:
            vga_offset = 1 + 2 + 1 # 4: n_images(1) + row_bits(2) + n_colors(1)
            raw_vga = list(pal_data[vga_offset : vga_offset + 48])
            max_v = max(raw_vga) if raw_vga else 0
            shift = 2 if max_v <= 63 else 0
            for c in range(16):
                r = min(255, pal_data[vga_offset + c*3 + 0] << shift)
                g = min(255, pal_data[vga_offset + c*3 + 1] << shift)
                b = min(255, pal_data[vga_offset + c*3 + 2] << shift)
                rgb.extend([r, g, b])
            # Force color 0 to black / transparent
            rgb[0] = rgb[1] = rgb[2] = 0
            rgb.extend([0] * (256 * 3 - len(rgb)))
        else:
            for c in range(256):
                rgb.extend([c, c, c])
        return rgb

    # Default palette for images
    default_pal_rgb = make_pal_rgb(next(iter(palettes.values())) if palettes else None)

    # Second pass: decode images
    img_count = 0
    with open(dat_path, 'rb') as f:
        for res_id, offset, size in entries:
            if size <= 6 or (size == 100 and res_id < 1000):
                continue
            f.seek(offset + 1)
            raw_data = f.read(size)
            if len(raw_data) < 6:
                continue
            
            h, w, flags = struct.unpack('<HHH', raw_data[:6])
            if h == 0 or w == 0 or h > 480 or w > 640:
                continue
            cmeth = (flags >> 8) & 0xf
            if cmeth > 4:
                continue
                
            out_buf = (ctypes.c_uint8 * (1024 * 1024))()
            out_w = ctypes.c_int()
            out_h = ctypes.c_int()
            
            src_buf = (ctypes.c_uint8 * len(raw_data)).from_buffer_copy(raw_data)
            ret = lib.decode_dat_image(src_buf, len(raw_data), out_buf, ctypes.byref(out_w), ctypes.byref(out_h))
            if ret == 0 and out_w.value > 0 and out_h.value > 0:
                w_dec, h_dec = out_w.value, out_h.value
                pixels = bytes(out_buf[:w_dec * h_dec])
                im = Image.frombytes('P', (w_dec, h_dec), pixels)
                
                # Check if there is a specific palette for this image range
                matched_pal = default_pal_rgb
                matching_pids = [pid for pid in palettes.keys() if pid < res_id]
                if matching_pids:
                    best_pid = max(matching_pids)
                    matched_pal = make_pal_rgb(palettes[best_pid])
                    
                im.putpalette(matched_pal)
                out_png = os.path.join(out_dir, f'res{res_id}.png')
                im.save(out_png, 'PNG')
                img_count += 1

    print(f'  [IMG] {os.path.basename(dat_path)}: {img_count} images decoded successfully.')

def main():
    base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    mods_snes_dir = os.path.join(base_dir, 'mods', 'snes')
    data_dir = os.path.join(base_dir, 'data')
    
    print("=== Extracting SNES graphics into data/ (keeping original levels) ===")
    for dat_file, folder in DAT_MAP.items():
        src_path = os.path.join(mods_snes_dir, dat_file)
        if not os.path.exists(src_path):
            src_path = os.path.join(mods_snes_dir, dat_file.lower())
        if os.path.exists(src_path):
            target_dir = os.path.join(data_dir, folder)
            print(f"Processing {dat_file} -> data/{folder}...")
            extract_dat(src_path, target_dir)
        else:
            print(f"Skipping {dat_file} (not found)")

if __name__ == '__main__':
    main()
