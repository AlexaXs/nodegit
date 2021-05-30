/**
 * \struct CommitsGraphNode
 */
struct CommitsGraphNode
{
  CommitsGraphNode(uint32_t aParentsLeft)
    : parentsLeft(aParentsLeft) {}
  CommitsGraphNode() = default;
  ~CommitsGraphNode() = default;
  CommitsGraphNode(const CommitsGraphNode &other) = delete;
  CommitsGraphNode(CommitsGraphNode &&other) = delete;
  CommitsGraphNode& operator=(const CommitsGraphNode &other) = delete;
  CommitsGraphNode& operator=(CommitsGraphNode &&other) = delete;

  std::vector<CommitsGraphNode *> children {};
  uint32_t parentsLeft {0}; // used when calculating the maximum history depth
};

/**
 * \class CommitsGraph
 */
class CommitsGraph
{
public:
  CommitsGraph() = default;
  ~CommitsGraph() = default;
  CommitsGraph(const CommitsGraph &other) = delete;
  CommitsGraph(CommitsGraph &&other) = delete;
  CommitsGraph& operator=(const CommitsGraph &other) = delete;
  CommitsGraph& operator=(CommitsGraph &&other) = delete;

  using CommitsGraphMap = std::unordered_map<std::string, std::unique_ptr<CommitsGraphNode>>;

  void AddNode(const git_oid *oid, const git_commit *commit, uint32_t numParents);
  uint32_t CalculateMaxDepth();

private:
  void addParentNode(const git_oid *oid, CommitsGraphNode *child);

  CommitsGraphMap m_mapOidNode {};
  std::vector<CommitsGraphNode *> m_roots {};
};

/**
 * CommitsGraph::AddNode
 * 
 * \param oid git_oid of the commit object to add.
 * \param node git_commit object source from which to extract information for the tree node.
 * \param numParents Number of parents of the commit object.
 */
void CommitsGraph::AddNode(const git_oid *oid, const git_commit *commit, uint32_t numParents)
{
  auto nodePair = m_mapOidNode.emplace(std::make_pair(
    std::string(reinterpret_cast<const char *>(oid->id), GIT_OID_RAWSZ),
    std::make_unique<CommitsGraphNode>(numParents)));

  CommitsGraphMap::iterator itNode = nodePair.first;

  // if this node already added by a child, update its parentsLeft
  if (nodePair.second == false) {
    itNode->second.get()->parentsLeft = numParents;
  }

  // set root
  if (numParents == 0) {
    m_roots.emplace_back(itNode->second.get());
  }

  // add parents
  for (unsigned int i = 0; i < numParents; ++i) {
    addParentNode(git_commit_parent_id(commit, i), itNode->second.get());
  }
}

/**
 * CommitsGraph::CalculateMaxDepth
 * \return Calculated maximum depth of the tree.
 * 
 * Uses iterative algorithm to count levels.
 * Considers multiple initial commits.
 * Considers that children of one level can have multiple parents, hence we insert unique children at each level.
 * Considers that same child can be in different levels. Here to prevent counting the same child multiple times,
 * we only add a child when the last parent (parentsLeft) inserts it. This is actually what makes the algorithm fast.
 * Recursive algorithm avoided to prevent stack overflow in case of excessive levels in the tree.
 */
uint32_t CommitsGraph::CalculateMaxDepth()
{
  uint32_t maxDepth {0};
  std::set<CommitsGraphNode *> parents {};
  std::set<CommitsGraphNode *> children {};

  // initial commmits
  for (CommitsGraphNode *root : m_roots) {
    children.insert(root);
  }

  while (!children.empty()) {
    ++maxDepth;
    parents = std::move(children);

    // add unique children of next level, and only if from the last parent
    for (CommitsGraphNode *parent : parents) {
      for (CommitsGraphNode *child : parent->children) {
        if (--child->parentsLeft == 0) {
          children.insert(child);
        }
      }
    }
  }

  return maxDepth;
}

/**
 * CommitsGraph::addParentNode
 * 
 * \param oid git_oid of the commit object to add.
 * \param child Child of the parent node being added.
 */
void CommitsGraph::addParentNode(const git_oid *oid_parent, CommitsGraphNode *child)
{
  CommitsGraphMap::iterator itParentNode = m_mapOidNode.emplace(std::make_pair(
    std::string(reinterpret_cast<const char *>(oid_parent->id), GIT_OID_RAWSZ),
    std::make_unique<CommitsGraphNode>())).first;

  // add child to parent
  itParentNode->second->children.emplace_back(child);
}

/**
 * \struct TreeStatistics
 *  Structure to store statistics for a git tree object.
 */
struct TreeStatistics
{
  TreeStatistics() = default;
  ~TreeStatistics() = default;
  TreeStatistics(const TreeStatistics &other) = delete;
  TreeStatistics(TreeStatistics &&other) = default;                 // no copies, just move
  TreeStatistics& operator=(const TreeStatistics &other) = delete;
  TreeStatistics& operator=(TreeStatistics &&other) = default;

  size_t numDirectories{0};
  size_t maxPathDepth {0};
  size_t maxPathLength {0};
  size_t numFiles {0};
  size_t totalFileSize {0};
  size_t numSymlinks {0};
  size_t numSubmodules {0};
};

/**
 * \struct Statistics
 * Stores statistics of the analyzed repository
 */
struct Statistics
{
  Statistics() = default;
  ~Statistics() = default;
  Statistics(const Statistics &other) = delete;
  Statistics(Statistics &&other) = delete;
  Statistics& operator=(const Statistics &other) = delete;
  Statistics& operator=(Statistics &&other) = delete;

  struct {
    struct { size_t count {0}; size_t size {0}; } commits;
    struct { size_t count {0}; size_t size {0}; size_t entries {0}; } trees;
    struct { size_t count {0}; size_t size {0}; } blobs;
    struct { size_t count {0}; } annotatedTags;
    struct { size_t count {0}; } references;
  } repositorySize {};

  struct {
    struct { size_t maxSize {0}; size_t maxParents {0}; } commits;
    struct { size_t maxEntries {0}; } trees;
    struct { size_t maxSize {0}; } blobs;
  } biggestObjects {};

  struct {
    uint32_t maxDepth {0};
    uint32_t maxTagDepth {0};
  } historyStructure {};

  TreeStatistics biggestCheckouts {};
};

/**
 * \struct OdbObjectsInfo
 */
struct OdbObjectsInfo
{
  struct TreeDataAndStats {
    TreeDataAndStats() = default;
    ~TreeDataAndStats() = default;
    TreeDataAndStats(const TreeDataAndStats &other) = delete;
    TreeDataAndStats(TreeDataAndStats &&other) = default;
    TreeDataAndStats& operator=(const TreeDataAndStats &other) = delete;
    TreeDataAndStats& operator=(TreeDataAndStats &&other) = default;

    std::vector<std::string> entryBlobs {};
    std::vector< std::pair<std::string, size_t> > entryTreesNameLen {};
    TreeStatistics stats {};
    bool statsDone {false};
  };

  struct TagData {
    static constexpr uint32_t kUnsetDepth = 0;

    TagData() = default;
    ~TagData() = default;
    TagData(const TagData &other) = delete;
    TagData(TagData &&other) = default;
    TagData& operator=(const TagData &other) = delete;
    TagData& operator=(TagData &&other) = default;

    std::string oidTarget {};
    git_object_t typeTarget {GIT_OBJECT_INVALID};
    uint32_t depth {kUnsetDepth};
  };

  OdbObjectsInfo() = default;
  ~OdbObjectsInfo() = default;
  OdbObjectsInfo(const OdbObjectsInfo &other) = delete;
  OdbObjectsInfo(OdbObjectsInfo &&other) = delete;
  OdbObjectsInfo& operator=(const OdbObjectsInfo &other) = delete;
  OdbObjectsInfo& operator=(OdbObjectsInfo &&other) = delete;

  struct {
    std::unordered_map<std::string, std::string> info {};
    // tree of commits (graph) to be build while reading the object database, to obtain maximum history depth
    CommitsGraph graph {};
    size_t totalSize {0};
    size_t maxSize {0};
    size_t maxParents {0};
  } commitsInfo {};

  struct {
    std::unordered_map<std::string, TreeDataAndStats> info;
    size_t totalSize {0};
    size_t totalEntries {0};
    size_t maxEntries {0};
  } treesInfo {};

  struct {
    std::unordered_map<std::string, size_t> info {};
    size_t totalSize {0};
    size_t maxSize {0};
  } blobsInfo {};

  struct {
    std::unordered_map<std::string, TagData> info;
  } tagsInfo {};

  using iterTreeInfo = std::unordered_map<std::string, TreeDataAndStats>::iterator;
  using iterBlobInfo = std::unordered_map<std::string, size_t>::iterator;
  using iterTagInfo = std::unordered_map<std::string, TagData>::iterator;
};

/**
 * \class WorkItemOdbData
 * WorkItem storing odb oids for the WorkPool.
 */
class WorkItemOdbData : public WorkItem{
public:
  WorkItemOdbData(const git_oid &oid, OdbObjectsInfo *odbObjectsInfo)
    : m_oid(oid), m_odbObjectsInfo(odbObjectsInfo) {}
  ~WorkItemOdbData() = default;
  WorkItemOdbData(const WorkItemOdbData &other) = delete;
  WorkItemOdbData(WorkItemOdbData &&other) = delete;
  WorkItemOdbData& operator=(const WorkItemOdbData &other) = delete;
  WorkItemOdbData& operator=(WorkItemOdbData &&other) = delete;
  
  const git_oid& GetOid() const { return m_oid; }
  OdbObjectsInfo* GetOdbObjectsInfo() { return m_odbObjectsInfo; }

private:
  git_oid m_oid {};
  OdbObjectsInfo *m_odbObjectsInfo {nullptr};
};

/**
 * \struct MutexWorkerStoreOdbData
 * mutex objects needed by WorkerStoreOdbData
 */
struct MutexWorkerStoreOdbData
{
  std::mutex mutexCommits {};
  std::mutex mutexTrees {};
  std::mutex mutexBlobs {};
  std::mutex mutexTags {};
};

/**
 * \class WorkerStoreOdbData
 * Worker for the WorkPool to store odb object data.
 */
class WorkerStoreOdbData : public IWorker
{
public:
  WorkerStoreOdbData(const std::string &repoPath, MutexWorkerStoreOdbData *mutex)
    : m_repoPath(repoPath), m_mutex(mutex) {}
  ~WorkerStoreOdbData();
  WorkerStoreOdbData(const WorkerStoreOdbData &other) = delete;
  WorkerStoreOdbData(WorkerStoreOdbData &&other) = delete;
  WorkerStoreOdbData& operator=(const WorkerStoreOdbData &other) = delete;
  WorkerStoreOdbData& operator=(WorkerStoreOdbData &&other) = delete;

  bool Initialize();
  bool Execute(std::unique_ptr<WorkItem> &&work);

private:
  OdbObjectsInfo::TreeDataAndStats thisTreeDataAndStats(git_tree *tree, size_t numEntries);

  std::string m_repoPath {};
  git_repository *m_repo {nullptr};
  git_odb *m_odb {nullptr};
  MutexWorkerStoreOdbData *m_mutex {nullptr};
};

/**
 * WorkerStoreOdbData::~WorkerStoreOdbData
 */
WorkerStoreOdbData::~WorkerStoreOdbData() {
  if (m_odb) {
    git_odb_free(m_odb);
  }
  if (m_repo) {
    git_repository_free(m_repo);
  }
}

/**
 * WorkerStoreOdbData::Initialize
 */
bool WorkerStoreOdbData::Initialize() {
  if (m_repo != nullptr) // if already initialized
    return true;

  if (m_repoPath.empty())
    return false;
  else {
    if (git_repository_open(&m_repo, m_repoPath.c_str()) != GIT_OK)
      return false;
    
    if (git_repository_odb(&m_odb, m_repo) != GIT_OK)
      return false;
  }
  return true;
}

/**
 * WorkerStoreOdbData::Execute
 */
bool WorkerStoreOdbData::Execute(std::unique_ptr<WorkItem> &&work)
{
  std::unique_ptr<WorkItemOdbData> wi {static_cast<WorkItemOdbData*>(work.release())};
  const git_oid &oid = wi->GetOid();

  // NOTE about PERFORMANCE (May 2021):
  // git_object_lookup() is as expensive as git_odb_read().
  // They give access to different information from the libgit2 API.
  // Try to call only one of them if possible.

  git_object *target {nullptr};
  if (git_object_lookup(&target, m_repo, &oid, GIT_OBJECT_ANY) != GIT_OK) {
    return false;
  }

  OdbObjectsInfo *odbObjectsInfo = wi->GetOdbObjectsInfo();
  switch (git_object_type(target))
  {
    case GIT_OBJECT_COMMIT:
    {
      git_commit *commit = (git_commit*)target;
      // NOTE about PERFORMANCE (May 2021):
      // calling git_odb_object_size() was slightly faster than calculating header size + message size + 1 with GK repo

      // obtain size
      git_odb_object *obj {nullptr};
      if (git_odb_read(&obj, m_odb, &oid) != GIT_OK) {
        git_object_free(target);
        return false;
      }
      size_t size = git_odb_object_size(obj);
      git_odb_object_free(obj);

      // obtain commit info
      std::string oidTree = std::string(reinterpret_cast<const char *>(git_commit_tree_id(commit)->id), GIT_OID_RAWSZ);
      size_t numParents  = git_commit_parentcount(commit);

      { // lock
        std::lock_guard<std::mutex> lock(m_mutex->mutexCommits);

        if (odbObjectsInfo->commitsInfo.info.emplace(std::make_pair(
            std::string(reinterpret_cast<const char *>(oid.id), GIT_OID_RAWSZ),
            std::move(oidTree))).second == true) {
          odbObjectsInfo->commitsInfo.totalSize += size;
          odbObjectsInfo->commitsInfo.maxSize = std::max(odbObjectsInfo->commitsInfo.maxSize, size);
          odbObjectsInfo->commitsInfo.maxParents = std::max(odbObjectsInfo->commitsInfo.maxParents, numParents);

          odbObjectsInfo->commitsInfo.graph.AddNode(&oid, commit, static_cast<uint32_t>(numParents));
        }
      }
    }
      break;

    case GIT_OBJECT_TREE:
    {
      git_tree *tree = (git_tree*)target;

      // do not count empty trees, like git's empty tree "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
      size_t numEntries = git_tree_entrycount(tree);
      if (numEntries == 0) {
        git_object_free(target);
        return true;
      }

      // obtain size
      git_odb_object *obj {nullptr};
      if (git_odb_read(&obj, m_odb, &oid) != GIT_OK) {
        git_object_free(target);
        return false;
      }
      size_t size = git_odb_object_size(obj);
      git_odb_object_free(obj);

      // obtain tree data and calculate statistics for only this tree (not recursively)
      OdbObjectsInfo::TreeDataAndStats treeDataAndStats = thisTreeDataAndStats(tree, numEntries);
     
      { // lock
        std::lock_guard<std::mutex> lock(m_mutex->mutexTrees);

        if (odbObjectsInfo->treesInfo.info.emplace(std::make_pair(
            std::string(reinterpret_cast<const char *>(oid.id), GIT_OID_RAWSZ),
            std::move(treeDataAndStats))).second == true) {
          odbObjectsInfo->treesInfo.totalSize += size;
          odbObjectsInfo->treesInfo.totalEntries += numEntries;
          odbObjectsInfo->treesInfo.maxEntries = std::max(odbObjectsInfo->treesInfo.maxEntries, numEntries);
        }
      }
    }
      break;

    case GIT_OBJECT_BLOB:
    {
      git_blob *blob = (git_blob*)target;
      size_t size = git_blob_rawsize(blob);

      { // lock
        std::lock_guard<std::mutex> lock(m_mutex->mutexBlobs);

        if (odbObjectsInfo->blobsInfo.info.emplace(std::make_pair(
            std::string(reinterpret_cast<const char *>(oid.id), GIT_OID_RAWSZ),
            size)).second == true) {
          odbObjectsInfo->blobsInfo.totalSize += size;
          odbObjectsInfo->blobsInfo.maxSize = std::max(odbObjectsInfo->blobsInfo.maxSize, size);
        }
      }
    }
      break;

    case GIT_OBJECT_TAG:
    {
      // obtain TagData
      git_tag *tag = (git_tag*)target;
      const git_oid *oid_target = git_tag_target_id(tag);
      OdbObjectsInfo::TagData tagData {
        std::string(reinterpret_cast<const char *>(oid_target->id), GIT_OID_RAWSZ),
        git_tag_target_type(tag),
        OdbObjectsInfo::TagData::kUnsetDepth};

      { // lock
        std::lock_guard<std::mutex> lock(m_mutex->mutexTags);
        odbObjectsInfo->tagsInfo.info.emplace(std::make_pair(
            std::string(reinterpret_cast<const char *>(oid.id), GIT_OID_RAWSZ),
            std::move(tagData)));
      }
    }
      break;

    default:
      break;
  }

  git_object_free(target);

  return true;
}

/**
 * WorkerStoreOdbData::thisTreeDataAndStats
 * 
 * Obtain tree data and calculate the part of this tree's statistics that each thread can do.
 * 
 * \param tree tree to get data from and calculate partial statistics of.
 * \param numEntries number of entries of this tree.
 * \return this tree's data and partial statistics.
  */
OdbObjectsInfo::TreeDataAndStats WorkerStoreOdbData::thisTreeDataAndStats(git_tree *tree, size_t numEntries)
{
  OdbObjectsInfo::TreeDataAndStats treeDataAndStats {};

  const git_tree_entry *te {nullptr};
  git_object_t te_type {GIT_OBJECT_INVALID};
  const char *teName {nullptr};
  size_t teNameLen {0};
  const git_oid *te_oid {nullptr};

  for (size_t i = 0; i < numEntries; ++i)
  {
    te = git_tree_entry_byindex(tree, i);
    if (te == nullptr) {
      continue;
    }

    te_type = git_tree_entry_type(te);

    switch (te_type)
    {
      // count submodules
      case GIT_OBJECT_COMMIT:
        if (git_tree_entry_filemode(te) == GIT_FILEMODE_COMMIT) {
          ++treeDataAndStats.stats.numSubmodules;
        }
        break;

      case GIT_OBJECT_BLOB:
      {
        // count symbolic links, but don't add them as blob entries
        if (git_tree_entry_filemode(te) == GIT_FILEMODE_LINK) {
          ++treeDataAndStats.stats.numSymlinks;
        }
        else {
          te_oid = git_tree_entry_id(te);
          treeDataAndStats.entryBlobs.emplace_back(reinterpret_cast<const char *>(te_oid->id), GIT_OID_RAWSZ);

          ++treeDataAndStats.stats.numFiles;
          teName = git_tree_entry_name(te);
          teNameLen = std::char_traits<char>::length(teName);
          treeDataAndStats.stats.maxPathLength = std::max(treeDataAndStats.stats.maxPathLength, teNameLen);
        }
      }
        break;
        
      case GIT_OBJECT_TREE:
      {
        // We store tree's name lenght to compare in posterior stage, after threads work
        teName = git_tree_entry_name(te);
        teNameLen = std::char_traits<char>::length(teName);

        te_oid = git_tree_entry_id(te);
        treeDataAndStats.entryTreesNameLen.emplace_back(std::make_pair(
          std::string(reinterpret_cast<const char *>(te_oid->id), GIT_OID_RAWSZ),
          teNameLen));
      }
        break;
        
      default:
        break;
    }
  }

  return treeDataAndStats;
}

/**
 * \struct ForEachOdbCbPayload
 * Payload for git_odb_foreach
 */
struct ForEachOdbCbPayload
{
  explicit ForEachOdbCbPayload(WorkerPool<WorkerStoreOdbData,WorkItemOdbData> *aWorkerPool, OdbObjectsInfo *aOdbObjectsInfo)
    : workerPool(aWorkerPool), odbObjectsInfo(aOdbObjectsInfo) {}
  ~ForEachOdbCbPayload() = default;
  ForEachOdbCbPayload(const ForEachOdbCbPayload &other) = delete;
  ForEachOdbCbPayload(ForEachOdbCbPayload &&other) = delete;
  ForEachOdbCbPayload& operator=(const ForEachOdbCbPayload &other) = delete;
  ForEachOdbCbPayload& operator=(ForEachOdbCbPayload &&other) = delete;

  WorkerPool<WorkerStoreOdbData,WorkItemOdbData> *workerPool {nullptr};
  OdbObjectsInfo *odbObjectsInfo {nullptr};
};

/**
 * forEachOdbCb. Callback for git_odb_foreach.
 * Returns GIT_OK on success; GIT_EUSER otherwise
 */
static int forEachOdbCb(const git_oid *oid, void *payloadToCast)
{
  ForEachOdbCbPayload *payload = static_cast<ForEachOdbCbPayload *>(payloadToCast);

  // Must insert copies of oid, since the pointers might not survive until worker thread picks it up
  if (!payload->workerPool->InsertWork(std::make_unique<WorkItemOdbData>(*oid, payload->odbObjectsInfo)))
    return GIT_EUSER;

  return GIT_OK;
}

/**
 * \class RepoAnalysis
 * Class to analyse and hold repository statistics
 */
class RepoAnalysis
{
public:
  explicit RepoAnalysis(git_repository *repo)
    : m_repo(repo) {}
    // DEBUG TIMES
    // : m_repo(repo) {  timeCreation = std::chrono::system_clock::now(); }
  ~RepoAnalysis() = default;
  // DEBUG TIMES
  // ~RepoAnalysis() { timeDestruction = std::chrono::system_clock::now(); printTimes(); }
  RepoAnalysis(const RepoAnalysis &other) = delete;
  RepoAnalysis(RepoAnalysis &&other) = delete;
  RepoAnalysis& operator=(const RepoAnalysis &other) = delete;
  RepoAnalysis& operator=(RepoAnalysis &&other) = delete;

  int Analyze();
  v8::Local<v8::Object> StatisticsToJS() const;

public:
  // DEBUG TIMES
  // std::chrono::time_point<std::chrono::system_clock> timeCreation {};
  // std::chrono::time_point<std::chrono::system_clock> timeStartOdbForEach {};
  // std::chrono::time_point<std::chrono::system_clock> timeEndOdbForEach {};
  // std::chrono::time_point<std::chrono::system_clock> timeEndCalculateBiggestCheckouts {};
  // std::chrono::time_point<std::chrono::system_clock> timeEndCalculateMaxTagDepth {};
  // std::chrono::time_point<std::chrono::system_clock> timeEndCalculateMaxDepth {};
  // std::chrono::time_point<std::chrono::system_clock> timeEndAnalyzeReferences {};  
  // std::chrono::time_point<std::chrono::system_clock> timeDestruction {};

private:
  int analyzeObjects();
  int calculateBiggestCheckouts();
  OdbObjectsInfo::iterTreeInfo calculateTreeStatistics(const std::string &oidTree);
  int calculateMaxTagDepth();
  OdbObjectsInfo::iterTagInfo calculateTagDepth(const std::string &oidTag);
  int analyzeReferences();
  void fillOutStatistics();
  v8::Local<v8::Object> repositorySizeToJS() const;
  v8::Local<v8::Object> biggestObjectsToJS() const;
  v8::Local<v8::Object> historyStructureToJS() const;
  v8::Local<v8::Object> biggestCheckoutsToJS() const;

  // DEBUG TIMES
  // void printTimes() const;

  git_repository *m_repo {nullptr};
  Statistics m_statistics {};
  // odb objects info to build while reading the object database by each thread
  OdbObjectsInfo m_odbObjectsInfo {};
};

/**
 * RepoAnalysis::Analyze
 */
int RepoAnalysis::Analyze()
{
  int errorCode {GIT_OK};

  if ((errorCode = analyzeObjects() != GIT_OK)) {
    return errorCode;
  }
  if ((errorCode = analyzeReferences() != GIT_OK)) {
    return errorCode;
  }

  // DEBUG TIMES
  // timeEndAnalyzeReferences = std::chrono::system_clock::now();
  
  fillOutStatistics();

  return errorCode;
}

/**
 * RepoAnalysis::analyzeObjects
 * Analyzes objects (commits, trees, blobs, annotated tags)
 */
int RepoAnalysis::analyzeObjects()
{
  int errorCode {GIT_OK};

  // initialize workers for the worker pool
  std::string repoPath = git_repository_path(m_repo);
  unsigned int numThreads = std::max(std::thread::hardware_concurrency(), static_cast<unsigned int>(4));

  MutexWorkerStoreOdbData mutex {};
  std::vector< std::shared_ptr<WorkerStoreOdbData> > workers {};
  for (unsigned int i=0; i<numThreads; ++i) {
    workers.emplace_back(std::make_shared<WorkerStoreOdbData>(repoPath, &mutex));
  }

  // initialize worker pool
  WorkerPool<WorkerStoreOdbData,WorkItemOdbData> workerPool {};  
  workerPool.Init(workers);

  // gets the objects database
  ForEachOdbCbPayload payload {&workerPool, &m_odbObjectsInfo};
  git_odb *odb {nullptr};
  if ((errorCode = git_repository_odb(&odb, m_repo)) != GIT_OK) {
    return errorCode;
  }

  // DEBUG TIMES
  // timeStartOdbForEach = std::chrono::system_clock::now();

  // analyze all the objects and build commit history tree
  if ((errorCode = git_odb_foreach(odb, forEachOdbCb, &payload)) != GIT_OK) {
    git_odb_free(odb);
    return errorCode;
  }

  // wait for the threads to finish analyzing each object from the database and shutdown the work pool
  workerPool.Shutdown();

  // DEBUG TIMES
  // timeEndOdbForEach = std::chrono::system_clock::now();

  git_odb_free(odb);

  if ((errorCode = calculateBiggestCheckouts()) != GIT_OK) {
    return errorCode;
  }

  // DEBUG TIMES
  // timeEndCalculateBiggestCheckouts = std::chrono::system_clock::now();

  if ((errorCode = calculateMaxTagDepth()) != GIT_OK) {
    return errorCode;
  }

  // DEBUG TIMES
  // timeEndCalculateMaxTagDepth = std::chrono::system_clock::now();

  // calculate max commit history depth
  m_statistics.historyStructure.maxDepth = m_odbObjectsInfo.commitsInfo.graph.CalculateMaxDepth();

  // DEBUG TIMES
  // timeEndCalculateMaxDepth = std::chrono::system_clock::now();

  return errorCode;
}

/**
 * RepoAnalysis::calculateBiggestCheckouts
 * 
 * Once threads have collected data from objects, biggest checkouts can be calculated.
 * Threads have already collected partial non-recursive tree statistics.
 */
int RepoAnalysis::calculateBiggestCheckouts()
{
  for (auto &commitInfo : m_odbObjectsInfo.commitsInfo.info)
  {
    // calculate this commit's data
    const std::string &commitOidTree = commitInfo.second;

    OdbObjectsInfo::iterTreeInfo itTreeInfo {};
    if ((itTreeInfo = calculateTreeStatistics(commitOidTree)) == m_odbObjectsInfo.treesInfo.info.end())
      return GIT_EUSER;

    // update biggestCheckouts data
    OdbObjectsInfo::TreeDataAndStats &treeDataAndStats = itTreeInfo->second;
    m_statistics.biggestCheckouts.numDirectories = std::max(
      m_statistics.biggestCheckouts.numDirectories, treeDataAndStats.stats.numDirectories);
    m_statistics.biggestCheckouts.totalFileSize = std::max(
      m_statistics.biggestCheckouts.totalFileSize, treeDataAndStats.stats.totalFileSize);
    m_statistics.biggestCheckouts.maxPathDepth = std::max(
      m_statistics.biggestCheckouts.maxPathDepth, treeDataAndStats.stats.maxPathDepth);
    m_statistics.biggestCheckouts.numFiles = std::max(
      m_statistics.biggestCheckouts.numFiles, treeDataAndStats.stats.numFiles);
    m_statistics.biggestCheckouts.maxPathLength = std::max(
      m_statistics.biggestCheckouts.maxPathLength, treeDataAndStats.stats.maxPathLength);
    m_statistics.biggestCheckouts.numSymlinks = std::max(
      m_statistics.biggestCheckouts.numSymlinks, treeDataAndStats.stats.numSymlinks);
    m_statistics.biggestCheckouts.numSubmodules = std::max(
      m_statistics.biggestCheckouts.numSubmodules, treeDataAndStats.stats.numSubmodules);
  }

  return GIT_OK;
}

/**
 * RepoAnalysis::calculateTreeStatistics
 * 
 * Calculates tree statistics recursively, considering individual tree's statistics
 * have already been calculated.
 * Returns an iterator to the tree info container, or to end of the tree info container if something went wrong.
 */
OdbObjectsInfo::iterTreeInfo RepoAnalysis::calculateTreeStatistics(const std::string &oidTree)
{
  OdbObjectsInfo::iterTreeInfo itTreeInfo = m_odbObjectsInfo.treesInfo.info.find(oidTree);
  if (itTreeInfo == m_odbObjectsInfo.treesInfo.info.end())
    return itTreeInfo;

  OdbObjectsInfo::TreeDataAndStats &treeDataAndStats = itTreeInfo->second;

  // prune recursivity
  if (treeDataAndStats.statsDone)
    return itTreeInfo;

  ++treeDataAndStats.stats.numDirectories;
  ++treeDataAndStats.stats.maxPathDepth;
  // the following partial statistics have also been calculated in previous stage with threads:
  // - treeDataAndStats.stats.numFiles
  // - treeDataAndStats.stats.maxPathLength
  // - treeDataAndStats.stats.numSymLinks
  // - treeDataAndStats.stats.numSubmodules

  // totalFileSize
  OdbObjectsInfo::iterBlobInfo itBlobInfo {};
  for (auto &oidBlob : treeDataAndStats.entryBlobs)
  {
    if ((itBlobInfo = m_odbObjectsInfo.blobsInfo.info.find(oidBlob)) == m_odbObjectsInfo.blobsInfo.info.end())
      return m_odbObjectsInfo.treesInfo.info.end(); // to let the caller know that something went wrong

    treeDataAndStats.stats.totalFileSize += itBlobInfo->second;
  }

  // recursively into subtrees
  for (const auto &subTreeNameLen : treeDataAndStats.entryTreesNameLen)
  {
    OdbObjectsInfo::iterTreeInfo itSubTreeInfo {};
    if ((itSubTreeInfo = calculateTreeStatistics(subTreeNameLen.first)) == m_odbObjectsInfo.treesInfo.info.end())
      return itSubTreeInfo;

    OdbObjectsInfo::TreeDataAndStats &subTreeDataAndStats = itSubTreeInfo->second;
    treeDataAndStats.stats.numDirectories += subTreeDataAndStats.stats.numDirectories;
    treeDataAndStats.stats.maxPathDepth = std::max(
      treeDataAndStats.stats.maxPathDepth, subTreeDataAndStats.stats.maxPathDepth + 1);
    treeDataAndStats.stats.maxPathLength = std::max(
      treeDataAndStats.stats.maxPathLength,
      subTreeNameLen.second + 1 + subTreeDataAndStats.stats.maxPathLength);
    treeDataAndStats.stats.numFiles += subTreeDataAndStats.stats.numFiles;
    treeDataAndStats.stats.totalFileSize += subTreeDataAndStats.stats.totalFileSize;
    treeDataAndStats.stats.numSymlinks += subTreeDataAndStats.stats.numSymlinks;
    treeDataAndStats.stats.numSubmodules += subTreeDataAndStats.stats.numSubmodules;
  }

  treeDataAndStats.statsDone = true;

  return itTreeInfo;
}

/**
 * RepoAnalysis::calculateMaxTagDepth
 */
int RepoAnalysis::calculateMaxTagDepth()
{
  for (auto &tagInfo : m_odbObjectsInfo.tagsInfo.info)
  {
    OdbObjectsInfo::iterTagInfo itTagInfo {};
    if ((itTagInfo = calculateTagDepth(tagInfo.first)) == m_odbObjectsInfo.tagsInfo.info.end())
      return GIT_EUSER;

    // update maxTagDepth
    OdbObjectsInfo::TagData &tagData = itTagInfo->second;
    m_statistics.historyStructure.maxTagDepth = std::max(
      m_statistics.historyStructure.maxTagDepth, tagData.depth);
  }

  return GIT_OK;
}

/**
 * RepoAnalysis::calculateTagDepth
 * 
 * Calculates recursively the tag depth of the oidTag passed as a parameter.
 * Returns an iterator to the tag info container, or to end of the tag info container if something went wrong.
 */
OdbObjectsInfo::iterTagInfo RepoAnalysis::calculateTagDepth(const std::string &oidTag)
{
  OdbObjectsInfo::iterTagInfo itTagInfo = m_odbObjectsInfo.tagsInfo.info.find(oidTag);
  if (itTagInfo == m_odbObjectsInfo.tagsInfo.info.end())
    return itTagInfo;

  OdbObjectsInfo::TagData &tagData = itTagInfo->second;

  // prune recursivity
  if (tagData.depth != OdbObjectsInfo::TagData::kUnsetDepth)
    return itTagInfo;

  ++tagData.depth;

  if (tagData.typeTarget == GIT_OBJECT_TAG)
  {
    OdbObjectsInfo::iterTagInfo itChainedTagInfo {};
    if ((itChainedTagInfo = calculateTagDepth(tagData.oidTarget)) == m_odbObjectsInfo.tagsInfo.info.end())
      return itChainedTagInfo;

    OdbObjectsInfo::TagData &chainedTagData = itChainedTagInfo->second;
    tagData.depth += chainedTagData.depth;
  }

  return itTagInfo;
}

/**
 * RepoAnalysis::analyzeReferences
 */
int RepoAnalysis::analyzeReferences()
{
  int errorCode {GIT_OK};
  git_strarray ref_list;

  if ((errorCode = git_reference_list(&ref_list, m_repo)) != GIT_OK) {
    return errorCode;
  }
  m_statistics.repositorySize.references.count = ref_list.count;
  git_strarray_dispose(&ref_list);

  return errorCode;
}

/**
 * RepoAnalysis::fillOutStatistics
 */
void RepoAnalysis::fillOutStatistics()
{
  m_statistics.repositorySize.commits.count = m_odbObjectsInfo.commitsInfo.info.size();
  m_statistics.repositorySize.commits.size = m_odbObjectsInfo.commitsInfo.totalSize;
  m_statistics.repositorySize.trees.count = m_odbObjectsInfo.treesInfo.info.size();
  m_statistics.repositorySize.trees.size = m_odbObjectsInfo.treesInfo.totalSize;
  m_statistics.repositorySize.trees.entries = m_odbObjectsInfo.treesInfo.totalEntries;
  m_statistics.repositorySize.blobs.count = m_odbObjectsInfo.blobsInfo.info.size();
  m_statistics.repositorySize.blobs.size = m_odbObjectsInfo.blobsInfo.totalSize;
  m_statistics.repositorySize.annotatedTags.count = m_odbObjectsInfo.tagsInfo.info.size();

  m_statistics.biggestObjects.commits.maxSize = m_odbObjectsInfo.commitsInfo.maxSize;
  m_statistics.biggestObjects.commits.maxParents = m_odbObjectsInfo.commitsInfo.maxParents;
  m_statistics.biggestObjects.trees.maxEntries = m_odbObjectsInfo.treesInfo.maxEntries;
  m_statistics.biggestObjects.blobs.maxSize = m_odbObjectsInfo.blobsInfo.maxSize;

  // m_statistics.biggestCheckouts have already been filled out while running
}

/**
 * RepoAnalysis::StatisticsToJS
 */
v8::Local<v8::Object> RepoAnalysis::StatisticsToJS() const
{
  v8::Local<v8::Object> result = Nan::New<Object>();

  v8::Local<v8::Object> repositorySize = repositorySizeToJS();
  Nan::Set(result, Nan::New("repositorySize").ToLocalChecked(), repositorySize);

  v8::Local<v8::Object> biggestObjects = biggestObjectsToJS();
  Nan::Set(result, Nan::New("biggestObjects").ToLocalChecked(), biggestObjects);

  v8::Local<v8::Object> historyStructure = historyStructureToJS();
  Nan::Set(result, Nan::New("historyStructure").ToLocalChecked(), historyStructure);

  v8::Local<v8::Object> biggestCheckouts = biggestCheckoutsToJS();
  Nan::Set(result, Nan::New("biggestCheckouts").ToLocalChecked(), biggestCheckouts);


  return result;
}

/**
 * RepoAnalysis::repositorySizeToJS
 */
v8::Local<v8::Object> RepoAnalysis::repositorySizeToJS() const
{
  v8::Local<v8::Object> commits = Nan::New<Object>();
  Nan::Set(commits, Nan::New("count").ToLocalChecked(), Nan::New<Number>(m_statistics.repositorySize.commits.count));
  Nan::Set(commits, Nan::New("size").ToLocalChecked(), Nan::New<Number>(m_statistics.repositorySize.commits.size));

  v8::Local<v8::Object> trees = Nan::New<Object>();
  Nan::Set(trees, Nan::New("count").ToLocalChecked(), Nan::New<Number>(m_statistics.repositorySize.trees.count));
  Nan::Set(trees, Nan::New("size").ToLocalChecked(), Nan::New<Number>(m_statistics.repositorySize.trees.size));
  Nan::Set(trees, Nan::New("entries").ToLocalChecked(), Nan::New<Number>(m_statistics.repositorySize.trees.entries));

  v8::Local<v8::Object> blobs = Nan::New<Object>();
  Nan::Set(blobs, Nan::New("count").ToLocalChecked(), Nan::New<Number>(m_statistics.repositorySize.blobs.count));
  Nan::Set(blobs, Nan::New("size").ToLocalChecked(), Nan::New<Number>(m_statistics.repositorySize.blobs.size));

  v8::Local<v8::Object> annotatedTags = Nan::New<Object>();
  Nan::Set(annotatedTags, Nan::New("count").ToLocalChecked(), Nan::New<Number>(m_statistics.repositorySize.annotatedTags.count));

  v8::Local<v8::Object> references = Nan::New<Object>();
  Nan::Set(references, Nan::New("count").ToLocalChecked(), Nan::New<Number>(m_statistics.repositorySize.references.count));

  v8::Local<v8::Object> result = Nan::New<Object>();
  Nan::Set(result, Nan::New("commits").ToLocalChecked(), commits);
  Nan::Set(result, Nan::New("trees").ToLocalChecked(), trees);
  Nan::Set(result, Nan::New("blobs").ToLocalChecked(), blobs);
  Nan::Set(result, Nan::New("annotatedTags").ToLocalChecked(), annotatedTags);
  Nan::Set(result, Nan::New("references").ToLocalChecked(), references);

  return result;
}

/**
 * RepoAnalysis::biggestObjectsToJS
 */
v8::Local<v8::Object> RepoAnalysis::biggestObjectsToJS() const
{
  v8::Local<v8::Object> commits = Nan::New<Object>();
  Nan::Set(commits, Nan::New("maxSize").ToLocalChecked(), Nan::New<Number>(m_statistics.biggestObjects.commits.maxSize));
  Nan::Set(commits, Nan::New("maxParents").ToLocalChecked(), Nan::New<Number>(m_statistics.biggestObjects.commits.maxParents));

  v8::Local<v8::Object> trees = Nan::New<Object>();
  Nan::Set(trees, Nan::New("maxEntries").ToLocalChecked(), Nan::New<Number>(m_statistics.biggestObjects.trees.maxEntries));

  v8::Local<v8::Object> blobs = Nan::New<Object>();
  Nan::Set(blobs, Nan::New("maxSize").ToLocalChecked(), Nan::New<Number>(m_statistics.biggestObjects.blobs.maxSize));

  v8::Local<v8::Object> result = Nan::New<Object>();
  Nan::Set(result, Nan::New("commits").ToLocalChecked(), commits);
  Nan::Set(result, Nan::New("trees").ToLocalChecked(), trees);
  Nan::Set(result, Nan::New("blobs").ToLocalChecked(), blobs);

  return result;
}

/**
 * RepoAnalysis::historyStructureToJS
 */
v8::Local<v8::Object> RepoAnalysis::historyStructureToJS() const
{
  v8::Local<v8::Object> result = Nan::New<Object>();
  Nan::Set(result, Nan::New("maxDepth").ToLocalChecked(), Nan::New<Number>(m_statistics.historyStructure.maxDepth));
  Nan::Set(result, Nan::New("maxTagDepth").ToLocalChecked(), Nan::New<Number>(m_statistics.historyStructure.maxTagDepth));

  return result;
}

/**
 * RepoAnalysis::biggestCheckoutsToJS
 */
v8::Local<v8::Object> RepoAnalysis::biggestCheckoutsToJS() const
{
  v8::Local<v8::Object> result = Nan::New<Object>();
  Nan::Set(result, Nan::New("numDirectories").ToLocalChecked(), Nan::New<Number>(m_statistics.biggestCheckouts.numDirectories));
  Nan::Set(result, Nan::New("maxPathDepth").ToLocalChecked(), Nan::New<Number>(m_statistics.biggestCheckouts.maxPathDepth));
  Nan::Set(result, Nan::New("maxPathLength").ToLocalChecked(), Nan::New<Number>(m_statistics.biggestCheckouts.maxPathLength));
  Nan::Set(result, Nan::New("numFiles").ToLocalChecked(), Nan::New<Number>(m_statistics.biggestCheckouts.numFiles));
  Nan::Set(result, Nan::New("totalFileSize").ToLocalChecked(), Nan::New<Number>(m_statistics.biggestCheckouts.totalFileSize));
  Nan::Set(result, Nan::New("numSymlinks").ToLocalChecked(), Nan::New<Number>(m_statistics.biggestCheckouts.numSymlinks));
  Nan::Set(result, Nan::New("numSubmodules").ToLocalChecked(), Nan::New<Number>(m_statistics.biggestCheckouts.numSubmodules));

  return result;
}

// DEBUG TIMES
// void RepoAnalysis::printTimes() const {
//   std::cout<< "duration ms (creation -> destruction): "
//     << chrono::duration_cast<chrono::milliseconds>(timeDestruction - timeCreation).count()
//     << std::endl;
//   std::cout<< "duration ms (creation -> timeStartOdbForEach): "
//     << chrono::duration_cast<chrono::milliseconds>(timeStartOdbForEach - timeCreation).count()
//     << std::endl;
//   std::cout<< "duration ms (timeStartOdbForEach -> timeEndOdbForEach): "
//     << chrono::duration_cast<chrono::milliseconds>(timeEndOdbForEach - timeStartOdbForEach).count()
//     << std::endl;
//   std::cout<< "duration ms (timeEndOdbForEach -> timeEndCalculateBiggestCheckouts): "
//     << chrono::duration_cast<chrono::milliseconds>(timeEndCalculateBiggestCheckouts - timeEndOdbForEach).count()
//     << std::endl;
//   std::cout<< "duration ms (timeEndCalculateBiggestCheckouts -> timeEndCalculateMaxTagDepth): "
//     << chrono::duration_cast<chrono::milliseconds>(timeEndCalculateMaxTagDepth - timeEndCalculateBiggestCheckouts).count()
//     << std::endl;
//   std::cout<< "duration ms (timeEndCalculateMaxTagDepth -> timeEndCalculateMaxDepth): "
//     << chrono::duration_cast<chrono::milliseconds>(timeEndCalculateMaxDepth - timeEndCalculateMaxTagDepth).count()
//     << std::endl;
//   std::cout<< "duration ms (timeEndCalculateMaxDepth -> timeEndAnalyzeReferences): "
//     << chrono::duration_cast<chrono::milliseconds>(timeEndAnalyzeReferences - timeEndCalculateMaxDepth).count()
//     << std::endl;
// }

NAN_METHOD(GitRepository::Statistics)
{
  if (!info[info.Length() - 1]->IsFunction()) {
    return Nan::ThrowError("Callback is required and must be a Function.");
  }

  StatisticsBaton* baton = new StatisticsBaton();

   baton->error_code = GIT_OK;
   baton->error = NULL;
   baton->repo = Nan::ObjectWrap::Unwrap<GitRepository>(info.This())->GetValue();
   baton->out = static_cast<void *>(new RepoAnalysis(baton->repo));
   
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[info.Length() - 1]));
  std::map<std::string, std::shared_ptr<nodegit::CleanupHandle>> cleanupHandles;
  StatisticsWorker *worker = new StatisticsWorker(baton, callback, cleanupHandles);
  worker->Reference<GitRepository>("repo", info.This());
  nodegit::Context *nodegitContext = reinterpret_cast<nodegit::Context *>(info.Data().As<External>()->Value());
  nodegitContext->QueueWorker(worker);
  return;
}

nodegit::LockMaster GitRepository::StatisticsWorker::AcquireLocks()
{
  nodegit::LockMaster lockMaster(true, baton->repo);

  return lockMaster;
}

void GitRepository::StatisticsWorker::Execute()
{
  git_error_clear();

  RepoAnalysis *repoAnalysis = static_cast<RepoAnalysis *>(baton->out);
  if ((baton->error_code = repoAnalysis->Analyze()) != GIT_OK)
  {
    if (git_error_last() != NULL) {
      baton->error = git_error_dup(git_error_last());
    }

    delete repoAnalysis;
    baton->out = nullptr;
  }
}

void GitRepository::StatisticsWorker::HandleErrorCallback()
{
  if (baton->error) {
    if (baton->error->message) {
      free((void *)baton->error->message);
    }

    free((void *)baton->error);
  }

  RepoAnalysis *repoAnalysis = static_cast<RepoAnalysis *>(baton->out);
  if (repoAnalysis) {
    delete repoAnalysis;
  }

  delete baton;
}

void GitRepository::StatisticsWorker::HandleOKCallback()
{
  if (baton->out != NULL)
  {
    RepoAnalysis *repoAnalysis = static_cast<RepoAnalysis *>(baton->out);
    Local<v8::Object> result = repoAnalysis->StatisticsToJS();

    delete repoAnalysis;

    Local<v8::Value> argv[2] = {
      Nan::Null(),
      result
    };
    callback->Call(2, argv, async_resource);
  }
  else if (baton->error)
  {
    Local<v8::Object> err;

    if (baton->error->message) {
      err = Nan::To<v8::Object>(Nan::Error(baton->error->message)).ToLocalChecked();
    } else {
      err = Nan::To<v8::Object>(Nan::Error("Method statistics has thrown an error.")).ToLocalChecked();
    }
    Nan::Set(err, Nan::New("errno").ToLocalChecked(), Nan::New(baton->error_code));
    Nan::Set(err, Nan::New("errorFunction").ToLocalChecked(), Nan::New("GitRepository.statistics").ToLocalChecked());
    Local<v8::Value> argv[1] = {
      err
    };

    callback->Call(1, argv, async_resource);

    if (baton->error->message) {
      free((void *)baton->error->message);
    }

    free((void *)baton->error);
  }
  else if (baton->error_code < 0)
  {
    Local<v8::Object> err = Nan::To<v8::Object>(Nan::Error("Method statistics has thrown an error.")).ToLocalChecked();
    Nan::Set(err, Nan::New("errno").ToLocalChecked(), Nan::New(baton->error_code));
    Nan::Set(err, Nan::New("errorFunction").ToLocalChecked(), Nan::New("GitRepository.statistics").ToLocalChecked());
    Local<v8::Value> argv[1] = {
      err
    };
    callback->Call(1, argv, async_resource);
  }
  else
  {
    callback->Call(0, NULL, async_resource);
  }

  delete baton;
}