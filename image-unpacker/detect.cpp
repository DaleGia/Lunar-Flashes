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
 #include <random>

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

// Function to generate the light curve data points and normalize between 0 and a given max value
std::vector<uint16_t> generateLightCurve(int numPoints, uint16_t riseTime, uint16_t decayRate, uint16_t maxNormalizedValue) 
{
    std::vector<double> lightCurveData;
    std::vector<uint16_t> returnData;

    for (int i = 0; i < numPoints; ++i) {
        uint16_t time = static_cast<uint16_t>(i);
        double value;

        if (time < riseTime) {
            // Fast rise time - linear increase
            value = (double)time / (double)riseTime;
        } else {
            // Exponential decay after the rise time
            value = exp(-(double)decayRate * ((double)time - (double)riseTime));
        }

        lightCurveData.push_back(value);
    }

    // Normalizing between 0 and the given max value
    double maxVal = *std::max_element(lightCurveData.begin(), lightCurveData.end());
    for (int i = 0; i < numPoints; ++i) {
        returnData.push_back((uint16_t)(maxNormalizedValue * (lightCurveData[i] / maxVal)));
    }

    return returnData;
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

    /** Parameters to be added to command line eventually */
    int averagedFramesSize = 100;
    int testFlashFramesSize = 20;
    int testFlashFramesMagnitude = 1000;

    /** Generate the fake light curve **/
    std::vector<uint16_t> lightCurve = generateLightCurve(
        testFlashFramesSize, 
        3, 
        1,
        testFlashFramesMagnitude);
    /** generate a random location for the flash */
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Generate a random number between 100 and 300
    std::uniform_int_distribution<uint16_t> distribution(100, 300);
    uint16_t randomNumber = distribution(gen);
    
    /** This does the actual processing */
    bool imageSizeSet = false;
    bool averageImageSet = false;
    std::size_t offset = 0;
    std::size_t imageCount = 0;
    int groupCount = 0;
    // Display the accumulated difference image
    double minVal, maxVal;
    cv::Scalar meanVal, stddev;
    cv::Mat averagedFrame;
    cv::Mat averagedProcessedFrame;
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
        
        /*****************************************************************************************/
        /************************ THIS INJECTS TEST FLASH ****************************************/
        /*****************************************************************************************/
        if(0 < groupCount)
        {
            if(10 >= imageCount)
            {
                originalImage.at<uint16_t>(99, randomNumber-1) += (uint16_t)((double)lightCurve[imageCount] * 0.25);
                originalImage.at<uint16_t>(100, randomNumber-1) += (uint16_t)((double)lightCurve[imageCount] * 0.25);
                originalImage.at<uint16_t>(101, randomNumber-1) += (uint16_t)((double)lightCurve[imageCount] * 0.25);

                originalImage.at<uint16_t>(99, randomNumber) += (uint16_t)((double)lightCurve[imageCount] * 0.75);
                originalImage.at<uint16_t>(100, randomNumber) += lightCurve[imageCount];
                originalImage.at<uint16_t>(101, randomNumber) += (uint16_t)((double)lightCurve[imageCount] * 0.75);

                originalImage.at<uint16_t>(99, randomNumber+1) += (uint16_t)((double)lightCurve[imageCount] * 0.25);
                originalImage.at<uint16_t>(100, randomNumber+1) += (uint16_t)((double)lightCurve[imageCount] * 0.25);
                originalImage.at<uint16_t>(101, randomNumber+1) += (uint16_t)((double)lightCurve[imageCount] * 0.25);
            }
        }
        /*****************************************************************************************/
        /*****************************************************************************************/
        if(false == averageImageSet)
        {
            cv::add(averagedFrame, originalImage, averagedFrame);
            averagedFrame /= 2;
        }
        originalImage.copyTo(processedImage);
        if(averageImageSet)
        {
            cv::subtract(originalImage, oldAveragedFrame, processedImage);
        }

        cv::add(averagedProcessedFrame, processedImage, averagedProcessedFrame);

        
        averagedProcessedFrame /=2;

        if(1 == groupCount)
        {
            // Scale the float image to fit the 8-bit range (0-255)
            cv::Mat scaledProcessedImage;
            cv::minMaxLoc(processedImage, &minVal, &maxVal);
            processedImage.convertTo(scaledProcessedImage, CV_8U, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
            cv::imshow(std::to_string(imageCount), scaledProcessedImage);          
        }
        // cv::minMaxLoc(originalImage, &minVal, &maxVal);
        // std::cout << "originalImage " << imageCount << " - Min: " << minVal << ", Max: " << maxVal << std::endl;
        // cv::minMaxLoc(oldAveragedFrame, &minVal, &maxVal);
        // std::cout << "oldAveragedFrame " << imageCount << " - Min: " << minVal << ", Max: " << maxVal << std::endl;
        // cv::minMaxLoc(processedImage, &minVal, &maxVal);  
        // std::cout << "processedImage " << imageCount << " - Min: " << minVal << ", Max: " << maxVal << std::endl;
        // cv::minMaxLoc(averagedFrame, &minVal, &maxVal);
        // std::cout << "averagedFrame " << imageCount << " - Min: " << minVal << ", Max: " << maxVal << std::endl;
        // std::cout << std::endl;

        imageCount++;

        if(imageCount >= averagedFramesSize)
        {



            // Scale the float image to fit the 8-bit range (0-255)
            cv::Mat scaledAverageImage;
            cv::minMaxLoc(averagedFrame, &minVal, &maxVal);
            averagedFrame.convertTo(scaledAverageImage, CV_8U, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));

            // Scale the float image to fit the 8-bit range (0-255)
            cv::Mat scaledAverageProcessedImage;
            cv::minMaxLoc(averagedProcessedFrame, &minVal, &maxVal);
            averagedProcessedFrame.convertTo(scaledAverageProcessedImage, CV_8U, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));

            // Scale the float image to fit the 8-bit range (0-255)
            cv::Mat scaledProcessedImage;
            cv::minMaxLoc(originalImage, &minVal, &maxVal);
            originalImage.convertTo(scaledProcessedImage, CV_8U, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));

        
            if(1 == groupCount)
            {
                // Display the scaled 8-bit imagconvertedImagee
                cv::imshow("A", scaledAverageImage);          
                cv::imshow("B", scaledProcessedImage);          
                cv::imshow("C", scaledAverageProcessedImage);          

                cv::waitKey(0);
            }
            averagedFrame.copyTo(oldAveragedFrame);
            averagedFrame = cv::Mat::zeros(image.getHeight(), image.getWidth(), CV_16U); //larger depth to avoid saturation
            averagedProcessedFrame = cv::Mat::zeros(image.getHeight(), image.getWidth(), CV_16U); //larger depth to avoid saturation
            std::cout << std::endl;

            imageCount = 0;
            groupCount++;
        }
    }    

    return 0;
}