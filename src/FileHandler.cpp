// compile with
// g++ -o filehandler FileHandler.cpp -lrt -lpthread
// g++ -o filehandler FileHandler.cpp -lrt -lpthread -lboost_log -lboost_log_setup -lboost_system -lboost_thread -DBOOST_ALL_DYN_LINK
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <fstream>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <string>
// #include "smem.h"
#include <sys/shm.h>
#include <algorithm>
#include <chrono>
#include <thread>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
// #include <boost/log/attributes.hpp>
// #include <boost/log/attributes/timer.hpp>
// #include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/manipulators/to_log.hpp>
// #include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/common.hpp>
#include <boost/version.hpp>
#include <boost/log/attributes/counter.hpp>
#include <boost/log/sinks/sync_frontend.hpp>



namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;
BOOST_LOG_ATTRIBUTE_KEYWORD(a_timestamp, "TimeStamp", boost::log::attributes::local_clock::value_type)
static std::atomic<int> log_counter(0);

#include <chrono>
#include <ctime>
#include <iomanip>

#define SHARED_MEM_CHUNK_SIZE 4095

const char *PipeName = "file_handler_pipe";
const char *BackingFile = "/file_handler_shm";
const char *SemaphoreName = "file_handler_sem";
int chunk_size;
// Function to list files in a directory
void listFiles(const std::string &directory)
{
    // Implement your logic to list files in the given directory
    std::cout << "Listing files in " << directory << ":" << std::endl;
    // // listing file in directory
    // std::string command = "ls " + directory;
    // system(command.c_str());
    // //print the result of  the command
    std::string command = "ls -p " + directory + " | grep -v /"; // List only files (not directories)
    FILE *ls_output = popen(command.c_str(), "r");

    if (ls_output)
    {
        std::cout << "Found files in " << directory << ":" << std::endl;

        char buffer[128];
        while (fgets(buffer, sizeof(buffer), ls_output))
        {
            std::cout << buffer;
        }

        pclose(ls_output);
    }
    else
    {
        std::cerr << "Failed to execute 'ls' command." << std::endl;
    }
}

// Function to read and display content of a file
std::string readFileContent(const std::string &filePath)
{
    // std::cout << "Reading file " << filePath << ":" << std::endl;
    std::cout << "Request: <<<Reading content of file" << filePath << ">>>" << std::endl;
    std::string command = "cat " + filePath;
    FILE *cat_output = popen(command.c_str(), "r");

    if (cat_output)
    {

        std::string file_content;
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), cat_output))
        {
            // std::cout << buffer;
            file_content += buffer;
        }

        pclose(cat_output);
        return file_content;
    }
    else
    {
        std::cerr << "Failed to execute 'cat' command." << std::endl;
        return std::string("FAILED");
    }
}

std::string listFilesToSharedMemory(const std::string &directory)
{
    std::string command = "ls -p " + directory + " | grep -v /"; // List only files (not directories)
    FILE *ls_output = popen(command.c_str(), "r");
    std::cout << "Listing files in " << directory << ":" << std::endl;
    if (ls_output)
    {
        std::string files_list;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), ls_output))
        {
            files_list += buffer;
        }
        pclose(ls_output);

        return files_list;
    }
    else
    {
        std::cerr << "Failed to execute 'ls' command." << std::endl;
        return std::string("FAILED");
    }
}

namespace boost::logging
{
    namespace attributes
    {
        class LocalClock
        {
        public:
            static std::shared_ptr<std::chrono::high_resolution_clock> get()
            {
                // Return a shared pointer to a high resolution clock instance
                return std::make_shared<std::chrono::high_resolution_clock>();
            }
        };
    }
}
void Loggger_init()
{
    logging::add_file_log(logging::keywords::file_name = "sample.log",
                          logging::keywords::auto_flush = true, // Enable auto-flushing
                          logging::keywords::format = "[%TimeStamp%]: %Message%");

    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info);
    // Add a timestamp attribute
    // logging::add_common_attributes();

    // logging::core::get()->add_global_attribute("TimeStamp", boost::logging::attributes::LocalClock::get());
    // auto clock = logging::attributes::local_clock();
}

void init_logging()
{
    // Create a file sink
    typedef sinks::text_file_backend file_backend_t;
    boost::shared_ptr<file_backend_t> file_backend = boost::make_shared<file_backend_t>(
        boost::log::keywords::file_name = "mylog_%N.log",
        boost::log::keywords::rotation_size = 10 * 1024 * 1024, // 10 MB
        boost::log::keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        boost::log::keywords::auto_flush = true);
    boost::shared_ptr<logging::core> core = logging::core::get();
    core->add_sink(boost::make_shared<sinks::synchronous_sink<file_backend_t>>(file_backend));

    // Set the logging filter to log all messages
    core->set_filter(expr::attr<boost::log::trivial::severity_level>("Severity") >= boost::log::trivial::trace);
}

std::string get_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_c);

    std::ostringstream timestamp;
    timestamp << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");

    return timestamp.str() + "." + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);
}
void init_logging2()
{
    // Create a file sink
    typedef sinks::text_file_backend file_backend_t;
    boost::shared_ptr<file_backend_t> file_backend = boost::make_shared<file_backend_t>(
        keywords::file_name = "../logs/run_logs.log",
        keywords::rotation_size = 10 * 1024 * 1024, // 10 MB
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        keywords::auto_flush = true);
    boost::shared_ptr<sinks::synchronous_sink<file_backend_t>> sink = boost::make_shared<sinks::synchronous_sink<file_backend_t>>(file_backend);
        // logging::core::get()->add_attribute("LineCounter", attrs::counter<int>());

    // Add a formatter to include a timestamp in each log message
    logging::formatter formatter = expr::stream
                                //    << "[" << expr::attr<int>("LineCounter")  << "] "
                                //    << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                                //    << "[" << a_timestamp << "] "
                                   << " [" << logging::trivial::severity << "] "
                                   << expr::smessage;

    // logging::formatter formatter = expr::stream
    //                                << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", boost::log::keywords::format = &get_timestamp)
    //                                << " [" << logging::trivial::severity << "] "
    //                                << expr::smessage;
    sink->set_formatter(formatter);
    // logging::add_common_attributes();

    // Add the sink to the logging core
    logging::core::get()->add_sink(sink);

    // Set the logging filter to log all messages
    logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::trace);
}

void init()
{

    logging::add_file_log(
        keywords::file_name = "run_logs%N.log",
        // This makes the sink to write log records that look like this:
        // YYYY-MM-DD HH:MI:SS: <normal> A normal severity message
        // YYYY-MM-DD HH:MI:SS: <error> An error severity message
        keywords::format =
            (expr::stream
             << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
             << ": <" << logging::trivial::severity
             << "> " << expr::smessage));

    // logging::add_common_attributes();
}
int main()
{
    // Loggger_init();
    init_logging2();
    // boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");

    // init();
    BOOST_LOG_TRIVIAL(trace) << "A trace severity message";
    BOOST_LOG_TRIVIAL(debug) << "A debug severity message";

    BOOST_LOG_TRIVIAL(info) << "Program Started";

    BOOST_LOG_TRIVIAL(warning) << "A warning severity message";
    BOOST_LOG_TRIVIAL(error) << "An error severity message";
    BOOST_LOG_TRIVIAL(fatal) << "A fatal severity message";

    char request_buffer[512]; // Adjust buffer size as needed
    ssize_t bytes_read;
    // ssize_t shared_memory_size = 1024;
     auto pipeFlag = mkfifo(PipeName, 0666);
        if (pipeFlag == -1)
        {
            if (errno != EEXIST)
            {
                std::cout << "Failed to open named pipe." << std::endl;
                return 1;
            }
        }
        // open pipe read and write
        //  int fd = open(PipeName, O_RDWR);
        int pipe_fd = open(PipeName, O_RDWR | O_CREAT, 0644);
        if (pipe_fd == -1)
        {
            std::cerr << "Failed to open named pipe for reading." << std::endl;
            return 1;
        }
        // sleep(1);
    while (true)
    {
       
        if ((bytes_read = read(pipe_fd, request_buffer, sizeof(request_buffer))) > 0)
        {
            std::cout << "************New Request************" << std::endl;
            BOOST_LOG_TRIVIAL(info) << "Received request: " << request_buffer;
            std::string request(request_buffer, bytes_read);
            std::istringstream iss(request);
            std::string command, argument;
            // Create shared memory
            int shm_fd = shm_open(BackingFile, O_RDWR | O_CREAT, 0644);
            if (shm_fd < 0)
            {
                perror("Can't open shared mem segment...");
                exit(-1);
            }
            else
            {
                std::cout << "shared mem segment opened successfully" << std::endl;
            }

            ftruncate(shm_fd, SHARED_MEM_CHUNK_SIZE + 1);

            caddr_t memptr = static_cast<caddr_t>(mmap(
                NULL, SHARED_MEM_CHUNK_SIZE + 1, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

            if (memptr == (caddr_t)-1)
            {
                perror("Can't get segment...");
                exit(-1);
            }

            fprintf(stderr, "shared mem address: %p [0..%d]\n", memptr, SHARED_MEM_CHUNK_SIZE);
            fprintf(stderr, "backing file:       /dev/shm%s\n", BackingFile);

            sem_t *semptr = sem_open(SemaphoreName, O_CREAT, 0644, 12);
            if (semptr == (void *)-1)
            {
                perror("sem_open");
                exit(-1);
            }
            int value;
            sem_init(semptr, 1, 0);
            // sem_getvalue(semptr, &value);
            // printf("Semaphore value: %d\n", value);

            if (iss >> command >> argument)
            {
                if (command == "list")
                {

                    auto data = listFilesToSharedMemory(argument);

                    int offset = 0;
                    while (offset < data.size())
                    {

                        // Chunking ************************
                        chunk_size = std::min(SHARED_MEM_CHUNK_SIZE + 1, ((int)data.size() + 1 - offset));
                        auto chunk = data.substr(offset, chunk_size);
                        std::cout << "current chunk" << chunk.c_str() << std::endl;

                        if (chunk_size < SHARED_MEM_CHUNK_SIZE + 1)
                        {
                            chunk[chunk_size - 1] = 'X';
                            std::cout << "**************chunk_size < SHARED_MEM_CHUNK_SIZE + 1***************" << std::endl;
                        }
                        else
                        {
                            chunk[chunk_size - 1] = '1';
                        }
                        strncpy(memptr, chunk.c_str(), chunk_size);
                        // printf("mem: %c\n", chunk[chunk_size - 1]);
                        offset += chunk_size;
                        // printf("offset = %d\n", offset);
                        // printf("chunk_size = %d\n", chunk_size);

                        /* Increment the semaphore so that the reader can read */
                        if (sem_post(semptr) < 0)
                        {
                            perror("sem_post");
                        }
                        sem_getvalue(semptr, &value);
                        printf("Semaphore value new: %d\n", value);
                        // sleep(1);
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                        if (write(pipe_fd, &chunk_size, sizeof(chunk_size)) == -1)
                        {
                            perror("write");
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    }
                    std::cout << "************End Of Request************" << std::endl;
                    // sleep(3);
                    munmap(memptr, SHARED_MEM_CHUNK_SIZE + 1);
                    close(shm_fd);
                    sem_close(semptr);
                    shm_unlink(BackingFile);
                    // close(pipe_fd);
                    std::cout << "************Resources Released************" << std::endl;
                }
                else if (command == "content")
                {
                    auto data = readFileContent(argument);

                    int offset = 0;
                    while (offset < data.size())
                    {

                        // Chunking ************************
                        chunk_size = std::min(SHARED_MEM_CHUNK_SIZE + 1, ((int)data.size() + 1 - offset));
                        auto chunk = data.substr(offset, chunk_size);
                        // std::cout << "current chunk" << chunk.c_str() << std::endl;

                        if (chunk_size < SHARED_MEM_CHUNK_SIZE + 1)
                        {
                            chunk[chunk_size - 1] = 'X';
                            std::cout << "**************chunk_size < SHARED_MEM_CHUNK_SIZE + 1***************" << std::endl;
                        }
                        else
                        {
                            chunk[chunk_size - 1] = '1';
                        }
                        strncpy(memptr, chunk.c_str(), chunk_size);
                        // printf("mem: %c\n", chunk[chunk_size - 1]);
                        offset += chunk_size;
                        // printf("offset = %d\n", offset);
                        // printf("chunk_size = %d\n", chunk_size);

                        /* Increment the semaphore so that the reader can read */
                        if (sem_post(semptr) < 0)
                        {
                            perror("sem_post");
                        }
                        sem_getvalue(semptr, &value);
                        printf("Semaphore value new: %d\n", value);
                        // sleep(2);
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                        if (write(pipe_fd, &chunk_size, sizeof(chunk_size)) == -1)
                        {
                            perror("write");
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    }
                    std::cout << "************End Of Request************" << std::endl;
                    // sleep(1);
                    munmap(memptr, SHARED_MEM_CHUNK_SIZE + 1);
                    close(shm_fd);
                    sem_close(semptr);
                    shm_unlink(BackingFile);
                    // close(pipe_fd);
                    std::cout << "************Resources Released************" << std::endl;
                }
                else
                {
                    std::cerr << "Unknown command: " << command << std::endl;
                }
            }
            else
            {
                std::cerr << "Invalid request format: " << request << std::endl;
            }
        }
        // close(pipe_fd);
    }
    close(pipe_fd);
    return 0;
}
