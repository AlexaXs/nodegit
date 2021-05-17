/**
 * \struct CommitTreeNode
 */
struct CommitTreeNode
{
  CommitTreeNode() = default;
  ~CommitTreeNode() = default;
  CommitTreeNode(const CommitTreeNode &other) = delete;
  CommitTreeNode(CommitTreeNode &&other) = delete;
  CommitTreeNode& operator=(const CommitTreeNode &other) = delete;
  CommitTreeNode& operator=(CommitTreeNode &&other) = delete;

  std::vector<CommitTreeNode *> children {};
};

/**
 * \class CommitTree
 */
class CommitTree
{
public:
  CommitTree() = default;
  ~CommitTree() = default;
  CommitTree(const CommitTree &other) = delete;
  CommitTree(CommitTree &&other) = delete;
  CommitTree& operator=(const CommitTree &other) = delete;
  CommitTree& operator=(CommitTree &&other) = delete;

  using CommitTreeMap = std::unordered_map<std::string, std::unique_ptr<CommitTreeNode>>;

  void AddNode(const git_oid *oid, const git_commit *commit, unsigned int numParents);
  size_t CalculateMaxDepth() const;

private:
  void addParentNode(const git_oid *oid, CommitTreeNode *child);

  CommitTreeMap m_mapOidNode {};
  std::vector<CommitTreeNode *> m_roots {};
};

/**
 * CommitTree::AddNode
 * 
 * \param oid git_oid of the commit object to add.
 * \param node git_commit object source from which to extract information for the tree node.
 * \param numParents Number of parents of the commit object.
  */
void CommitTree::AddNode(const git_oid *oid, const git_commit *commit, unsigned int numParents)
{
  std::string oidStr(reinterpret_cast<const char *>(oid->id), GIT_OID_RAWSZ);
  CommitTreeMap::iterator it = m_mapOidNode.find(oidStr);

  // add node if not added yet
  if (it == m_mapOidNode.end()) {
    std::pair<CommitTreeMap::iterator, bool> pairInsert = m_mapOidNode.insert({oidStr, std::make_unique<CommitTreeNode>()});
    it = pairInsert.first;
  }

  // set root
  if (numParents == 0) {
    m_roots.emplace_back(it->second.get());
  }

  // add parents
  for (unsigned int i = 0; i < numParents; ++i) {
    addParentNode(git_commit_parent_id(commit, i), it->second.get());
  }
}

/**
 * CommitTree::CalculateMaxDepth
 * \return Calculated maximum depth of the tree.
 * 
 * Uses iterative algorithm to count levels.
 * Considers that children of one level can have multiple parents, hence we insert unique children at each level.
 * Considers that same child can be in different levels, needed to count the longest path to that level.
 * Considers multiple initial commits.
 * Recursive algorithm avoided to prevent stack overflow in case of excessive levels in the tree.
  */
size_t CommitTree::CalculateMaxDepth() const
{
  size_t maxDepth {0};
  std::set<const CommitTreeNode *> parents {};
  std::set<const CommitTreeNode *> children {};

  // initial commmits
  for (const CommitTreeNode *root : m_roots) {
    children.insert(root);
  }

  while (!children.empty())
  {
    ++maxDepth;

    parents = std::move(children);

    // add unique children of next level
    for (const CommitTreeNode *parent : parents) {
      for (const CommitTreeNode *child : parent->children) {
          children.insert(child);
      }
    }
  }

  return maxDepth;
}

/**
 * CommitTree::addParentNode
 * 
 * \param oid git_oid of the commit object to add.
 * \param child Child of the parent node being added.
  */
void CommitTree::addParentNode(const git_oid *oid_parent, CommitTreeNode *child)
{
  std::string oidParentStr(reinterpret_cast<const char *>(oid_parent->id), GIT_OID_RAWSZ);
  CommitTreeMap::iterator it = m_mapOidNode.find(oidParentStr);

  // add parent node if not added yet
  if (it == m_mapOidNode.end()) {
    std::pair<CommitTreeMap::iterator, bool> pairInsert = m_mapOidNode.insert({oidParentStr, std::make_unique<CommitTreeNode>()});
    it = pairInsert.first;
  }

  // add child to parent
  it->second->children.emplace_back(child);
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
  } repositorySize;

  struct {
    size_t maxDepth {0};
    size_t maxTagDepth {0};
  } historyStructure;

  struct {
    struct { size_t maxSize {0}; size_t maxParents {0}; } commits;
    struct { size_t maxEntries {0}; } trees;
    struct { size_t maxSize {0}; } blobs;
  } biggestObjects;
  
  TreeStatistics biggestCheckouts {};
};

/**
 * \struct ForeachOdbCbPayload
 * Payload for git_odb_foreach
 */
struct ForeachOdbCbPayload
{
  explicit ForeachOdbCbPayload(git_repository *aRepo, Statistics *aStatistics, CommitTree *aCommitTree)
    : repo(aRepo), statistics(aStatistics), commitTree(aCommitTree) {};
  ForeachOdbCbPayload() = delete;
  ~ForeachOdbCbPayload() = default;
  ForeachOdbCbPayload(const Statistics &other) = delete;
  ForeachOdbCbPayload(ForeachOdbCbPayload &&other) = delete;
  ForeachOdbCbPayload& operator=(const ForeachOdbCbPayload &other) = delete;
  ForeachOdbCbPayload& operator=(ForeachOdbCbPayload &&other) = delete;

  git_repository *repo {nullptr};
  Statistics *statistics {nullptr};
  CommitTree *commitTree {nullptr};

  git_odb *odb {nullptr};
  std::unordered_set<std::string> uniqueObjects {};
  std::unordered_map<std::string, TreeStatistics> treesStatistics {};
  std::unordered_map<std::string, size_t> tagsDepth {};  // annotated tags depth (chain of annot. tags to reach a commit object)
};

/**
 * calculateTreeStatistics.
 * 
 * Recursive function to calculate statistics of a tree object.
 * For better performance, make sure this function is only called once for each tree object in the object database.
 * Caller must make sure oid_tree belongs to an object of type GIT_OBJECT_TREE.
 * Returns GIT_OK on success; GIT_EUSER otherwise.
 */
static int calculateTreeStatistics(const git_oid *oid_tree, ForeachOdbCbPayload *payload)
{
  // calculate statistics for properties "repositorySize" and "biggestObjects"
  git_tree *tree {nullptr};
  if (git_tree_lookup(&tree, payload->repo, oid_tree) != GIT_OK) {
    return GIT_EUSER;
  }

  // do not count empty trees, like git's empty tree "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
  size_t numTE = git_tree_entrycount(tree);
  if (numTE == 0) {
    git_tree_free(tree);
    return GIT_OK;
  }

  ++payload->statistics->repositorySize.trees.count;
  payload->statistics->repositorySize.trees.entries += numTE;

  git_odb_object *obj {nullptr};
  if (git_odb_read(&obj, payload->odb, oid_tree) != GIT_OK) {
    return GIT_EUSER;
  }

  size_t osize = git_odb_object_size(obj);
  payload->statistics->repositorySize.trees.size += osize;
  git_odb_object_free(obj);

  payload->statistics->biggestObjects.trees.maxEntries = std::max(payload->statistics->biggestObjects.trees.maxEntries, numTE);

  // calculate statistics for property "biggestCheckouts"
  TreeStatistics treeStats {};
  ++treeStats.numDirectories;
  ++treeStats.maxPathDepth;

  const git_tree_entry *te {nullptr};
  const char *teName {nullptr};
  size_t teNameLen {0};

  for (size_t i = 0; i < numTE; ++i)
  {
    te = git_tree_entry_byindex(tree, i);
    if (te != nullptr)
    {
      teName = git_tree_entry_name(te);
      teNameLen = std::char_traits<char>::length(teName);
      git_object_t otype = git_tree_entry_type(te);
      switch (otype)
      {
        // count submodules
        case GIT_OBJECT_COMMIT:
          if (git_tree_entry_filemode(te) == GIT_FILEMODE_COMMIT) {
            ++treeStats.numSubmodules;
          }
          break;

        case GIT_OBJECT_BLOB:
        {
          ++treeStats.numFiles;
          treeStats.maxPathLength = std::max(treeStats.maxPathLength, teNameLen);

          git_blob *blob {nullptr};
          const git_oid *te_oid = git_tree_entry_id(te);
          if (git_blob_lookup(&blob, payload->repo, te_oid) != GIT_OK) {
            git_tree_free(tree);
            return GIT_EUSER;
          }
          treeStats.totalFileSize += git_blob_rawsize(blob);
          git_blob_free(blob);

          // count symbolic links
          if (git_tree_entry_filemode(te) == GIT_FILEMODE_LINK) {
            ++treeStats.numSymlinks;
          }
        }
          break;
          
        case GIT_OBJECT_TREE:
        {
          // recursively calculate statistics of this subTree.
          const git_oid *oid_subTree = git_tree_entry_id(te);
          std::string oid_subTreeStr = std::string(reinterpret_cast<const char *>(oid_subTree->id), GIT_OID_RAWSZ);

          // emplace (to mark it as analyzed) or retrieve the tree statistics already calculated.
          if (payload->uniqueObjects.emplace(oid_subTreeStr).second == true) {
            if (calculateTreeStatistics(oid_subTree, payload) != GIT_OK) {
              git_tree_free(tree);
              return GIT_EUSER;
            }
          }

          // update tree statistics with subtree statistics
          std::unordered_map<std::string, TreeStatistics>::iterator itSubTreeStats = payload->treesStatistics.find(oid_subTreeStr);
          if (itSubTreeStats != payload->treesStatistics.end())
          {
            TreeStatistics &subTreeStats = itSubTreeStats->second;

            treeStats.numDirectories += subTreeStats.numDirectories;
            treeStats.maxPathDepth = std::max(treeStats.maxPathDepth, subTreeStats.maxPathDepth + 1);
            treeStats.maxPathLength = std::max(treeStats.maxPathLength, teNameLen + 1 + subTreeStats.maxPathLength);
            treeStats.numFiles += subTreeStats.numFiles;
            treeStats.totalFileSize += subTreeStats.totalFileSize;
            treeStats.numSymlinks += subTreeStats.numSymlinks;
            treeStats.numSubmodules += subTreeStats.numSubmodules;
          }
        }
          break;
          
        default:
          break;
      }
    }
  }

  git_tree_free(tree);

  payload->treesStatistics.emplace(std::string(reinterpret_cast<const char *>(oid_tree->id), GIT_OID_RAWSZ), std::move(treeStats));
  
  return GIT_OK;
}

/**
 * calculateTagStatistics.
 * 
 * Recursive function to calculate statistics of a tag object.
 * For better performance, make sure this function is only called once for each tag object in the object database.
 * Caller must make sure oid_tree belongs to an object of type GIT_OBJECT_TAG.
 * Returns GIT_OK on success; GIT_EUSER otherwise.
 */
static int calculateTagStatistics(const git_oid *oid_tag, ForeachOdbCbPayload *payload)
{
  git_tag *tag {nullptr};
  if (git_tag_lookup(&tag, payload->repo, oid_tag) != GIT_OK) {
    return GIT_EUSER;
  }

  // calculate statistics for properties "repositorySize" and "historyStructure"
  ++payload->statistics->repositorySize.annotatedTags.count;
  size_t tagDepth {1};

	git_object_t target_type;
  target_type = git_tag_target_type(tag);
  
  if (target_type == GIT_OBJECT_TAG)
  {
    const git_oid *oid_target = git_tag_target_id(tag);
    std::string oid_targetStr = std::string(reinterpret_cast<const char *>(oid_target->id), GIT_OID_RAWSZ);
    
    // emplace (to mark it as analyzed) or retrieve the tag depth already calculated.
    if (payload->uniqueObjects.emplace(oid_targetStr).second == true) {
      if (calculateTagStatistics(oid_target, payload) != GIT_OK) {
        git_tag_free(tag);
        return GIT_EUSER;
      }
    }

    // update tag depth with target depth
    std::unordered_map<std::string, size_t>::iterator itTargetDepth = payload->tagsDepth.find(oid_targetStr);
    if (itTargetDepth != payload->tagsDepth.end()) {
      tagDepth += itTargetDepth->second;
    }
  }

  git_tag_free(tag);

  payload->tagsDepth.emplace(std::string(reinterpret_cast<const char *>(oid_tag->id), GIT_OID_RAWSZ), std::move(tagDepth));

  return GIT_OK;
}

/**
 * foreachOdbCb. Callback for git_odb_foreach.
 * Returns GIT_OK on success; GIT_EUSER otherwise
 */
static int foreachOdbCb(const git_oid *oid, void *payloadToCast)
{
  ForeachOdbCbPayload *payload = static_cast<ForeachOdbCbPayload *>(payloadToCast);
  
  // // emplace (to mark it as analyzed) or return if object already analyzed
  // if (payload->uniqueObjects.emplace(reinterpret_cast<const char *>(oid->id), GIT_OID_RAWSZ).second == false) {
  //   return GIT_OK;
  // }

  git_odb_object *obj {nullptr};
  if (git_odb_read(&obj, payload->odb, oid) != GIT_OK) {
    return GIT_EUSER;
  }

  git_object_t otype = git_odb_object_type(obj);
  // size_t osize = git_odb_object_size(obj);
  git_odb_object_free(obj);

  switch (otype)
  {
    case GIT_OBJECT_COMMIT:
    {
      // calculate statistics for properties "repositorySize" and "biggestObjects"
      // ++payload->statistics->repositorySize.commits.count;
      // payload->statistics->repositorySize.commits.size += osize;

      // payload->statistics->biggestObjects.commits.maxSize = std::max(payload->statistics->biggestObjects.commits.maxSize, osize);

      git_commit *commit {nullptr};
      if (git_commit_lookup(&commit, payload->repo, oid) != GIT_OK) {
        return GIT_EUSER;
      }
      
      // size_t numParents  = git_commit_parentcount(commit);
      // payload->statistics->biggestObjects.commits.maxParents = std::max(payload->statistics->biggestObjects.commits.maxParents, numParents);

      // // add commit to tree, to build commit history
      // payload->commitTree->AddNode(oid, commit, numParents);
      
      // // calculate statistics of the tree pointed by this commit
      // const git_oid *oid_tree = git_commit_tree_id(commit);
      // std::string oid_treeStr = std::string(reinterpret_cast<const char *>(oid_tree->id), GIT_OID_RAWSZ);
      // std::unordered_map<std::string, TreeStatistics>::iterator itTreeStats {};

      // // emplace (to mark it as analyzed) or retrieve the tree statistics already calculated.
      // if (payload->uniqueObjects.emplace(oid_treeStr).second == true) {
      //   if (calculateTreeStatistics(oid_tree, payload) != GIT_OK) {
      //     git_commit_free(commit);
      //     return GIT_EUSER;
      //   }
      // }

      git_commit_free(commit);
      
      // // update statistics for property "biggestCheckouts"
      // itTreeStats = payload->treesStatistics.find(oid_treeStr);
      // if (itTreeStats != payload->treesStatistics.end())
      // {
      //   TreeStatistics &treeStats = itTreeStats->second;
      //   TreeStatistics &biggestCheckouts = payload->statistics->biggestCheckouts;

      //   biggestCheckouts.numDirectories = std::max(biggestCheckouts.numDirectories, treeStats.numDirectories);
      //   biggestCheckouts.maxPathDepth = std::max(biggestCheckouts.maxPathDepth, treeStats.maxPathDepth);
      //   biggestCheckouts.maxPathLength = std::max(biggestCheckouts.maxPathLength, treeStats.maxPathLength);
      //   biggestCheckouts.numFiles = std::max(biggestCheckouts.numFiles, treeStats.numFiles);
      //   biggestCheckouts.totalFileSize = std::max(biggestCheckouts.totalFileSize, treeStats.totalFileSize);
      //   biggestCheckouts.numSymlinks = std::max(biggestCheckouts.numSymlinks, treeStats.numSymlinks);
      //   biggestCheckouts.numSubmodules = std::max(biggestCheckouts.numSubmodules, treeStats.numSubmodules);
      // }
    }
      break;

    case GIT_OBJECT_TREE:
    {
      git_tree *tree {nullptr};
      if (git_tree_lookup(&tree, payload->repo, oid) != GIT_OK) {
        return GIT_EUSER;
      }
      git_tree_free(tree);
    }

      // // calculate statistics for property "biggestCheckouts"
      // if (calculateTreeStatistics(oid, payload) != GIT_OK) {
      //   return GIT_EUSER;
      // }
      break;

    case GIT_OBJECT_BLOB:
    {
      git_blob *blob {nullptr};
      if (git_blob_lookup(&blob, payload->repo, oid) != GIT_OK) {
        return GIT_EUSER;
      }
      git_blob_free(blob);
    }
      // // calculate statistics for properties "repositorySize" and "biggestObjects"
      // ++payload->statistics->repositorySize.blobs.count;
      // payload->statistics->repositorySize.blobs.size += osize;

      // payload->statistics->biggestObjects.blobs.maxSize = std::max(payload->statistics->biggestObjects.blobs.maxSize, osize);
      break;

    case GIT_OBJECT_TAG:
    {
      git_tag *tag {nullptr};
      if (git_tag_lookup(&tag, payload->repo, oid) != GIT_OK) {
        return GIT_EUSER;
      }
      git_tag_free(tag);

      // // calculate statistics for property "repositorySize" and "historyStructure"
      // if (calculateTagStatistics(oid, payload) != GIT_OK) {
      //   return GIT_EUSER;
      // }

      // // update maximum tag depth for property "historyStructure"
      // std::unordered_map<std::string, size_t>::iterator itTagDepth = payload->tagsDepth.find(std::string(reinterpret_cast<const char *>(oid->id), GIT_OID_RAWSZ));
      // if (itTagDepth != payload->tagsDepth.end()) {
      //   payload->statistics->historyStructure.maxTagDepth = std::max(payload->statistics->historyStructure.maxTagDepth, itTagDepth->second);
      // }
    }
      break;

    default:
      break;
  }

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
    : m_repo(repo) {  timeCreation = std::chrono::system_clock::now(); };
  RepoAnalysis() = delete;
  ~RepoAnalysis() { timeDestruction = std::chrono::system_clock::now(); printTimes(); };
  RepoAnalysis(const RepoAnalysis &other) = delete;
  RepoAnalysis(RepoAnalysis &&other) = delete;
  RepoAnalysis& operator=(const RepoAnalysis &other) = delete;
  RepoAnalysis& operator=(RepoAnalysis &&other) = delete;

  int Analyze();
  v8::Local<v8::Object> StatisticsToJS() const;

public:
  std::chrono::time_point<std::chrono::system_clock> timeCreation {};
  std::chrono::time_point<std::chrono::system_clock> timeStartOdbForEach {};
  std::chrono::time_point<std::chrono::system_clock> timeEndOdbForEach {};
  std::chrono::time_point<std::chrono::system_clock> timeEndCalculateMaxDepth {};
  std::chrono::time_point<std::chrono::system_clock> timeEndAnalyzeReferences {};  
  std::chrono::time_point<std::chrono::system_clock> timeDestruction {};

private:
  int analyzeObjects();
  int analyzeReferences();
  v8::Local<v8::Object> repositorySizeToJS() const;
  v8::Local<v8::Object> biggestObjectsToJS() const;
  v8::Local<v8::Object> historyStructureToJS() const;
  v8::Local<v8::Object> biggestCheckoutsToJS() const;

  void printTimes() {
    std::cout<< "duration ms (creation -> destruction): " << chrono::duration_cast<chrono::milliseconds>(timeDestruction - timeCreation).count() << std::endl;
    std::cout<< "duration ms (creation -> timeStartOdbForEach): " << chrono::duration_cast<chrono::milliseconds>(timeStartOdbForEach - timeCreation).count() << std::endl;
    std::cout<< "duration ms (timeStartOdbForEach -> timeEndOdbForEach): " << chrono::duration_cast<chrono::milliseconds>(timeEndOdbForEach - timeStartOdbForEach).count() << std::endl;
    std::cout<< "duration ms (timeEndOdbForEach -> timeEndCalculateMaxDepth): " << chrono::duration_cast<chrono::milliseconds>(timeEndCalculateMaxDepth - timeEndOdbForEach).count() << std::endl;
    std::cout<< "duration ms (timeEndCalculateMaxDepth -> timeEndAnalyzeReferences): " << chrono::duration_cast<chrono::milliseconds>(timeEndAnalyzeReferences - timeEndCalculateMaxDepth).count() << std::endl;
  }

  git_repository *m_repo {nullptr};
  Statistics m_statistics {};
  // Tree to be build while reading the object database.
  // Will be used to obtain maximum history depth.
  CommitTree m_commitTree {};
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

  timeEndAnalyzeReferences = std::chrono::system_clock::now();

  return errorCode;
}

/**
 * RepoAnalysis::analyzeObjects
 * Analyzes objects (commits, trees, blobs, annotated tags)
 */
int RepoAnalysis::analyzeObjects()
{
  int errorCode {GIT_OK};
  ForeachOdbCbPayload payload {m_repo, &m_statistics, &m_commitTree};

  if ((errorCode = git_repository_odb(&payload.odb, m_repo)) != GIT_OK) {
    return errorCode;
  }

  timeStartOdbForEach = std::chrono::system_clock::now();

  // analyze all the objects and build commit history tree
  if ((errorCode = git_odb_foreach(payload.odb, foreachOdbCb, &payload)) != GIT_OK) {
    return errorCode;
  }

  timeEndOdbForEach = std::chrono::system_clock::now();

  git_odb_free(payload.odb);

  // calculate max commit history depth
  m_statistics.historyStructure.maxDepth = m_commitTree.CalculateMaxDepth();

  timeEndCalculateMaxDepth = std::chrono::system_clock::now();

  return errorCode;
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