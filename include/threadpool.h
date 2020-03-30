#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <algorithm>
#include <future>

class ThreadPool
{
public:
  using Priority = unsigned;

protected:
  class BasicTask
  {
  public:
    Priority priority;

    BasicTask(Priority _priority) : priority(_priority)
    {
    }

    virtual ~BasicTask() = default;

    virtual void perform() = 0;
  };

  using TaskPtr = std::unique_ptr<BasicTask>;

  static bool less_priority(const TaskPtr& task1, const TaskPtr& task2)
  {
    return task1->priority < task2->priority;
  }

  template<typename F, typename... Args>
  class Task : public BasicTask
  {
  public:
    Task(Priority priority, F&& _f, Args&&... _args) :
      BasicTask(priority), f(std::forward<F>(_f)), args(std::forward<Args>(_args)...)
    {
    }

    void perform() override
    {
      std::apply(std::move(f), std::move(args));
    }

  protected:
    std::decay_t<F> f;
    std::tuple<std::decay_t<Args>...> args;
  };

  template<typename R, typename F, typename... Args>
  class FutureTask : public Task<F, Args...>
  {
  public:
    FutureTask(std::future<R>& future, Priority priority, F&& _f, Args&&... _args) :
      Task<F, Args...>(priority, std::forward<F>(_f), std::forward<Args>(_args)...)
    {
      future = promise.get_future();
    }

    void perform() override
    {
      if constexpr (std::is_void_v<R>)
      {
        std::apply(std::move(this->f), std::move(this->args));
        promise.set_value();
      }
      else
        promise.set_value(std::apply(std::move(this->f), std::move(this->args)));
    }

  protected:
	  std::promise<R> promise;
  };

public:

  ThreadPool(size_t thread_count) : stop(false)
  {
    threads.reserve(thread_count);
    for (size_t i = 0; i < thread_count; ++i)
      threads.emplace_back(&ThreadPool::threadFunc, this);
  }

  ~ThreadPool()
  {
    queue_lock.lock();
    stop = true;
    queue_lock.unlock();

    new_task.notify_all();
    for (auto& thread : threads)
      thread.join();
  }

  size_t getThreadCount()
  {
    return threads.size();
  }

  template<typename F, typename... Args>
  void push(Priority priority, F&& f, Args&&... args)
  {
    pushTask(new Task<F, Args...>(priority, std::forward<F>(f), std::forward<Args>(args)...));
  }

  template<typename R, typename F, typename... Args>
  void push(std::future<R>& future, Priority priority, F&& f, Args&&... args)
  {
    pushTask(new FutureTask<R, F, Args...>(future, priority, std::forward<F>(f), std::forward<Args>(args)...));
  }

  void clearQueue()
  {
    std::unique_lock<std::mutex> lock(queue_lock);
    task_queue.clear();
  }

protected:

  std::vector<std::thread> threads;

  std::vector<TaskPtr> task_queue;

  std::mutex queue_lock;

  std::condition_variable new_task;

  bool stop;

  void pushTask(BasicTask* task)
  {
    {
      std::unique_lock lock(queue_lock);
      task_queue.emplace_back(task);
      std::push_heap(task_queue.begin(), task_queue.end(), less_priority);
    }
    new_task.notify_one();
  }

  void threadFunc();
};

#endif // THREAD_POOL_H
