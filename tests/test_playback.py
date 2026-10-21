"""Exercise the actual C++ spawn path with URL metacharacters and player errors."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    player = root / 'mpvr'
    output = root / 'args.json'
    player.write_text('#!/usr/bin/python3\nimport json,os,sys\nopen(os.environ["ARGS_OUT"],"w").write(json.dumps(sys.argv[1:]))\nsys.exit(int(os.environ.get("PLAYER_EXIT",0)))\n')
    player.chmod(0o755)
    env = dict(os.environ, ARGS_OUT=str(output))
    url = 'https://www.youtube.com/watch?v=abc&label="ü";$(touch injected)'
    command = [sys.argv[1], str(player), '--title="A film" --pause', url, '/tmp/custom-yt-dlp']
    assert subprocess.run(command, env=env, cwd=root).returncode == 0
    args = json.loads(output.read_text())
    assert args[:2] == ['--title=A film', '--pause']
    assert args[-2:] == ['--', url]
    assert '--sbs-layout=half-tb' in args
    assert '--script-opts-append=ytdl_hook-ytdl_path=/tmp/custom-yt-dlp' in args
    assert not (root / 'injected').exists()
    assert subprocess.run(command, env=dict(env, PLAYER_EXIT='7')).returncode == 7
    command[2] = '$(touch injected)'
    assert subprocess.run(command, env=env, cwd=root).returncode != 0
    assert not (root / 'injected').exists()
print('PASS: lossless URLs, options, layout, yt-dlp selection, exit status, no shell evaluation')
