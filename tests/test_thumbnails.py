#!/usr/bin/env python3
"""Decode actual thumbnail bytes, including misleading suffixes and failures."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from PIL import Image

driver = sys.argv[1]
with tempfile.TemporaryDirectory(prefix='fltube-thumbnails-') as directory:
    root = Path(directory)
    for format in ['JPEG', 'PNG', 'WEBP']:
        path = root / (format + '.jpg')
        Image.new('RGB', (160, 90), (30, 210, 90)).save(path, format=format)
        result = json.loads(subprocess.check_output([driver, str(path), '95', '80']))
        assert result['w'] == 95 and result['h'] == 53, result
        assert max(abs(a-b) for a,b in zip(result['rgb'], (30,210,90))) < 5, result
        print('PASS', format, 'bytes decode correctly regardless of suffix')
    path = root / 'portrait.png'
    Image.new('RGB', (80, 200), (30, 210, 90)).save(path)
    result = json.loads(subprocess.check_output([driver, str(path), '95', '80']))
    assert (result['w'],result['h']) == (32,80), result
    print('PASS portrait thumbnail fits inside the row')
    for name, content in [('error.jpg', b'<html>Not found</html>'), ('broken.webp', b'RIFF\x14\0\0\0WEBPgarbage'), ('empty.jpg', b'')]:
        path = root / name
        path.write_bytes(content)
        assert subprocess.run([driver, str(path), '95', '80']).returncode == 2
    print('PASS empty, invalid and corrupt responses fail safely')

    # Exercise real GL widget drawing: CPU decoding alone missed the old blank-image bug.
    import os, time
    from Xlib import X, display
    rd, wr = os.pipe()
    xvfb = subprocess.Popen(['Xvfb','-displayfd',str(wr),'-screen','0','640x480x24','-nolisten','tcp'],pass_fds=(wr,),stderr=subprocess.DEVNULL)
    os.close(wr)
    with os.fdopen(rd) as pipe: screen = ':' + pipe.readline().strip()
    env = dict(os.environ, DISPLAY=screen, FLTK_BACKEND='x11', LIBGL_ALWAYS_SOFTWARE='1')
    env.pop('WAYLAND_DISPLAY',None); env.pop('WAYLAND_SOCKET',None)
    child = None
    xd = display.Display(screen)
    try:
        for format in ['JPEG','PNG','WEBP']:
            child = subprocess.Popen([driver,'--render',str(root/(format+'.jpg'))],env=env)
            deadline = time.monotonic()+6
            window = None
            while time.monotonic()<deadline:
                window = next((w for w in xd.screen().root.query_tree().children if w.get_wm_name()=='FLTube image test'),None)
                if window: break
                time.sleep(.05)
            assert window is not None, 'Image test window did not open'
            time.sleep(.4)
            raw = window.get_image(0,0,160,120,X.ZPixmap,0xffffffff)
            frame = Image.frombytes('RGB',(160,120),raw.data,'raw','BGRX')
            color = frame.getpixel((67,60))
            assert max(abs(a-b) for a,b in zip(color,(30,210,90))) < 5, (format,color)
            print('PASS',format,'thumbnail visibly renders inside a GPU widget')
            child.terminate(); child.wait(timeout=3); child = None
    finally:
        if child and child.poll() is None: child.terminate(); child.wait(timeout=3)
        xd.close(); xvfb.terminate(); xvfb.wait(timeout=3)
