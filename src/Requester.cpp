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

const char *SemaphoreName = "file_handler_sem";
const char *BackingFile = "/file_handler_shm";

const char *PipeName = "file_handler_pipe";
// byte size 5000
ssize_t ByteSize = 10000;
void readSharedMemory()
{
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
        NULL, ByteSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

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
    printf("Semaphore value: %d\n", value);
    if (sem_wait(semptr) < 0)
    {
        perror("sem_wait");
        exit(-1);
    }
    sem_getvalue(semptr, &value);
    printf("Semaphore value after post: %d\n", value);
    std::string sharedContent(memptr, ByteSize);
    std::cout << "Content read from shared memory:" << std::endl;
    std::cout << sharedContent << std::endl;

    sem_close(semptr);
    munmap(memptr, ByteSize);
    close(fd);
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

    int pipe_fd = open(PipeName, O_WRONLY);
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
    sleep(2);
    readSharedMemory();

    close(pipe_fd);

    return 0;
}
