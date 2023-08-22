/**
 * @file Main.c
 * Copyright (c) 2023 Dale Giancono All rights reserved.
 * 
 * @brief
 * TODO add me
 */
/*****************************************************************************/
/*INLCUDES                                                                   */
/*****************************************************************************/
#include <VmbCPP/VmbCPP.h>
#include <VmbImageTransform/VmbTransform.h>
#include <iostream>
#include <unistd.h>
#include <Magick++.h>
#include <mosquitto.h>
#include "ConcurrentFileIOBuffer.h"
#include "Image.h"
#include "ImageHistogram.h"
#include "ImageToJPEG.h"
#include "ImageConvert.h"
#include "RateMonitor.h"
#include "MQTTLog.h"


using namespace VmbCPP;

/*****************************************************************************/
/*MACROS                                                             */
/*****************************************************************************/
#define FRAME_BUFFER_SIZE  (300)
#define MQTT_HOST "localhost" 
#define MQTT_PORT 1883  

/*****************************************************************************/
/*OBJECT DECLARATIONS                                                */
/*****************************************************************************/

/*****************************************************************************/
/*FUNCTION declarations                                              */
/*****************************************************************************/
void checkForAndHandleError(
   bool error,
   VmbSystem &sys,
   VmbErrorType err,
   std::string message);

double setGain(Camera* cam, char* value);
double setExposure(Camera* cam, char* value);
double setFrameRate(Camera* cam, char* value);
std::string setBitDepth(Camera* cam, char* value);
void* signalPreviewFrameFunction(void* period);
void startCapture(CameraPtr cam);
void stopCapture();

void on_message_callback(
   struct mosquitto *mosq, 
   void *userdata, 
   const struct mosquitto_message *message);

void bufferOverflowHandler(void* self);

void addImageToBufferCallback(
   ConcurrentFileIoBufferElement<Image>* element,
   void* arg);

void copyImageCallback(
   ConcurrentFileIoBufferElement<Image>* element,
   void* arg);

void writeDataCallbackFunction(Image* image, FILE* stream);
bool saveConfiguration(Camera* cam);
bool loadConfiguration(Camera* cam);

static bool isMountPoint(const std::string& path);

/*****************************************************************************/
/*GLOBAL Data                                                                */
/*****************************************************************************/
const char* imageTopic = "/image";
const char* histogramTopic = "/histogram";
const char* maxTopic = "/max";
const char* minTopic = "/min";
const char* averageTopic = "/average";
const char* frameReceivedTopic = "/framereceived";
const char* frameSavedTopic = "/framesaved";
const char* invalidFrameTopic = "/invalidframe";
const char* frameSavedDataTopic = "/framesavedatareceived";

const char* gainsetTopic = "/gainset";
const char* gaingetTopic = "/gainget";
const char* exposuresetTopic = "/exposureset";
const char* exposuregetTopic = "/exposureget";
const char* frameratesetTopic = "/framerateset";
const char* framerategetTopic = "/framerateget";

const char* previewframeratesetTopic = "/previewframerateset";
const char* previewframerategetTopic = "/previewframerateget";
const char* bitdepthsetTopic = "/bitdepthset";
const char* bitdepthgetTopic = "/bitdepthget";
const char* getAllValuesTopic = "/getallvalues";

const char* saveConfigurationTopic = "/save";
const char* loadConfigurationTopic = "/load";

const char* saveGetTopic = "/saveget";
const char* saveSetTopic = "/saveset";
const char* captureGetTopic = "/captureget";
const char* captureSetTopic = "/captureset";



const int qos = 0;
struct mosquitto* mosq;
RateMonitor rateMonitor;
Counter frameReceivedCounter;
Counter frameSavedCounter;
Counter invalidFrameCounter;
Counter frameSavedDataReceivedCounter;
ConcurrentFileIoBuffer<Image> buffer;
MQTTLog logger;
CameraPtr camera;
pthread_mutex_t signalMutex;
uint32_t signalPeriod;
bool isCameraAcquiring = false;


/*****************************************************************************/
/* CLASS DECLARATION                                                                      */
/*****************************************************************************/
/**
 * \brief IFrameObserver implementation for asynchronous image acquisition
 */
class FrameObserver : public IFrameObserver
{
   public:
      FrameObserver(CameraPtr camera) : IFrameObserver(camera) {};
      void FrameReceived(const FramePtr frame)
      {
         frameReceivedCounter.increment(1);
         

         Frame* f = frame.get();
         VmbFrameStatusType status;
         f->GetReceiveStatus(status);
         VmbUint64_t frameId;
         VmbUint64_t timestamp; 
         f->GetFrameID(frameId);
         f->GetTimestamp(timestamp);
         switch(status)
         {
            case VmbFrameStatusIncomplete: 
            {
               logger.log("Frame %lu at %lu is incomplete... Try slowing the frame rate", frameId, timestamp);
               invalidFrameCounter.increment(1);
               break;
            }
            case VmbFrameStatusInvalid: 
            {
               logger.log("Frame %lu at %lu is invalid... Try slowing the frame rate", frameId, timestamp);
               invalidFrameCounter.increment(1);
               break;
            }
            case VmbFrameStatusTooSmall: 
            {
               logger.log("Frame %lu at %lu is too small... Try slowing the frame rate", frameId, timestamp);
               invalidFrameCounter.increment(1);
               break;
            }
            case VmbFrameStatusComplete:
            {
               buffer.add(addImageToBufferCallback, (void*)f);
               break;
            }            
            default:
            {
               assert(false);
            }
         }

         m_pCamera->QueueFrame(frame);
      };
};

/*****************************************************************************/
/* MAIN                                                                      */
/*****************************************************************************/
int main(int argc, const char **argv)
{   
   rateMonitor.registerCounter(frameReceivedCounter);
   rateMonitor.registerCounter(frameSavedCounter);
   rateMonitor.registerCounter(frameSavedDataReceivedCounter);
   rateMonitor.start();

   std::string cameraName;
   std::string filepath = "/media/dg/DFN/log.txt";
   logger.initialise(filepath.c_str());
   VmbSystem &sys = VmbSystem::GetInstance (); // Create and get Vmb singleton
   
   CameraPtrVector cameras; // Holds camera handles

   // Start the API, get and open cameras
   VmbErrorType err = sys.Startup();
   checkForAndHandleError(
      err != VmbErrorSuccess,
      sys,
      err,
      "Could not start api");

   logger.log("Vimba API started");

   /* Get the list of cameras and make sure atleast one is connected */
   err = sys.GetCameras(cameras);
   checkForAndHandleError(
      err != VmbErrorSuccess,
      sys,
      err,
      "Could not get cameras");

   logger.log("List of cameras got...");

   checkForAndHandleError(
      cameras.empty() == true,
      sys,
      err,
      "No cameras found");

   logger.log("Cameras found!...");

   /* Open the first camera */
   camera = cameras[0];
   err = camera->Open( VmbAccessModeExclusive );
   checkForAndHandleError(
      err != VmbErrorSuccess,
      sys,
      err,
      "Could not open camera");

   if(camera->GetName(cameraName) == VmbErrorSuccess)
   {
      logger.log("Opened Camera %s!...", cameraName.c_str());
   }

  // Initialize the Mosquitto library
   mosquitto_lib_init();

   // Create a Mosquitto client instance
   mosq = mosquitto_new(nullptr, true, nullptr);
   if(!mosq) 
   {
      logger.log("Failed to create Mosquitto client instance.");
      mosquitto_lib_cleanup();
      return 1;
   }

   // Connect to the MQTT broker
   if(mosquitto_connect(
      mosq, 
      MQTT_HOST, 
      MQTT_PORT, 
      0) != MOSQ_ERR_SUCCESS) 
   {
      logger.log("Failed to connect to mosquitto client.");
      mosquitto_destroy(mosq);
      mosquitto_lib_cleanup();
      return 1;
   }

   mosquitto_subscribe(mosq, NULL, gainsetTopic, qos);
   mosquitto_subscribe(mosq, NULL, exposuresetTopic, qos);
   mosquitto_subscribe(mosq, NULL, frameratesetTopic, qos);
   mosquitto_subscribe(mosq, NULL, previewframeratesetTopic, qos);
   mosquitto_subscribe(mosq, NULL, bitdepthsetTopic, qos);
   mosquitto_subscribe(mosq, NULL, saveSetTopic, qos);
   mosquitto_subscribe(mosq, NULL, getAllValuesTopic, qos);
   mosquitto_subscribe(mosq, NULL, captureSetTopic, qos);
   mosquitto_subscribe(mosq, NULL, saveConfigurationTopic, qos);
   mosquitto_subscribe(mosq, NULL, loadConfigurationTopic, qos);
   mosquitto_message_callback_set(mosq, on_message_callback);

   /* Allocate the buffers for image data*/
   buffer.allocate(FRAME_BUFFER_SIZE);
   // Test if mountpoint
   if(false == isMountPoint("/media/dg/DFN"))
   {
      logger.log("/media/dg/DFN is not a mount point. Exiting...");
      return -1;
   }

   system("mkdir /media/dg/DFN/images");
    
   buffer.setSaveDirectory("/media/dg/DFN/images");
   // system("mkdir /home/dg/Desktop/test-images");
   // buffer.setSaveDirectory("/home/dg/Desktop/test-images");
   buffer.setBufferOverflowHandler(bufferOverflowHandler, NULL);
   buffer.pause();
   buffer.start(writeDataCallbackFunction);

   mosquitto_loop_start(mosq);

   pthread_t signalThread;
   pthread_mutex_init(&signalMutex, NULL);
   pthread_mutex_lock(&signalMutex);
   signalPeriod = 100000;
   pthread_create(&signalThread, NULL, signalPreviewFrameFunction, &signalPeriod);



   ConcurrentFileIoBufferElement<Image>* element;
   Image lastImage;
   Image convertedImage;
   ImageHistogram histogram;
   VmbImage destinationImage;
   uint32_t destinationImageSize;
   VmbError_t error;
   VmbPixelFormat_t format;
   uint8_t bitDepth;
   uint32_t width;
   uint32_t height;
   bool packed;

   while(1)
   {
      pthread_mutex_lock(&signalMutex);
      /* Copy the last written element */
      buffer.copyLastAddedElement(copyImageCallback, &lastImage);
      /* If it is not in 8 bit format, convert it to it */
      if(8 != lastImage.getBitDepth())
      {
         ImageConvert::convert(
            lastImage,
            lastImage.getWidth(),
            lastImage.getHeight(),
            8);
      }
      /* Compute the histogram and some statistics*/
      histogram.compute(lastImage);
      /* Convert the 8 bit image to jpeg so it can be easily displayed*/
      Magick::Blob jpegBlob = ImageToJPEG::convert(
         lastImage.getBuffer(), 
         lastImage.getWidth(), 
         lastImage.getHeight(), 
         lastImage.getBitDepth(), 
         100);
      /* Convert the image to base64 */
      std::string base64image = jpegBlob.base64();
      std::string histogramString = histogram.getArrayString();
      std::string histogramMax = std::to_string(histogram.getMax());
      std::string histogramMin = std::to_string(histogram.getMin());
      std::string histogramAverage = std::to_string(histogram.getAverage());

      /* Publish the image to mqtt broker*/
      if(mosquitto_publish(
         mosq, 
         nullptr, 
         imageTopic, 
         base64image.size(), 
         base64image.c_str(), 
         qos,
         false)
         != MOSQ_ERR_SUCCESS) 
      {
         logger.log("Failed to publish image");
      }

      if(mosquitto_publish(
         mosq, 
         nullptr, 
         histogramTopic, 
         histogramString.length(), 
         histogramString.c_str(), 
         qos,
         false)
         != MOSQ_ERR_SUCCESS) 
      {
         logger.log("Failed to publish histogram");
      }

      if(mosquitto_publish(
         mosq, 
         nullptr, 
         averageTopic, 
         histogramAverage.length(), 
         (void*)histogramAverage.c_str(), 
         qos,
         false)
         != MOSQ_ERR_SUCCESS) 
      {
         logger.log("Failed to publish average");
      }

      if(mosquitto_publish(
         mosq, 
         nullptr, 
         maxTopic, 
         histogramMax.length(), 
         (void*)histogramMax.c_str(), 
         qos,
         false)
         != MOSQ_ERR_SUCCESS) 
      {
         logger.log("Failed to publish max");
      }

      if(mosquitto_publish(
         mosq, 
         nullptr, 
         minTopic, 
         histogramMin.length(), 
         (void*)histogramMin.c_str(), 
         qos,
         false)
         != MOSQ_ERR_SUCCESS) 
      {
         logger.log("Failed to publish min");
      }
   }
}

void on_message_callback(
   struct mosquitto *mosq, 
   void *userdata, 
   const struct mosquitto_message *message) 
{
    // This function will be called when a new message is received
   Camera* cam;
   cam = camera.get();

   if(strcmp(message->topic, gainsetTopic) == 0) 
   {
      double value = setGain(cam, (char*)message->payload);
      logger.log("Have set gain to %lf", value);
   }

   else if(strcmp(message->topic, exposuresetTopic) == 0) 
   {
      double value = setExposure(cam, (char*)message->payload);
      logger.log("Have set exposure to %lf", value);
   }

   else if (strcmp(message->topic, frameratesetTopic) == 0) 
   {

      double value = setFrameRate(cam, (char*)message->payload);
      logger.log("Have set acquisition frame rate to %lf", value);
   }

   else if (strcmp(message->topic, previewframeratesetTopic) == 0) 
   {
      char* value = (char*)message->payload;
      signalPeriod = (1.0 / std::stod(value)) * 1000000.0;
      logger.log("Have set preview frame rate to %s", value);
   }

   else if (strcmp(message->topic, bitdepthsetTopic) == 0) 
   {
      std::string value = setBitDepth(cam, (char*)message->payload);
      logger.log("Have set bit depth frame rate to %s", value.c_str());
   }
   else if (strcmp(message->topic, saveSetTopic) == 0) 
   {
      if(strcmp("0", (char*)message->payload) == 0)
      {
         buffer.pause();
         std::string response = "0";
         mosquitto_publish(
            mosq, 
            NULL, 
            saveGetTopic, 
            response.length(), 
            (void*)response.c_str(), 
            qos, 
            false); 
   
         logger.log("Not saving data...");
      }
      else if(strcmp("1", (char*)message->payload) == 0)
      {
         buffer.unpause();
         std::string response = "1";
         mosquitto_publish(
            mosq, 
            NULL, 
            saveGetTopic, 
            response.length(), 
            (void*)response.c_str(), 
            qos, 
            false); 
         logger.log("Saving data");
      }
      else
      {
         logger.log("Error with save status: ", (char*)message->payload);
      }
   }
   else if (strcmp(message->topic, captureSetTopic) == 0) 
   {
      if(strcmp("1", (char*)message->payload) == 0)
      {
         startCapture(camera);
         std::string response = "1";
         mosquitto_publish(
            mosq, 
            NULL, 
            captureGetTopic, 
            response.length(), 
            (void*)response.c_str(), 
            qos, 
            false); 
   
         logger.log("Capture Started...");
      }
      else if(strcmp("0", (char*)message->payload) == 0)
      {
         stopCapture();
         std::string response = "0";
         mosquitto_publish(
            mosq, 
            NULL, 
            captureGetTopic, 
            response.length(), 
            (void*)response.c_str(), 
            qos, 
            false); 
         logger.log("Capture Stoped data");
      }
      else
      {
         logger.log("Error with capture status: ", (char*)message->payload);
      }
   }
   else if (strcmp(message->topic, saveConfigurationTopic) == 0) 
   {
      saveConfiguration(cam);
   }
   else if (strcmp(message->topic, loadConfigurationTopic) == 0) 
   {
      loadConfiguration(cam);
   }
   else if (strcmp(message->topic, getAllValuesTopic) == 0) 
   {
      // The below code executes no matter what.  
   }

   FeaturePtr feature;
   double doublevalue;
   std::string stringvalue;

   cam->GetFeatureByName("ExposureTime", feature);
   feature.get()->GetValue(doublevalue);
   stringvalue = std::to_string(doublevalue);
   mosquitto_publish(mosq, NULL, exposuregetTopic, stringvalue.length(), (void*)stringvalue.c_str(), qos, false);

   cam->GetFeatureByName("Gain", feature);
   feature.get()->GetValue(doublevalue);
   stringvalue = std::to_string(doublevalue);
   mosquitto_publish(mosq, NULL, gaingetTopic, stringvalue.length(), (void*)stringvalue.c_str(), qos, false);

   cam->GetFeatureByName("AcquisitionFrameRate", feature);
   feature.get()->GetValue(doublevalue);
   stringvalue = std::to_string(doublevalue);
   mosquitto_publish(mosq, NULL, framerategetTopic, stringvalue.length(), (void*)stringvalue.c_str(), qos, false);

   cam->GetFeatureByName("PixelFormat", feature);
   feature.get()->GetValue(stringvalue);
   mosquitto_publish(mosq, NULL, bitdepthgetTopic, stringvalue.length(), (void*)stringvalue.c_str(), qos, false); 
   
   doublevalue = 1.0/((double)signalPeriod/1000000.0);
   stringvalue = std::to_string(doublevalue);
   mosquitto_publish(mosq, NULL, previewframerategetTopic, stringvalue.length(), (void*)stringvalue.c_str(), qos, false); 
}
void* signalPreviewFrameFunction(void* period)
{
   uint32_t* period_us = (uint32_t*)period;
   while(true)
   {
      usleep(*period_us);
      if(isCameraAcquiring)
      {
         pthread_mutex_unlock(&signalMutex);
      }

       
      std::string frameReceivedCounterString = std::to_string(frameReceivedCounter.getRatePerSecond());
      std::string frameSavedCounterString = std::to_string(frameSavedCounter.getRatePerSecond());
      std::string invalidFrameCounterString = std::to_string(invalidFrameCounter.getRunningTotal());
      std::string frameSavedDataReceivedCounterString = std::to_string(frameSavedDataReceivedCounter.getRatePerSecond());

      if(mosquitto_publish(
         mosq, 
         nullptr, 
         frameReceivedTopic, 
         frameReceivedCounterString.length(), 
         (void*)frameReceivedCounterString.c_str(), 
         qos,
         false)
         != MOSQ_ERR_SUCCESS) 
      {
         logger.log("Failed to publish frames received");
      }

      if(mosquitto_publish(
         mosq, 
         nullptr, 
         frameSavedTopic, 
         frameSavedCounterString.length(), 
         (void*)frameSavedCounterString.c_str(), 
         qos,
         false)
         != MOSQ_ERR_SUCCESS) 
      {
         logger.log("Failed to publish frames saved");
      }


      if(mosquitto_publish(
         mosq, 
         nullptr, 
         invalidFrameTopic, 
         invalidFrameCounterString.length(), 
         (void*)invalidFrameCounterString.c_str(), 
         qos,
         false)
         != MOSQ_ERR_SUCCESS) 
      {
         logger.log("Failed to publish invalidframe");
      }

      if(mosquitto_publish(
         mosq, 
         nullptr, 
         frameSavedDataTopic, 
         frameSavedDataReceivedCounterString.length(), 
         (void*)frameSavedDataReceivedCounterString.c_str(), 
         qos,
         false)
         != MOSQ_ERR_SUCCESS) 
      {
         logger.log("Failed to publish frameSavedDataTopic");
      }
   }
}
double setExposure(Camera* cam, char* value)
{
   FeaturePtr feature;
   VmbError_t error;
   error = cam->GetFeatureByName("ExposureTime", feature);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get exposure time feature");
      return -1;
   }
   std::string str = (char*)value;
   error = feature.get()->SetValue(std::stod(str));
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not set exposure time feature");
      return -1;
   }
   double retval;
   feature.get()->GetValue(retval);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get exposure time feature after setting feature...");
      return -1;
   }
   else
   {
      return retval;
   }
}
double setGain(Camera* cam, char* value)
{
   FeaturePtr feature;
   VmbError_t error;
   error = cam->GetFeatureByName("Gain", feature);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get gain feature");
      return -1;
   }
   std::string str = value;
   error = feature.get()->SetValue(std::stod(str));
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not set gain feature");
      return -1;
   }
   double retval;
   feature.get()->GetValue(retval);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get gain feature after setting...");
      return -1;
   }
   else
   {
      return retval;
   }
}

double setFrameRate(Camera* cam, char* value)
{
   FeaturePtr feature;
   VmbError_t error;

   /* Stop the acquisition */
   if(isCameraAcquiring)
   {
      stopCapture();
   }

   /* Enable the setting of the acquistion frame rate*/
   error = cam->GetFeatureByName("AcquisitionFrameRateEnable", feature);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get AcquisitionFrameRateEnable feature");
   }
   else
   {
      bool value;
      feature.get()->GetValue(value);
      if(true != value)
      {
         logger.log("AcquisitionFrameRateEnable is set to %u", value);
      }
   }

   error = cam->GetFeatureByName("AcquisitionFrameRate", feature);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get AcquisitionFrameRateEnable feature");
   }
   else
   {
      double value;
      feature.get()->GetValue(value);
   }
   std::string str = value;
   error = feature.get()->SetValue(std::stof(str));
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not set AcquisitionFrameRate feature as %s", str);
   }

   double retval;
   error = feature.get()->GetValue(retval);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get AcquisitionFrameRateEnable feature after setting...");
   }
   
   return retval;
}

bool saveConfiguration(Camera* cam)
{
   FeaturePtr feature;
   VmbError_t error;

   /* Stop the acquisition */
   if(isCameraAcquiring)
   {
      stopCapture();
   }

   error = cam->GetFeatureByName("UserSetDefault", feature);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get UserSetDefault feature...");
      return false;
   }
   
   error = feature.get()->SetValue("UserSet1");
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get UserSetDefault feature as UserSet1");
      return false;
   }

   error = cam->GetFeatureByName("UserSetSelector", feature);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get UserSetSelector feature...");
      return false;
   }

   error = feature.get()->SetValue("UserSet1");
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get UserSetSelector feature as UserSet1");
      return false;
   }

   error = cam->GetFeatureByName("UserSetSave", feature);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get UserSetSave feature...");
      return false;
   }

   error = feature.get()->RunCommand();
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could execute UserSetSave as command");
      return false;
   }

   logger.log("Configuration Saved");   
   return true;
}

bool loadConfiguration(Camera* cam)
{
   FeaturePtr feature;
   VmbError_t error;

   /* Stop the acquisition */
   if(isCameraAcquiring)
   {
      stopCapture();
   }

   error = cam->GetFeatureByName("UserSetDefault", feature);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get UserSetDefault feature...");
      return false;
   }
   
   error = feature.get()->SetValue("UserSet1");
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get UserSetDefault feature as UserSet1");
      return false;
   }

   error = cam->GetFeatureByName("UserSetSelector", feature);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get UserSetSelector feature...");
      return false;
   }

   error = feature.get()->SetValue("UserSet1");
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get UserSetSelector feature as UserSet1");
      return false;
   }

   error = cam->GetFeatureByName("UserSetLoad", feature);
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could not get UserSetLoad feature...");
      return false;
   }

   error = feature.get()->RunCommand();
   if(VmbErrorSuccess != error)
   {         
      logger.log("Could execute UserSetLoad as command");
      return false;
   }

   logger.log("Configuration Loaded");   
   return true;
}

std::string setBitDepth(Camera* cam, char* value)
{
   FeaturePtr feature;
   VmbError_t error;

   /* Stop the acquisition */
   if(isCameraAcquiring)
   {
      stopCapture();
   }

   error = cam->GetFeatureByName("PixelFormat", feature);
   if(VmbErrorSuccess != error)
   {         
      std::cout << "Could not get PixelFormat feature" << std::endl;
   }
   else
   {
      std::string value;
      feature.get()->GetValue(value);
      std::cout << "Current PixelFormat is " <<  value <<  std::endl;
   }
   std::string str = value;
   if(str == "Mono8")
   {
      error = feature.get()->SetValue("Mono8");
      if(VmbErrorSuccess != error)
      {         
         std::cout << "Could not set PixelFormat feature as Mono8" << error << std::endl;
      }
   }
   else if(str == "Mono10")
   {
      error = feature.get()->SetValue("Mono10");
      if(VmbErrorSuccess != error)
      {         
         std::cout << "Could not set PixelFormat feature as Mono10" << error << std::endl;
      }
   }
   else if(str == "Mono10p")
   {
      error = feature.get()->SetValue("Mono10p");
      if(VmbErrorSuccess != error)
      {         
         std::cout << "Could not set PixelFormat feature as Mono10p" << error << std::endl;
      }
   }
   else if(str == "Mono12")
   {
      error = feature.get()->SetValue("Mono12");
      if(VmbErrorSuccess != error)
      {         
         std::cout << "Could not set PixelFormat feature as Mono12" << error << std::endl;
      }
   }
   else if(str == "Mono12p")
   {
      error = feature.get()->SetValue("Mono12p");
      if(VmbErrorSuccess != error)
      {         
         std::cout << "Could not set PixelFormat feature as Mono12p" << error << std::endl;
      }
   }
   else
   {
      std::cout << "No valid bit depth" << error << std::endl;
   }

   std::string retval;
   error = feature.get()->GetValue(retval);
   if(VmbErrorSuccess != error)
   {         
      std::cout << "Could not get PixelFormat feature after setting" << std::endl;
   }

   return retval;
}


void bufferOverflowHandler(void* self)
{
   logger.log("Buffer overflow detected");
}

void checkForAndHandleError(
   bool error,
   VmbSystem &sys,
   VmbErrorType err,
   std::string message)
{
   if(error)
   {
      sys.Shutdown();
      throw std::runtime_error(message + std::to_string(err));
   }
}

void startCapture(CameraPtr cam)
{
   VmbErrorType err;
   err = camera->StartContinuousImageAcquisition(
      FRAME_BUFFER_SIZE, 
      IFrameObserverPtr(new FrameObserver(cam)));
   if(VmbErrorSuccess != err)
   {
      std::cout << "Could not start acquisition: " << err << std::endl;
   }
   else
   {
      isCameraAcquiring = true;
      logger.log("Started capture");
   }
}

void stopCapture()
{
   VmbErrorType err;
   err = camera->StopContinuousImageAcquisition();
   if(VmbErrorSuccess != err)
   {
      std::cout << "Could not stop acquisition: " << err << std::endl;
   }
   else
   {
      isCameraAcquiring = false;
      logger.log("Stopped capture");
   }
}

void addImageToBufferCallback(ConcurrentFileIoBufferElement<Image>* element,void* arg)
{
   std::time_t currentTime = std::time(nullptr);

   Frame* frame = (Frame*)arg;
   Image* image = element->get();
   uint8_t* bufferptr;
   uint32_t bufferSize;
   uint32_t imageHeight;
   uint32_t imageWidth;
   uint64_t frameId;
   uint64_t timestamp;
   bool packed;
   uint8_t bitDepth;
   VmbPixelFormatType format;

   frame->GetBuffer((VmbUchar_t*&)bufferptr);
   frame->GetBufferSize((VmbUint32_t&)bufferSize);
   frame->GetHeight((VmbUint32_t&)imageHeight);
   frame->GetWidth((VmbUint32_t&)imageWidth);
   frame->GetFrameID((VmbUint64_t&)frameId);
   frame->GetTimestamp((VmbUint64_t&)timestamp);

   
   frame->GetPixelFormat((VmbPixelFormatType&)format);
   switch(format)
   {
      case VmbPixelFormatMono8: 
      {

         packed = false;
         bitDepth = 8;
         break;
      };
      case VmbPixelFormatMono10: 
      {
         packed = false;
         bitDepth = 10;
         break;
      };
      case VmbPixelFormatMono10p: 
      {
         packed = true;
         bitDepth = 10;
         break;
      };
      case VmbPixelFormatMono12: 
      {
         packed = false;   
         bitDepth = 12;
         break;
      };
      case VmbPixelFormatMono12Packed: 
      {
         packed = false;
         bitDepth = 12;
         break;
      };
      case VmbPixelFormatMono12p: 
      {
         packed = true;
         bitDepth = 12;
         break;
      };
      case VmbPixelFormatMono14: 
      {
         packed = false;
         bitDepth = 14;
         break;
      };
      case VmbPixelFormatMono16: 
      {
         packed = false;
         bitDepth = 16;
         break;
      };
      default:
      {
         packed = false;
         bitDepth = 8;
         break;
      }
   }

   image->setBitDepth(bitDepth);
   image->setFrameId(frameId);
   image->setHeight(imageHeight);
   image->setPackedStatus(packed);
   image->setTimestamp(timestamp);
   image->setSystemReceiveTimestamp(currentTime);
   image->setWidth(imageWidth);
   image->setBuffer(bufferptr, bufferSize);
      
   element->setFilename(
      std::to_string(timestamp) + 
      "_" +
      std::to_string(frameId));

   frameSavedDataReceivedCounter.increment((double)bufferSize/1000000);
   if(buffer.isPaused() == false)
   {
      frameSavedCounter.increment(1);
   }

   return;
}

void copyImageCallback(ConcurrentFileIoBufferElement<Image>* element,void* arg)
{
   Image* buffer = (Image*)arg;
   Image* image = element->get();
   image->copy(buffer);
   return;
}


void writeDataCallbackFunction(Image* image, FILE* stream)
{
   int count = 0;
   uint64_t frameId;
   uint64_t timestamp;
   uint64_t systemTimestamp;
   uint8_t bitDepth; 
   bool packed;
   uint32_t height;
   uint32_t width;
   uint32_t bufferSize;

   frameId = image->getFrameId();
   timestamp = image->getTimestamp();
   systemTimestamp = image->getSystemReceiveTimestamp();
   bitDepth = image->getBitDepth();
   height = image->getHeight();
   width = image->getWidth();
   packed = image->isPacked();
   bufferSize = image->getBufferSize();

   count = fwrite(&frameId, sizeof(frameId), 1, stream);
   count += fwrite(&timestamp, sizeof(timestamp), 1, stream);
   count += fwrite(&systemTimestamp, sizeof(systemTimestamp), 1, stream);
   count += fwrite(&width, sizeof(width), 1, stream);
   count += fwrite(&height, sizeof(height), 1, stream);
   count += fwrite(&packed, sizeof(packed), 1, stream);
   count += fwrite(&bitDepth, sizeof(bitDepth), 1, stream);
   count += fwrite(&bufferSize, sizeof(bufferSize), 1, stream);
   count += fwrite(image->getBuffer(), bufferSize, 1, stream);
}

static bool isMountPoint(const std::string& path) 
{
   struct statfs fs;
   if (statfs(path.c_str(), &fs) == 0) 
   {
         // Check if the given path is a mount point
         if (fs.f_type != 0x6969 && fs.f_type != 0x9fa0 && fs.f_type != 0xef53 && fs.f_type != 0x5346414F) 
         {
            return true;
         }
   }
   return false;
}
