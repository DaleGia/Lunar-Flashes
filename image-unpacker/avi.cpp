#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cassert>
#include <sys/stat.h>
#include <unistd.h>
#include "../ImageConvert.h"
#include "../Image.h"
#include <vector>
#include <opencv2/opencv.hpp>
 
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

    std::size_t offset = 0;
    std::size_t count = 0;
    std::vector<cv::Mat> images;
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
        cv::Mat cvimage;
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

                cvimage = cv::Mat(image.getHeight(), image.getWidth(), CV_16UC1, image.getBuffer()); // Create a single-channel (grayscale) image matrix
            }
            else if(bitDepth == 8)
            {
                ImageConvert::convert(
                    image,
                    image.getWidth(),
                    image.getHeight(),
                    8);
                cvimage = cv::Mat(image.getHeight(), image.getWidth(), CV_8UC1, image.getBuffer()); // Create a single-channel (grayscale) image matrix
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
                cvimage = cv::Mat(image.getHeight(), image.getWidth(), CV_16UC1, image.getBuffer()); // Create a single-channel (grayscale) image matrix
            }
            else
            {
                //GOTTA GET THSI WORKING!!!
                assert("GOTTA GET THIS WORKING!!!");
            }
        }

        cvimage = cvimage < 4;
        images.push_back(cvimage);
    }

    std::string savePath = filePath + ".avi";
    std::cout << "Saving " << savePath << std::endl;
    cv::Size frameSize;  // Size of each frame
    if (!images.empty()) 
    {
        frameSize = images[0].size();
        std::cout << "frameSize " << frameSize << std::endl;

    } 
    else 
    {
        std::cerr << "Image buffer is empty. Cannot create video." << std::endl;
        return -1;
    }

    cv::VideoWriter video(savePath, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 30, frameSize, false);

    if (!video.isOpened()) 
    {
        std::cerr << "Could not open the output video file for writing." << std::endl;
        return 1;
    }

    for (size_t i = 0; i < images.size(); ++i) {
        // Print the frame number
        std::cout << "Writing frame " << i << " to the video." << std::endl;

        // Write the frame to the video
        video.write(images[i]);
    }

    video.release();
    return 0;
}


// Function to create a cv::Mat from raw grayscale pixel values in a buffer
cv::Mat createMatFromBuffer(const uchar* buffer, int width, int height) 
{
    cv::Mat image(height, width, CV_8UC1); // Create a single-channel (grayscale) image matrix

    // Copy the pixel data from the buffer to the cv::Mat
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            image.at<uchar>(y, x) = buffer[y * width + x]; // Assuming uchar buffer for grayscale values
        }
    }

    return image;
}
