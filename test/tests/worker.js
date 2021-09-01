const garbageCollect = require("../utils/garbage_collect.js");
const path = require("path");
const assert = require("assert");
const fse = require("fs-extra");
const local = path.join.bind(path, __dirname);

let Worker;

try {
  Worker = require("worker_threads").Worker;
} catch (e) {}

if (Worker) {
  describe("Worker", function() {

    function delay(t, v) {
      return new Promise(function(resolve) { 
          setTimeout(resolve.bind(null, v), t);
      });
    }

    
    const clonePath = local("../repos/clone");

    console.log("pid", process.pid);
    
    // Set a reasonable timeout here now that our repository has grown.
    this.timeout(30000);

    beforeEach(function() {

      // return delay(14000)
      return delay(1)
      .then(function() {
        return fse.remove(clonePath).catch(function(err) {
          console.log(err);
  
          throw err;
        });
      });
    });

    it("can perform basic functionality via worker thread", function(done) {
      const workerPath = local("../utils/worker.js");
      const worker = new Worker(workerPath, {
        workerData: {
          clonePath,
          url: "https://github.com/nodegit/test.git"
        }
      });
      worker.on("message", (message) => {
        switch (message) {
          case "init":
            break;
          case "success":
            done();
            break;
          case "failure":
            assert.fail();
            break;
        }
      });
      worker.on("error", () => assert.fail());
      worker.on("exit", (code) => {
        if (code !== 0) {
          assert.fail();
        }
      });
    });

    for (let i = 0; i < 1; ++i) {
      it(`can kill worker thread while in use #${i}`, function(done) { // jshint ignore:line
        const workerPath = local("../utils/worker.js");
        const worker = new Worker(workerPath, {
          workerData: {
            clonePath,
            url: "https://github.com/nodegit/test.git"
          }
        });
        worker.on("message", (message) => {
          switch (message) {
            case "init":
              console.log("main on init\n");
              break;
            case "success":
              console.log("main on success\n");
              worker.terminate();
              break;
            case "failure":
              console.log("main on failure\n");
              assert.fail();
              break;
          }
        });
        worker.on("error", () => {
          console.log("main on error\n");
          assert.fail();
        });
        worker.on("exit", (code, signal) => {
          console.log("main on exit with code", code);
          console.log(" and signal ", signal, "\n");
          if (code === 1) {
            done();
          } else {
            assert.fail();
          }
        });
      });
    }

    it.only("can free objects on context shutdown", function(done) {
      garbageCollect();
      garbageCollect();
      garbageCollect();
      console.log("memory usage before\n", process.memoryUsage());
      const workerPath = local("../utils/worker_context_aware.js");
      const worker = new Worker(workerPath, {
        workerData: {
          clonePath,
          url: "https://github.com/nodegit/test.git"
        }
      });
      worker.on("message", (message) => {
        switch (message) {
          case "init":
            console.log("init received\n");
            break;
          // case "stop":
          //   worker.terminate();
          //   break;
          case "success":
            // worker.terminate();
            console.log("success received\n");
            break;
          case "failure":
            console.log("failure received\n");
            assert.fail();
            break;
        }
      });
      worker.on("error", () => assert.fail());
      worker.on("exit", (code) => {
        if (code === 1) {
          done();
        }
        else if (code === 2) {
          garbageCollect();
          garbageCollect();
          garbageCollect();
          console.log("memory usage on worker exit 2\n", process.memoryUsage());
          done();
          // setTimeout(function() {
          //   // garbageCollect();
          //   // garbageCollect();
          //   // garbageCollect();
          //   console.log("memory usage on worker exit\n",
          //     process.memoryUsage());
          // }, 10000);
        }
        else {
          assert.fail();
        }
      });
    });
  });
}
