const { WASI } = require('@wasmer/wasi');
const bindings = require("@wasmer/wasi/lib/bindings/browser").default;


  class AbortError extends Error {
    constructor(msg = 'abort') { super(msg); }
  }
  
  class AssertError extends Error {
    constructor(msg) { super(msg); }
  }
  
  function assert(cond) {
    if (!cond) {
      throw new AssertError('assertion failed.');
    }
  }
  
  
  function getModuleFromBuffer(buffer) {
    return WebAssembly.compile(buffer);
  }
  
  function getInstance(module, imports) {
    return WebAssembly.instantiate(module, imports);
  }
  
  function getImportObject(obj, names) {
    const result = {};
    for (let name of names) {
      result[name] = obj[name].bind(obj);
    }
    return result;
  }
  
  function readStr(u8, o, len = -1) {
    let str = '';
    let end = u8.length;
    if (len != -1)
      end = o + len;
    for (let i = o; i < end && u8[i] != 0; ++i)
      str += String.fromCharCode(u8[i]);
    return str;
  }
  
  const ESUCCESS = 0;
  
  export class Memory {
    constructor(memory) {
      this.memory = memory;
      this.buffer = this.memory.buffer;
      this.u8 = new Uint8Array(this.buffer);
      this.u32 = new Uint32Array(this.buffer);
    }
  
    check() {
      if (this.buffer.byteLength === 0) {
        this.buffer = this.memory.buffer;
        this.u8 = new Uint8Array(this.buffer);
        this.u32 = new Uint32Array(this.buffer);
      }
    }
  
    read8(o) { return this.u8[o]; }
    read32(o) { return this.u32[o >> 2]; }
    write8(o, v) { this.u8[o] = v; }
    write32(o, v) { this.u32[o >> 2] = v; }
    write64(o, vlo, vhi = 0) { this.write32(o, vlo); this.write32(o + 4, vhi); }
  
    readStr(o, len) {
      return readStr(this.u8, o, len);
    }
  
    // Null-terminated string.
    writeStr(o, str) {
      o += this.write(o, str);
      this.write8(o, 0);
      return str.length + 1;
    }
  
    write(o, buf) {
      if (buf instanceof ArrayBuffer) {
        return this.write(o, new Uint8Array(buf));
      } else if (typeof buf === 'string') {
        return this.write(o, buf.split('').map(x => x.charCodeAt(0)));
      } else {
        const dst = new Uint8Array(this.buffer, o, buf.length);
        dst.set(buf);
        return buf.length;
      }
    }
  };
  
  class HostWriteBuffer {
    constructor() {
      this.buffer = '';
    }
  
    write(str) {
      this.buffer += str;
    }
  
    flush() {
        return this.buffer;
    }
  }
  class HostReadBuffer {
    constructor() {
        this.str = "";
        this.pos = 0;
    }
    setBuffer(str){
        this.str = str;
        this.pos = 0;
    }
    read(n) {
        if(this.pos + n > this.str.length){
            n = this.str.length - this.pos;
        }
        this.pos += n
        return this.str.slice(this.pos,n);
    }
  
  }
  
  export class MemFS {
    constructor(memfsModule,wasmFs) {
        this.hostWriteBufferStdOut = new HostWriteBuffer();
        this.hostWriteBufferStdErr = new HostWriteBuffer();
        this.hostReadBuffer = new HostReadBuffer();
        this.hostMem_ = null;  // Set later when wired up to application.
  
      // Imports for memfs module.
      const env = getImportObject(
          this, [ 'abort', 'host_write','host_read','main', 'memfs_log', 'copy_in', 'copy_out' ]);
  
          let wasi  = new WASI({
            preopens:{},          
            env: {
                PWD:'/',
            },            
            args: [],           
            bindings: {
                ...bindings,
                fs: wasmFs.fs
            }});
      
      this.ready = getModuleFromBuffer(memfsModule)
                       .then(module => {                           
                            let imports = wasi.getImports(module);
                           return getInstance(
                               module, 
                                    {
                                        env,
                                    ...imports
                                })
                           })
                       .then(instance => {
                         this.instance = instance;
                         this.exports = instance.exports;
                         this.mem = new Memory(this.exports.memory);
                         this.exports.init();
                       })
    }
  
    set hostMem(mem) {
      this.hostMem_ = mem;
    }
    setStdIn(buf){
        this.hostReadBuffer.setBuffer(buf);
    } 
    addDirectory(path) {
      this.mem.check();
      this.mem.write(this.exports.GetPathBuf(), path);
      this.exports.AddDirectoryNode(path.length);
    }
  
    addFile(path, contents) {
      const length =
          contents instanceof ArrayBuffer ? contents.byteLength : contents.length;
      this.mem.check();
      this.mem.write(this.exports.GetPathBuf(), path);
      const inode = this.exports.AddFileNode(path.length, length);
      const addr = this.exports.GetFileNodeAddress(inode);
      this.mem.check();
      this.mem.write(addr, contents);
    }
  
    getFileContents(path) {
      this.mem.check();
      this.mem.write(this.exports.GetPathBuf(), path);
      const inode = this.exports.FindNode(path.length);
      const addr = this.exports.GetFileNodeAddress(inode);
      const size = this.exports.GetFileNodeSize(inode);
      return new Uint8Array(this.mem.buffer, addr, size);
    }
  
    hostFlush() {
      return {
          stdout:this.hostWriteBufferStdOut.flush(),
          stderr:this.hostWriteBufferStdErr.flush()
      }
    }
  
    abort() { throw new AbortError(); }
  
    host_write(fd, iovs, iovs_len, nwritten_out) {
      this.hostMem_.check();
      assert(fd <= 2);
      let size = 0;
      let str = '';
      for (let i = 0; i < iovs_len; ++i) {
        const buf = this.hostMem_.read32(iovs);
        iovs += 4;
        const len = this.hostMem_.read32(iovs);
        iovs += 4;
        str += this.hostMem_.readStr(buf, len);
        size += len;
      }
      this.hostMem_.write32(nwritten_out, size);
      if(fd == 1)
        this.hostWriteBufferStdOut.write(str);
      else if (fd == 2){
        this.hostWriteBufferStdErr.write(str);
      }
      return ESUCCESS;
    }
    host_read(fd, iovs, iovs_len, nread){
        this.hostMem_.check();
        assert(fd <= 2);
        let size = 0;
        for (let i = 0; i < iovs_len; ++i) {
          const buf = this.hostMem_.read32(iovs);
          iovs += 4;
          const len = this.hostMem_.read32(iovs);
          iovs += 4;
          const str = this.hostReadBuffer.read(len);
          const strlen = str.length;
          this.hostMem_.writeStr(buf, strlen);
          size += strlen;
        }
        this.hostMem_.write32(nread, size);
        return ESUCCESS;
  
    }
    memfs_log(buf, len) {
      this.mem.check();
      console.log(this.mem.readStr(buf, len));
    }
  
    copy_out(clang_dst, memfs_src, size) {
      this.hostMem_.check();
      const dst = new Uint8Array(this.hostMem_.buffer, clang_dst, size);
      this.mem.check();
      const src = new Uint8Array(this.mem.buffer, memfs_src, size);
      // console.log(`copy_out(${clang_dst.toString(16)}, ${memfs_src.toString(16)}, ${size})`);
      dst.set(src);
    }
    main(argv,argc){
        return 0;
    }
    copy_in(memfs_dst, clang_src, size) {
      this.mem.check();
      const dst = new Uint8Array(this.mem.buffer, memfs_dst, size);
      this.hostMem_.check();
      const src = new Uint8Array(this.hostMem_.buffer, clang_src, size);
      // console.log(`copy_in(${memfs_dst.toString(16)}, ${clang_src.toString(16)}, ${size})`);
      dst.set(src);
    }
  }
  