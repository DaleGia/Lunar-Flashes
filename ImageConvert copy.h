/*
  ImageConvert.h
  Copyright (c) 2020 Dale Giancono. All rights reserved..

  This class implements a way to convert convert raw images

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*****************************************************************************/
/*INLCUDE GUARD                                                              */
/*****************************************************************************/
#ifndef IMAGECONVERT_H_
#define IMAGECONVERT_H_

/*****************************************************************************/
/*INLCUDES                                                                   */
/*****************************************************************************/
#include "Image.h"
#include <assert.h>
#include <VmbCPP/VmbCPP.h>
#include <VmbImageTransform/VmbTransform.h>

/*****************************************************************************/
/*MACROS                                                                     */
/*****************************************************************************/

/*****************************************************************************/
/*GLOBAL Data                                                                */
/*****************************************************************************/

/*****************************************************************************/
/*CLASS DECLARATION                                                          */
/*****************************************************************************/
class ImageConvert
{
    public:
      enum BitDepth
      {
        BITDEPTH8,
        BITDEPTH10,
        BITDEPTH12,
        BITDEPTH16
      };

      static void convertTo8Bit(
        Image& image);

    private:
};
void ImageConvert::convertTo8Bit(
  Image& image)
{
  VmbError_t error;
  VmbPixelFormat_t sourceFormat;
  VmbImage sourceImage;
  Image newImage;
  VmbImage destinationImage;
  uint8_t bitDepth;
  uint32_t width;
  uint32_t height;

  bitDepth = image.getBitDepth();
  width = image.getWidth();
  height = image.getHeight();
  /* Packed formats not yet supported */
  assert(VmbPixelFormatMono10p != format);
  assert(VmbPixelFormatMono12p != format);

  if(8 == image.getBitDepth())
  {
      sourceFormat = VmbPixelFormatMono8;
  }
  if(10 == image.getBitDepth())
  {
      if(true == image.isPacked())
      {
        sourceFormat = VmbPixelFormatMono10p;
      }
      else
      {
        sourceFormat = VmbPixelFormatMono10;
      }
  }
  else if(12 == image.getBitDepth())
  {
      if(true == image.isPacked())
      {
        sourceFormat = VmbPixelFormatMono12Packed;
      }
      else
      {
        sourceFormat = VmbPixelFormatMono12;
      }
  }
  else
  {
    assert(false && "Bit depth not currently supported");
  }

  sourceImage.Size = sizeof(sourceImage);
  sourceImage.Data = image.getBuffer();

  error = VmbSetImageInfoFromPixelFormat(
    sourceFormat,
    image.getWidth(),
    image.getHeight(),
    &sourceImage);
  assert(error == VmbErrorSuccess);

  destinationImage.Size = sizeof(destinationImage);
  if(bitDepth <= 8)
  {
    destinationImage.Data = malloc(width*height);
  }
  else
  {
    destinationImage.Data = malloc(width*height*2);
  }

  assert(NULL != destinationImage.Data);
  VmbPixelLayout layout;
  error = VmbSetImageInfoFromInputParameters(
    format,
    width,
    height,
    VmbPixelLayoutMono,
    bitDepth,
    &destinationImage);
  if(error != VmbErrorSuccess)
  {
    std::cerr << "error: " << error <<  std::endl;
  }
  assert(error == VmbErrorSuccess);

  error = VmbImageTransform(
    &sourceImage,
    &destinationImage,
    NULL,
    1);
  if(error != VmbErrorSuccess)
  {
    std::cerr << "error: " << error <<  std::endl;
  }
  assert(error == VmbErrorSuccess);

  if(bitDepth <= 8)
  {

    image.setBuffer(
      destinationImage.Data,
      width*height);
  }
  else
  {
    image.setBuffer(
      destinationImage.Data,
      width*height*2);
  }

  image.setBitDepth(bitDepth);
  image.setHeight(height);
  image.setWidth(width);
  image.setPackedStatus(false);
  free(destinationImage.Data);
}

void ImageConvert::convert(
  Image& image,
  ImageConvert::BitDepth bitDepth)
{
  uint8_t sourceBitDepth;
  bool isPacked;
  uint8_t bitShiftAmount;
  void* newImage;
  void* oldImage;

  sourceBitDepth = image.getBitDepth();
  isPacked = image.isPacked();

  assert(sourceBitDepth != 8 && "Source bit depth is 8. Not yet implemented");
  assert(sourceBitDepth != 16 && "Source bit depth is 16. Not yet implemented");

  
  if(true == isPacked)
  {
    uint16_t* oldImage;
    uint8_t* packedImage;
    uint32_t bufferSize;
    bufferSize = image.getBufferSize();
    packedImage = (uint8_t*)image.getBuffer();
    oldImage = (uint16_t*)malloc(
          image.getWidth() * image.getHeight());
    uint32_t unpackedCount = 0;

    for(int i = 0; i < bufferSize; i+=3)
    {
      oldImage[unpackedCount] = (uint16_t)(packedImage[i] << 4) & 0xFF00;
      oldImage[unpackedCount++] |= (uint16_t)packedImage[i+1] & 0x00F0;
      oldImage[unpackedCount] = (uint16_t)(packedImage[i+1] << 4) & 0xFF00;
      oldImage[unpackedCount++] |= (uint16_t)(packedImage[i+2] << 4) & 0x00F0;
    }

  }
  else
  {
    oldImage = image.getBuffer();
  }


  if(bitDepth == ImageConvert::BITDEPTH8)
  {
    newImage = 
        malloc(
          image.getWidth() * image.getHeight());
   /* This will be a right bit shift */
    switch(image.getBitDepth())
    {
      case 14:
      
        bitShiftAmount = 6;
        break;
      }
      case 12:
      {
        bitShiftAmount = 4;
        break;
      }
      case 10:
      {
        bitShiftAmount = 2;
      }
      default:
      {
        assert(false && "Invalid source bit depth");
      }
    }
    uint16_t* oldImagePointer = (uint16_t*)oldImage;
    uint8_t* newImagePointer = (uint8_t*)newImage;
    for(int i = 0; i < image.getBufferSize(); i++)
    {
      newImagePointer[i] = 
        (uint8_t)(0x00FF & (oldImagePointer[i] >> bitShiftAmount));
    }
  }
  else 
  {
    newImage = 
        malloc(
          image.getWidth() * image.getHeight() * 2);
          switch(image.getBitDepth())
    /* This will be a left bit shift */
    switch(image.getBitDepth())
    {
      case 14:
      {
        bitShiftAmount = 2;
        break;
      }
      case 12:
      {
        bitShiftAmount = 4;
        break;
      }
      case 10:
      {
        bitShiftAmount = 6;
      }
      default:
      {
        assert(false && "Invalid source bit depth");
      }
    }
    uint16_t* oldImagePointer = (uint16_t*)oldImage;
    uint16_t* newImagePointer = (uint16_t*)newImage;
    for(int i = 0; i < image.getBufferSize(); i++)
    {
      newImagePointer[i] = 
        (uint16_t)(oldImagePointer[i] << bitShiftAmount);
    }
  }

  if(false == isPacked)
  {
    if

  }
}

#endif /* IMAGECONVERT_H_ */