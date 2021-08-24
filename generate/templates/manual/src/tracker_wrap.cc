#include "../include/tracker_wrap.h"

#include <unordered_set>

namespace {
  /**
   * \class TrackerWrapTreeNode
   */
  class TrackerWrapTreeNode
  {
  public:
    TrackerWrapTreeNode(nodegit::TrackerWrap *trackerWrap) : m_trackerWrap(trackerWrap) {}
    TrackerWrapTreeNode() = delete;
    ~TrackerWrapTreeNode();
    TrackerWrapTreeNode(const TrackerWrapTreeNode &other) = delete;
    TrackerWrapTreeNode(TrackerWrapTreeNode &&other) = delete;
    TrackerWrapTreeNode& operator=(const TrackerWrapTreeNode &other) = delete;
    TrackerWrapTreeNode& operator=(TrackerWrapTreeNode &&other) = delete;

    void addOwner(TrackerWrapTreeNode *owner);
    void addOwned(TrackerWrapTreeNode *owned);
    void removeOwned(TrackerWrapTreeNode *owned);

  private:
    std::vector<TrackerWrapTreeNode *> m_owners {};
    std::unordered_set<TrackerWrapTreeNode *> m_owned {};
    nodegit::TrackerWrap *m_trackerWrap {};
  };

  /**
   * TrackerWrapTreeNode::~TrackerWrapTreeNode()
   * Frees the memory of the TrackerWrap pointer it holds.
   */
  TrackerWrapTreeNode::~TrackerWrapTreeNode() {
    assert(owned.empty());
    if (m_trackerWrap != nullptr) {
      delete m_trackerWrap;
    }
  }

  /**
   * TrackerWrapTreeNode::addOwner()
   */
  void TrackerWrapTreeNode::addOwner(TrackerWrapTreeNode *owner) {
    assert(owner != nullptr);
    m_owners.push_back(owner);
  }

  /**
   * TrackerWrapTreeNode::addOwned()
   */
  void TrackerWrapTreeNode::addOwned(TrackerWrapTreeNode *owned) {
    assert(owned != nullptr);
    m_owned.push_back(owned);
  }

  /**
   * TrackerWrapTreeNode::removeOwned()
   */
  void TrackerWrapTreeNode::removeOwned(TrackerWrapTreeNode *owned) {
    assert(owned != nullptr);
    m_owned.erase(owned);
  }


  /**
   * \class TrackerWrapTrees
   * Class holding a group of independent TrackerWrap trees, where for
   * each tree, owner objects will be the parent nodes of the owned ones.
   * On destruction, nodes will be freed in a owned-first way.
   * 
   * NOTE: code previous to this change is prepared to add an array of owners,
   * so this code should be prepared for that case.
   */
  class TrackerWrapTrees
  {
  public:
    TrackerWrapTrees(nodegit::TrackerList *trackerList);
    TrackerWrapTrees() = delete;
    ~TrackerWrapTrees();
    TrackerWrapTrees(const TrackerWrapTrees &other) = delete;
    TrackerWrapTrees(TrackerWrapTrees &&other) = delete;
    TrackerWrapTrees& operator=(const TrackerWrapTrees &other) = delete;
    TrackerWrapTrees& operator=(TrackerWrapTrees &&other) = delete;

  private:
    void addNode(nodegit::TrackerWrap *trackerWrap);
    TrackerWrapTreeNode* addOwnerNode(nodegit::TrackerWrap *owner, TrackerWrapTreeNode *owned);
    void deleteTree(TrackerWrapTreeNode *tree);
    void freeAllOwnedFirst();

    using TrackerWrapTreeNodeMap = std::unordered_map<nodegit::TrackerWrap*, std::unique_ptr<TrackerWrapTreeNode>>;

    TrackerWrapTreeNodeMap m_mapTrackerWrapNode {};
    std::vector<TrackerWrapTreeNode *> m_roots {};
  };

  /**
   * TrackerWrapTrees::TrackerWrapTrees(nodegit::TrackerList *trackerList)
   * 
   * Unlinks items from trackerList and adds them to one tree.
   * For each root (TrackerList item without owners) it will add a new tree.
   * 
   * \param trackerList TrackerList pointer from which the TrackerWrapTrees object will be created.
   */
  TrackerWrapTrees::TrackerWrapTrees(nodegit::TrackerList *trackerList)
  {
    while (trackerList->m_next != nullptr) {
      TrackerWrap *trackerWrap = trackerList->m_next->Unlink();
      addNode(trackerWrap);
    }
  }

  /*
  * TrackerWrapTrees::~TrackerWrapTrees
  */
  TrackerWrapTrees::~TrackerWrapTrees() {
    freeAllOwnedFirst();
  }

  /**
   * TrackerWrapTrees::addNode
   * 
   * \param trackerWrap pointer to the TrackerWrap object to add to a tree.
   */
  void TrackerWrapTrees::addNode(nodegit::TrackerWrap *trackerWrap) {
    // add trackerWrap as a node
    auto emplacePair = m_mapTrackerWrapNode.emplace(std::make_pair(
      trackerWrap, std::make_unique<TrackerWrapTreeNode>(trackerWrap)));

    TrackerWrapTreeNode *newNode = emplacePair.first->second.get();

    // if trackerWrap has no owners, add it as a root node
    const std::vector<nodegit::TrackerWrap*> *owners = trackerWrap->GetOwners();
    if (owners == nullptr) {
      m_roots.emplace(newNode);
    }
    else {
      // add newNode's owners
      for (nodegit::TrackerWrap *o : *owners) {
        newNode->addOwner(addOwnerNode(o, trackerWrap));
      }
    }
  }

  /**
   * TrackerWrapTrees::addOwnerNode
   * 
   * \param owner pointer to the TrackerWrap owner object to add to the tree if not added yet.
   * \param owned pointer to the TrackerWrap owned object to add as owned node in the owner.
   */
  TrackerWrapTreeNode* TrackerWrapTrees::addOwnerNode(nodegit::TrackerWrap *owner,
    TrackerWrapTreeNode *owned)
  {
    TrackerWrapTreeNodeMap::iterator itOwnerNode = m_mapTrackerWrapNode.emplace(std::make_pair(
      owner, std::make_unique<TrackerWrapTreeNode>(owner))).first;

    itOwnerNode->second->addOwned(owned);
    return itOwnerNode->second.get();
  }

  /**
   * TrackerWrapTrees::deleteTree
   * 
   * Delete owned-first in a recursive way.
   * 
   * \param tree root node of the tree to delete.
   */
  void TrackerWrapTrees::deleteTree(TrackerWrapTreeNode *tree)
  {
    while (tree...)

  }

  /**
   * TrackerWrapTrees::FreeAllOwnedFirst
   * 
   * Deletes all the trees held in a owned-first way.
   */
  void TrackerWrapTrees::freeAllOwnedFirst() {
    for (TrackerWrapTreeNode *tree : m_roots) {
      deleteTree(tree);
    }
  }
} // end anonymous namespace


namespace nodegit {
  void TrackerWrap::DeleteAll(TrackerList* listStart) {
    // creates an object TrackerWrapTrees, which will free
    // the nodes of its trees in a owned-first way
    TrackerWrapTrees trackerWrapTrees(listStart);

    // while (listStart->m_next != nullptr) {
    //   delete listStart->m_next;
    // }
  }
}