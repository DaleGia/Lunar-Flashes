/*
  ImageHistogram.h
  Copyright (c) 2020 Dale Giancono. All rights reserved..

  This class implements a way to calculate a histogram and other statistics.

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
#ifndef IMAGEHISTOGRAM_H_
#define IMAGEHISTOGRAM_H_

/*****************************************************************************/
/*INLCUDES                                                                   */
/*****************************************************************************/
#include "Image.h"
#include <cmath>
#include <limits>

/*****************************************************************************/
/*MACROS                                                                     */
/*****************************************************************************/

/*****************************************************************************/
/*GLOBAL Data                                                                */
/*****************************************************************************/

/*****************************************************************************/
/*CLASS DECLARATION                                                          */
/*****************************************************************************/
class ImageHistogram
{
    public:

        void compute(Image& image);
        uint32_t getMax();
        uint32_t getMin();
        uint32_t getAverage();
        uint32_t getSize();
        uint32_t getLength();
        uint32_t* getHistogram();
        std::string getArrayString();

    protected:

    private:
        uint32_t* histogram = NULL;
        uint32_t length;
        uint32_t size;
        uint32_t min;
        uint32_t max;
        uint32_t average;
        uint64_t total;
};

void ImageHistogram::compute(Image& image)
{
  this->total = 0;
  this->length = 
    (uint32_t)pow(2, image.getBitDepth());
  this->size = this->length*sizeof(uint32_t);
  this->min = std::numeric_limits<uint32_t>::max();
  this->max = std::numeric_limits<uint32_t>::min();

  assert(true != image.isPacked());
  assert(16 >= image.getBitDepth());

  if(NULL == this->histogram)
  {
    this->histogram = (uint32_t*)malloc(this->size);    
    assert(NULL != this->histogram);
  }
  else
  {
    void* ptr = realloc(this->histogram, this->size);
    assert(NULL != ptr);
  }
  memset(this->histogram, 0, this->size);
  uint32_t value;
  
  if(image.getBitDepth() <= 8)
  {
    uint8_t* buffer;
    buffer = (uint8_t*)image.getBuffer();
    for(int i = 0; i < image.getPixelCount(); i++)
    {
      value = buffer[i];
      histogram[value]++;
      this->total += value;
      if(value > this->max)
      {
        this->max = value;
      }

      if(value < this->min)
      {
        this->min = value;
      }
    }
    this->average = this->total/image.getPixelCount();
  }
  else
  {
    uint16_t* buffer;
    buffer = (uint16_t*)image.getBuffer();
    for(int i = 0; i < image.getPixelCount(); i++)
    {
      value = buffer[i];
      histogram[value]++;
      this->total += value;
      if(value > this->max)
      {
        this->max = value;
      }

      if(value < this->min)
      {
        this->min = value;
      }
    }
    this->average = this->total/image.getPixelCount();
  }
}

uint32_t ImageHistogram::getMax()
{
  return this->max;
}

uint32_t ImageHistogram::getMin()
{
  return this->min;
}
uint32_t ImageHistogram::getAverage()
{
  return this->average;
}
uint32_t ImageHistogram::getLength()
{
  return this->length;
}
uint32_t ImageHistogram::getSize()
{
    return this->size;
}
uint32_t* ImageHistogram::getHistogram()
{
    return this->histogram;
}

std::string ImageHistogram::getArrayString()
{
  std::string string;
  string = "[";
  string += std::to_string(this->histogram[0]);
  for(int i = 1; i < this->length; i++)
  {
    string += ",";
    string += std::to_string(this->histogram[i]);
  }
  string += "]";
  return string;
};
#endif /* IMAGEHISTOGRAM_H_ */