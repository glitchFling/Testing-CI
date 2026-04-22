import createModule from "./index.js";

createModule().then(Module => {
  const hash1024 = Module.cwrap("hash1024_c", "string", ["string"]);
  const hash2048 = Module.cwrap("hash2048_c", "string", ["string"]);
  const hash4096 = Module.cwrap("hash4096_c", "string", ["string"]);

  console.log("1024:", hash1024("demo-input"));
  console.log("2048:", hash2048("demo-input"));
  console.log("4096:", hash4096("demo-input"));
});
