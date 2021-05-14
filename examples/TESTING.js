var nodegit = require("../");

function delay(t, v) {
  return new Promise(function(resolve) { 
      setTimeout(resolve.bind(null, v), t);
  });
}

//var repoPath = "/Users/alex/Repos/AlexaXs/nodegit/";
// var repoPath = "/Users/alex/Repos/Axosoft/nodegit/";
// var repoPath = "/Users/alex/Repos/AlexaXs/Test/";
var repoPath = "/Users/alex/Repos/AlexaXs/Test3/";
//var repoPath = "/Users/alex/Repos/AlexaXs/Test3-local-notGCd/";
// var repoPath = "/Users/alex/Repos/AlexaXs/GitKraken/";

console.log("pid", process.pid);
console.log(repoPath);

return delay(1)//(14000)
.then(function() {
  nodegit.Repository.open(repoPath)
  .then(function(repo) {
    return repo.statistics()
      .then(function(analysisReport) {

        console.log(JSON.stringify(analysisReport,null,2));
      });
  });
});

// create arbitrary chained tags
// const createDeepTag = async (repoLocation, atTargetCommitSha, depth = 1) => {
//   const repo = await nodegit.Repository.open(repoLocation);
//   const signature = await repo.defaultSignature();
//   let targetObject = await nodegit.Object.lookup(repo, atTargetCommitSha, nodegit.Object.TYPE.COMMIT);
//   for (let i = 0; i < depth; ++i) {
//     const tagOid = await nodegit.Tag.create(repo, i.toString(), targetObject, signature, '', 0);
//     targetObject = await nodegit.Object.lookup(repo, tagOid, nodegit.Object.TYPE.TAG);
//   }
// };

// createDeepTag("/Users/alex/Repos/AlexaXs/Test/", "55b670fa37a592b0bf67da2ef44e35167073ba81", 3);