#include <opencv2/core.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <string>
namespace bi = boost::interprocess;
namespace bip = boost::interprocess;
// 使用共享内存分配器定义的 vector 来存储图像数据
typedef bip::allocator<unsigned char, bip::managed_shared_memory::segment_manager> ShmemAllocator;
typedef bip::vector<unsigned char, ShmemAllocator> ShmemVector;
struct SharedImage
{    
    int imgWidth;
    int imgHeight;
    int imgChannels;
    int imgStep;
    bool used;
    ShmemVector data;

    // 构造函数
    SharedImage(const ShmemAllocator& allocator) : data(allocator) {}
};




class TickCount
{
    public:
    TickCount() : start_time_(boost::posix_time::microsec_clock::universal_time()) {}
    double elapsed() const {
        return (boost::posix_time::microsec_clock::universal_time() - start_time_).total_microseconds()/1000.0;
    }
    void reset() {
        start_time_ = boost::posix_time::microsec_clock::universal_time();
    }
    private:
    boost::posix_time::ptime start_time_;
};

static boost::posix_time::ptime construct_timeout_seconds(int seconds)
{

    return boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(seconds);
}

static bool my_timed_wait(boost::posix_time::ptime t,std::function<bool()> pred)
{
       
        while(!pred())
        {           
            if(boost::posix_time::second_clock::universal_time() > t)
            {
                return false;
            }
            std::this_thread::yield();
        }
    
        return pred();
}

static bip::managed_shared_memory create_shared_memory(std::string shm_name, size_t size) {
    try {

        // Check if the shared memory already exists
        bool shm_exists = false;
        try {
            bi::shared_memory_object shm(bi::open_only, shm_name.c_str(), bi::read_write);
            shm_exists = true;
        } catch (const bi::interprocess_exception& e) {
            // Shared memory does not exist
            shm_exists = false;
        }

        // If shared memory exists, remove it
        if (shm_exists) {
            std::cout << "Shared memory already exists. Removing it..." << std::endl;
            bi::shared_memory_object::remove(shm_name.c_str());
        }

        // Create new shared memory        
        bip::managed_shared_memory shm(bi::create_only, shm_name.c_str(), size);
        
        std::cout << "Shared memory created successfully." << std::endl;
        return shm;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return bip::managed_shared_memory();
}