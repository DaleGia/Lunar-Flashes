#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cassert>
#include <sys/stat.h>
#include <unistd.h>
#include "../ImageConvert.h"
#include "../Image.h"
#include "../ImageToTIFF.h"

bool directoryExists(const std::string& directoryPath) 
{
    struct stat info;
    if (stat(directoryPath.c_str(), &info) != 0)
        return false;
    return (info.st_mode & S_IFDIR) != 0;
}

bool createDirectory(const std::string& directoryPath) 
{
    int status = mkdir(directoryPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    return (status == 0);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./image-unpacker <file_path>" << std::endl;
        return 1;
    }

    const std::string filePath = argv[1];

    std::ifstream file(filePath);
    if (!file)
    {
        std::cerr << "Error: Invalid file path: " << filePath << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string fileContents = buffer.str();

    if (fileContents.empty())
    {
        std::cerr << "Error: File is empty: " << filePath << std::endl;
        return 1;
    }

    std::size_t lastSlashPos = filePath.find_last_of('/');
    std::string savePath;

    if(lastSlashPos != std::string::npos && lastSlashPos < filePath.length() - 1) 
    {
        // Extract the substring without including the last character
        savePath = filePath.substr(0, lastSlashPos + 1);
    } 
    else
    {
        return 1;
    }

    // savePath = filePath + "/";

    // if(false == directoryExists(savePath))
    // {
    //         // Create the directory
    //     bool directoryCreated = createDirectory(savePath);

    //     if (directoryCreated) 
    //     {
    //         std::cout << "Directory created successfully: " << savePath << std::endl;
    //     } 
    //     else 
    //     {
    //         std::cout << "Failed to create directory: " << savePath << std::endl;
    //     }
    // }
    // else
    // {
    //     std::cout << "Directory already exists: " << savePath << std::endl;
    // }

    std::size_t offset = 0;
    std::size_t count = 0;
    while (offset < fileContents.size())
    {
        Image image;
        uint64_t frameid;
        uint64_t timestamp;
        uint64_t systemReceiveTimestamp;
        uint32_t width;
        uint32_t height;
        uint8_t packed;
        uint8_t bitDepth;
        uint32_t bufferSize;

        memcpy(&frameid, fileContents.data() + offset, sizeof(frameid));
        offset += sizeof(frameid);

        memcpy(&timestamp, fileContents.data() + offset, sizeof(timestamp));
        offset += sizeof(timestamp);

        memcpy(&systemReceiveTimestamp, fileContents.data() + offset, sizeof(systemReceiveTimestamp));
        offset += sizeof(systemReceiveTimestamp);

        memcpy(&width, fileContents.data() + offset, sizeof(width));
        offset += sizeof(width);

        memcpy(&height, fileContents.data() + offset, sizeof(height));
        offset += sizeof(height);

        memcpy(&packed, fileContents.data() + offset, sizeof(packed));
        offset += sizeof(packed);

        memcpy(&bitDepth, fileContents.data() + offset, sizeof(bitDepth));
        offset += sizeof(bitDepth);

        memcpy(&bufferSize, fileContents.data() + offset, sizeof(bufferSize));
        offset += sizeof(bufferSize);
        
        uint8_t* bufferptr;
        bufferptr = (uint8_t* )(fileContents.data() + offset);
        
        image.setFrameId(frameid);
        image.setTimestamp(timestamp);
        image.setSystemReceiveTimestamp(systemReceiveTimestamp);
        image.setHeight(height);
        image.setWidth(width);
        image.setPackedStatus(packed);
        image.setBitDepth(bitDepth);
        image.setBuffer(bufferptr, bufferSize);
        offset += bufferSize;

        // std::cout << "frameId " << image.getFrameId() << std::endl;
        // std::cout << "timestamp " << image.getTimestamp() << std::endl;
        // std::cout << "systemReceiveTimestamp " << image.getSystemReceiveTimestamp() << std::endl;
        // std::cout << "width " << image.getWidth() << std::endl;
        // std::cout << "height " << image.getHeight() << std::endl;
        // std::cout << "packed " << image.isPacked() << std::endl;
        // printf("bitDepth %u\n", image.getBitDepth());
        // std::cout << "bufferSize " << image.getBufferSize() << std::endl;
        
        if(1 == packed)
        {
            if(bitDepth == 12)
            {
                // std::cout << "Converting to 12 bit unpacked " << std::endl;
                ImageConvert::convert(
                    image,
                    image.getWidth(),
                    image.getHeight(),
                    16);    
            }
            else if(bitDepth == 8)
            {
                ImageConvert::convert(
                    image,
                    image.getWidth(),
                    image.getHeight(),
                    8);
            }
        }
        else
        {
            if(bitDepth != 8)
            {
                // printf("Converting to 12 bit");
                ImageConvert::convert(
                    image,
                    image.getWidth(),
                    image.getHeight(),
                    16);
            }
            else
            {
                //GOTTA GET THSI WORKING!!!
                assert("GOTTA GET THIS WORKING!!!");
            }
        }
        // printf("image.getBitDepth(): %d\n", image.getBitDepth());
        /* Convert the 8 bit image to jpeg so it can be easily displayed*/
        Magick::Blob blob = ImageToTIFF::convert(
            image.getBuffer(), 
            image.getWidth(), 
            image.getHeight(), 
            image.getBitDepth());


        std::string saveFilePath;
        saveFilePath = 
            savePath + 
            "/" + 
            std::to_string(timestamp) +
            "_" + 
            std::to_string(frameid) +
            ".tiff";
        

        Magick::Image magickImage(blob);
        magickImage.write(saveFilePath);
        std::cout << "saved " << saveFilePath << " with " << (int)image.getBitDepth() << " bit depth" << std::endl;
        count++;
    }

    return 0;
}