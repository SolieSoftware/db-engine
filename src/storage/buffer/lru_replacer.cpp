#include "storage/buffer/lru_replacer.h"


namespace dbengine {
    LRUReplacer::LRUReplacer(size_t num_frames): max_size_(num_frames) {
    }

    bool LRUReplacer::Victim(frame_id_t *frame_id_) {
        if (lru_list_.empty()) {
            return false;
        }
        frame_id_t lru_frame = lru_list_.back();
        *frame_id_ = lru_frame;
        lru_list_.pop_back();
        lru_map_.erase(lru_frame);
        return true;
    }

    void LRUReplacer::Pin(frame_id_t frame_id) {
        auto it = lru_map_.find(frame_id);
        if (it != lru_map_.end()) {
            lru_list_.erase(it->second);
            lru_map_.erase(it);
        }
    }

    void LRUReplacer::Unpin(frame_id_t frame_id) {
        if (lru_map_.count(frame_id)) {
            lru_list_.erase(lru_map_[frame_id]);
            lru_map_.erase(frame_id);
        }
        lru_list_.push_front(frame_id);
        lru_map_[frame_id] = lru_list_.begin();
    }

    size_t LRUReplacer::Size() {
        return lru_list_.size();
    }

    
}