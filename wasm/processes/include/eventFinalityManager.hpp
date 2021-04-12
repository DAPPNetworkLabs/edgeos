#include <list>
#include <chrono>
using namespace std::chrono;
template <class T>
struct queue_entry{
    T obj;
    system_clock::time_point block_time;
    std::string id;
    long blockNumber;
};
template <class T>
class  EventFinalityManager{

    public:
    int SecondsToFinality;
    EventFinalityManager(int secondsToFinality){
        this->SecondsToFinality = secondsToFinality;
    }

    std::list<queue_entry<T>> queue;
    void addEvent(long blockNumber, bool removed, T value, std::string id){
        if(!removed){
            // cancel event
            queue_entry<T> entry;
            // auto ptr = new T(value);
            entry.obj = value;
            entry.block_time = system_clock::now();
            entry.id = id;
            entry.blockNumber = blockNumber;
            this->queue.push_back(entry);
        }
        else{
            auto it = std::find_if(
                this->queue.begin(), 
                this->queue.end(), 
                [&id](const queue_entry<T>& x) { return x.id == id;});
            if (it != this->queue.end()) {
                this->queue.erase(it);
            }                        
        }       
        this->scheudle();
 
    }
    void popHandleEvents(){
        auto event = this->popEvent();
        while(event != NULL){            
            // handle event
            this->handleEvent(event->obj, event->id);
            event = this->popEvent();
        }
        if(!this->queueEmpty()){
            this->scheudle();
        }

    }
    bool queueEmpty(){
        return this->queue.empty();        
    }
    queue_entry<T>* popEvent(){
        if(!this->queue.empty()){
            duration<int> finality_duration (this->SecondsToFinality);
            if((this->queue.front().block_time+ finality_duration) < system_clock::now() ){
               queue_entry<T> iter = this->queue.front();
               this->queue.pop_front();        
               return new queue_entry<T>(iter);
            }
        }
        // check if oldest message is older then secondsToFinality
        return NULL;
    }
    
    virtual void scheudle() = 0;
    
    virtual void handleEvent(T event, std::string id) = 0;
};

