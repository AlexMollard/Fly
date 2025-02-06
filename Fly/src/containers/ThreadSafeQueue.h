#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

template<typename BufferType>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue() = default;

	// Prevent copying
	ThreadSafeQueue(const ThreadSafeQueue&) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

	// Allow moving
	ThreadSafeQueue(ThreadSafeQueue&& other) noexcept
	{
		std::lock_guard<std::mutex> lock(other.mutex);
		queue = std::move(other.queue);
		terminated = other.terminated.load();
	}

	ThreadSafeQueue& operator=(ThreadSafeQueue&& other) noexcept
	{
		if (this != &other)
		{
			// Lock both queues to prevent race conditions
			std::scoped_lock lock(mutex, other.mutex);
			queue = std::move(other.queue);
			terminated = other.terminated.load();
		}
		return *this;
	}

	void push(BufferType&& buffer)
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			queue.push(std::move(buffer));
		}
		cond.notify_one();
	}

	bool pop(BufferType& buffer)
	{
		std::unique_lock<std::mutex> lock(mutex);
		while (queue.empty() && !terminated)
		{
			cond.wait(lock);
		}

		if (terminated && queue.empty())
		{
			return false;
		}

		buffer = std::move(queue.front());
		queue.pop();
		return true;
	}

	void clear()
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::queue<BufferType> empty;
		std::swap(queue, empty);
	}

	void terminate()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			terminated = true;
		}
		cond.notify_all();
	}

	std::size_t size() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return queue.size();
	}

private:
	std::queue<BufferType> queue;
	mutable std::mutex mutex;
	std::condition_variable cond;
	std::atomic<bool> terminated{ false };
};
