#!/usr/bin/env python
import sys
import os
'''
Example:
python replace.py FourPeople_1280x720_60.yuv 1280 720 30
'''
w = int(sys.argv[2])
h = int(sys.argv[3])
keyint = int(sys.argv[4])
name = sys.argv[1].split('.')[0]
cnt = 0;
ori_frame = [];
with open(sys.argv[1],'rb') as f:
    while 1:
        frame = f.read(w*h*3/2)
        if len(frame) == 0: break
        ori_frame.append(frame)
        cnt += 1
print "Total Frame:" + cnt.__str__()

temp_name = []
for i in range(keyint,cnt,keyint):
    temp_name.append(name + '_' + (i - keyint).__str__() + '_' + i.__str__() + '.yuv')
    with open(name + '_' + (i - keyint).__str__() + '_' + i.__str__() + '.yuv','wb')as out:
        out.write(ori_frame[i-keyint])
        out.write(ori_frame[i])

print temp_name
if keyint == 30:
    bframe = 4
for qp in [37, 42]:
    rec_frame=[]
    for clip in temp_name:
        os.system('x265 --input %s --input-res %dx%d --qp %d --fps 2 --keyint %d -p medium --no-scenecut --b-adapt 0 --bframes %d -o %s_%d.h265'%( clip,\
                              w, h,
                              qp, keyint,
                              bframe,
                              clip.split('.')[0], qp))
        os.system('ffmpeg -i %s_%d.h265 -vsync 0 -y %s_rec_%d.yuv'%(clip.split('.')[0],qp,\
                                                                    clip.split('.')[0],qp))
        with open('%s_rec_%d.yuv'%(clip.split('.')[0],qp),'rb') as f:
            f.read(w*h*3/2)
            frame = f.read(w*h*3/2)
            rec_frame.append(frame)
    with open('%s_qp%drec.yuv'%(name.split('_')[0],qp),'wb') as out:
        for frame in rec_frame:
            out.write(frame)
    os.system('./unix_build/sample2 %s.yuv %s_qp%drec.yuv %d %d %d %s_qp%dout.yuv'%(name,\
                                    name.split('_')[0],qp,w,h,keyint,name.split('_')[0],qp))
os.system('rm *h265')
os.system('rm %s_*_*'%(name))



