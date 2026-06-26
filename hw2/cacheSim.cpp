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

struct CacheLine {
    bool valid;
    unsigned long tag;
    CacheLine() : valid(false), tag(0) {}
    CacheLine(unsigned long t) : valid(true), tag(t) {}
};

// lines[0] = MRU, lines[back] = LRU
class CacheSet {
    int num_ways;
    vector<CacheLine> lines;
public:
    CacheSet(int ways) : num_ways(ways) {}

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

    // Returns true if a valid line was evicted, sets evicted_tag
    bool insert(unsigned long tag, unsigned long &evicted_tag) {
        // Reuse any invalid slot left by invalidate()
        for (int i = 0; i < (int)lines.size(); i++) {
            if (!lines[i].valid) {
                lines.erase(lines.begin() + i);
                lines.insert(lines.begin(), CacheLine(tag));
                return false;
            }
        }
        // Room left (cache not yet full)
        if ((int)lines.size() < num_ways) {
            lines.insert(lines.begin(), CacheLine(tag));
            return false;
        }
        // Evict LRU
        evicted_tag = lines.back().tag;
        lines.pop_back();
        lines.insert(lines.begin(), CacheLine(tag));
        return true;
    }

    void invalidate(unsigned long tag) {
        for (int i = 0; i < (int)lines.size(); i++) {
            if (lines[i].valid && lines[i].tag == tag)
                lines[i].valid = false;
        }
    }
};

class Cache {
    int num_sets;
    int set_bits;
    int bsize_bits;
    vector<CacheSet> sets;
public:
    Cache(int size_bits, int assoc_bits, int bsize_bits_)
        : bsize_bits(bsize_bits_) {
        set_bits  = size_bits - assoc_bits - bsize_bits_;
        num_sets  = 1 << set_bits;
        int ways  = 1 << assoc_bits;
        for (int i = 0; i < num_sets; i++)
            sets.emplace_back(ways);
    }

    void getSetTag(unsigned long addr, unsigned long &set_idx, unsigned long &tag) const {
        unsigned long block = addr >> bsize_bits;
        set_idx = block & (num_sets - 1);
        tag     = block >> set_bits;
    }

    // Reconstruct block-aligned address from set + tag
    unsigned long blockAddr(unsigned long set_idx, unsigned long tag) const {
        return ((tag << set_bits) | set_idx) << bsize_bits;
    }

    bool access(unsigned long addr) {
        unsigned long set_idx, tag;
        getSetTag(addr, set_idx, tag);
        return sets[set_idx].access(tag);
    }

    // Returns true if a block was evicted; evicted_addr is block-aligned
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

        if (L1.access(addr)) {
            continue; // L1 hit
        }

        // L1 miss
        l1_miss++;

        if (L2.access(addr)) {
            // L2 hit: bring block to L1 unless no-write-alloc write
            if (op == 'r' || WrAlloc) {
                unsigned long l1_evicted;
                if (L1.insert(addr, l1_evicted)) {
                    L2.access(l1_evicted);
                }
            }
            continue;
        }

        // L2 miss
        l2_miss++;

        if (op == 'w' && !WrAlloc) {
            continue; // write goes straight to memory, no allocation
        }

        // Fetch from memory → fill L2 then L1
        unsigned long l2_evicted;
        if (L2.insert(addr, l2_evicted)) {
            // L2 evicted a block → invalidate it in L1 (inclusive policy)
            L1.invalidate(l2_evicted);
        }

        unsigned long l1_evicted;
        if (L1.insert(addr, l1_evicted)) {
            L2.access(l1_evicted);
        }
    }

    double L1MissRate = (total  > 0) ? (double)l1_miss / total   : 0.0;
    double L2MissRate = (l1_miss > 0) ? (double)l2_miss / l1_miss : 0.0;
    double avgAccTime = L1Cyc + L1MissRate * (L2Cyc + L2MissRate * MemCyc);

    printf("L1miss=%.03f ", L1MissRate);
    printf("L2miss=%.03f ", L2MissRate);
    printf("AccTimeAvg=%.03f\n", avgAccTime);

    return 0;
}
