// g++ -o requester Requester.cpp -lrt -lpthread

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

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
#include <thread>
#include <chrono>
#define SHARED_MEM_CHUNK_SIZE 4095
const char *SemaphoreName = "file_handler_sem";
const char *BackingFile = "/file_handler_shm";

const char *PipeName = "file_handler_pipe";
// byte size 5000
// ssize_t DATA_SIZE = 10000;
int pipe_fd;
int readSharedMemory(int bytes_to_read)
{
    // std::cout << "bytes_to_read: " << bytes_to_read << std::endl;
    int fd = shm_open(BackingFile, O_RDWR, 0644);
    if (fd < 0)
    {
        perror("Can't open shared mem segment...");
        exit(-1);
    }
    // else
    // {
    //     std::cout << "Shared memory segment opened successfully." << std::endl;
    // }

    caddr_t memptr = static_cast<caddr_t>(mmap(
        NULL, bytes_to_read, PROT_READ, MAP_SHARED, fd, 0));
    // print sysconf(_SC_PAGE_SIZE)
    //  std::cout << "Page size: " << sysconf(_SC_PAGE_SIZE) << std::endl;

    if (memptr == MAP_FAILED)
    {
        perror("Can't get segment...");
        exit(-1);
    }

    // std::string semaphoreName = SemaphoreName;
    sem_t *semptr = sem_open(SemaphoreName, 0);
    if (semptr == (void *)-1)
    {
        perror("sem_open");
        exit(-1);
    }
    int value;
    sem_getvalue(semptr, &value);
    // printf("Semaphore value: %d\n", value);
    if (sem_wait(semptr) < 0)
    {
        perror("sem_wait");
        exit(-1);
    }
    sem_getvalue(semptr, &value);
    // printf("Semaphore value after post: %d\n", value);
    std::string sharedContent(memptr, bytes_to_read);
    // std::string sharedContent(memptr);

    // std::cout << "Content read from shared memory:" << std::endl;
    auto flag = sharedContent.back();

    // std::cout << "*****flag>>>>>>>>>>>>" << flag << std::endl;
    auto output = sharedContent.substr(0, bytes_to_read - 1);
    std::cout << "output size: " << output.size() << std::endl;
    std::cout << output;
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    sem_close(semptr);
    munmap(memptr, bytes_to_read);
    close(fd);

    if (flag == 'X')
    {
        // std::cout << "";
        // std::cout << output;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return 1;
    }

        return static_cast<int>(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <list|content> <path>" << std::endl;
        return 1;
    }
    if (strcmp(argv[1], "list") != 0 && strcmp(argv[1], "content"))
    {
        std::cout << argv[1] << std::endl;
        std::cerr << "Invalid Command!\nUsage: " << argv[0] << " <list|content> <path>" << std::endl;
        return 1;
    }

    // Create or open the named pipe
    // auto pipeFlag = mkfifo(PipeName, 0666);
    //  if (pipeFlag == -1)
    // {
    //     if (errno != EEXIST)
    //     {
    //         std::cout << "Failed to open named pipe." << std::endl;
    //         return 1;
    //     }
    // }

    pipe_fd = open(PipeName, O_RDWR);
    // validate pipe_fd
    if (pipe_fd == -1)
    {
        std::cerr << "Failed to open named pipe for writing." << std::endl;
        return 1;
    }

    // Send request to the file handler
    std::string operation = argv[1];
    std::string path = argv[2];
    std::string request = operation + " " + path;
    write(pipe_fd, request.c_str(), request.size());
    write(pipe_fd, "\n", 1);
    std::cout << "Request Sent!" << std::endl;
    // sleep(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    int response_buffer;
    int terminate_flag = 0;
    while (true)
    { // read pipe response
        // sleep(2);
        int bytes_to_read = 0;
        read(pipe_fd, &bytes_to_read, sizeof(bytes_to_read));
        // std::cout << "bytes_to_read: " << bytes_to_read << std::endl;
        terminate_flag = readSharedMemory(bytes_to_read);
        if (terminate_flag == 1)
        {
            // std::cout << "\nterminating" << std::endl;
            std::cout << std::endl;
            break;
        }
        // int bytes_read = read(pipe_fd, &response_buffer, sizeof(response_buffer));
        // // printf("response_buffer: %d\n", response_buffer);
        // if (bytes_read == 0)
        // {
        //     std::cout << "No response from file handler." << std::endl;
        //     break;
        // }
        // else if (bytes_read == -1)
        // {
        //     std::cerr << "Failed to read from pipe." << std::endl;
        //     break;
        // }
        // else
        // {
        //     // std::cout << "Response from file handler:" << std::endl;
        //     // std::cout << response_buffer << std::endl;
        //     readSharedMemory();
        //     // break;
        // }
    }

    close(pipe_fd);

    return 0;
}
