#ifndef TRACKERWRAP_H
#define TRACKERWRAP_H

#include <nan.h>
#include <memory>

namespace nodegit {
  // Base class used to track objects.
  // Implementation based on node.js's class RefTracker (napi).
  class TrackerWrap : public Nan::ObjectWrap {
  public:
    TrackerWrap() = default;
    virtual ~TrackerWrap() = default;
    TrackerWrap(const TrackerWrap &other) = delete;
    TrackerWrap(TrackerWrap &&other) = delete;
    TrackerWrap& operator=(const TrackerWrap &other) = delete;
    TrackerWrap& operator=(TrackerWrap &&other) = delete;

    using TrackerList = TrackerWrap;

    // Links 'this' right after 'listStart'
    inline void Link(TrackerList* listStart) {
      m_prev = listStart;
      m_next = listStart->m_next;
      if (m_next != nullptr) {
        m_next->m_prev = this;
      }
      listStart->m_next = this;
    }

    // Unlinks itself
    inline void Unlink() {
      if (m_prev != nullptr) {
        m_prev->m_next = m_next;
      }
      if (m_next != nullptr) {
        m_next->m_prev = m_prev;
      }
      m_prev = nullptr;
      m_next = nullptr;
    }

    inline void SetOwners(std::unique_ptr< std::vector<TrackerList*> > &&owners) {
      m_owners = std::move(owners);
    }

    inline const std::vector<TrackerList*>* GetOwners() const {
      return m_owners.get();
    }

    // Deletes 'listStart'
    static void DeleteAll(TrackerList* listStart) {
      while (listStart->m_next != nullptr) {
        delete listStart->m_next;
      }
    }

  private:
    TrackerList* m_next {};
    TrackerList* m_prev {};
    std::unique_ptr< std::vector<TrackerList*> > m_owners {};
  };
}

#endif
