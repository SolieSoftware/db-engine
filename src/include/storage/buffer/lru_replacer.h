#pragma once

#include <list>
#include <unordered_map>
#include <cstdint>

namespace dbengine {
    using frame_id_t = int32_t; // Type alias for frame IDs

    /**
    * LRUReplacer implements the Least Recently Used replacement policy
    *
    * The replacer tracks UNPINNSED frames and evicts the least recently used one.
    * When a frame is pinned (being used), it's removed from the replacer. 
    */
    class LRUReplacer {
        public:
        /**
        * Create a new LRUReplacer
        * @param num_rames the maximum number of frames the replacer can track
        */
        explicit LRUReplacer(size_t num_frames);

        /** 
        * Destroys the LRUReplacer
        */
        ~LRUReplacer() = default;

        /**
        * Remove the least recently used frame.
        * @param[out] frame_id the ID of the frame that was removed
        * @return true if a frame was removed, false if no frames were available
         */
        bool Victim(frame_id_t *frame_id);

        /**
        * Pin a frame, indicating it's being used and should not be evicted.
        * Removes the frame from the replacer.
        * @param frame_id the ID of the frame to pin
         */
        void Pin(frame_id_t frame_id);
        
        /** 
        * Unpin a frame, making it available fro eviction.
        * Adds the frame to the replacer as most recetnly used.
        * @param frame_id the frame to unpin 
        */
        void Unpin(frame_id_t frame_id);

        /**
        * @return the number of frames that are currently unpinned (evictable) 
        */
        size_t Size();

        private:
        // TODO: Add your data members here
        // Hint: You need a list and a map!
        std::list<frame_id_t> lru_list_;
        std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> lru_map_;
        size_t max_size_;
    };
    } // namespace dbengine