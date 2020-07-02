#include <iostream>
#include <random>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    /* Create a lock and pass it to the condition variable */
	std::unique_lock<std::mutex> u_lock(_mutex);
	_conditional_variable.wait(u_lock, [this] { return !_queue.empty(); });

	/* return last element in queue */
	T msg = std::move(_queue.back());
	_queue.pop_back();
	return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // aquire lock, lock should be released when function is popped off the stack
    std::lock_guard<std::mutex> u_lock(_mutex);

    // add message to the queue
    _queue.push_back(std::move(msg));
	_conditional_variable.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _messageQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.

    while(true) {
        TrafficLightPhase _tlPhase = _messageQueue->receive();
        if (_tlPhase == TrafficLightPhase::green) {
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
    _threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // pulled from help here https://knowledge.udacity.com/questions/219289
    int randSleep = (rand() % 2000) + 4000;
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::system_clock::now();
    int64_t duration;

    while (true) {
        // sleep for a millisecond
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // calculate the duration
        duration = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now() - t1 ).count();;
        
        // if the duration at a certain color is longer thant the sleep time then flip the light
        if (duration > randSleep) {
            // flip the traffic light
            if (this->_currentPhase == TrafficLightPhase::red) {
                this->_currentPhase = TrafficLightPhase::green;
            } else {
                this->_currentPhase = TrafficLightPhase::red;
            }

            // send out the current phase
			auto msg = _currentPhase;
			auto is_sent = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _messageQueue, std::move(msg));
            is_sent.wait();

            // reset the timing values
            t1 = std::chrono::system_clock::now();
            randSleep = (rand() % 2000) + 4000;
        }
    }
}
