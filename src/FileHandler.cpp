#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <fstream>

const char *PipeName = "file_handler_pipe";

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

    while ((bytes_read = read(pipe_fd, request_buffer, sizeof(request_buffer))) > 0)
    {
        std::string request(request_buffer, bytes_read);
        std::istringstream iss(request);
        std::string command, argument;

        if (iss >> command >> argument)
        {
            if (command == "list")
            {
                listFiles(argument);
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
    }

    close(pipe_fd);
    return 0;
}
