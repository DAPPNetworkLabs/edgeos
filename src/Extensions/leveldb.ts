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

export const leveldbextension = { // per processid db
    "leveldb_get":async({json, pid, cb, edgeOSKernel})=>{
        const {key,db} = json;
        var database = level(`db/${pid}.${db}`);
        return await database.get(key);
    },   
    "leveldb_put":async({json, pid, cb, edgeOSKernel})=>{
        const {key,db, value} = json;
        var database = level(`db/${pid}.${db}`);
        return await database.put(key, value);
    },   
    "leveldb_clear":async({json, pid, cb, edgeOSKernel})=>{
        const {db} = json;
        var database = level(`db/${pid}.${db}`);
        return await database.clear();
    },   
    "leveldb_create_read_stream":async({json, pid, cb, edgeOSKernel})=>{
        const {db, options} = json;
        var database = level(`db/${pid}.${db}`);
        database.createReadStream(options)
            .on('data', function (data) {
                // invoke cb
                edgeOSKernel.workers[pid].call('callback',[{
                    cb:cb,
                    cbcb:json.onValue,
                    result: data,
                    request:json
                }]);
            })
            .on('error', function (err) {
                
            })
            .on('close', function () {
                
            })
            .on('end', function () {
                
            });
        
    },   
    

}