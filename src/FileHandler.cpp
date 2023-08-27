// compile with
// g++ -o filehandler FileHandler.cpp -lrt -lpthread
// g++ -o filehandler FileHandler.cpp -lrt -lpthread -lboost_log -lboost_log_setup -lboost_system -lboost_thread -DBOOST_ALL_DYN_LINK
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <semaphore.h>
#include <sstream>
#include <string>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include <boost/version.hpp>

#include <boost/log/utility/manipulators/to_log.hpp>

#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/core.hpp>
#include <boost/log/common.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/counter.hpp>
#include <boost/log/attributes.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

BOOST_LOG_ATTRIBUTE_KEYWORD(a_timestamp, "TimeStamp", boost::log::attributes::local_clock::value_type)
static std::atomic<int> log_counter(0);

#define SHARED_MEM_CHUNK_SIZE 4095

const char *PipeName = "file_handler_pipe";
const char *BackingFile = "/file_handler_shm";
const char *SemaphoreName = "file_handler_sem";
int chunk_size;
static std::atomic<unsigned int> n_requests{1};

inline std::string get_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_c);

    std::ostringstream timestamp;
    timestamp << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");

    return timestamp.str() + "." + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);
}

inline std::string message(std::string const &s)
{
    // return "[ " + std::to_string(log_counter++) + " ] " + "[" + get_timestamp() + "] " + s;
    std::ostringstream oss;
    oss << "[" << std::setfill('0') << std::setw(4) << log_counter++ << "] [" << get_timestamp() << "] " << s;
    return oss.str();
}
// Function to read and display content of a file
std::string readFileContent(const std::string &filePath)
{
    // std::cout << "Reading file " << filePath << ":" << std::endl;
    BOOST_LOG_TRIVIAL(info) << message("Request: Reading content of file: ") << filePath;
    BOOST_LOG_TRIVIAL(info) << message("Trying to excute cat command");
    std::cout << "Request: Reading content of file: " << filePath << std::endl;
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
    {   BOOST_LOG_TRIVIAL(error) << message("Failed to execute 'cat' command.");
        std::cerr << "Failed to execute 'cat' command." << std::endl;
        return std::string("FAILED");
    }
}

std::string listFileInDirectory(const std::string &directory)
{
    BOOST_LOG_TRIVIAL(info) << message("Request: Listing files in directory: ") << directory;
    BOOST_LOG_TRIVIAL(info) << message("Trying to excute ls -p command");
    std::string command = "ls -p " + directory + " | grep -v /"; // List only files (not directories)
    FILE *ls_output = popen(command.c_str(), "r");
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
        BOOST_LOG_TRIVIAL(error) << message("Failed to execute 'ls' command.");
        std::cerr << "Failed to execute 'ls' command." << std::endl;
        return std::string("FAILED");
    }
}

void init_logging()
{
    // Create a file sink
    // allow multiple processes to write to the same file
    typedef sinks::text_file_backend file_backend_t;
    boost::shared_ptr<file_backend_t> file_backend = boost::make_shared<file_backend_t>(
        boost::log::keywords::file_name = "mylog_%N.log",
        boost::log::keywords::open_mode = (std::ios::out | std::ios::app),

        boost::log::keywords::rotation_size = 10 * 1024 * 1024, // 10 MB
        boost::log::keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        boost::log::keywords::auto_flush = true);
    boost::shared_ptr<logging::core> core = logging::core::get();
    core->add_sink(boost::make_shared<sinks::synchronous_sink<file_backend_t>>(file_backend));

    // Set the logging filter to log all messages
    core->set_filter(expr::attr<boost::log::trivial::severity_level>("Severity") >= boost::log::trivial::trace);
}

void init_logging2()
{
    // Create a file sink
    typedef sinks::text_file_backend file_backend_t;
    boost::shared_ptr<file_backend_t> file_backend = boost::make_shared<file_backend_t>(
        keywords::file_name = "../logs/run_logs.log",
        // boost::log::keywords::open_mode = (std::ios::out | std::ios::app),
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

int main()
{
    // Loggger_init();
    init_logging2();
    // boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");

    // init();

    BOOST_LOG_TRIVIAL(info) << message("FileHandler program Started");

    char request_buffer[512]; // Adjust buffer size as needed
    ssize_t bytes_read;
    // ssize_t shared_memory_size = 1024;
    auto pipeFlag = mkfifo(PipeName, 0666);
    if (pipeFlag == -1)
    {
        if (errno != EEXIST)
        {
            BOOST_LOG_TRIVIAL(error) << message("Failed to open named pipe.");
            std::cout << "Failed to open named pipe." << std::endl;
            return 1;
        }
    }
    // open pipe read and write
    //  int fd = open(PipeName, O_RDWR);
    int pipe_fd = open(PipeName, O_RDWR | O_CREAT, 0644);
    if (pipe_fd == -1)
    {
        BOOST_LOG_TRIVIAL(error) << message("Failed to open named pipe for reading.");
        std::cerr << "Failed to open named pipe for reading." << std::endl;
        return 1;
    }
    // sleep(1);
    while (true)
    {

        if ((bytes_read = read(pipe_fd, request_buffer, sizeof(request_buffer))) > 0)
        {
            std::cout << "************New Request************" << std::endl;
            BOOST_LOG_TRIVIAL(info) << message("************ Received request Number ") << std::to_string(n_requests++) << "  : " << request_buffer<< " ************";
            std::string request(request_buffer, bytes_read);
            std::istringstream iss(request);
            std::string command, argument;
            // Create shared memory
            int shm_fd = shm_open(BackingFile, O_RDWR | O_CREAT, 0644);
            BOOST_LOG_TRIVIAL(info) << message("Trying to open Shared memory backing file : ") << BackingFile;

            if (shm_fd < 0)
            {
                BOOST_LOG_TRIVIAL(error) << message("Can't open shared mem segment...");
                perror("Can't open shared mem segment...");
                exit(-1);
            }
            else
            {
                BOOST_LOG_TRIVIAL(info) << message("Shared mem segment opened successfully");
                std::cout << "shared mem segment opened successfully" << std::endl;
            }

            ftruncate(shm_fd, SHARED_MEM_CHUNK_SIZE + 1);

            caddr_t memptr = static_cast<caddr_t>(mmap(
                NULL, SHARED_MEM_CHUNK_SIZE + 1, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

            if (memptr == (caddr_t)-1)
            {
                BOOST_LOG_TRIVIAL(error) << message("Can't get memory segment segment with mmap...");
                perror("Can't get segment...");
                exit(-1);
            }
            else
            {
                BOOST_LOG_TRIVIAL(info) << message("Shared mem segment acquired successfully");
                // std::cout << "shared mem segment acquired successfully" << std::endl;
            }
            // int address = (int)memptr;
            intptr_t address = reinterpret_cast<intptr_t>(memptr);
            BOOST_LOG_TRIVIAL(trace) << message("Shared mem address: ") << std::hex << address;
            // fprintf(stderr, "shared mem address: %p [0..%d]\n", memptr, SHARED_MEM_CHUNK_SIZE);
            // fprintf(stderr, "backing file:       /dev/shm%s\n", BackingFile);

            sem_t *semptr = sem_open(SemaphoreName, O_CREAT, 0644, 12);
            BOOST_LOG_TRIVIAL(info) << message("Trying to open Semaphore : ") << SemaphoreName;
            if (semptr == (void *)-1)
            {
                BOOST_LOG_TRIVIAL(error) << message("sem_open failed");
                perror("sem_open");
                exit(-1);
            }
            int value;
            sem_init(semptr, 1, 0);
            sem_getvalue(semptr, &value);
            printf("Semaphore value: %d\n", value);
            BOOST_LOG_TRIVIAL(info) << message("Semaphore value at initialization: ") << value;
            std::string data;
            if (iss >> command >> argument)
            {
                if (command == "list")
                {
                    data = listFileInDirectory(argument);
                }
                else if (command == "content")
                {
                    data = readFileContent(argument);
                }
                else
                {   BOOST_LOG_TRIVIAL(error) << message("Unknown command: ") << command;
                    std::cerr << "Unknown command: " << command << std::endl;
                }
            }
            else
            {
                std::cerr << "Invalid request format: " << request << std::endl;
            }
            BOOST_LOG_TRIVIAL(info) << message("Command Excuted successfully!");

            int offset = 0;
            unsigned int chunks = 1;
            while (offset < data.size())
            {

                // Chunking ************************
                chunk_size = std::min(SHARED_MEM_CHUNK_SIZE + 1, ((int)data.size() + 1 - offset));
                auto chunk = data.substr(offset, chunk_size);
                // std::cout << "current chunk" << chunk.c_str() << std::endl;

                if (chunk_size < SHARED_MEM_CHUNK_SIZE + 1)
                {
                    chunk[chunk_size - 1] = 'X';
                    BOOST_LOG_TRIVIAL(info) << message("Sending Last Data Chunk");
                    std::cout << "************** Sending Last Chunk ***************" << std::endl;
                }
                else
                {   BOOST_LOG_TRIVIAL(trace) << message("Sending Data Chunk ")<< std::to_string(chunks++)<< " .....";
                    chunk[chunk_size - 1] = '1';
                }
                strncpy(memptr, chunk.c_str(), chunk_size);
                offset += chunk_size;
               

                /* Increment the semaphore so that the reader can read */
                if (sem_post(semptr) < 0)
                {
                    BOOST_LOG_TRIVIAL(error) << message("sem_post failed");
                    perror("sem_post");
                }
                sem_getvalue(semptr, &value);
                BOOST_LOG_TRIVIAL(trace) << message("Semaphore value after posting the data: ") << value;
                // printf("Semaphore value after posting data: %d\n", value);
                // sleep(2);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if (write(pipe_fd, &chunk_size, sizeof(chunk_size)) == -1)
                {
                    BOOST_LOG_TRIVIAL(error) << message("Writing the number of bytes to the pipe failed");
                    perror("write");
                }
                else
                {
                    BOOST_LOG_TRIVIAL(trace) << message("Written the number of bytes to the pipe successfully");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            BOOST_LOG_TRIVIAL(info) << message("All data sent successfully!");
            BOOST_LOG_TRIVIAL(info) << message("Releasing resources...");
            std::cout << "************End Of Request************" << std::endl;
            // sleep(1);
            munmap(memptr, SHARED_MEM_CHUNK_SIZE + 1);
            close(shm_fd);
            sem_close(semptr);
            shm_unlink(BackingFile);
            BOOST_LOG_TRIVIAL(info) << message("************ Resources Released! ************");
            BOOST_LOG_TRIVIAL(info) << message("Waiting for new request...");

            // close(pipe_fd);
            std::cout << "************Resources Released************" << std::endl;
        }
    }
    close(pipe_fd);
    BOOST_LOG_TRIVIAL(info) << message("FileHandler program Ended");
    return 0;
}
