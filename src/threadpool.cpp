#include "threadpool.h"

void ThreadPool::threadFunc()
{
  auto wake_up = [&]()
  {
    return stop || !task_queue.empty();
  };

  TaskPtr task;

  for (; ; )
  {
    {
      std::unique_lock lock(queue_lock);
      new_task.wait(lock, wake_up);

      //finish by stop signal after performing all tasks in queue
      if (stop && task_queue.empty())
        return;

      task = std::move(task_queue.front());
      std::pop_heap(task_queue.begin(), task_queue.end(), less_priority);
      task_queue.pop_back();
    }
    task->perform();
  }
}
