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
      static void convert(
        Image& image, 
        VmbPixelFormatType format,
        uint32_t width, 
        uint32_t height,
        VmbPixelLayout_t layout,
        uint8_t bitDepth);

    private:
};

void ImageConvert::convert(
  Image& image, 
  VmbPixelFormatType format,
  uint32_t width, 
  uint32_t height,
  VmbPixelLayout_t layout,
  uint8_t bitDepth) 
{
  VmbError_t error;
  VmbPixelFormat_t sourceFormat;
  VmbImage sourceImage;
  Image newImage;
  VmbImage destinationImage;

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
        sourceFormat = VmbPixelFormatMono12p;
        
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

  error = VmbSetImageInfoFromInputParameters(
    format,
    width,
    height,
    layout,
    bitDepth,
    &destinationImage);
  assert(error == VmbErrorSuccess);

  error = VmbImageTransform(
    &sourceImage, 
    &destinationImage, 
    NULL, 
    1);
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

#endif /* IMAGECONVERT_H_ */