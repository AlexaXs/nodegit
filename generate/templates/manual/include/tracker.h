#ifndef TRACKER_H
#define TRACKER_H

namespace nodegit {
  // Base class used to track objects.
  // Implementation based on node.js's class RefTracker (napi).
  class Tracker {
  public:
    Tracker() = default;
    virtual ~Tracker() = default;
    Tracker(const Tracker &other) = delete;
    Tracker(Tracker &&other) = delete;
    Tracker& operator=(const Tracker &other) = delete;
    Tracker& operator=(Tracker &&other) = delete;

    using TrackerList = Tracker;

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

    // Deletes 'listStart'
    static void DeleteAll(TrackerList* listStart) {
      while (listStart->m_next != nullptr) {
        delete listStart->m_next;
      }
    }

  private:
    TrackerList* m_next {nullptr};
    TrackerList* m_prev {nullptr};
  };
}

#endif
