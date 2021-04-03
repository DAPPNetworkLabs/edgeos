var level = require('level')

// 1) Create our database, supply location and options.
//    This will create or open the underlying store.
// var db = level('my-db')

// // 2) Put a key & value
// db.put('name', 'Level', function (err) {
//   if (err) return console.log('Ooops!', err) // some kind of I/O error

//   // 3) Fetch by key
//   db.get('name', function (err, value) {
//     if (err) return console.log('Ooops!', err) // likely the key was not found

//     // Ta da!
//     console.log('name=' + value)
//   })

export const leveldbextension = {
    "leveldb_get":{
        args:["string"],
        callbackType: "string",
        fn:(Str, edgeOSKernel)=>{
            return "tests syscall::" + Str;
        }
    },
    "leveldb_clear":{
        args:["string"],
        callbackType: "string",
        fn:(Str, edgeOSKernel)=>{
            return "tests syscall::" + Str;
        }
    },
    "leveldb_import":{
        args:["string"],
        callbackType: "string",
        fn:(Str, edgeOSKernel)=>{
            return "tests syscall::" + Str;
        }
    },
    "leveldb_export":{
        args:["string"],
        callbackType: "string",
        fn:(Str, edgeOSKernel)=>{
            return "tests syscall::" + Str;
        }
    },
    "leveldb_create_key_stream":{
        args:["string"],
        callbackType: "string",
        fn:(Str, edgeOSKernel)=>{
            return "tests syscall::" + Str;
        }
    },
    "leveldb_put":{
        args:["string"],
        callbackType: "string",
        kernelCallback: true,
        fn:async (tst1, edgeOSKernel)=>{
            await delay(1000);
            // do something async
            return "did async stuff:" + tst1;
        }
    },

}