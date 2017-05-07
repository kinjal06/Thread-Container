/*
 * main.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: Kinjal Thaker
 */
#include<queue>
#include<mutex>
#include<thread>
#include<atomic>
#include<iostream>
#include<condition_variable>
#include<gtest/gtest.h>
template <typename T>
class BlockingQueue
{
public:
	//initialize the queue with capacity
	explicit BlockingQueue(size_t max_size = 0):shutdown_(false),current_size(0),max_size(max_size) {}
	//The add? method will add one element to the FIFO and, if needed, block the caller until space is available.
	void add(T const & item)
	{
		std::unique_lock<std::mutex> mutex_lock(mutex_);//creates synchronized block
		//blocks the caller until the space is available
		if (current_size == max_size)
		{
			cond_add.wait(mutex_lock);
		}
		// checks if the queue is closed and throws runtime shutdown exception
		if (shutdown_ == true)
		{
			throw std::runtime_error("Shutdown Exception: The queue is closed");
		}
		//push the data to the queue
		current_size += 1;
		queue_.push(item);
		//mlock.unlock();
		//wake up on popping thread
		cond_remove.notify_one();
	}
	//this method will block the caller until an element is available for retrieval.
	T remove()
	{
		std::unique_lock<std::mutex> mutex_lock(mutex_);//creates synchronized block
		//if the queue is empty and the queue is running, we wait for the item to be added
		while (queue_.empty()&& shutdown_ != true)
		{
			cond_remove.wait(mutex_lock);
		}
		//if queue is empty or closed throw shutdown exception
		if (queue_.empty() && shutdown_ == true)
		{
			throw std::runtime_error("Shutdown Exception: The queue is closed");
		}
		current_size -= 1;
		auto rem_queue = queue_.front();
		queue_.pop();
		return rem_queue;
		//wake up on pushing thread
		cond_add.notify_one();
	}
	// This method will remove any objects which were added, but not yet removed.
	void clear(std::queue<T> & q)
	{
		std::unique_lock<std::mutex> mutex_lock(mutex_);//creates synchronized block
		std::queue<T> empty;
		std::swap(q,empty);
		cond_remove.notify_all();
		cond_add.notify_all();

	}
	//method will cause any blocked or future calls to add ?or remove ?to throw a ShutdownException.?
	void shutdown() noexcept
	{
		std::lock_guard<std::mutex> mutex_lock(mutex_);
		shutdown_ = true;
		//notify all consumers
		cond_remove.notify_all();
		//notify all producers
		cond_add.notify_all();
	}
private:
	bool shutdown_;
	size_t current_size,max_size;
	std::queue<T> queue_;
	std::mutex mutex_;
	std::condition_variable cond_add, cond_remove;
};
//if the queue doesnt have item,remove() will wait for item to be added in queue
TEST(BlockingQueueTest, testcase_remove)
{
	BlockingQueue<int> queue1(10);
	EXPECT_EQ(queue1.remove(),0);
}
//this is to check functionality of shutdown
TEST(BlockingQueueTest, testcase_shutdown)
{
	BlockingQueue<int> queue(10);
	queue.shutdown();
	queue.add(2);
	EXPECT_EQ(queue.remove(),2);
}


int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();

}
