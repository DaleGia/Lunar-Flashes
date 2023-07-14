/*
  RateMonitor.h
  Copyright (c) 2023 Dale Giancono. All rights reserved..

  This class implements a way to keep track of how much something is happening.
  Useful for benchmarking or Keeping track of some quanity over time.

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
#ifndef RATEMONITOR_H_
#define RATEMONITOR_H_

/*****************************************************************************/
/*INLCUDES                                                                   */
/*****************************************************************************/
#include <pthread.h>
#include <vector>
#include <chrono>
#include <unistd.h>

/*****************************************************************************/
/*MACROS                                                                     */
/*****************************************************************************/

/*****************************************************************************/
/*GLOBAL Data                                                                */
/*****************************************************************************/

/*****************************************************************************/
/*CLASS DECLARATION                                                          */
/*****************************************************************************/
class Counter
{
    public:
        void increment(double value);
        void reset(void);
        double getRatePerSecond(void);
        double getRunningTotal(void);
        void processRate(int64_t periodMicroSeconds);
        pthread_mutex_t incrementLock;


    private:
        double runningTotal = 0;
        double measureTotal = 0;    
        double ratePerSecond = 0;
};

class RateMonitor
{

    public:
        void registerCounter(Counter &counter);
        void incrementCounter(Counter& counter, double value);

        void start(void);
        void resetCounters();

        friend class Counter;

    protected:

    private:
        static void* rateMonitorFunction(void* self);
        pthread_t thread;
        std::vector<Counter*> counters; 
};

void Counter::increment(double value)
{
    pthread_mutex_lock(&this->incrementLock);
    this->runningTotal += value;
    this->measureTotal += value;
    pthread_mutex_unlock(&this->incrementLock);
}    

void Counter::reset(void)
{
    pthread_mutex_lock(&this->incrementLock);
    this->runningTotal = 0;
    this->measureTotal = 0;
    this->ratePerSecond = 0;
    pthread_mutex_unlock(&this->incrementLock);
}    

double Counter::getRatePerSecond(void)
{
    double rps;
    pthread_mutex_lock(&this->incrementLock);
    rps = this->ratePerSecond;
    pthread_mutex_unlock(&this->incrementLock);
    return rps;
}    

double Counter::getRunningTotal(void)
{
    double rt;
    pthread_mutex_lock(&this->incrementLock);
    rt = this->runningTotal;
    pthread_mutex_unlock(&this->incrementLock);
    return rt;
}    


void Counter::processRate(int64_t periodMicroSeconds)
{
    pthread_mutex_lock(&this->incrementLock);
    double timePeriodSeconds = (double)periodMicroSeconds / 1000000.0;
    this->ratePerSecond = this->measureTotal / timePeriodSeconds;
    this->measureTotal = 0;

    pthread_mutex_unlock(&this->incrementLock);
}    


void RateMonitor::registerCounter(Counter &counter)
{
    pthread_mutex_init(&counter.incrementLock, NULL);
    this->counters.push_back(&counter);
}

void RateMonitor::start(void)
{
    pthread_create(&this->thread, NULL, rateMonitorFunction, this);
}    

void RateMonitor::resetCounters(void)
{
    for(int i = 0; i < this->counters.size(); i++)
    {
        this->counters.at(i)->reset();
    }
}    


void* RateMonitor::rateMonitorFunction(void* self)
{
    RateMonitor* instance = (RateMonitor*)self;
    auto endTime = std::chrono::high_resolution_clock::now();
    auto startTime = std::chrono::high_resolution_clock::now();
    sleep(1);
    while(1)
    {
        endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        startTime = std::chrono::high_resolution_clock::now();

        for(int i = 0; i < instance->counters.size(); i++)
        {
            instance->counters.at(i)->processRate(duration);
        }
        sleep(1);
    }
}

#endif /* RATEMONITOR_H_ */