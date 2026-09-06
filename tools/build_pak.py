#!/usr/bin/env python3
"""
Pack all 1,023 loose resource files in data/ into a single high-speed binary
archive data/res.pak for instant loading on PSP.
"""
import os
import glob
import struct
from PIL import Image

entry_struct = struct.Struct('<16s4sHHHBBII')
header_struct = struct.Struct('<8sII')

def build_pak(data_dir, out_path=None):
    if out_path is None:
        out_path = os.path.join(data_dir, 'res.pak')
    files = glob.glob(os.path.join(data_dir, '**', 'res*.*'), recursive=True)
    
    entries_data = []
    payloads = bytearray()
    
    for f in files:
        if os.path.abspath(f) == os.path.abspath(out_path):
            continue
        rel = os.path.relpath(f, data_dir)
        parts = rel.split(os.sep)
        if len(parts) != 2:
            continue
        folder = parts[0]
        filename = parts[1]
        res_str, ext = os.path.splitext(filename)
        if not res_str.startswith('res'):
            continue
        try:
            res_id = int(res_str[3:])
        except ValueError:
            continue
        ext = ext[1:].lower()
        
        if ext == 'png':
            im = Image.open(f)
            w, h = im.size
            pal = im.getpalette() or []
            if len(pal) >= 16 * 3:
                pal = pal[:16 * 3]
            n_pal_colors = min(16, len(pal) // 3)
            pal_bytes = bytes(pal)
            pixels = bytes(im.getdata())
            payload = pal_bytes + pixels
            entries_data.append({
                'folder': folder,
                'ext': ext,
                'res_id': res_id,
                'w': w,
                'h': h,
                'n_pal': n_pal_colors,
                'flags': 1, # colorkey 0
                'payload': payload
            })
        else:
            with open(f, 'rb') as fp:
                payload = fp.read()
            entries_data.append({
                'folder': folder,
                'ext': ext,
                'res_id': res_id,
                'w': 0,
                'h': 0,
                'n_pal': 0,
                'flags': 0,
                'payload': payload
            })
            
    # Sort for fast bsearch
    entries_data.sort(key=lambda e: (e['folder'].upper(), e['res_id'], e['ext']))
    
    num_entries = len(entries_data)
    header_size = header_struct.size
    table_size = num_entries * entry_struct.size
    data_offset = header_size + table_size
    
    final_entries = []
    for e in entries_data:
        p_len = len(e['payload'])
        packed_entry = entry_struct.pack(
            e['folder'].encode('ascii')[:15],
            e['ext'].encode('ascii')[:3],
            e['res_id'],
            e['w'],
            e['h'],
            e['n_pal'],
            e['flags'],
            data_offset,
            p_len
        )
        final_entries.append(packed_entry)
        payloads.extend(e['payload'])
        data_offset += p_len
        
    header = header_struct.pack(b'POPSPAK1', 1, num_entries)
    
    with open(out_path, 'wb') as fp:
        fp.write(header)
        for fe in final_entries:
            fp.write(fe)
        fp.write(payloads)
        
    print(f"[✓] Successfully generated {out_path} ({num_entries} entries, {os.path.getsize(out_path)/1024/1024:.2f} MB)")

if __name__ == '__main__':
    import sys
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    target_data_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(base_dir, 'data')
    target_out_pak = sys.argv[2] if len(sys.argv) > 2 else os.path.join(target_data_dir, 'res.pak')
    build_pak(target_data_dir, target_out_pak)
