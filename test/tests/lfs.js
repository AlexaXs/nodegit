var assert = require("assert");
var RepoUtils = require("../utils/repository_setup");
var path = require("path");
var local = path.join.bind(path, __dirname);
const fse = require("fs-extra");

describe("LFS", function() {
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

  // TODO: need to test against bare repositories?
  it("can initialize LFS repository", function(done) {
    LFS.initialize(test.repository, { local: true })
    .then(function() {
      let gitLfsDirPath = path.join(test.repository.path(), "/lfs");
      fse.stat(gitLfsDirPath, function(err, stat) {
        if (!err && stat && stat.isDirectory()) {
          done();
        }
        else {
          assert.fail("LFS repository initialization failed!");
        }
      });
    });
  });
});
