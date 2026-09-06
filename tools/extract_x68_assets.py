#!/usr/bin/env python3
"""
Extract Sharp X68000 graphics from mods/x68 and bake into data/res_x68.pak
while preserving the authentic original 12 levels and audio assets.
"""
import os
import sys
import struct
import shutil
import ctypes
import subprocess
from PIL import Image

# Import build_pak
sys.path.append(os.path.dirname(__file__))
from build_pak import build_pak

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
    'vdungeon.dat': 'VDUNGEON',
    'VPALACE.DAT': 'VPALACE',
    'KID.DAT': 'KID',
    'GUARD.DAT': 'GUARD',
    'GUARD1.DAT': 'GUARD1',
    'GUARD2.DAT': 'GUARD2',
    'FAT.DAT': 'FAT',
    'SKEL.DAT': 'SKEL',
    'SHADOW.DAT': 'SHADOW',
    'VIZIER.DAT': 'VIZIER',
    'prince.dat': 'PRINCE',
}

def make_pal_rgb(pal_data):
    rgb = []
    if pal_data and len(pal_data) >= 100:
        vga_offset = 4 # 1 (n_images) + 2 (row_bits) + 1 (n_colors)
        raw_vga = list(pal_data[vga_offset : vga_offset + 48])
        max_v = max(raw_vga) if raw_vga else 0
        shift = 2 if max_v <= 63 else 0
        for c in range(16):
            r = min(255, pal_data[vga_offset + c*3 + 0] << shift)
            g = min(255, pal_data[vga_offset + c*3 + 1] << shift)
            b = min(255, pal_data[vga_offset + c*3 + 2] << shift)
            rgb.extend([r, g, b])
        rgb[0] = rgb[1] = rgb[2] = 0 # Transparent colorkey
        rgb.extend([0] * (256 * 3 - len(rgb)))
    else:
        for c in range(256):
            rgb.extend([c, c, c])
    return rgb

def extract_dat(dat_path, out_dir, fallback_pal_data=None):
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
        
    # Read palettes & binary chunks
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
            elif res_id in (10, 20) and 'prince' in dat_path.lower():
                # Guard palettes (10, 336 bytes) or level palettes (20, 240 bytes)
                f.seek(offset + 1)
                bin_data = f.read(size)
                bin_out = os.path.join(out_dir, f'res{res_id}.bin')
                with open(bin_out, 'wb') as b_out:
                    b_out.write(bin_data)
                print(f'  [BIN] {os.path.basename(dat_path)}: res{res_id}.bin ({size} bytes)')

    # If no palette in DAT, look for fallback palette
    if not palettes and fallback_pal_data:
        default_pal_rgb = make_pal_rgb(fallback_pal_data)
    elif palettes:
        default_pal_rgb = make_pal_rgb(next(iter(palettes.values())))
    else:
        default_pal_rgb = make_pal_rgb(None)

    # Decode images
    img_count = 0
    with open(dat_path, 'rb') as f:
        for res_id, offset, size in entries:
            if size <= 6 or (size == 100 and res_id < 1000) or res_id in (1, 10, 20, 65535):
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
                
                # Mirror 1000+ IDs down to 200/300 range for complete VGA tile compatibility
                if res_id >= 1000 and (res_id - 1000) >= 200:
                    mirror_png = os.path.join(out_dir, f'res{res_id - 1000}.png')
                    im.save(mirror_png, 'PNG')

    print(f'  [IMG] {os.path.basename(dat_path)}: {img_count} images decoded successfully.')

def main():
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    mods_x68_dir = os.path.join(base_dir, 'mods', 'x68')
    staging_dir = os.path.join(base_dir, 'build', 'data_x68')
    
    print("=== Extracting Sharp X68000 Graphics Pack ===")
    
    # 1. Clean and initialize staging directory
    if os.path.exists(staging_dir):
        shutil.rmtree(staging_dir)
    os.makedirs(staging_dir, exist_ok=True)
    
    # 2. Extract base DOS assets from git e29d027
    print("-> Unpacking pristine DOS base assets (authentic levels, fonts, sounds)...")
    p = subprocess.Popen(['git', 'archive', 'e29d027', 'data/'], stdout=subprocess.PIPE, cwd=base_dir)
    tar = subprocess.Popen(['tar', '-x', '-C', staging_dir, '--strip-components=1'], stdin=p.stdout)
    p.stdout.close()
    tar.communicate()
    
    # Read FAT/res750.pal from mods/x68/FAT.DAT for fallback guard palette
    fat_dat_path = os.path.join(mods_x68_dir, 'FAT.DAT')
    fallback_guard_pal = None
    if os.path.exists(fat_dat_path):
        with open(fat_dat_path, 'rb') as f:
            f.seek(6)
            # Table offset
            header = open(fat_dat_path, 'rb').read(6)
            tbl_off, tbl_sz = struct.unpack('<IH', header)
            f.seek(tbl_off)
            tbl = f.read(tbl_sz)
            count = struct.unpack('<H', tbl[:2])[0]
            for i in range(count):
                rid, off, sz = struct.unpack('<HIH', tbl[2+i*8 : 2+(i+1)*8])
                if sz == 100 and rid == 750:
                    f.seek(off + 1)
                    fallback_guard_pal = f.read(sz)
                    break

    # 3. Extract X68000 assets into staging directory
    for dat_file, folder in DAT_MAP.items():
        src_path = os.path.join(mods_x68_dir, dat_file)
        if not os.path.exists(src_path):
            src_path = os.path.join(mods_x68_dir, dat_file.lower())
        if not os.path.exists(src_path):
            src_path = os.path.join(mods_x68_dir, dat_file.upper())
            
        if os.path.exists(src_path):
            target_dir = os.path.join(staging_dir, folder)
            print(f"Processing {dat_file} -> {folder}...")
            extract_dat(src_path, target_dir, fallback_guard_pal if folder == 'GUARD' else None)
        else:
            print(f"Skipping {dat_file} (not found)")
            
    # 4. Remove any loose .pak files that might have been unpacked in staging
    for f in os.listdir(staging_dir):
        if f.endswith('.pak'):
            os.remove(os.path.join(staging_dir, f))
            
    # 5. Build res_x68.pak
    out_pak = os.path.join(base_dir, 'data', 'res_x68.pak')
    print(f"-> Building PAK archive: {out_pak}...")
    build_pak(staging_dir, out_pak)
    
    # 6. Also copy to data/res.pak as the active pack
    active_pak = os.path.join(base_dir, 'data', 'res.pak')
    shutil.copy2(out_pak, active_pak)
    print(f"[✓] Active pack data/res.pak set to Sharp X68000!")

if __name__ == '__main__':
    main()
