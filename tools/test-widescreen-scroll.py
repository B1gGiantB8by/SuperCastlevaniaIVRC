"""Stage 1 moving-camera regression and real-snapshot hold-rewind smoke test."""
from pathlib import Path
import os, re, struct, subprocess
from PIL import Image, ImageChops

root=Path(__file__).resolve().parents[1]
out=root/'build/widescreen-scroll-tests'
out.mkdir(exist_ok=True)
config=out/'test.ini'
config.write_text('[General]\nAutosave = 0\n[Graphics]\nFullscreen = 0\n'
                  'OutputMethod = SDL\nNewRenderer = 1\nVSync = 0\n[Sound]\nEnableAudio = 0\n')
script=out/'scroll.txt'
script.write_text('turbo on\nuntil16 0070 == 0005 2500\nwait 60\ndump start\n'
                  'press right 60\ndump move1\npress right+b 60\ndump move2\n'
                  'press right 180\nturbo off\nwait 2\ndump end\nwait 4\nquit\n')
word=lambda data,a:struct.unpack_from('<H',data,a)[0]
def room_tile(r,layer,x,y):
    scene,blocks=(0x8000,0x6000) if layer==0 else (0xe000,0xc000)
    block=word(r,scene+((y//256*16+x//256)*64+y%256//32*8+x%256//32)*2)
    tx,ty=x%32//8,y%32//8
    if block&0x8000:tx=3-tx
    if block&0x4000:ty=3-ty
    return word(r,blocks+(block&255)*32+ty*8+tx*2)^((block&0x8000)>>1)^((block&0x4000)<<1)

def run(name,extra,rewind=False):
    dest=out/name;dest.mkdir(exist_ok=True)
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',SNESRECOMP_WS_EXTRA=str(extra),
             SNESRECOMP_DUMP_DIR=str(dest),SNESRECOMP_SCREENSHOT=str(dest/'frame.ppm'),
             SNESRECOMP_SCREENSHOT_FRAME='1344')
    if rewind:env['SNESRECOMP_HOLD_REWIND_SELFTEST']='1200'
    else:env.pop('SNESRECOMP_HOLD_REWIND_SELFTEST',None)
    with (dest/'run.log').open('w') as log:
        subprocess.run([str(root/'build/SuperCastlevaniaIvSNESRecomp.exe'),'--no-launcher',
                        '--widescreen' if extra else '--no-widescreen','--level-select=1',
                        '--config',str(config),'--script',str(script),
                        str(root.parent/'Super Castlevania IV (USA).sfc')],
                       cwd=root,env=env,stdout=log,stderr=log,check=True,timeout=30)
    if rewind:
        text=(dest/'run.log').read_text()
        match=re.search(r'hold_rewind_selftest\] width=(\d+) steps=(\d+) restored_frame=(\d+)',text)
        assert match and tuple(map(int,match.groups()))==(256+2*extra,20,1143),text
        assert word((dest/'end.wram.bin').read_bytes(),0x1c)<word((out/'native/end.wram.bin').read_bytes(),0x1c)
        print(name,': 20 real snapshots restored; camera moved backwards; gameplay resumed',flush=True)
        return
    im=Image.open(dest/'frame.ppm').convert('RGB');im.save(dest/'frame.png')
    assert im.size==(256+2*extra,224)
    if extra:
        native=Image.open(out/'native/frame.ppm').convert('RGB')
        assert ImageChops.difference(native,im.crop((extra,0,extra+256,224))).getbbox() is None
        for tag in ['start','move1','move2','end']:
            assert (dest/(tag+'.wram.bin')).read_bytes()==(out/'native'/(tag+'.wram.bin')).read_bytes()
        # Independently verify decoded room entries against the tiles actually
        # DMA'd for the native scrolling camera, on both scenery layers.
        for tag in ['start','move1','move2','end']:
            r=(dest/(tag+'.wram.bin')).read_bytes();v=(dest/(tag+'.vram.bin')).read_bytes()
            camera=word(r,0x1c)
            import json
            regs=json.loads((dest/(tag+'.regs.json')).read_text())
            for layer in range(2):
                scroll=regs['hscroll'][layer];base=(regs['bgsc'][layer]&252)<<8
                for y in range(32,224,8):
                    for sx in range(0,256,8):
                        x=(scroll&1023)+sx
                        addr=base+(y//8&31)*32+(x//8&31)+(1024 if x&256 else 0)
                        assert room_tile(r,layer,x%512 if layer else x,y)==word(v,(addr&32767)*2),(tag,layer,x,y)
        print(name,': moving center/WRAM preserved; room tiles match native DMA on both layers',flush=True)
    return im

run('native',0)
run('wide',43)
run('ultrawide',272)
run('rewind-wide',43,True)
run('rewind-ultrawide',272,True)

# The drawbridge approach mixes periodic BG2 cliffs with a world-addressed
# ledge strip. Check it before/after crossing the native streaming boundary.
bridge_script=out/'bridge.txt'
bridge_script.write_text('turbo on\nuntil16 0070 == 0005 2500\nwait 60\n'
    'press right 60\npress right+b 60\npress right 180\nturbo off\n'
    'press right 60\ndump before\npress right 20\ndump ledge\n'
    'press right+b 60\ndump bridge\npress right 80\ndump end\nquit\n')
for name,extra in [('bridge-native',0),('bridge-wide',43),('bridge-ultrawide',272)]:
    dest=out/name;dest.mkdir(exist_ok=True)
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',SNESRECOMP_WS_EXTRA=str(extra),
        SNESRECOMP_DUMP_DIR=str(dest),SNESRECOMP_SCREENSHOT=str(dest/'frame.ppm'),
        SNESRECOMP_SCREENSHOT_FRAME='1404',SCIV_WIDESCREEN_CAMERA_TRACE='1')
    env.pop('SNESRECOMP_HOLD_REWIND_SELFTEST',None)
    with (dest/'run.log').open('w') as log:
        subprocess.run([str(root/'build/SuperCastlevaniaIvSNESRecomp.exe'),'--no-launcher',
            '--widescreen' if extra else '--no-widescreen','--level-select=1','--config',str(config),
            '--script',str(bridge_script),str(root.parent/'Super Castlevania IV (USA).sfc')],
            cwd=root,env=env,stdout=log,stderr=log,check=True,timeout=30)
    im=Image.open(dest/'frame.ppm').convert('RGB');im.save(dest/'frame.png')
    if not extra:continue
    native=Image.open(out/'bridge-native/frame.ppm').convert('RGB')
    assert ImageChops.difference(native,im.crop((extra,0,extra+256,224))).getbbox() is None
    for tag in ['before','ledge','bridge','end']:
        assert (dest/(tag+'.wram.bin')).read_bytes()==(out/'bridge-native'/(tag+'.wram.bin')).read_bytes()
        import json
        r=(dest/(tag+'.wram.bin')).read_bytes();v=(dest/(tag+'.vram.bin')).read_bytes()
        regs=json.loads((dest/(tag+'.regs.json')).read_text())
        for y in range(32,224,8):
            for sx in range(0,256,8):
                x=regs['hscroll'][1]+sx
                addr=((regs['bgsc'][1]&252)<<8)+(y//8&31)*32+(x//8&31)+(1024 if x&256 else 0)
                assert room_tile(r,1,x%512 if y<192 else x,y)==word(v,(addr&32767)*2),(tag,x,y)
    print(name,': native gameplay/pixels preserved; distant backdrop and ledge use their correct coordinate spaces',flush=True)

# Test fixture: hold Simon above the pit to reach the real drawbridge camera
# lock without requiring a manually timed jump. Both views receive identical
# fixture writes; the normal guest still loads/animates the room and bridge.
edge_script=out/'edge.txt'
edge_script.write_text('turbo on\nuntil16 0070 == 0005 2500\nwait 60\n'
    'press right 60\npress right+b 60\npress right 260\npress right+b 60\n'
    'press right 80\nforcepoke 054e 9000\nforcepoke 0556 0000\nforcepoke 13f4 1000\n'
    'press right 160\ndump bridge\npress right 60\nturbo off\nwait 1\ndump end\nwait 3\nquit\n')
for name,extra in [('edge-native',0),('edge-wide',43),('edge-ultrawide',272)]:
    dest=out/name;dest.mkdir(exist_ok=True)
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',SNESRECOMP_WS_EXTRA=str(extra),
        SNESRECOMP_DUMP_DIR=str(dest),SNESRECOMP_SCREENSHOT=str(dest/'frame.ppm'),
        SNESRECOMP_SCREENSHOT_FRAME='1793',SCIV_WIDESCREEN_CAMERA_TRACE='1')
    env.pop('SNESRECOMP_HOLD_REWIND_SELFTEST',None)
    with (dest/'run.log').open('w') as log:
        subprocess.run([str(root/'build/SuperCastlevaniaIvSNESRecomp.exe'),'--no-launcher',
            '--widescreen' if extra else '--no-widescreen','--level-select=1','--config',str(config),
            '--script',str(edge_script),str(root.parent/'Super Castlevania IV (USA).sfc')],
            cwd=root,env=env,stdout=log,stderr=log,check=True,timeout=30)
    im=Image.open(dest/'frame.ppm').convert('RGB');im.save(dest/'frame.png')
    r=(dest/'end.wram.bin').read_bytes()
    assert word(r,0x1c)==768 and word(r,0xa0)==word(r,0xa2)==768
    if not extra:continue
    assert r==(out/'edge-native/end.wram.bin').read_bytes()
    native=Image.open(out/'edge-native/frame.ppm').convert('RGB')
    assert ImageChops.difference(native.crop((0,0,256,32)),im.crop((extra,0,extra+256,32))).getbbox() is None
    assert ImageChops.difference(native.crop((0,32,256,224)),im.crop((2*extra,32,2*extra+256,224))).getbbox() is None
    assert re.search(rf'\[wide-view\] frame=1793 camera=768 offset={2*extra} first=0 last=768', (dest/'run.log').read_text())
    print(name,': drawbridge lock keeps the view at the right edge; Mode 7 and all native sprites preserved',flush=True)
