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
#include <chrono>
#include <sys/vfs.h>

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
        /* Data will get overwritten when this pause function is used */
        void pause();
        void unpause();
        bool isPaused();

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
                if(self->pauseWriteFlag == false)
                {
                    pthread_mutex_unlock(&self->writeMutexSignal);
                }
                else
                {
                    // Writing is paused at the moment
                }
            }
            else
            {
                if(self->bufferOverflowFunction != NULL)
                {
                    self->bufferOverflowFunction(self);
                }
            }
        };

        // void writeBufferToDisk(
        //     void(*writeDataCallback)(T*, FILE*),
        //     ConcurrentFileIoBufferElement<T>** buffer)
        // {
        //     this->currentWriteState = 
        //         ConcurrentFileIoBuffer::BUFFER_WRITE_STATE::WRITE_IN_PROGRESS;

        //     for(int i = 0; i < this->size(); i++)
        //     {
        //         std::string filepath;
        //         ConcurrentFileIoBufferElement<T>* element;
        //         element = buffer[i];
        //         size_t count = 0;

        //         filepath = 
        //             this->savePath + 
        //             "/" + 
        //             element->getFilename(); 

        //         FILE* stream = fopen(filepath.c_str(), "a");
 
        //         if(stream == NULL)
        //         {
        //             std::cout << "Unable to save file at " << filepath << std::endl;
        //             return;
        //         }
        //         else
        //         {
        //             writeDataCallback(
        //                 element->get(),
        //                 stream);

        //             fclose(stream);
                    

        //         }
        //     }

        //     this->currentWriteState = ConcurrentFileIoBuffer::BUFFER_WRITE_STATE::WRITE_COMPLETE;
        // }

        void writeBufferToDisk(
            void(*writeDataCallback)(T*, FILE*),
            ConcurrentFileIoBufferElement<T>** buffer)
        {
            this->currentWriteState = 
                ConcurrentFileIoBuffer::BUFFER_WRITE_STATE::WRITE_IN_PROGRESS;
            
            std::string filepath;
            ConcurrentFileIoBufferElement<T>* element;

            element = buffer[0];
            filepath = this->savePath + "/";
            filepath += element->getFilename();
            element = buffer[this->size()-1];
            filepath += "-";
            filepath += element->getFilename();
            filepath += ".blob";
            FILE* stream = fopen(filepath.c_str(), "a");
            if(NULL == stream)
            {
                std::cout << filepath << " cannot be opened to write buffer..." << std::endl;
                return;
            }

            for(int i = 0; i < this->size(); i++)
            {
                element = buffer[i];
                size_t count = 0;

                writeDataCallback(
                    element->get(),
                    stream);
            }

            fclose(stream);                    
            this->currentWriteState = ConcurrentFileIoBuffer::BUFFER_WRITE_STATE::WRITE_COMPLETE;
        }


        enum BUFFER_WRITE_STATE currentWriteState =
            BUFFER_WRITE_STATE::WRITE_COMPLETE;
        std::string savePath;
        pthread_t writeThread;
        pthread_mutex_t writeMutexSignal;
        /* This provides a mechanism to pause writing without causing buffer overflows*/
        bool pauseWriteFlag = false;
        ConcurrentFileIoBufferElement<T>** writeBufferPointer;
        void(*writeDataCallback)(T*, FILE*);
        void(*bufferOverflowFunction)(void*) = NULL;
        void* bufferOverflowObject = NULL;

};

template<class T> void ConcurrentFileIoBuffer<T>::setSaveDirectory(
    std::string path)
{
    this->savePath = path;
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

template<class T> void ConcurrentFileIoBuffer<T>::pause(void)
{
    this->pauseWriteFlag = true;
}

template<class T> void ConcurrentFileIoBuffer<T>::unpause(void)
{
    this->pauseWriteFlag = false;
}

template<class T> bool ConcurrentFileIoBuffer<T>::isPaused(void)
{
    return this->pauseWriteFlag;
}
#endif /* CONCURRENTFILEIOBUFFER_H_ */