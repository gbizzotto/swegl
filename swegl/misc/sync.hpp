
#pragma once

#include <mutex>
#include <atomic>
#include <condition_variable>

namespace swegl
{
	
	struct sync_point_t
	{
		std::mutex mutex;
		std::condition_variable cv;
		std::atomic_int  count;
		std::atomic_bool all_there;

		inline sync_point_t()
			: count(0)
			, all_there(false)
		{}

		void sync(int thread_count)
		{
			std::unique_lock guard(mutex);
			if (++count == thread_count)
			{
				all_there = true;
				cv.notify_all();
				if (--count == 0)
					all_there = false;
			}
			else
			{
				cv.wait(guard, [&]{ return !!all_there; });
				if (--count == 0)
					all_there = false;
			}
		}
	};

}