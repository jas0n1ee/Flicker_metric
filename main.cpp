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
vector<Mat* >ori_yuv;
vector<Mat* >rec_yuv;

struct JobPack {
    Mat * rgb_ori;
    Mat * mask;
    int id = 0;
    bool operator<(const JobPack& rhs) const {
        return id < rhs.id;
    }
    bool operator>(const JobPack& rhs) const {
        return id > rhs.id;
    }
};
vector<int> job_que;
vector<JobPack> job_finished;
pthread_mutex_t lock_job_que;
pthread_mutex_t lock_finish_que;
pthread_cond_t new_job;
int w=0, h=0;
bool JobPublish = false;
#define block_size 32.0
Size block(block_size,block_size);

void * process(void * arg)
{
    
    int frame = 0;
    /*lock 1*/
    pthread_mutex_lock(&lock_job_que);

    if (job_que.size() < 2)
    {
        pthread_mutex_unlock(&lock_job_que);
        pthread_exit(NULL);
    }
    frame = job_que[1];
    job_que.erase(job_que.begin()+1);
    pthread_mutex_unlock(&lock_job_que);
    
    Mat *rgb_ori = new Mat(h,w,CV_8UC3);
    Mat *mask = new Mat(h,w,CV_8UC1);
    memset(mask->data,0,w*h*sizeof(uint8_t));
    Mat y_ori((*ori_yuv[frame])(Rect(Point(0,0),Point(w,h))));
    Mat y_ori_p((*ori_yuv[frame-1])(Rect(Point(0,0),Point(w,h))));
    Mat y_rec((*rec_yuv[frame])(Rect(Point(0,0),Point(w,h))));
    Mat y_rec_p((*rec_yuv[frame-1])(Rect(Point(0,0),Point(w,h))));
    Mat diff = abs(y_ori - y_ori_p);
    Mat diff_rec = abs(y_rec - y_rec_p);
    for (int x = 0; x < ceil(w/block_size); x++) {
        for (int y = 0; y < ceil(h/block_size); y++) {
            Point tl(x*block_size,y*block_size);
            Rect sq(tl,Point(tl.x+block_size>=w?w-1:tl.x+block_size,
                             tl.y+block_size>=h?h-1:tl.y+block_size));
            Scalar t = mean(diff(sq));
            if (t(0) < 10 ) {
            //TODO : THIS THRESHOLD NEED TO ADJUST
                for (int i = sq.tl().x; i <= sq.br().x; i++) {
                    for (int j = sq.tl().y; j <= sq.br().y; j++) {
                        int d = MAX(0, diff_rec.data[j*w+i] - diff.data[j*w+i]);
                        mask->data[j*w+i] = d;
                    }
                }
            }
        }
    }
    threshold(*mask,*mask,1,255,THRESH_BINARY);
    //threshold(src, dst, threshold, max_value, THRESH_BINARY);
    //TODO: THIS THRESHOLD NEED TO ADJUST
    JobPack t;
    cvtColor(*ori_yuv[frame], *rgb_ori, CV_YUV2BGR_I420);
    t.rgb_ori = rgb_ori;
    t.mask = mask;
    t.id = frame;
    pthread_mutex_lock(&lock_finish_que);
    job_finished.push_back(t);
    pthread_mutex_unlock(&lock_finish_que);
    
    cout<<"finish"<<frame + 1<<endl;
    pthread_exit(NULL);
}
int main(int argc, char* argv[])
{
    FILE *ori, *rec;
    if(argc < 5)
    {
        cout<<"./sample ori.yuv enc.yuv w h\n";
        exit(1);
    }
    pthread_mutex_init(&lock_job_que, NULL);
    pthread_mutex_init(&lock_finish_que, NULL);
    pthread_cond_init(&new_job, NULL);
    
    ori = fopen(argv[1], "rb");
    rec = fopen(argv[2], "rb");
    w = atoi(argv[3]);
    h = atoi(argv[4]);
    int max_frame = -1;
    if(argc == 6)
        max_frame = atoi(argv[5]);
    int thread_num = 4;

    vector<pthread_t *> thread_id;

    int i=0;
    while (1)
    {
        Mat *t_ori = new Mat(h*3/2,w,CV_8UC1);
        Mat *t_rec = new Mat(h*3/2,w,CV_8UC1);
        int ok = fread(t_ori->data,sizeof(uint8_t),w*h*3/2,ori);
        if (!ok)
        {
            cout<<"Failed to open"<<argv[1];
            break;
        }
        ok = fread(t_rec->data,sizeof(uint8_t),w*h*3/2,rec);
        if (!ok)
        {
            cout<<"Failed to open"<<argv[2];
            break;
        }
        ori_yuv.push_back(t_ori);
        rec_yuv.push_back(t_rec);
        pthread_mutex_lock(&lock_job_que);
        job_que.push_back(i++);
        pthread_mutex_unlock(&lock_job_que);
        cout<<i<<endl;
        pthread_t *t = new pthread_t;
        pthread_create(t, NULL, process, NULL);
        thread_id.push_back(t);
        if (i == max_frame) break;
    }
    JobPublish = true;
    for (int i = 0; i < thread_id.size(); i++)
    {
        pthread_join(*thread_id[i], NULL);
    }
    
    sort(job_finished.begin(),job_finished.end());
    
    
    
    
    for (int i = 0; i < MIN(ori_yuv.size(),rec_yuv.size());i++)
    {
        Mat *t = ori_yuv[i];
        delete t;
        t = rec_yuv[i];
        delete t;
    }
    ori_yuv.clear();
    rec_yuv.clear();
    
    VideoWriter wt("mask.mov",wt.fourcc('P', 'I', 'M', '1'),25,Size(w,h),false);

    for (int i = 0; i < job_finished.size();i++)
    {
        if (wt.isOpened()) wt<<*(job_finished[i].mask);
//        imshow("test",*(job_finished[i].mask));
//        char name[20];
//        sprintf(name,"img%d.img",i);
//        imwrite(name,*(job_finished[i].mask));
//        cvWaitKey(1);
    }
    wt.release();
    for (int i = 0; i < job_finished.size();i++)
    {
        Mat *t = (job_finished[i]).rgb_ori;
        if (t)
            delete t;
        t = (job_finished[i]).mask;
        if (t)
            delete t;
    }
    job_finished.clear();
    thread_id.clear();
    
}