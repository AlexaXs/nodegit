const {
  isMainThread,
  parentPort,
  workerData
} = require("worker_threads");
const assert = require("assert");
const NodeGit = require("../../");
const { promisify } = require("util");

if (isMainThread) {
  throw new Error("Must be run via worker thread");
}

parentPort.postMessage("init");

const { clonePath, url } = workerData;
const opts = {
    fetchOpts: {
      callbacks: {
        certificateCheck: () => 0
    }
  }
};


let repository;
const oid = "fce88902e66c72b5b93e75bdb5ae717038b221f6";

return NodeGit.Clone(url, clonePath, opts)
.then((_repository) => {
  repository = _repository;
  assert.ok(repository instanceof NodeGit.Repository);
  return repository.getCommit(oid);
}).then((commit) => {
  assert.ok(commit instanceof NodeGit.Commit);
  var historyCount = 0;
  // var expectedHistoryCount = 364;
  var history = commit.history();

  history.on("commit", function(commit) {
    if (++historyCount > 200) {
      console.log("historyCount: ", historyCount);
      process.exitCode = 2;
      process.exit();
      // parentPort.postMessage("stop");
    }
  });

  history.on("end", function(commits) {
    // it should not get this far
    console.log("historyCount: ", historyCount);
    parentPort.postMessage("failure");
    // assert.equal(historyCount, expectedHistoryCount);
    // assert.equal(commits.length, expectedHistoryCount);
  });

  history.on("error", function(err) {
    assert.ok(false);
  });

  history.start();

  console.log("     REACHES HERE     ");
  parentPort.postMessage("success");
  return promisify(setTimeout)(50000);
}).catch(() => parentPort.postMessage("failure"));



// const oid = "fce88902e66c72b5b93e75bdb5ae717038b221f6";
// let repository;
// return NodeGit.Clone(url, clonePath, opts).then((_repository) => {
//   repository = _repository;
//   assert.ok(repository instanceof NodeGit.Repository);
//   return repository.getCommit(oid);
// }).then((commit) => {
//   assert.ok(commit instanceof NodeGit.Commit);
//   var historyCount = 0;
//   // var expectedHistoryCount = 364;
//   var history = commit.history();

//   history.on("commit", function(commit) {
//     if (++historyCount > 200) {
//       process.exitCode = 2;
//       process.exit();
//       // parentPort.postMessage("stop");
//     }
//   });

//   history.on("end", function(commits) {
//     // it should not get this far
//     console.log("historyCount: ", historyCount);
//     parentPort.postMessage("failure");
//     // assert.equal(historyCount, expectedHistoryCount);
//     // assert.equal(commits.length, expectedHistoryCount);
//   });

//   history.on("error", function(err) {
//     assert.ok(false);
//   });

//   history.start();

//   console.log("     SHOULDN'T REACH HERE     ");
//   parentPort.postMessage("success");
//   return promisify(setTimeout)(5000);
// }).catch(() => parentPort.postMessage("failure"));
