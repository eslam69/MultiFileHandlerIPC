// compile with
// g++ -o filehandler FileHandler.cpp -lrt -lpthread
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
#define SHARED_MEM_CHUNK_SIZE 512

const char *PipeName = "file_handler_pipe";
const char *BackingFile = "/file_handler_shm";
const char *SemaphoreName = "file_handler_sem";
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

int main()
{
    // mkfifo(PipeName, 0666);

    // int pipe_fd = open(PipeName, O_RDONLY);
    // if (pipe_fd == -1)
    // {
    //     std::cerr << "Failed to open named pipe for reading." << std::endl;
    //     return 1;
    // }

    char request_buffer[512]; // Adjust buffer size as needed
    ssize_t bytes_read;
    // ssize_t shared_memory_size = 1024;

    while (true)
    {
        // mkfifo(PipeName, 0666);
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
        int pipe_fd = open(PipeName, O_RDWR);
        if (pipe_fd == -1)
        {
            std::cerr << "Failed to open named pipe for reading." << std::endl;
            return 1;
        }
        sleep(1);
        if ((bytes_read = read(pipe_fd, request_buffer, sizeof(request_buffer))) > 0)
        {

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

            ftruncate(shm_fd, SHARED_MEM_CHUNK_SIZE);

            caddr_t memptr = static_cast<caddr_t>(mmap(
                NULL, SHARED_MEM_CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

            if (memptr == (caddr_t)-1)
            {
                perror("Can't get segment...");
                exit(-1);
            }

            fprintf(stderr, "shared mem address: %p [0..%d]\n", memptr, SHARED_MEM_CHUNK_SIZE - 1);
            fprintf(stderr, "backing file:       /dev/shm%s\n", BackingFile);

            sem_t *semptr = sem_open(SemaphoreName, O_CREAT, 0644, 12);
            if (semptr == (void *)-1)
            {
                perror("sem_open");
                exit(-1);
            }
            int value;
            sem_init(semptr, 1, 0);
            sem_getvalue(semptr, &value);
            printf("Semaphore value: %d\n", value);

            if (iss >> command >> argument)
            {
                if (command == "list")
                {

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

                    ftruncate(shm_fd, SHARED_MEM_CHUNK_SIZE);

                    caddr_t memptr = static_cast<caddr_t>(mmap(
                        NULL, SHARED_MEM_CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

                    if (memptr == (caddr_t)-1)
                    {
                        perror("Can't get segment...");
                        exit(-1);
                    }

                    fprintf(stderr, "shared mem address: %p [0..%d]\n", memptr, SHARED_MEM_CHUNK_SIZE - 1);
                    fprintf(stderr, "backing file:       /dev/shm%s\n", BackingFile);

                    sem_t *semptr = sem_open(SemaphoreName, O_CREAT, 0644, 12);
                    if (semptr == (void *)-1)
                    {
                        perror("sem_open");
                        exit(-1);
                    }
                    int value;
                    sem_init(semptr, 1, 0);
                    sem_getvalue(semptr, &value);
                    printf("Semaphore value: %d\n", value);
                    // listFiles(argument);
                    auto data = listFilesToSharedMemory(argument);

                    strncpy(memptr, data.c_str(), SHARED_MEM_CHUNK_SIZE); // Copy the list to the shared memory

                    /* Increment the semaphore so that the reader can read */
                    if (sem_post(semptr) < 0)
                    {
                        perror("sem_post");
                    }
                    sem_getvalue(semptr, &value);
                    printf("Semaphore value new: %d\n", value);
                    sleep(1);
                    munmap(memptr, SHARED_MEM_CHUNK_SIZE);
                    close(shm_fd);
                    sem_close(semptr);
                    shm_unlink(BackingFile);
                }
                else if (command == "content")
                {
                    auto data = readFileContent(argument);

                    int offset = 0;
                    while (offset < data.size())
                    {

                        // Chunking ************************
                        int chunk_size = std::min(SHARED_MEM_CHUNK_SIZE, ((int)data.size() - offset));
                        strncpy(memptr, data.substr(offset, chunk_size).c_str(), chunk_size);
                        std::cout << "current chunk" << data.substr(offset, chunk_size).c_str() << std::endl;
                        offset += chunk_size;
                        // printf("offset+chunk_size: %d\n", offset + chunk_size);
                        printf("offset = %d\n", offset);
                        printf("chunk_size = %d\n", chunk_size);
                        // break;
                        // strncpy(memptr, data.c_str(), SHARED_MEM_CHUNK_SIZE); // Copy the list to the shared memory
                        // **********************************

                        /* Increment the semaphore so that the reader can read */
                        if (sem_post(semptr) < 0)
                        {
                            perror("sem_post");
                        }
                        sem_getvalue(semptr, &value);
                        printf("Semaphore value new: %d\n", value);
                        // sleep(1);
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));

                        // munmap(memptr, SHARED_MEM_CHUNK_SIZE);
                        // close(shm_fd);
                        // sem_close(semptr);
                        // shm_unlink(BackingFile);
                        int PipeEndFlag = 1;
                        printf("offset: %d\n", offset);
                        // write(pipe_fd, &PipeEndFlag, sizeof(PipeEndFlag));
                        // sleep(5);
                    }
                }
                else
                {
                    std::cerr << "Unknown command: " << command << std::endl;
                }
                sleep(1);
                munmap(memptr, SHARED_MEM_CHUNK_SIZE);
                close(shm_fd);
                sem_close(semptr);
                shm_unlink(BackingFile);
            }
            else
            {
                std::cerr << "Invalid request format: " << request << std::endl;
            }
        }
        close(pipe_fd);
    }
    // sleep(12);

    // close(pipe_fd);

    return 0;
}
