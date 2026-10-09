"""Headless stage regression: native pixels and guest state must be preserved.

Run with the bundled Python (Pillow required) after build.ps1.
Optional stage numbers allow focused reruns; default checks all 30 entrances.
"""
import os
import re
from pathlib import Path
import subprocess
import sys
from PIL import Image, ImageChops

root = Path(__file__).resolve().parents[1]
out = root / 'build/widescreen-tests'
out.mkdir(parents=True, exist_ok=True)
config = out / 'regression.ini'
config.write_text('[General]\nAutosave = 0\n[Graphics]\nFullscreen = 0\n'
                  'OutputMethod = SDL\nVSync = 0\nNewRenderer = 1\n'
                  '[Sound]\nEnableAudio = 0\n')
script = out / 'regression.txt'
script.write_text('turbo on\nuntil16 0070 == 0005 2500\nwait 90\n'
                  'turbo off\nwait 1\ndump stage\nwait 3\nquit\n')
for stage in map(int, sys.argv[1:] or range(1, 31)):
    images, states = [], []
    capture_frame = 1
    for mode in ('probe', 'native', 'wide'):
        dest = out / f'stage-{stage}-{mode}'
        dest.mkdir(exist_ok=True)
        env = dict(os.environ, SDL_VIDEODRIVER='dummy',
                   SNESRECOMP_DUMP_DIR=str(dest), SNESRECOMP_WS_EXTRA='43',
                   SCIV_WIDESCREEN_CAMERA_TRACE='1',
                   SNESRECOMP_SCREENSHOT=str(dest / 'frame.ppm'),
                   SNESRECOMP_SCREENSHOT_FRAME=str(capture_frame))
        with (dest / 'run.log').open('w') as log:
            subprocess.run([str(root / 'build/SuperCastlevaniaIvSNESRecomp.exe'),
                            '--no-launcher', '--widescreen' if mode == 'wide' else '--no-widescreen',
                            f'--level-select={stage}', '--config', str(config),
                            '--script', str(script), str(root.parent / 'Super Castlevania IV (USA).sfc')],
                           cwd=root, env=env, stdout=log, stderr=log, check=True, timeout=30)
        if mode == 'probe':
            capture_frame = int(re.search(r'script f=(\d+) dump stage ok',
                                          (dest / 'run.log').read_text()).group(1))
            continue
        image = Image.open(dest / 'frame.ppm').convert('RGB')
        image.save(dest / 'frame.png')
        assert image.size == (342 if mode == 'wide' else 256, 224), image.size
        images.append(image)
        states.append((dest / 'stage.wram.bin').read_bytes())
    assert states[0] == states[1], f'stage {stage}: guest WRAM changed'
    assert ImageChops.difference(images[0].crop((0,0,256,32)),images[1].crop((43,0,299,32))).getbbox() is None, f'stage {stage}: HUD changed'
    trace=(out/f'stage-{stage}-wide/run.log').read_text()
    match=re.search(rf'\[wide-view\] frame={capture_frame} camera=\d+ offset=(\d+)',trace)
    offset=int(match.group(1)) if match else 43
    count=min(256,342-offset)
    difference=ImageChops.difference(images[0].crop((0,32,count,224)),images[1].crop((offset,32,offset+count,224)))
    assert difference.getbbox() is None, f'stage {stage}: shifted native field changed at offset {offset}: {difference.getbbox()}'
    print(f'Stage {stage}: guest WRAM and HUD and repositioned native pixels match', flush=True)
