#ifndef TRACKERWRAP_H
#define TRACKERWRAP_H

#include <nan.h>
#include <memory>
#include <vector>

namespace nodegit {
  // Base class used to track wrapped objects, so that we can
  // free the objects that were not freed (because their
  // WeakCallback didn't trigger) at the time of context closing.
  // Implementation based on node.js's class RefTracker (napi).
  class TrackerWrap : public Nan::ObjectWrap {
  public:
    TrackerWrap() = default;
    virtual ~TrackerWrap() = default;
    TrackerWrap(const TrackerWrap &other) = delete;
    TrackerWrap(TrackerWrap &&other) = delete;
    TrackerWrap& operator=(const TrackerWrap &other) = delete;
    TrackerWrap& operator=(TrackerWrap &&other) = delete;

    // aliases:
    // 'TrackerList': used in functionality related to a list.
    // 'TrackerWrap' used in functionality not related to a list.
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

    // Unlinks itself from the list it's linked to
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

    inline void SetTrackerWrapOwners(std::unique_ptr< std::vector<TrackerWrap*> > &&owners) {
      m_owners = std::move(owners);
    }

    inline const std::vector<TrackerWrap*>* GetTrackerWrapOwners() const {
      return m_owners.get();
    }

    // Unlinks and returns the first item of 'listStart'
    static TrackerWrap* UnlinkFirst(TrackerList* listStart) {
      assert(listStart != nullptr);
      return listStart->m_next == nullptr ? nullptr : listStart->m_next->Unlink();
    }

    // Deletes items following 'listStart', but not 'listStart' itself
    static void DeleteFromList(TrackerList* listStart);

  private:
    TrackerList* m_next {};
    TrackerList* m_prev {};
    // m_owners will store pointers to native objects
    std::unique_ptr< std::vector<TrackerWrap*> > m_owners {};
  };
}

#endif
