/*
  ConcurrentFileIOBuffer.h
  Copyright (c) 2023 Dale Giancono. All rights reserved..

  This class implements a buffer that can read data, and concurrently
  write data to disk using pthreads.

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
#ifndef CONCURRENTFILEIOBUFFER_H_
#define CONCURRENTFILEIOBUFFER_H_

/*****************************************************************************/
/*INLCUDES                                                                   */
/*****************************************************************************/
#include "DoubleBuffer.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/*MACROS                                                                     */
/*****************************************************************************/

/*****************************************************************************/
/*GLOBAL Data                                                                */
/*****************************************************************************/

/*****************************************************************************/
/*CLASS DECLARATION                                                          */
/*****************************************************************************/
template<class T>
class ConcurrentFileIoBufferElement
{
    public:
        unsigned int size(void)
        {
            return this->elementSize;
        }

        T* get()
        {
            T* element;
            element = &this->element;
            return element;
        }


        std::string getFilename(void)
        {
            return this->filename;
        }

        void setFilename(std::string filename)
        {
            this->filename = filename;
        }

        void set(T &buffer,  std::string filename)
        {
            memcpy(&this->element, buffer, this->elementSize);
            this->filename = filename;
        }
    private:
        std::string filename;
        T element;
        unsigned int elementSize;
};

template<class T>
class ConcurrentFileIoBuffer: public DoubleBuffer<ConcurrentFileIoBufferElement<T>>
{
    public:
        void setSaveDirectory(std::string path);
        void start(void(*writeDataCallback)(T*, FILE*));
        void setBufferOverflowHandler(
            void(*bufferOverflowFunction)(void*),
            void* bufferOverflowObject);
    private:
        enum BUFFER
        {
            A = 0,
            B = 1
        };

        enum BUFFER_WRITE_STATE
        {
            WRITE_IN_PROGRESS,
            WRITE_COMPLETE
        };

        static void* bufferWriterFunction(void* arg)
        {
            ConcurrentFileIoBuffer* self = (ConcurrentFileIoBuffer*)arg;
            while(1)
            {
                //Wait for mutex
                pthread_mutex_lock(&self->writeMutexSignal);
                self->writeBufferToDisk(
                    self->writeDataCallback, 
                    self->writeBufferPointer);
            }  
        };

        static void triggerBufferWrite(
            ConcurrentFileIoBufferElement<T>** buffer,
            void* arg)
        {
            ConcurrentFileIoBuffer* self = (ConcurrentFileIoBuffer*)arg;
            self->writeBufferPointer = buffer;
            if(ConcurrentFileIoBuffer::BUFFER_WRITE_STATE::WRITE_COMPLETE == self->currentWriteState)
            {
                pthread_mutex_unlock(&self->writeMutexSignal);
            }
            else
            {
                if(self->bufferOverflowFunction != NULL)
                {
                    self->bufferOverflowFunction(self);
                }
            }
        };

        void writeBufferToDisk(
            void(*writeDataCallback)(T*, FILE*),
            ConcurrentFileIoBufferElement<T>** buffer)
        {
            this->currentWriteState = 
                ConcurrentFileIoBuffer::BUFFER_WRITE_STATE::WRITE_IN_PROGRESS;

            for(int i = 0; i < this->size(); i++)
            {
                std::string filepath;
                ConcurrentFileIoBufferElement<T>* element;
                element = buffer[i];
                size_t count = 0;

                filepath = 
                    this->frameSavePath + 
                    "/" + 
                    element->getFilename(); 

                
                FILE* stream = fopen(filepath.c_str(), "w");
                if(stream == NULL)
                {
                    std::cout << "Unable to save file at " << filepath << std::endl;
                    return;
                }
                else
                {
                    writeDataCallback(
                        element->get(),
                        stream);
                    fclose(stream);
                }
            }

            this->currentWriteState = ConcurrentFileIoBuffer::BUFFER_WRITE_STATE::WRITE_COMPLETE;
        }

        enum BUFFER_WRITE_STATE currentWriteState =
            BUFFER_WRITE_STATE::WRITE_COMPLETE;
        std::string frameSavePath;
        pthread_t writeThread;
        pthread_mutex_t writeMutexSignal;
        ConcurrentFileIoBufferElement<T>** writeBufferPointer;
        void(*writeDataCallback)(T*, FILE*);
        void(*bufferOverflowFunction)(void*) = NULL;
        void* bufferOverflowObject = NULL;

};

template<class T> void ConcurrentFileIoBuffer<T>::setSaveDirectory(
    std::string path)
{
    this->frameSavePath = path;
}

template<class T> void ConcurrentFileIoBuffer<T>::setBufferOverflowHandler(
    void(*bufferOverflowFunction)(void*),
    void* bufferOverflowObject)
{
    this->bufferOverflowFunction = bufferOverflowFunction;
    this->bufferOverflowObject = bufferOverflowObject;
}


template<class T> void ConcurrentFileIoBuffer<T>::start(
    void(*writeDataCallback)(T*, FILE*))
{
    pthread_mutex_init(&this->writeMutexSignal, NULL);
    pthread_mutex_lock(&this->writeMutexSignal);

    this->setBufferChangeCallback(triggerBufferWrite, this);
    this->writeDataCallback = writeDataCallback;
    pthread_create(&this->writeThread, NULL, bufferWriterFunction, this);
};
#endif /* CONCURRENTFILEIOBUFFER_H_ */