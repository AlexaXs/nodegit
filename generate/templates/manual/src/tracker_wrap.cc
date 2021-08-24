#include "../include/tracker_wrap.h"

#include <unordered_set>

namespace {
  /**
   * \class TrackerWrapTreeNode
   * 
   * Parents of a TrackerWrapTreeNode will be the nodes holding TrackerWrap objects that
   * are owners of the TrackerWrap object that this node holds. The same way for its children.
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

    inline const std::unordered_set<TrackerWrapTreeNode *>& Parents() const;
    inline const std::unordered_set<TrackerWrapTreeNode *>& Children() const;
    inline nodegit::TrackerWrap* TrackerWrap();

    inline void AddParent(TrackerWrapTreeNode *parent);
    inline void AddChild(TrackerWrapTreeNode *child);
    inline void RemoveFromParents();
    inline void RemoveChild(TrackerWrapTreeNode *child);

  private:
    std::unordered_set<TrackerWrapTreeNode *> m_parents {};
    std::unordered_set<TrackerWrapTreeNode *> m_children {};
    nodegit::TrackerWrap *m_trackerWrap {};
  };

  /**
   * TrackerWrapTreeNode::~TrackerWrapTreeNode()
   * Frees the memory of the TrackerWrap pointer it holds.
   */
  TrackerWrapTreeNode::~TrackerWrapTreeNode() {
    assert(m_children.empty());
    if (m_trackerWrap != nullptr) {
      delete m_trackerWrap;
    }
  }

  /**
   * TrackerWrapTreeNode::Parents()
   * 
   * Returns a reference to the parents of this node.
   */
  const std::unordered_set<TrackerWrapTreeNode *>& TrackerWrapTreeNode::Parents() const {
    return m_parents;
  }

  /**
   * TrackerWrapTreeNode::Children()
   * 
   * Returns a reference to the children nodes of this.
   */
  const std::unordered_set<TrackerWrapTreeNode *>& TrackerWrapTreeNode::Children() const {
    return m_children;
  }

  /**
   * TrackerWrapTreeNode::TrackerWrap()
   * 
   * Returns a pointer to the node's TrackerWrap object.
   */
  nodegit::TrackerWrap* TrackerWrapTreeNode::TrackerWrap() {
    return m_trackerWrap;
  }

  /**
   * TrackerWrapTreeNode::AddParent()
   */
  void TrackerWrapTreeNode::AddParent(TrackerWrapTreeNode *parent) {
    assert(parent != nullptr);
    m_parents.emplace(parent)
    ;
  }

  /**
   * TrackerWrapTreeNode::AddChild()
   */
  void TrackerWrapTreeNode::AddChild(TrackerWrapTreeNode *child) {
    assert(child != nullptr);
    m_children.emplace(child);
  }

  /**
   * TrackerWrapTreeNode::RemoveFromParents()
   * 
   * Removes this node from its parents children.
   */
  void TrackerWrapTreeNode::RemoveFromParents() {
    for (TrackerWrapTreeNode *parent : m_parents) {
      parent->RemoveChild(this);
    }
  }

  /**
   * TrackerWrapTreeNode::RemoveChild()
   */
  void TrackerWrapTreeNode::RemoveChild(TrackerWrapTreeNode *child) {
    assert(child != nullptr);
    m_children.erase(child);
  }


  /**
   * \class TrackerWrapTrees
   * 
   * Class holding a group of independent TrackerWrap trees, where for each
   * tree, owner objects will be in the parent nodes of the nodes for the owned ones.
   * On destruction, nodes will be freed in a children-first way.
   * 
   * NOTE: code previous to this change is prepared to manage an array of owners,
   * so this code considers multiple owners too.
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
    TrackerWrapTreeNode* addParentNode(nodegit::TrackerWrap *owner, TrackerWrapTreeNode *child);
    void deleteTree(TrackerWrapTreeNode *node);
    void freeAllTreesChildrenFirst();

    using TrackerWrapTreeNodeMap = std::unordered_map<nodegit::TrackerWrap*, std::unique_ptr<TrackerWrapTreeNode>>;

    TrackerWrapTreeNodeMap m_mapTrackerWrapNode {};
    std::vector<TrackerWrapTreeNode *> m_roots {};
  };

  /**
   * TrackerWrapTrees::TrackerWrapTrees(nodegit::TrackerList *trackerList)
   * 
   * Unlinks items from trackerList and adds them to one tree.
   * For each root (TrackerWrap item without owners) it will add a new tree.
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
    freeAllTreesChildrenFirst();
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
        newNode->AddParent(addParentNode(o, trackerWrap));
      }
    }
  }

  /**
   * TrackerWrapTrees::addParentNode
   * 
   * \param owner TrackerWrap pointer for the new parent node to add.
   * \param child TrackerWrapTreeNode pointer to be the child node of the new parent node to add.
   */
  TrackerWrapTreeNode* TrackerWrapTrees::addParentNode(nodegit::TrackerWrap *owner,
    TrackerWrapTreeNode *child)
  {
    // adds a new parent node (holding the owner)
    TrackerWrapTreeNodeMap::iterator itAdded = m_mapTrackerWrapNode.emplace(std::make_pair(
      owner, std::make_unique<TrackerWrapTreeNode>(owner))).first;

    // adds child to the parent
    TrackerWrapTreeNode *parentNode = itAdded->second.get();
    parentNode->AddChild(child);

    // return the added parent node
    return parentNode;
  }

  /**
   * TrackerWrapTrees::deleteTree
   * 
   * Deletes the tree held below the node passed as a parameter
   * in a children-first way and recursively.
   * 
   * \param node node from where to delete all its children and itself.
   */
  void TrackerWrapTrees::deleteTree(TrackerWrapTreeNode *node)
  {
    const std::unordered_set<TrackerWrapTreeNode *> &children = node->Children();

    // delete children first
    for (TrackerWrapTreeNode *child : children) {
      deleteTree(child);
    }

    // then deletes itself if no children left
    if (children.empty()) {
      // remove 'node' from the children list of its parents, to prevent
      // multiple deletions
      node->RemoveFromParents();

      // erase from the container, which will actually
      // free 'node' and the TrackerWrap object it holds
      m_mapTrackerWrapNode->erase(node->TrackerWrap());
    }
  }

  /**
   * TrackerWrapTrees::freeAllTreesChildrenFirst
   * 
   * Deletes all the trees held in a children-first way.
   */
  void TrackerWrapTrees::freeAllTreesChildrenFirst() {
    for (TrackerWrapTreeNode *tree : m_roots) {
      deleteTree(tree);
    }
  }
} // end anonymous namespace


namespace nodegit {
  void TrackerWrap::DeleteAll(TrackerList* listStart) {
    // creates an object TrackerWrapTrees, which will free
    // the nodes of its trees in a children-first way
    TrackerWrapTrees trackerWrapTrees(listStart);

    // while (listStart->m_next != nullptr) {
    //   delete listStart->m_next;
    // }
  }
}