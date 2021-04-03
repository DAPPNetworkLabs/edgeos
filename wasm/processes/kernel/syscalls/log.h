EDGEOS_SYSCALL(log)

void elog(std::string str){
   json j2 = {
        {"message", str},
   };

//    edgeos_log(&j2, [](json * res){
//        // done
//    });
    edgeos_log(&j2, [](json * jsonRes){
       // done
   });
}