#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cassert>
#include <sys/stat.h>
#include <unistd.h>
#include "../ImageConvert.h"
#include "../Image.h"
#include <fitsio.h>

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

    // Open a new FITS file for writing
    fitsfile* fitsFile;
    int status = 0; // CFITSIO status
    std::string saveFileName;
    saveFileName = filePath + ".fits";
    fits_create_file(&fitsFile, saveFileName.c_str(), &status);
    if (status != 0) 
    {
        std::cerr << "Error creating FITS file " << saveFileName << std::endl;
        return -1;
    }

    std::size_t offset = 0;
    uint32_t count = 1;
    while (offset < fileContents.size())
    {
        Image image;
        uint64_t frameid;
        uint64_t timestamp;
        uint64_t systemReceiveTimestamp;
        uint32_t width;
        uint32_t height;
        double gain;
        double exposure;
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

        memcpy(&gain, fileContents.data() + offset, sizeof(gain));
        offset += sizeof(gain);

        memcpy(&exposure, fileContents.data() + offset, sizeof(exposure));
        offset += sizeof(exposure);

        memcpy(&packed, fileContents.data() + offset, sizeof(packed));
        offset += sizeof(packed);

        memcpy(&bitDepth, fileContents.data() + offset, sizeof(bitDepth));
        offset +=         count++;
sizeof(bitDepth);

        memcpy(&bufferSize, fileContents.data() + offset, sizeof(bufferSize));
        offset += sizeof(bufferSize);
        
        uint8_t* bufferptr;
        bufferptr = (uint8_t* )(fileContents.data() + offset);
        
        image.setFrameId(frameid);
        image.setTimestamp(timestamp);
        image.setSystemReceiveTimestamp(systemReceiveTimestamp);
        image.setHeight(height);
        image.setWidth(width);
        image.setGain(gain);
        image.setExposure(exposure);
        image.setPackedStatus(packed);
        image.setBitDepth(bitDepth);
        image.setBuffer(bufferptr, bufferSize);
        offset += bufferSize;

        std::cout << "frameId " << image.getFrameId() << std::endl;
        std::cout << "timestamp " << image.getTimestamp() << std::endl;
        std::cout << "systemReceiveTimestamp " << image.getSystemReceiveTimestamp() << std::endl;
        std::cout << "width " << image.getWidth() << std::endl;
        std::cout << "height " << image.getHeight() << std::endl;
        std::cout << "gain " << image.getGain() << std::endl;
        std::cout << "exposure " << image.getExposure() << std::endl;
    
        std::cout << "packed " << image.isPacked() << std::endl;
        printf("bitDepth %u\n", image.getBitDepth());
        std::cout << "bufferSize " << image.getBufferSize() << std::endl;
        
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

        std::string hduName;
        hduName = std::to_string(image.getTimestamp()) + "_" + std::to_string(image.getFrameId());
        long naxes[2] = {image.getWidth(), image.getHeight()};
        fits_create_img(fitsFile, image.getBitDepth(), 2, naxes, &status);
        if (status != 0) 
        {
            std::cerr << "Error creating image HDU " << hduName << std::endl;
            return -1;
        }
        
        // Write image data to the FITS file
        fits_write_img(fitsFile, TFLOAT, 1, image.getWidth() * image.getHeight(), image.getBuffer(), &status);
        if (status != 0) 
        {
            std::cerr << "Error writing image data for HDU " << hduName << std::endl;
            return -1;
        }
        count++;
        fits_movabs_hdu(fitsFile, count, NULL, &status);
    }
    // Close the FITS file
    fits_close_file(fitsFile, &status);
    if (status != 0) 
    {
        std::cerr << "Error closing FITS file" << std::endl;
        return -1;
    }
    std::cout << "FITS file written successfully" << std::endl;

    return 0;
}