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

    bool imageSizeSet = false;
    std::size_t offset = 0;
    std::size_t count = 0;
    int groupCount = 0;
    int groupSize = 60;
    // Display the accumulated difference image
    double minVal, maxVal;
    cv::Scalar meanVal, stddev;
    cv::Mat averagedFrame;
    cv::Mat oldAveragedFrame;
    cv::Mat originalImage;
    cv::Mat processedImage;
    cv::Mat convertedImage;
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

        // std::cout << "frameId " << image.getFrameId() << std::endl;
        // std::cout << "timestamp " << image.getTimestamp() << std::endl;
        // std::cout << "systemReceiveTimestamp " << image.getSystemReceiveTimestamp() << std::endl;
        // std::cout << "width " << image.getWidth() << std::endl;
        // std::cout << "height " << image.getHeight() << std::endl;
        // std::cout << "gain " << image.getGain() << std::endl;
        // std::cout << "exposure " << image.getExposure() << std::endl;
    
        // std::cout << "packed " << image.isPacked() << std::endl;
        // printf("bitDepth %u\n", image.getBitDepth());
        // std::cout << "bufferSize " << image.getBufferSize() << std::endl;
        // cv::Mat cvimage;
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
                originalImage = cv::Mat(image.getHeight(), image.getWidth(), CV_16UC1, image.getBuffer());
            }
            else if(bitDepth == 8)
            {
                ImageConvert::convert(
                    image,
                    image.getWidth(),
                    image.getHeight(),
                    8);
                originalImage = cv::Mat(image.getHeight(), image.getWidth(), CV_8UC1, image.getBuffer());

            }
        }
        else
        {
            if(bitDepth != 8)
            {
                ImageConvert::convert(
                    image,
                    image.getWidth(),
                    image.getHeight(),
                    16);
                originalImage = cv::Mat(image.getHeight(), image.getWidth(), CV_16UC1, image.getBuffer());
            }
            else
            {
                //GOTTA GET THSI WORKING!!!
                assert("GOTTA GET THIS WORKING!!!");
            }
        }
        count++;

        if(false == imageSizeSet)
        {
            averagedFrame = cv::Mat::zeros(image.getHeight(), image.getWidth(), CV_16U); //larger depth to avoid saturation
            imageSizeSet = true;
        }

        // convertedImage = cv::Mat::zeros(image.getHeight(), image.getWidth(), CV_16U); //larger depth to avoid saturation
        // originalImage.convertTo(convertedImage, CV_16U);

        // processedImage = cv::Mat::zeros(image.getHeight(), image.getWidth(), CV_32F); //larger depth to avoid saturation

        originalImage.copyTo(processedImage);
        if(0 < groupCount)
        {
            cv::absdiff(originalImage, oldAveragedFrame, processedImage);
            std::cout << "subtracted image" << std::endl;
        }
        else
        {
            std::cout << "just original image"  << std::endl;
        }

        cv::add(averagedFrame, originalImage, averagedFrame);
        averagedFrame /= 2;

        cv::minMaxLoc(originalImage, &minVal, &maxVal);
        std::cout << "originalImage " << count << " - Min: " << minVal << ", Max: " << maxVal << std::endl;
        cv::minMaxLoc(oldAveragedFrame, &minVal, &maxVal);
        std::cout << "oldAveragedFrame " << count << " - Min: " << minVal << ", Max: " << maxVal << std::endl;
        cv::minMaxLoc(processedImage, &minVal, &maxVal);  
        std::cout << "processedImage " << count << " - Min: " << minVal << ", Max: " << maxVal << std::endl;
        cv::minMaxLoc(averagedFrame, &minVal, &maxVal);
        std::cout << "averagedFrame " << count << " - Min: " << minVal << ", Max: " << maxVal << std::endl;
        std::cout << std::endl;


        if(count >= groupSize)
        {
            count = 0;
            groupCount++;


            // Scale the float image to fit the 8-bit range (0-255)
            cv::Mat scaledAverageImage;
            averagedFrame.convertTo(scaledAverageImage, CV_8U, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));

            cv::Mat scaledProcessedFrame;
            processedImage.convertTo(scaledProcessedFrame, CV_8U, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
        
            // Display the scaled 8-bit imagconvertedImagee
            cv::imshow("Scaled 30 frame Averaged Image", scaledAverageImage);          
            cv::imshow("Last average subtracted frame in group", scaledProcessedFrame);           
            cv::waitKey(0);
            averagedFrame.copyTo(oldAveragedFrame);
            averagedFrame = cv::Mat::zeros(image.getHeight(), image.getWidth(), CV_16U); //larger depth to avoid saturation
            std::cout << std::endl;
        }
    }    

    return 0;
}