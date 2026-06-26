/*
 * cacheSim.cpp
 * Two-level (L1/L2) cache simulator with LRU replacement and inclusive policy.
 *
 * Parameters (all size/assoc values are log2):
 *   BSize   - block size in bytes (log2)
 *   L1/L2 Size  - total cache size in bytes (log2)
 *   L1/L2 Assoc - associativity (log2 of number of ways)
 *   WrAlloc - 1 = write-allocate, 0 = no-write-allocate
 *
 * Address breakdown: [ tag | set index | block offset ]
 *   offset bits  = BSize
 *   set bits     = Size - Assoc - BSize
 *   tag bits     = remaining upper bits
 *
 * Cache policy:
 *   - LRU replacement within each set
 *   - Inclusive L2: L2 always contains a superset of L1
 *   - On L2 eviction, the evicted block is invalidated in L1
 *   - On L1 eviction, the block is written back to L2 (updates L2 LRU)
 *   - Write miss with WrAlloc=0: write goes to memory, no cache allocation
 *
 * Output:
 *   L1miss   = L1 misses / total accesses
 *   L2miss   = L2 misses / L1 misses
 *   AvgTime  = L1Cyc + L1miss*(L2Cyc + L2miss*MemCyc)
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using std::vector;

// A single cache line: valid bit + tag
struct CacheLine {
    bool valid;
    unsigned long tag;
    CacheLine() : valid(false), tag(0) {}
    CacheLine(unsigned long t) : valid(true), tag(t) {}
};

/*
 * CacheSet: one set in the cache, holding up to num_ways lines.
 * lines[0] = MRU (most recently used), lines[back] = LRU (least recently used).
 * On every hit we move the accessed line to front; on eviction we remove from back.
 */
class CacheSet {
    int num_ways;
    vector<CacheLine> lines;
public:
    CacheSet(int ways) : num_ways(ways) {}

    // Returns true on hit and moves the line to MRU position
    bool access(unsigned long tag) {
        for (int i = 0; i < (int)lines.size(); i++) {
            if (lines[i].valid && lines[i].tag == tag) {
                CacheLine hit = lines[i];
                lines.erase(lines.begin() + i);
                lines.insert(lines.begin(), hit);
                return true;
            }
        }
        return false;
    }

    // Inserts a new tag at MRU position.
    // Reuses invalid slots (left by invalidate()) before evicting.
    // Returns true if a valid line was evicted; sets evicted_tag in that case.
    bool insert(unsigned long tag, unsigned long &evicted_tag) {
        // Reuse any invalid slot left by a previous invalidate()
        for (int i = 0; i < (int)lines.size(); i++) {
            if (!lines[i].valid) {
                lines.erase(lines.begin() + i);
                lines.insert(lines.begin(), CacheLine(tag));
                return false;
            }
        }
        // Cache not yet full — just add
        if ((int)lines.size() < num_ways) {
            lines.insert(lines.begin(), CacheLine(tag));
            return false;
        }
        // Evict LRU (back of the list)
        evicted_tag = lines.back().tag;
        lines.pop_back();
        lines.insert(lines.begin(), CacheLine(tag));
        return true;
    }

    // Marks a line invalid (used when L2 evicts a block that L1 still holds)
    void invalidate(unsigned long tag) {
        for (int i = 0; i < (int)lines.size(); i++) {
            if (lines[i].valid && lines[i].tag == tag)
                lines[i].valid = false;
        }
    }
};

/*
 * Cache: a full cache with multiple sets, each holding multiple ways.
 * Sizes and associativity are given as log2 values.
 */
class Cache {
    int num_sets;
    int set_bits;   // bits used for set index
    int bsize_bits; // bits used for block offset
    vector<CacheSet> sets;
public:
    Cache(int size_bits, int assoc_bits, int bsize_bits_)
        : bsize_bits(bsize_bits_) {
        set_bits = size_bits - assoc_bits - bsize_bits_;
        num_sets = 1 << set_bits;
        int ways = 1 << assoc_bits;
        for (int i = 0; i < num_sets; i++)
            sets.emplace_back(ways);
    }

    // Splits a byte address into set index and tag
    void getSetTag(unsigned long addr, unsigned long &set_idx, unsigned long &tag) const {
        unsigned long block = addr >> bsize_bits;
        set_idx = block & (num_sets - 1);
        tag     = block >> set_bits;
    }

    // Reconstructs the block-aligned byte address from a (set, tag) pair
    unsigned long blockAddr(unsigned long set_idx, unsigned long tag) const {
        return ((tag << set_bits) | set_idx) << bsize_bits;
    }

    // Returns true on hit (also updates LRU order)
    bool access(unsigned long addr) {
        unsigned long set_idx, tag;
        getSetTag(addr, set_idx, tag);
        return sets[set_idx].access(tag);
    }

    // Inserts block into cache. Returns true if eviction occurred;
    // evicted_addr is the block-aligned address of the evicted block.
    bool insert(unsigned long addr, unsigned long &evicted_addr) {
        unsigned long set_idx, tag;
        getSetTag(addr, set_idx, tag);
        unsigned long evicted_tag;
        if (sets[set_idx].insert(tag, evicted_tag)) {
            evicted_addr = blockAddr(set_idx, evicted_tag);
            return true;
        }
        return false;
    }

    // Invalidates the block containing addr (called when L2 evicts a block)
    void invalidate(unsigned long addr) {
        unsigned long set_idx, tag;
        getSetTag(addr, set_idx, tag);
        sets[set_idx].invalidate(tag);
    }
};

int main(int argc, char **argv) {

    if (argc < 19) {
        cerr << "Not enough arguments" << endl;
        return 0;
    }

    char* fileString = argv[1];
    ifstream file(fileString);
    string line;
    if (!file || !file.good()) {
        cerr << "File not found" << endl;
        return 0;
    }

    unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
             L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

    for (int i = 2; i < 19; i += 2) {
        string s(argv[i]);
        if      (s == "--mem-cyc")  MemCyc  = atoi(argv[i+1]);
        else if (s == "--bsize")    BSize   = atoi(argv[i+1]);
        else if (s == "--l1-size")  L1Size  = atoi(argv[i+1]);
        else if (s == "--l2-size")  L2Size  = atoi(argv[i+1]);
        else if (s == "--l1-cyc")   L1Cyc   = atoi(argv[i+1]);
        else if (s == "--l2-cyc")   L2Cyc   = atoi(argv[i+1]);
        else if (s == "--l1-assoc") L1Assoc = atoi(argv[i+1]);
        else if (s == "--l2-assoc") L2Assoc = atoi(argv[i+1]);
        else if (s == "--wr-alloc") WrAlloc = atoi(argv[i+1]);
        else { cerr << "Error in arguments" << endl; return 0; }
    }

    Cache L1(L1Size, L1Assoc, BSize);
    Cache L2(L2Size, L2Assoc, BSize);

    unsigned long total = 0, l1_miss = 0, l2_miss = 0;

    while (getline(file, line)) {
        stringstream ss(line);
        string address;
        char op = 0;
        if (!(ss >> op >> address)) continue;

        string cutAddress = address.substr(2); // strip "0x"
        unsigned long addr = strtoul(cutAddress.c_str(), NULL, 16);

        total++;

        // --- L1 lookup ---
        if (L1.access(addr)) {
            continue; // L1 hit — done
        }

        // L1 miss: forward to L2
        l1_miss++;

        // --- L2 lookup ---
        if (L2.access(addr)) {
            // L2 hit: bring block into L1 (unless no-write-alloc write miss)
            if (op == 'r' || WrAlloc) {
                unsigned long l1_evicted;
                if (L1.insert(addr, l1_evicted)) {
                    // L1 write-back: evicted block goes back to L2, updating its LRU
                    L2.access(l1_evicted);
                }
            }
            continue;
        }

        // L2 miss: must go to main memory
        l2_miss++;

        // With no-write-allocate, a write miss bypasses the cache entirely
        if (op == 'w' && !WrAlloc) {
            continue;
        }

        // Fetch block from memory → fill L2 first, then L1
        unsigned long l2_evicted;
        if (L2.insert(addr, l2_evicted)) {
            // L2 evicted a block → enforce inclusivity by invalidating it in L1
            L1.invalidate(l2_evicted);
        }

        unsigned long l1_evicted;
        if (L1.insert(addr, l1_evicted)) {
            // L1 write-back: evicted block goes back to L2, updating its LRU
            L2.access(l1_evicted);
        }
    }

    // Compute statistics
    double L1MissRate = (total   > 0) ? (double)l1_miss / total   : 0.0;
    double L2MissRate = (l1_miss > 0) ? (double)l2_miss / l1_miss : 0.0;
    double avgAccTime = L1Cyc + L1MissRate * (L2Cyc + L2MissRate * MemCyc);

    printf("L1miss=%.03f ", L1MissRate);
    printf("L2miss=%.03f ", L2MissRate);
    printf("AccTimeAvg=%.03f\n", avgAccTime);

    return 0;
}
