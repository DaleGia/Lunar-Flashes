/*
  DoubleBuffer.h
  Copyright (c) 2020 Dale Giancono. All rights reserved..

  This class implements a double buffer that is thread safe. 

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
#ifndef DOUBLEBUFFER_H_
#define DOUBLEBUFFER_H_

/*****************************************************************************/
/*INLCUDES                                                                   */
/*****************************************************************************/
#include <pthread.h>

/*****************************************************************************/
/*MACROS                                                                     */
/*****************************************************************************/

/*****************************************************************************/
/*GLOBAL Data                                                                */
/*****************************************************************************/

/*****************************************************************************/
/*CLASS DECLARATION                                                          */
/*****************************************************************************/
enum DOUBLEBUFFER
{
    A = 0,
    B = 1
};

template<class T>
class DoubleBuffer
{
    public:

        void allocate(unsigned int  numberOfElements);
        void setBufferChangeCallback(
            void(*bufferChangeCallbackFunction)(T**, void*),
            void* bufferChangeObject);
        void add(T& element);
        void add(
            void(*addCallback)(T*,void*),
            void* arg);

        void copyLastAddedElement(T* buffer);
        void copyLastAddedElement(
            void(*copyCallback)(T*,void*),
            void* arg);

        T* peek(void);
        T** getBuffer(enum DOUBLEBUFFER buffer);

        uint32_t size(void);
    protected:

    private:


        enum DOUBLEBUFFER currentReadBuffer;
        enum DOUBLEBUFFER currentWriteBuffer;
        uint32_t currentIndex;
        uint32_t elementSize;
        uint32_t numberOfElements;
        T** buffers[2];
        T* lastAddedElement = NULL;
        pthread_mutex_t lastAddedElementMutex;
        void(*bufferChangeCallback)(T**,void*);
        void* bufferChangeObject;
};

template<class T> void DoubleBuffer<T>::allocate( 
    unsigned int numberOfElements)
{
    this->currentReadBuffer =
        DOUBLEBUFFER::A;
    this->currentWriteBuffer =
        DOUBLEBUFFER::B;
    this->currentIndex = 0;
    this->elementSize = 0;
    this->numberOfElements = 0;
    this->bufferChangeObject = NULL;
    pthread_mutex_init(&this->lastAddedElementMutex, NULL);

    this->elementSize = sizeof(T);
    this->numberOfElements = numberOfElements;
    assert(0 != this->numberOfElements);

    this->buffers[DOUBLEBUFFER::A] = 
        (T**)malloc(sizeof(T*)*this->numberOfElements);
    this->buffers[DOUBLEBUFFER::B] = 
        (T**)malloc(sizeof(T*)*this->numberOfElements);

    assert(NULL != this->buffers[DOUBLEBUFFER::A]);
    assert(NULL != this->buffers[DOUBLEBUFFER::B]);

    for(int i = 0; i < this->numberOfElements; i++)
    {
        this->buffers[DOUBLEBUFFER::A][i] = new T;
        assert(NULL != this->buffers[DOUBLEBUFFER::A][i]);
        this->buffers[DOUBLEBUFFER::B][i] = new T;
        assert(NULL != this->buffers[DOUBLEBUFFER::B][i]);
    }
}

template<class T> void DoubleBuffer<T>::setBufferChangeCallback(
    void(*bufferChangeCallbackFunction)(T**, void*),
    void* bufferChangeObject)
{
    this->bufferChangeCallback = bufferChangeCallbackFunction;
    this->bufferChangeObject = bufferChangeObject;
}

template<class T> void DoubleBuffer<T>::add(
    T& element)
{
    enum DOUBLEBUFFER readBufferIndex;
    uint32_t readBufferElementIndex;
    readBufferIndex = this->currentReadBuffer;
    readBufferElementIndex = this->currentIndex;
    memcpy(this->buffers[readBufferIndex][readBufferElementIndex], element, sizeof(element));

    this->currentIndex++;
    if(this->currentIndex >= this->numberOfElements)
    {
        this->currentIndex = 0;
        if(DOUBLEBUFFER::A == this->currentReadBuffer)
        {
            this->currentReadBuffer = DOUBLEBUFFER::B;
            this->currentWriteBuffer = DOUBLEBUFFER::A;
        }
        else
        {
            this->currentReadBuffer = DOUBLEBUFFER::A;
            this->currentWriteBuffer = DOUBLEBUFFER::B;
        }

        if(NULL != this->bufferChangeCallback)
        {
            this->bufferChangeCallback(
                this->currentWriteBuffer, 
                this->bufferChangeObject);
        }
    }

    pthread_mutex_lock(&this->lastAddedElementMutex);
    this->lastAddedElement = this->buffers[readBufferIndex][readBufferElementIndex];
    pthread_mutex_unlock(&this->lastAddedElementMutex);
}

template<class T> void DoubleBuffer<T>::add(
    void(*addCallback)(T*,void*),
    void* arg)
{
    enum DOUBLEBUFFER readBufferIndex;
    uint32_t readBufferElementIndex;
    readBufferIndex = this->currentReadBuffer;
    readBufferElementIndex = this->currentIndex;

    addCallback(
        this->buffers[readBufferIndex][readBufferElementIndex],
        arg);

    this->currentIndex++;
    if(this->currentIndex >= this->numberOfElements)
    {
        this->currentIndex = 0;
        if(DOUBLEBUFFER::A == this->currentReadBuffer)
        {
            this->currentReadBuffer = DOUBLEBUFFER::B;
            this->currentWriteBuffer = DOUBLEBUFFER::A;
        }
        else
        {
            this->currentReadBuffer = DOUBLEBUFFER::A;
            this->currentWriteBuffer = DOUBLEBUFFER::B;
        }

        if(NULL != this->bufferChangeCallback)
        {
            this->bufferChangeCallback(
                this->buffers[this->currentWriteBuffer], 
                this->bufferChangeObject);
        }
    }

    pthread_mutex_lock(&this->lastAddedElementMutex);
    this->lastAddedElement = this->buffers[readBufferIndex][readBufferElementIndex];
    pthread_mutex_unlock(&this->lastAddedElementMutex);
}

template<class T> T** DoubleBuffer<T>::getBuffer(enum DOUBLEBUFFER buffer)
{
    return this->buffers[buffer];
};

template<class T> void DoubleBuffer<T>::copyLastAddedElement(T* buffer)
{
    assert(NULL != buffer);
    pthread_mutex_lock(&this->lastAddedElementMutex);
    memcpy(buffer, this->lastAddedElement, sizeof(T));
    pthread_mutex_unlock(&this->lastAddedElementMutex);
    return;
}

template<class T> void DoubleBuffer<T>::copyLastAddedElement(
    void(*copyCallback)(T*,void*),
    void* arg)
{
    pthread_mutex_lock(&this->lastAddedElementMutex);
    if(NULL != this->lastAddedElement)
    {
        copyCallback(
            this->lastAddedElement,
            arg);
    }
    pthread_mutex_unlock(&this->lastAddedElementMutex);
}


template<class T> T* DoubleBuffer<T>::peek(void)
{
    T* element;
    pthread_mutex_lock(&this->lastAddedElementMutex);
    element = this->lastAddedElement;
    pthread_mutex_unlock(&this->lastAddedElementMutex);
    return element;
};

template<class T> uint32_t DoubleBuffer<T>::size(void)
{
    return this->numberOfElements;
};
#endif /* DOUBLEBUFFER_H_ */