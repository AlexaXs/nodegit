#ifndef WORK_POOL_H
#define WORK_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

// this class was originally based on https://github.com/progschj/ThreadPool
class WorkPool {
public:
  WorkPool(size_t numThreads, bool init);
  template<class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>;
  ~WorkPool();
  
  void Init();
  void Shutdown();
  
private:
  size_t m_numThreads {0};
  std::vector< std::thread > m_workers {};
  std::queue< std::function<void()> > m_tasks {};
  // synchronization
  std::mutex m_queueMutex {};
  std::condition_variable m_condition {};
  bool m_stop {true};
};

// the constructor just launches some amount of workers
inline WorkPool::WorkPool(size_t numThreads, bool init)
:   m_numThreads(numThreads)
{
  if (init) {
    Init();
  }
}

// add new work item to the pool
template<class F, class... Args>
auto WorkPool::enqueue(F&& f, Args&&... args)
  -> std::future<typename std::result_of<F(Args...)>::type>
{
  using return_type = typename std::result_of<F(Args...)>::type;
  
  auto task = std::make_shared< std::packaged_task<return_type()> >(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  
  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    
    // don't allow enqueueing after stopping the pool
    if(m_stop)
      throw std::runtime_error("enqueue on stopped ThreadPool");
    
    m_tasks.emplace([task](){ (*task)(); });
  }
  m_condition.notify_one();
  return res;
}

// the destructor joins all threads
inline WorkPool::~WorkPool()
{
  Shutdown();
}

inline void WorkPool::Init()
{
  {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    if (!m_stop)
      return;
    m_stop = false;
  }
  
  for (size_t i = 0; i < m_numThreads; ++i) {
    m_workers.emplace_back( [this] {
        while (true)
        {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->m_queueMutex);
            
            this->m_condition.wait(lock, [this]
              { return this->m_stop || !this->m_tasks.empty(); }
            );
            
            if (this->m_stop && this->m_tasks.empty())
              return;
            
            task = std::move(this->m_tasks.front());
            this->m_tasks.pop();
          }
          task();
        }
      }
    );
  }
}

inline void WorkPool::Shutdown()
{
  {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    if (m_stop)
      return;
    m_stop = true;
  }
  m_condition.notify_all();
  for (std::thread &worker: m_workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

#endif  // WORK_POOL_H

