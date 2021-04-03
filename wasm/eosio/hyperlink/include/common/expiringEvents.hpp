      events_t _events;
      void Flush(uint64_t depth) {
         uint64_t removed = 0;
         auto config = _config_t.get_or_default();
         auto oldEvent = _events.begin();
         while(oldEvent != _events.end() && removed <= depth) {
            if(oldEvent->timestamp + config.eventTTL > current_time()) return;
            _events.erase(oldEvent);
            ++removed;
            oldEvent = _events.begin();
         }
      }
