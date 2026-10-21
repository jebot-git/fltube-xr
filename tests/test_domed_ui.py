#!/usr/bin/env python3
"""Real Wayland DBusMenu export on a private DomeD and session bus (Python GI).

Run with /usr/bin/python3 tests/test_domed_ui.py. No live session is modified.
"""
import json
import os
from pathlib import Path
import socket
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.request

if os.environ.get('FLTUBE_MENU_TEST_BUS') != '1':
    os.execvpe('dbus-run-session', ['dbus-run-session', '--', sys.executable, *sys.argv],
               dict(os.environ, FLTUBE_MENU_TEST_BUS='1'))
from gi.repository import Gio, GLib
from Xlib import X, display as xdisplay
from Xlib.ext import xtest

ROOT = Path(__file__).resolve().parents[1]
DOME = Path(os.environ.get('MPVR_DOME_SOURCE', str(ROOT.parent / 'DomeD')))
ART = ROOT / 'artifacts/menu'
ART.mkdir(parents=True, exist_ok=True)
children, logs = [], []
bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)


def call(service, path, interface, method, signature, args):
    return bus.call_sync(service, path, interface, method, GLib.Variant(signature, args),
                         None, Gio.DBusCallFlags.NONE, 3000, None).unpack()


def wait(check, timeout=20):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        try:
            value = check()
            if value:
                return value
        except (OSError, ValueError, KeyError, GLib.Error):
            pass
        time.sleep(.05)
    raise AssertionError('Timed out; inspect artifacts/menu')


def start(args, name, env, **kwargs):
    log = (ART / (name + '.log')).open('w')
    logs.append(log)
    child = subprocess.Popen(list(map(str, args)), env=env, stdout=log, stderr=log, **kwargs)
    children.append(child)
    return child


def owner(pid):
    names = call('org.freedesktop.DBus', '/org/freedesktop/DBus', 'org.freedesktop.DBus',
                 'ListNames', '()', ())[0]
    for name in names:
        if name.startswith(':'):
            found = call('org.freedesktop.DBus', '/org/freedesktop/DBus', 'org.freedesktop.DBus',
                         'GetConnectionUnixProcessID', '(s)', (name,))[0]
            if found == pid:
                try:
                    call(name, '/org/de_xr/FLTK/Menu1', 'com.canonical.dbusmenu', 'GetLayout', '(iias)', (0, -1, []))
                    return name
                except GLib.Error: pass


def flatten(node):
    yield node
    for child in node[2]:
        yield from flatten(child)


try:
    with tempfile.TemporaryDirectory(prefix='fltube-menu-') as directory:
        runtime = Path(directory); runtime.chmod(0o700)
        (runtime / '.cache/fltube').mkdir(parents=True)
        with socket.socket() as sock:
            sock.bind(('127.0.0.1', 0)); port = sock.getsockname()[1]
        http = urllib.request.build_opener(urllib.request.ProxyHandler({}))
        def request(path, value=None):
            req = urllib.request.Request(f'http://127.0.0.1:{port}{path}',
                data=None if value is None else json.dumps(value).encode(),
                headers={'Content-Type': 'application/json'})
            with http.open(req, timeout=3) as response: return json.load(response)
        rd, wr = os.pipe()
        start(['Xvfb', '-displayfd', wr, '-screen', '0', '1280x900x24', '-nolisten', 'tcp'],
              'xvfb', os.environ.copy(), pass_fds=(wr,))
        os.close(wr)
        with os.fdopen(rd) as pipe: display = ':' + pipe.readline().strip()
        env = dict(os.environ, DISPLAY=display, HOME=directory, XDG_RUNTIME_DIR=directory,
            XDG_CONFIG_HOME=str(runtime / '.config'), XDG_DATA_HOME=str(runtime / 'data'),
            XDG_CACHE_HOME=str(runtime / '.cache'),
            DOME_PREVIEW_PORT=str(port), DOME_START_XR='0', LIBGL_ALWAYS_SOFTWARE='1',
            XR_RUNTIME_JSON=str(runtime / 'no-runtime.json'), RUST_LOG='error')
        env.pop('WAYLAND_DISPLAY', None); env.pop('WAYLAND_SOCKET', None)
        start([str(DOME / 'target/release/dome-compositor'), '--backend', 'nested'], 'compositor', env)
        wait(lambda: request('/api/state'))
        records = wait(lambda: list(runtime.glob('*.dome-session.json')))
        record = next(p for p in records if json.loads(p.read_text())['realm'] == 'desktop')
        env['WAYLAND_DISPLAY'] = str(record).removesuffix('.dome-session.json')
        env['XDG_CURRENT_DESKTOP'] = 'Dome'
        env['FLTK_BACKEND'] = 'wayland'
        env['DE_XR_APPEARANCE_TRACE'] = '1'
        app = start([os.environ.get('FLTUBE_BINARY', str(ROOT / 'build-release/fltube')), '--debug'], 'fltube', env)
        service = wait(lambda: owner(app.pid))
        print('MENU SERVICE', service, flush=True)
        def menu(method, sig='()', args=()):
            return call(service, '/org/de_xr/FLTK/Menu1', 'com.canonical.dbusmenu', method, sig, args)
        def layout(): return menu('GetLayout', '(iias)', (0, -1, []))[1]
        native = layout()
        (ART / 'layout.json').write_text(json.dumps(native, indent=2))
        assert len(native[2]) >= 4
        def find(label): return next(n for n in flatten(layout()) if n[1].get('label') == label)
        assert find('Quit')[1]['shortcut'] == [['Control', 'Q']]
        assert find('Search YouTube or open URL')[1]['shortcut'] == [['Control', 'L']]
        assert not find('Dark Theme')[1]['enabled']
        assert find('Automatic')[1]['toggle-state'] == 1
        def click(label):
            menu('Event', '(isvu)', (find(label)[0], 'clicked', GLib.Variant('i', 0), 0))
        xd = xdisplay.Display(display)
        host = wait(lambda: next((w for w in xd.screen().root.query_tree().children
                    if str(w.get_wm_name() or '').startswith('Dome')), None))
        host.set_input_focus(X.RevertToParent, X.CurrentTime); xd.sync()
        def pointer_click(x, y):
            pos = xd.screen().root.translate_coords(host, 0, 0)
            xtest.fake_input(xd, X.MotionNotify, x=pos.x + x, y=pos.y + y); xd.sync()
            for event in (X.ButtonPress, X.ButtonRelease):
                xtest.fake_input(xd, event, 1); xd.sync(); time.sleep(.1)
        # Select Full SBS through the actual DomeD universal menu.
        time.sleep(.6)
        pointer_click(450, 18); time.sleep(.3); pointer_click(450, 78)
        wait(lambda: find('Full SBS')[1]['toggle-state'] == 1)
        click('Half top-bottom')
        wait(lambda: find('Half top-bottom')[1]['toggle-state'] == 1)
        assert find('Automatic')[1]['toggle-state'] == 0
        # Exercise an existing FLUID static command as well as the new menu.
        click('240p')
        assert find("240p")[1]["toggle-state"] == 1
        def capture(name):
            from PIL import Image
            time.sleep(.7)
            geo = host.get_geometry()
            raw = host.get_image(0, 0, geo.width, geo.height, X.ZPixmap, 0xffffffff)
            image = Image.frombytes('RGB', (geo.width,geo.height), raw.data, 'raw', 'BGRX')
            image.save(ART / (name + '.png'))
            return image
        dark = capture('dark')
        appearance = runtime / '.config/dome/appearance.json'
        appearance.parent.mkdir(parents=True, exist_ok=True)
        appearance.write_text(json.dumps(dict(light=True, accent='#ff6633', font='Unifont / Sans-Serif', font_size=16)))
        time.sleep(1)
        light = capture('light')
        from PIL import ImageChops
        assert ImageChops.difference(dark, light).getbbox(), 'No live appearance update'
        assert 'DX_STYLE' in (ART / 'fltube.log').read_text()
        # Close the app through its exported command, including cleanup.
        try: click('Quit')
        except GLib.Error: pass # Quit can close the bus before replying.
        assert app.wait(timeout=5) == 0
        assert 'XR_VIDEO_LAYOUT=4' in (runtime / '.config/fltube/app.conf').read_text()
        # Reopen to verify persisted layout and the real Ctrl+Q keyboard path.
        app = start([os.environ.get('FLTUBE_BINARY', str(ROOT / 'build-release/fltube'))], 'reopened', env)
        service = wait(lambda: owner(app.pid))
        wait(lambda: find('Half top-bottom')[1]['toggle-state'] == 1)
        pointer_click(250, 160)
        from Xlib import XK
        ctrl = xd.keysym_to_keycode(XK.string_to_keysym('Control_L'))
        quit_key = xd.keysym_to_keycode(XK.string_to_keysym('q'))
        for code in (ctrl, quit_key): xtest.fake_input(xd, X.KeyPress, code); xd.sync()
        for code in (quit_key, ctrl): xtest.fake_input(xd, X.KeyRelease, code); xd.sync()
        assert app.wait(timeout=5) == 0
        report = dict(universal_menu_mouse=True, keyboard_quit=True, persisted_layout=True, native_wayland_menu=True, nested_options=True, live_radio=True,
                      shortcuts=True, system_appearance=True, live_appearance=True, clean_quit=True)
        (ART / 'report.json').write_text(json.dumps(report, indent=2))
        print(json.dumps(report))
        xd.close()
finally:
    for child in reversed(children):
        if child.poll() is None:
            child.terminate()
            try: child.wait(timeout=5)
            except subprocess.TimeoutExpired: child.kill(); child.wait()
    for log in logs: log.close()
