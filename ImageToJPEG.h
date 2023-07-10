/*
  ImageToJPEG.h
  Copyright (c) 2020 Dale Giancono. All rights reserved..

  This class implements a way to convert raw images to JPEG

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
#ifndef IMAGETOJPEG_H_
#define IMAGETOJPEG_H_

/*****************************************************************************/
/*INLCUDES                                                                   */
/*****************************************************************************/
#include "Image.h"
#include "Magick++.h"

/*****************************************************************************/
/*MACROS                                                                     */
/*****************************************************************************/

/*****************************************************************************/
/*GLOBAL Data                                                                */
/*****************************************************************************/

/*****************************************************************************/
/*CLASS DECLARATION                                                          */
/*****************************************************************************/
class ImageToJPEG
{
    public:
      ImageToJPEG();
      static Magick::Blob convert(Image& image, uint8_t quality);
      static Magick::Blob convert(
        void* image, 
        uint32_t width, 
        uint32_t height, 
        uint8_t bitDepth,
        uint8_t quality);
    protected:

    private:
};

Magick::Blob ImageToJPEG::convert(Image& image, uint8_t quality)
{
  assert(100 >= quality && "Only quality values between 1 and 100 supported");
  assert(8 == image.getBitDepth() && "Only a bitdepth of 8 is supported");

  // Convert to JPEG
  Magick::Blob conversionBlob(image.getBuffer(), image.getPixelCount());
  Magick::Blob jpegBlob;
  Magick::Geometry geometry(
    image.getWidth(), 
    image.getHeight(), 
    0, 
    0, 
    false, 
    false);
        
  Magick::Image magickImage(
    conversionBlob, 
    geometry, 
    image.getBitDepth(), 
    "GRAY");

  /* Set output to jpg*/
  magickImage.magick("JPEG");
  /* Set output quality*/
  magickImage.quality(quality);
  magickImage.depth(image.getBitDepth());
      
  // /* Write result to blob*/
  magickImage.write(&jpegBlob);

  return jpegBlob;
}

Magick::Blob ImageToJPEG::convert(
  void* image, 
  uint32_t width, 
  uint32_t height, 
  uint8_t bitDepth,
  uint8_t quality)
{
  assert(100 >= quality && "Only quality values between 1 and 100 supported");
  assert(8 == bitDepth && "Only a bitdepth of 8 is supported");

  // Convert to JPEG
  Magick::Blob conversionBlob(image, width*height);
  Magick::Blob jpegBlob;
  Magick::Geometry geometry(
    width, 
    height, 
    0, 
    0, 
    false, 
    false);
        
  Magick::Image magickImage(
    conversionBlob, 
    geometry, 
    bitDepth, 
    "GRAY");

  /* Set output to jpg*/
  magickImage.magick("JPEG");
  /* Set output quality*/
  magickImage.quality(quality);
  magickImage.depth(bitDepth);
      
  // /* Write result to blob*/
  magickImage.write(&jpegBlob);

  return jpegBlob;
}

#endif /* IMAGETOJPEG_H_ */