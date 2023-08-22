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
    
    if (ls_output) {
        std::cout << "Found files in " << directory << ":" << std::endl;
        
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), ls_output)) {
            std::cout << buffer;
        }

        pclose(ls_output);
    } else {
        std::cerr << "Failed to execute 'ls' command." << std::endl;
    }


}

// Function to read and display content of a file
void readFileContent(const std::string &filePath)
{   std::cout << "Reading file " << filePath << ":" << std::endl;
    std::string command = "cat " + filePath;
    FILE *cat_output = popen(command.c_str(), "r");
    
    if (cat_output) {
        std::cout << "Content of file " << filePath << ":" << std::endl;
        
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), cat_output)) {
            std::cout << buffer;
        }

        pclose(cat_output);
    } else {
        std::cerr << "Failed to execute 'cat' command." << std::endl;
    }
}


void listFilesToSharedMemory(const std::string &directory) {
    std::string command = "ls -p " + directory + " | grep -v /"; // List only files (not directories)
    FILE *ls_output = popen(command.c_str(), "r");
    std::cout << "Listing files in " << directory << ":" << std::endl;
    if (ls_output) {
        std::string files_list;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), ls_output)) {
            files_list += buffer;
        }
        
        //print files_list
        std::cout << "files_list: " << files_list << std::endl;

        int shm_fd = shm_open(BackingFile, O_RDWR | O_CREAT, 0644);
        if (shm_fd < 0) {
            perror("Can't open shared mem segment...");
            exit(-1);
        }
        else{
            std::cout<< "shared mem segment opened successfully"<<std::endl;
        }
        // struct stat sb;
        // fstat(shm_fd, &sb);
        auto data_size = sizeof(files_list);
        data_size = 5000;
        ftruncate(shm_fd, data_size);

        caddr_t memptr = static_cast<caddr_t>(mmap(
            NULL, data_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

        if (memptr == (caddr_t)-1) {
            perror("Can't get segment...");
            exit(-1);
        }

        fprintf(stderr, "shared mem address: %p [0..%ld]\n", memptr, data_size - 1);
        fprintf(stderr, "backing file:       /dev/shm%s\n", BackingFile);

        sem_t *semptr = sem_open(SemaphoreName, O_CREAT, 0644, 0);
        if (semptr == (void *)-1) {
            perror("sem_open");
            exit(-1);
        }


        strncpy(memptr, files_list.c_str(), data_size); // Copy the list to the shared memory

        pclose(ls_output);

        /* Increment the semaphore so that the reader can read */
        if (sem_post(semptr) < 0) {
            perror("sem_post");
        }
         sleep(1); 
    munmap(memptr, data_size);
    close(shm_fd);
    sem_close(semptr);
    shm_unlink(BackingFile);
    } else {
        std::cerr << "Failed to execute 'ls' command." << std::endl;
    }
}

int main()
{
    mkfifo(PipeName, 0666);

    int pipe_fd = open(PipeName, O_RDONLY);
    if (pipe_fd == -1)
    {
        std::cerr << "Failed to open named pipe for reading." << std::endl;
        return 1;
    }

    char request_buffer[512]; // Adjust buffer size as needed
    ssize_t bytes_read;
    // ssize_t shared_memory_size = 1024;
    while (true)
    {if ((bytes_read = read(pipe_fd, request_buffer, sizeof(request_buffer))) > 0)
    {    
        std::string request(request_buffer, bytes_read);
        std::istringstream iss(request);
        std::string command, argument;

        
        if (iss >> command >> argument)
        {
            if (command == "list")
            {
                // listFiles(argument);
                listFilesToSharedMemory(argument);

            }
            else if (command == "content")
            {
                readFileContent(argument);
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
    sleep(5);
    }}
    // sleep(12);
    
    
    
    
    close(pipe_fd);

    

   
    return 0;
}
