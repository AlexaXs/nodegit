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

    // alias:
    // 'TrackerList': used for functionality related to a list.
    // 'TrackerWrap' used for functionality not related to a list.
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
    inline TrackerWrap* Unlink() {
      if (m_prev != nullptr) {
        m_prev->m_next = m_next;
      }
      if (m_next != nullptr) {
        m_next->m_prev = m_prev;
      }
      m_prev = nullptr;
      m_next = nullptr;
      return this;
    }

    inline void SetOwners(std::unique_ptr< std::vector<TrackerWrap*> > &&owners) {
      m_owners = std::move(owners);
    }

    inline const std::vector<TrackerWrap*>* GetOwners() const {
      return m_owners.get();
    }

    // Deletes 'listStart'
    static void DeleteAll(TrackerList* listStart);

  private:
    TrackerList* m_next {};
    TrackerList* m_prev {};
    std::unique_ptr< std::vector<TrackerWrap*> > m_owners {};
  };
}

#endif
