#include "SharedStruct.h"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/process.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/time_clock.hpp>
#include <iostream>
#include <thread>
#include <opencv2/opencv.hpp>
namespace bip = boost::interprocess;

class ParentProcess
{
    // const char* shm_name = "MySharedMemory";
    // std::string shm_sufix_name(const char* name){return std::string(shm_name)+"_"+name;}
    // #define SHM_SUFIX_NAME(name) shm_sufix_name(name).c_str()
#define shm_name "MySharedMemory"
#define SHM_SUFIX_NAME(name) shm_name#name
public:
    ParentProcess()
    {
    }

    ~ParentProcess()
    {
        *quit = true;
        heartbeat_thread.join();
        child.detach();
        bi::shared_memory_object::remove(shm_name);
    }

    void init()
    {
        shm = create_shared_memory(shm_name, 640*480*10);
        std::cout<<"shm size:"<<shm.get_size()<<std::endl;
        if(shm.get_size() < 640*480*10){
           throw std::runtime_error("Shared memory is too small.");
        }
        // 创建分配器
        SharedImage::ShmemAllocator allocator(shm.get_segment_manager());
        // 在共享内存中构造 Image 对象
        shared_image = shm.construct<SharedImage>(SHM_SUFIX_NAME(Image))(allocator);
        quit = shm.construct<bool>(SHM_SUFIX_NAME(quit))();
        timestamp_parent = shm.construct<long long>(SHM_SUFIX_NAME(heartbeat_parent))();
        timestamp_child = shm.construct<long long>(SHM_SUFIX_NAME(heartbeat_child))();
        
        *timestamp_parent = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        *timestamp_child = 0;
        last_start_timestamp=0;
        *quit = false;
        //start_child_process();
        heartbeat_thread = std::move(std::thread(&ParentProcess::heartbeat, this));

    }
    void run(int argc, char* argv[])
    {
        try
        {

            std::vector<std::string> image_files;
            cv::glob(argv[1], image_files);
            // Read the images from the file system
            for (auto &file : image_files)
            {
                cv::Mat img = cv::imread(file);
                if (img.empty())
                    continue;
                std::cout << "read image from file: " << file << std::endl;
                while(!child.running())
                {
                    //std::cout<<"wait for child process run"<<std::endl;
                    std::this_thread::yield();
                }
                process(img);
          
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
    void process(cv::Mat img)
    {
        TickCount tic;
        std::cout << "send data to child process" << std::endl;
        shared_image->imgWidth = img.cols;
        shared_image->imgHeight = img.rows;
        shared_image->imgChannels = img.channels();
        shared_image->imgStep = img.step;
        shared_image->data.assign(img.data, img.data + img.total() * img.elemSize());
        shared_image->used = false;
        std::cout << "Waiting for child process" << std::endl;

        static int count = 0;
        static int timeout_count=0;
        std::cout << "wait" << std::endl;
        bool timeout_flag = !my_timed_wait(construct_timeout_seconds(2), [&]()
                                           { return shared_image->used || *quit; }); // Wait until the condition variable is notified
        if (!timeout_flag){
            count++; 
            timeout_count=0;

        }
            
        else{
            timeout_count++;
            if(timeout_count > 2){
                stop_child_process();
               
            }
        }
        std::cout <<"time:"<<tic.elapsed()<<"ms"<<std::endl;
        std::cout << "child process done" << std::endl;
        std::cout << "count: " << count << std::endl;
    }
private:

    void heartbeat()
    {
        while (!*quit)
        {
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (now - *timestamp_child > 3000 && check_start_timestamp())
            {
                // 子进程崩溃
                printf("child process crash\n");
                start_child_process();
                //wait for child process to start
                //std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            }
            *timestamp_parent = now;
            std::this_thread::yield();
        }
    }
    void start_child_process()
    {
       if(!check_start_timestamp()) return;
       if(child.running()) child.terminate();
       std::cout<<"start child process"<<std::endl;
       child = boost::process::child("child.exe");   
       std::cout<<"child pid:"<<child.id()<<std::endl; 
       std::cout<<"start child process end"<<std::endl;  
       last_start_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
    void stop_child_process()
    {
       child.terminate();
    }
    bool check_start_timestamp()
    {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (now - last_start_timestamp < 5000)
            return false;
        return true;
    }

private:
    bip::managed_shared_memory shm;
    std::thread heartbeat_thread;
    SharedImage *shared_image;
    boost::process::child child;
    long long *timestamp_parent;
    long long *timestamp_child;
    bool *quit;
    long long last_start_timestamp;
};

int main(int argc, char* argv[]) 
{   
    try{
        boost::process::child killchild("killchild.bat");
        killchild.wait();
        ParentProcess parent;
        parent.init();        
        parent.run(argc, argv);
        
    }catch(const std::exception& e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }
   
    return 0;
}
