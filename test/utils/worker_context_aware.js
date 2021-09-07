const {
  isMainThread,
  parentPort,
  workerData
} = require("worker_threads");
const garbageCollect = require("./garbage_collect.js");
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
    // number of commits is known to be higher than 200
    if (++historyCount == 10) {//200) {
      // Tracked objects must work as well when
      // the Garbage Collector is triggered
      garbageCollect();


      // const countCert =
      // //getSelfFreeingInstanceCount();
      //   NodeGit.Cert.getNonSelfFreeingConstructedCount();
      // const countRepo =
      //   NodeGit.Repository.getSelfFreeingInstanceCount();
      // const countCommit =
      //   NodeGit.Commit.getSelfFreeingInstanceCount();
      // const countOid =
      //   NodeGit.Oid.getSelfFreeingInstanceCount();
      // const countRevwalk =
      //   NodeGit.Revwalk.getSelfFreeingInstanceCount();


      // count total of objects left after being
      // created/destroyed
      const selfFreeingCount =
        NodeGit.Cert.getNonSelfFreeingConstructedCount() +
        NodeGit.Repository.getSelfFreeingInstanceCount() +
        NodeGit.Commit.getSelfFreeingInstanceCount() +
        NodeGit.Oid.getSelfFreeingInstanceCount() +
        NodeGit.Revwalk.getSelfFreeingInstanceCount();

      // NOTE: getTotalOfTrackedObjects() can be called from any NodeGitWrapper
      // object. The reason is it obtains the information from
      // nodegit::Context, which is not accessible from javascript.
      const totalTrackedObjects = NodeGit.Repository.getTotalOfTrackedObjects();

      // console.log(
      //   "countCert: ", countCert,
      //   "countRepo: ", countRepo,
      //   "countCommit: ", countCommit,
      //   "countOid: ", countOid,
      //   "countRevwalk: ", countRevwalk,
      //   "tracked objects: ", NodeGit.Repository.getTotalOfTrackedObjects());


      console.log(
        "selfFreeingCount objects: ", selfFreeingCount,
        "tracked objects: ", NodeGit.Repository.getTotalOfTrackedObjects());

      if (selfFreeingCount === totalTrackedObjects) {
        parentPort.postMessage("numbersMatch");
      }
      else {
        parentPort.postMessage("numbersDoNotMatch");
      }
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

  console.log("     IN WORKER. 1     ");

  // const report2 = process.report.getReport();
  // console.log("START REPORT 2 (IN WORKER):\n");
  // console.log(JSON.stringify(report2, null, 2));
  // console.log("END REPORT 2 (IN WORKER):\n");


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
