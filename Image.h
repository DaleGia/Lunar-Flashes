/*
  Image.h
  Copyright (c) 2023 Dale Giancono. All rights reserved..

  This class implements a way to represent image data.

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
#ifndef IMAGE_H_
#define IMAGE_H_

/*****************************************************************************/
/*INLCUDES                                                                   */
/*****************************************************************************/
#include <stdint.h>
#include <stdlib.h>

/*****************************************************************************/
/*MACROS                                                                     */
/*****************************************************************************/

/*****************************************************************************/
/*GLOBAL Data                                                                */
/*****************************************************************************/

/*****************************************************************************/
/*CLASS DECLARATION                                                          */
/*****************************************************************************/
class Image
{
    public:
        Image(){};
        Image(
            uint8_t* buffer, 
            uint32_t bufferSize, 
            uint32_t width,
            uint32_t height)
            : 
            buffer{buffer}, 
            bufferSize{bufferSize}, 
            width{width},
            height{height},
            bitDepth{bitDepth}
            {};

        ~Image();
        void* allocate(uint32_t size);
        uint64_t getFrameId();
        uint64_t getTimestamp();
        uint32_t getHeight();
        uint32_t getWidth();
        uint8_t getBitDepth();
        bool isPacked();
        uint32_t getBufferSize();
        uint32_t getPixelCount();
        void* getBuffer(void);

        void copy(Image& buffer);
        void copy(Image* buffer);

        void copyBuffer(void* buffer, uint32_t bufferSize);

        void setFrameId(uint64_t frameId);
        void setTimestamp(uint64_t timestamp);
        void setHeight(uint32_t height);
        void setWidth(uint32_t width);
        void setBitDepth(uint8_t bitDepth);
        void setPackedStatus(bool status);
        void setBuffer(void* buffer, uint32_t bufferSize);        
        
    protected:

    private:
        uint64_t frameId;
        uint64_t timestamp;
        uint32_t height;
        uint32_t width;
        uint8_t bitDepth;
        bool packed;
        uint32_t bufferSize;
        uint32_t pixelCount;
        uint8_t* buffer = NULL;
};

Image::~Image()
{
    if(NULL != this->buffer)
    {
        free(this->buffer);
    }
}
void* Image::allocate(uint32_t size)
{
    if(NULL == this->buffer)
    {
        this->buffer = (uint8_t*)malloc(size);
    }
    else if(this->bufferSize < size)
    {
        void* ptr = realloc(this->buffer, size);
        assert(ptr != NULL);
    }

    if(NULL == this->buffer)
    {
        this->bufferSize = 0;
    }
    else
    {
        this->bufferSize = size;
    }

    return this->buffer;
}

uint64_t Image::getFrameId()
{
    return this->frameId;
}

uint64_t Image::getTimestamp()
{
    return this->timestamp;
}

uint32_t Image::getHeight()
{
    return this->height;
}

uint32_t Image::getWidth()
{
    return this->width;
}

uint8_t Image::getBitDepth()
{
    return this->bitDepth;
}

void* Image::getBuffer()
{
    return (void*)this->buffer;
}

void Image::copy(Image& buffer)
{
    buffer.bufferSize = this->bufferSize;
    buffer.frameId = this->frameId;
    buffer.height = this->height;
    buffer.timestamp = this->timestamp;
    buffer.width = this->width;
    buffer.packed = this->packed;
    buffer.bitDepth = this->bitDepth;
    buffer
    .setBuffer(this->buffer, this->bufferSize);
}

void Image::copy(Image* buffer)
{
    buffer->bufferSize = this->bufferSize;
    buffer->frameId = this->frameId;
    buffer->height = this->height;
    buffer->timestamp = this->timestamp;
    buffer->width = this->width;
    buffer->packed = this->packed;
    buffer->bitDepth = this->bitDepth;
    buffer->setBuffer(this->buffer, this->bufferSize);
}

void Image::copyBuffer(
    void* buffer, 
    uint32_t bufferSize)
{
    memcpy(buffer, this->buffer, bufferSize);
}



bool Image::isPacked()
{
    return this->packed;
}

uint32_t Image::getBufferSize()
{
    return this->bufferSize;
}

uint32_t Image::getPixelCount()
{
    return  this->height*this->width;
}


void Image::setFrameId(uint64_t frameId)
{
    this->frameId = frameId;
}

void Image::setTimestamp(uint64_t timestamp)
{
    this->timestamp = timestamp;
}

void Image::setHeight(uint32_t height)
{
    this->height = height;
}

void Image::setWidth(uint32_t width)
{
    this->width = width;
}

void Image::setBitDepth(uint8_t bitDepth)
{
    this->bitDepth = bitDepth;
}

void Image::setPackedStatus(bool status)
{
    this->packed = status;
}

void Image::setBuffer(void* buffer, uint32_t bufferSize)
{
    this->allocate(bufferSize);
    memcpy(this->buffer, buffer, bufferSize);
}



#endif /* IMAGE_H_ */