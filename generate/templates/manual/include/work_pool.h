#ifndef WORK_POOL_H
#define WORK_POOL_H

#include <type_traits>
#include <functional>
#include <memory>
#include <queue>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <thread>

/**
 * \class WorkItem
 * Abstract class for work items in the WorkerPool.
 */
class WorkItem
{
public:
  WorkItem() = default;
  virtual ~WorkItem() = default;
  WorkItem(const WorkItem &other) = default;
  WorkItem(WorkItem &&other) = default;
  WorkItem& operator=(const WorkItem &other) = default;
  WorkItem& operator=(WorkItem &&other) = default;
};

/**
 * \class Worker
 * Interface for Workers in the WorkerPool.
 */
class IWorker
{
public:
  IWorker() = default;
  virtual ~IWorker() = default;
  IWorker(const IWorker &other) = delete;
  IWorker(IWorker &&other) = delete;
  IWorker& operator=(const IWorker &other) = delete;
  IWorker& operator=(IWorker &&other) = delete;

  virtual bool Initialize() = 0;
  virtual bool Execute(WorkItem *item) = 0;
};


/**
 * \class WorkerPool
 * To leverage this class, a Worker must inherit from IWorker.
 * WorkItem is an abstract class from which to inherit too.
 */
template<class Worker, class WorkItem>
class WorkerPool {
public:
  WorkerPool() {
    static_assert(std::is_base_of<IWorker, Worker>::value, "Worker must inherit from IWorker");
  }
  
  ~WorkerPool() = default;
  WorkerPool(const WorkerPool &other) = delete;
  WorkerPool(WorkerPool &&other) = delete;
  WorkerPool& operator=(const WorkerPool &other) = delete;
  WorkerPool& operator=(WorkerPool &&other) = delete;

  // returns true it it wasn't initialized previously; false otherwise
  bool Init(std::vector< std::shared_ptr<Worker> > workers) {
    if (m_init)
      return false;
    
    m_init = true;
    std::for_each (workers.begin(), workers.end(), [this](std::shared_ptr<Worker> worker) {
      m_threads.emplace_back(std::make_unique<std::thread>(std::bind(&WorkerPool::DoWork, this, worker)));
    });
    
    return true;
  }
  
  // returns true if successfully inserted work; false otherwise
  bool InsertWork(const WorkItem &item) {
    if(!m_init)
      return false;
    
    {
      std::lock_guard<std::mutex> lock(m_workQueueMutex);
      m_workQueue.emplace(item);
    }
    m_workQueueCondition.notify_one();
    
    return true;
  }
  
  void Shutdown() {
    {
      std::lock_guard<std::mutex> lock(m_workQueueMutex);
      m_workQueue.emplace();    // emplaces SHUTDOWN_SIGNAL to signal worker threads to end
    }
    m_workQueueCondition.notify_all();
    std::for_each (m_threads.begin(), m_threads.end(), [](std::unique_ptr<std::thread> &wt) {
      if (wt->joinable()) {
        wt->join();
      }
    });
    
    m_init = false;
  }
  
private:
  /**
   * \class QueueEntity
   */
  class QueueEntity {
  public:
    enum class QEType { WORKITEM, SHUTDOWN_SIGNAL };
    
    QueueEntity() : m_type(QEType::SHUTDOWN_SIGNAL) {}
    QueueEntity(const WorkItem &item) : m_item(item) {}
    ~QueueEntity() = default;
    QueueEntity(const QueueEntity &other) = default;
    QueueEntity(QueueEntity &&other) = default;
    QueueEntity& operator=(const QueueEntity &other) = default;
    QueueEntity& operator=(QueueEntity &&other) = default;
    
    const QEType& Type() const { return m_type; }
    const WorkItem& Item() const { return m_item; }
    WorkItem& Item() { return m_item; }
    
  private:
    QEType m_type {QEType::WORKITEM};
    WorkItem m_item {};
  };
  
  QueueEntity GetWork() {
    std::unique_lock<std::mutex> lock(m_workQueueMutex);
    m_workQueueCondition.wait(lock, [this] { return !this->m_workQueue.empty(); });
    
    QueueEntity entity = m_workQueue.front();
    if (entity.Type() == QueueEntity::QEType::WORKITEM) {
      m_workQueue.pop();
    }
    return entity;
  }
  
  void DoWork(std::shared_ptr<Worker> worker) {
    if (!worker->Initialize()) {
      // signal main that we've exited early
      return;
    }
    
    while (true)
    {
      QueueEntity entity = GetWork();
      if (entity.Type() == QueueEntity::QEType::SHUTDOWN_SIGNAL) {
        return;
      }
      if (!worker->Execute(&entity.Item())) {
        // signal main that we've exited early
        return;
      }
    }
  }
  
  std::mutex m_workQueueMutex {};
  std::condition_variable m_workQueueCondition {};
  std::queue<QueueEntity> m_workQueue {};
  std::vector<std::unique_ptr<std::thread>> m_threads {};
  bool m_init {false}; // initially the workpool has no worker threads
};

#endif  // WORK_POOL_H

