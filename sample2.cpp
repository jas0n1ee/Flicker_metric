#include <iostream>
#include "opencv2/opencv.hpp"
#include "opencv2/video.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <pthread.h>
#include <vector>
#include <algorithm>
using namespace std;
using namespace cv;
#define nodisplay
void display(Mat * t)
{
#ifndef nodisplay
    Mat rgb;
    cvtColor(*t,rgb,CV_YUV2BGR_I420);
    imshow("snapshot", rgb);
    cvWaitKey(500);
#endif
}
int main(int argc, char* argv[])
{
    FILE *ori, *rec;
    vector<Mat *> ori_yuv;
    vector<Mat *> rec_yuv;
    if(argc < 5)
    {
        cout<<"./sample2 ori.yuv rec.yuv w h keyint\n";
        exit(1);
    }
    
    ori = fopen(argv[1], "rb");
    rec = fopen(argv[2], "rb");
    
    int w = atoi(argv[3]);
    int h = atoi(argv[4]);
    int keyint = atoi(argv[5]);
    
    int ori_cnt=0, rec_cnt=0;
    while (1)
    {
        Mat *t_ori = new Mat(h*3/2,w,CV_8UC1);
        int ok = fread(t_ori->data,sizeof(uint8_t),w*h*3/2,ori);
        if (!ok)
        {
            break;
        }
        ori_cnt++;
        ori_yuv.push_back(t_ori);
    }
    while (1)
    {
        Mat *t = new Mat(h*3/2,w,CV_8UC1);
        int ok = fread(t->data,sizeof(uint8_t),w*h*3/2,rec);
        if (!ok)
        {
            break;
        }
        rec_cnt++;
        rec_yuv.push_back(t);
    }
    for(int keyframe = keyint; keyframe < ori_yuv.size(); keyframe += keyint)
    {
        display(ori_yuv[keyframe]);
        for(int x = 0; x < floor(w/8.0); x++)
        {
            for(int y = 0; y < floor(h/8.0); y++)
            {
                int s_cnt = 0;
                Rect block(Point(x*8,y*8),Point(MIN(x*8+8,w),MIN(y*8+8,h)));
                Mat ref = (*ori_yuv[keyframe])(block);
                for( int i = keyframe-keyint+1; i < keyframe; i++)
                {
                    Mat diff =abs((*ori_yuv[i])(block) - ref);
                    Scalar t = mean(diff);
                    if (t(0) > 20) { break; continue;}
                    else if (t(0) < 5) s_cnt++;
//                    cout << t(0) <<endl;
                }
                if (s_cnt > keyint/3*2)
                {
                    //replace oriyuv
                    for (int j = x*8; j < MIN(x*8+8,w); j++)
                        for (int k = y*8; k < MIN(y*8+8,h); k++)
                            ori_yuv[keyframe]->data[k*w + j] = rec_yuv[keyframe/keyint-1]->data[k*w + j];
                    //TODO : maybe need to add U,V plane
#ifndef nodisplay
                    rectangle(*ori_yuv[keyframe],block, Scalar(255,255,255));
#endif
                }
            }
        }
        display(ori_yuv[keyframe]);
    }
    FILE *out=fopen(argv[6], "wb");
    for (int i = 0; i < ori_yuv.size(); i++)
    {
        Mat *t = ori_yuv[i];
        fwrite(t->data,sizeof(uint8_t),w*h*3/2,out);
        t->release();
        delete t;
    }
    for (int i = 0; i < rec_yuv.size(); i++)
    {
        Mat *t = rec_yuv[i];
        t->release();
        delete t;
    }
    ori_yuv.clear();
    rec_yuv.clear();
}




