#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
            // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();
    // std::cout << "   Message " << msg << " has been received to the queue" << std::endl;
 
    return msg; 
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
 
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);
   
    std::cout << "   Message " << msg << " has been sent to the queue" << std::endl;

    _queue.clear();
    // add message to queue after clearing it
    _queue.push_back(std::move(msg));
    _cond.notify_one(); // notify client after pushing new message into vector

}


/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true){
   
        TrafficLightPhase msg = _message->receive();
        if(msg == TrafficLightPhase::green){
            std::cout<<"Traffic light changed to green \n";
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
    
    // create monitor object as a shared pointer to enable access by multiple threads
    _message = std::make_shared<MessageQueue<TrafficLightPhase>>();

    std::random_device rd; // obtain a random number
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_real_distribution<> distr(4000, 6000); // define the range
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;  //use for keeping track of time for a cycle

    double cycleDuration;

    std::future<void> ftr;
    
    // init stop watch
    lastUpdate = std::chrono::system_clock::now();

    while(true){

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        cycleDuration = distr(gen);

        // compute time difference to stop watch
        long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();

        if(timeSinceLastUpdate>=cycleDuration){

            if (_currentPhase == green)
            {
                _currentPhase = red;
               
            }
            else if (_currentPhase == red)
            {   
                _currentPhase = green;
 
            }

            // sending update via message queue
            ftr = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _message, std::move(_currentPhase));
            
            // reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();

        }

    }
    ftr.wait();


}

