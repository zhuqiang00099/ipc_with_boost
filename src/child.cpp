#include "SharedStruct.h"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/time_clock.hpp>
#include <iostream>
#include <thread>
#include <opencv2/opencv.hpp>
namespace bip = boost::interprocess;

class ChildProcess
{
#define shm_name "MySharedMemory"
#define SHM_SUFIX_NAME(name) shm_name#name
public:
    ChildProcess() {}
    ~ChildProcess() 
    {
        heartbeat_thread.join();
    }
    void init()
    {
        try{
            // bool* key = nullptr;
            // if(*key == 0) *key=1; // 模拟异常退出
            shm = bip::managed_shared_memory(bip::open_only, shm_name);
            quit = shm.find<bool>(SHM_SUFIX_NAME(quit)).first;
            timestamp_parent = shm.find<long long>(SHM_SUFIX_NAME(heartbeat_parent)).first;
            timestamp_child = shm.find<long long>(SHM_SUFIX_NAME(heartbeat_child)).first;
            image = shm.find<SharedImage>(SHM_SUFIX_NAME(Image)).first;  
            heartbeat_thread = std::move(std::thread(&ChildProcess::heartbeat, this)); // 启动心跳线程
        }catch(bip::interprocess_exception &e){
            std::cerr << "Error: " << e.what() << std::endl;
            exit(1);
        }catch(...){
            std::cerr << "Unknown error" << std::endl;
            exit(1);
        }
   
    }
    void run()
    {
        cv::Mat img;

        while (*quit == false)
        {

            //std::cout << "Waiting for task" << std::endl;
            auto timeout = construct_timeout_seconds(2);
            my_timed_wait(timeout, [&]()
                          { return *quit == true || image->used == false; }); // Wait until the condition variable is notified
            if (*quit)
                break;
            if (image->used == true)
                continue;

            //std::cout << "get task" << std::endl;

            img = cv::Mat(image->imgHeight, image->imgWidth, CV_8UC3, image->data.data(), image->imgStep).clone();
            image->used = true;
            //printf("Image size: %d x %d\n", img.cols, img.rows);
            // cond->notify_all();

            cv::imshow("image", img);
            cv::waitKey(100);
            //static int c=0;
            //++c;
            //if(c % 10 == 0)
            //{
                //int* key=nullptr;
                //*key=1;//模拟异常退出
                // while(true){
                //      std::this_thread::yield(); //模拟卡死
                // }
            //}
        }
    }
    void heartbeat()
    {
        while (!*quit)
        {
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (now - *timestamp_parent > 3000)
            {
                // 父进程退出或崩溃，退出子进程
                printf("parent process crash, exit\n");
                exit(0);
            }
            *timestamp_child = now;
            std::this_thread::yield();
        }
    }

private:
    bip::managed_shared_memory shm;
    std::thread heartbeat_thread;
    bool *quit;
    long long *timestamp_parent;
    long long *timestamp_child;
    SharedImage *image;
};

int main() {
     ChildProcess child;
     child.init();
     child.run();

    //std::cout<<"i am child process"<<std::endl;

    return 0;
}
