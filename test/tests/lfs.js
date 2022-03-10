var assert = require("assert");
var RepoUtils = require("../utils/repository_setup");
var path = require("path");
var local = path.join.bind(path, __dirname);

// TODO: must test it once 'initialize' is done
describe.only("LFS", function() {
  var NodeGit = require("../../");

  var LFS = NodeGit.LFS;

  var test;
  var fileName = "foobar.js";
  var repoPath = local("../repos/lfsRepo");

  beforeEach(function() {
    test = this;

    return RepoUtils.createRepository(repoPath)
      .then(function(repository) {
        test.repository = repository;

        return RepoUtils.commitFileToRepo(
          repository,
          fileName,
          "line1\nline2\nline3"
        );
      });
  });

  it("can initialize LFS repository", function() {
    return LFS.initialize(test.repository, { local: true })
      .then(function() {
        console.log("LFS repo initialized in JS test\n");
        assert(true);
      });
  });
});
