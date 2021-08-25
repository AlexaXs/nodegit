#include "../include/tracker_wrap.h"

#include <unordered_set>
#include <unordered_map>

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

    inline const std::unordered_set<TrackerWrapTreeNode *>& Children() const;
    inline nodegit::TrackerWrap* TrackerWrap();

    inline void AddParent(TrackerWrapTreeNode *parent);
    inline void AddChild(TrackerWrapTreeNode *child);
    inline void RemoveFromParents();

  private:
    inline void removeChild(TrackerWrapTreeNode *child);

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
    m_parents.insert(parent)
    ;
  }

  /**
   * TrackerWrapTreeNode::AddChild()
   */
  void TrackerWrapTreeNode::AddChild(TrackerWrapTreeNode *child) {
    assert(child != nullptr);
    m_children.insert(child);
  }

  /**
   * TrackerWrapTreeNode::RemoveFromParents()
   * 
   * Removes this node from its parents children.
   */
  void TrackerWrapTreeNode::RemoveFromParents() {
    for (TrackerWrapTreeNode *parent : m_parents) {
      parent->removeChild(this);
    }
  }

  /**
   * TrackerWrapTreeNode::removeChild()
   */
  void TrackerWrapTreeNode::removeChild(TrackerWrapTreeNode *child) {
    assert(child != nullptr);
    m_children.erase(child);
  }


  /**
   * \class TrackerWrapTrees
   * 
   * Class containing a list of trees with nodes holding TrackerWrap objects.
   * For a TrackerWrap object 'P' which owns another TrackerWrap object 'C',
   * 'P' will be held in a node which will be the parent of the child node
   * that holds 'C'.
   * On destruction, nodes will be freed in a children-first way.
   * 
   * NOTE: code previous to this change is prepared to manage an array of owners,
   * so this code considers multiple owners too.
   */
  class TrackerWrapTrees
  {
  public:
    TrackerWrapTrees(nodegit::TrackerWrap::TrackerList *trackerList);
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
   * TrackerWrapTrees::TrackerWrapTrees(nodegit::TrackerWrap::TrackerList *trackerList)
   * 
   * Unlinks items from trackerList and adds them to a tree.
   * For each root (TrackerWrap item without owners) it add a new tree root.
   * 
   * \param trackerList TrackerList pointer from which the TrackerWrapTrees object will be created.
   */
  TrackerWrapTrees::TrackerWrapTrees(nodegit::TrackerWrap::TrackerList *trackerList)
  {
    nodegit::TrackerWrap *trackerWrap {};
    while ((trackerWrap = nodegit::TrackerWrap::UnlinkFirst(trackerList)) != nullptr) {
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
   * \param trackerWrap pointer to the TrackerWrap object to add as a node in a tree.
   */
  void TrackerWrapTrees::addNode(nodegit::TrackerWrap *trackerWrap) {
    // add trackerWrap as a node
    // NOTE: emplace will create a temporal TrackerWrapTreeNode and will
    // free it if trackerWrap already exists as a key. To prevent freeing
    // the node at this moment we have to find it first.
    auto addedNodeIter = m_mapTrackerWrapNode.find(trackerWrap);
    if (addedNodeIter == m_mapTrackerWrapNode.end()) {
      addedNodeIter = m_mapTrackerWrapNode.emplace(std::make_pair(
        trackerWrap, std::make_unique<TrackerWrapTreeNode>(trackerWrap))).first;
    }
    TrackerWrapTreeNode *addedNode = addedNodeIter->second.get();

    // if trackerWrap has no owners, add it as a root node
    const std::vector<nodegit::TrackerWrap*> *owners = trackerWrap->GetTrackerOwners();
    if (owners == nullptr) {
      m_roots.push_back(addedNode);
    }
    else {
      // add addedNode's owners and link child to parent
      for (nodegit::TrackerWrap *owner : *owners) {
        addedNode->AddParent(addParentNode(owner, addedNode));
      }
    }
  }

  /**
   * TrackerWrapTrees::addParentNode
   * 
   * \param owner TrackerWrap pointer to the new parent node to add.
   * \param child TrackerWrapTreeNode pointer to be the child node of the new parent node to add.
   */
  TrackerWrapTreeNode* TrackerWrapTrees::addParentNode(nodegit::TrackerWrap *owner,
    TrackerWrapTreeNode *child)
  {
    // adds a new parent node (holding the owner)
    // NOTE: emplace will create a temporal TrackerWrapTreeNode and will
    // free it if trackerWrap already exists as a key. To prevent freeing
    // the node at this moment we have to find it first.
    auto addedParentNodeIter = m_mapTrackerWrapNode.find(owner);
    if (addedParentNodeIter == m_mapTrackerWrapNode.end()) {
      addedParentNodeIter = m_mapTrackerWrapNode.emplace(std::make_pair(
        owner, std::make_unique<TrackerWrapTreeNode>(owner))).first;
    }
    TrackerWrapTreeNode *addedParentNode = addedParentNodeIter->second.get();

    // links parent to child
    addedParentNode->AddChild(child);

    return addedParentNode;
  }

  /**
   * TrackerWrapTrees::deleteTree
   * 
   * Deletes the tree that has the node passed as parameter as root,
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
      m_mapTrackerWrapNode.erase(node->TrackerWrap());
    }
  }

  /**
   * TrackerWrapTrees::freeAllTreesChildrenFirst
   * 
   * Deletes all the trees held, in a children-first way.
   */
  void TrackerWrapTrees::freeAllTreesChildrenFirst() {
    for (TrackerWrapTreeNode *tree : m_roots) {
      deleteTree(tree);
    }
    m_roots.clear();
  }
} // end anonymous namespace


namespace nodegit {
  void TrackerWrap::DeleteFromList(TrackerList* listStart) {
    assert(listStart != nullptr);
    // creates an object TrackerWrapTrees, which will free
    // the nodes of its trees in a children-first way
    TrackerWrapTrees trackerWrapTrees(listStart);
  }
}