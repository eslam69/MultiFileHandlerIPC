#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

const char *PipeName = "file_handler_pipe";

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <list|content> <path>" << std::endl;
        return 1;
    }

    // Create or open the named pipe
    auto pipeFlag = mkfifo(PipeName, 0666);
     if (pipeFlag == -1)
    {
        if (errno != EEXIST)
        {
            std::cout << "Failed to open named pipe." << std::endl;
            return 1;
        }
    }

    int pipe_fd = open(PipeName, O_WRONLY);
   

    // Send request to the file handler
    std::string operation = argv[1];
    std::string path = argv[2];
    std::string request = operation + " " + path;
    write(pipe_fd, request.c_str(), request.size());
    write(pipe_fd, "\n", 1);
    std::cout << "request sent" << std::endl;
    // Close the pipe
    close(pipe_fd);

    

    return 0;
}
