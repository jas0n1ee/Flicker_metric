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
with open(sys.argv[1],'rb') as f:
    while 1:
        frame = f.read(w*h*3/2)
        if len(frame) == 0: break
        cnt += 1
print "Total Frame:" + cnt.__str__()


if keyint == 30:
    bframe = 4
for qp in [37, 42]:
    os.system('cp %s.yuv %s_rec.yuv'%(name,name))
    for i in range(keyint,cnt,keyint):
        ori_frame = [];
        with open('%s_rec.yuv'%(name),'rb') as f:
            while 1:
                frame = f.read(w*h*3/2)
                if len(frame) == 0: break
                ori_frame.append(frame)
        with open('%s_tem.yuv'%(name),'wb') as out:
            out.write(ori_frame[i-keyint]);
            out.write(ori_frame[i]);
        os.system('x265 --input %s_tem.yuv --input-res %dx%d --qp %d --fps 2 -p medium --no-scenecut --b-adapt 0 --bframes 4 -o %s_tem.h265'%(name, w, h, qp, name))
        os.system('ffmpeg -i %s_tem.h265 -vsync 0 -y %s_tem.yuv'%(name,name))
        os.system('./unix_build/sample2 %s_rec.yuv %s_tem.yuv %d %d %d %d %s_rec.yuv'%(name,\
                                name,w,h,keyint,i,name))
    os.system('mv %s_rec.yuv %s_qp%d_rec.yuv'%(name,name,qp))
    os.system('rm *tem.yuv')

os.system('rm *h265')



