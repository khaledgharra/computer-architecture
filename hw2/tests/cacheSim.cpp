/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstddef>
#include <string>
#include <math.h>

#define WRITE	'w'
#define READ	'r'

#define BY_LRU	true
#define BY_TAG  false


using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using std::vector;

class Block {
public:
	vector<char> data; //may not need all this size
	unsigned Bsize;
	bool valid;
	bool is_dirty;

	Block()
	{
		this->valid = false;
		this->is_dirty = false;		
	}

	Block(unsigned BlockSize)
	{
		this->valid = false;
		this->is_dirty = false;
		this->Bsize = BlockSize;
		this->data = vector<char>(Bsize, 0); // Bsized vector of bytes
		//cout << "block not basic ctor" << endl;

	}
};

class Way {
public:
	unsigned tag;
	unsigned lru_counter;
	//unsigned long int address;
	//std::vector<Block> Blocks;
	Block block;


	Way()
	{
		this->tag = 0;
		this->lru_counter = 0;
		//cout << "way basic ctor" << endl;

	}

	Way(unsigned tag, unsigned BlockSize)
	{
		this->tag = tag;
		this->lru_counter = 0;
		this->block = Block(BlockSize);
		//cout << "way not basic ctor" << endl;

	}
};


class Set {
public:
	vector<Way> ways;

	Set()
	{
		//cout << "set basic ctor" << endl;

	}

	// ways will be the actual size
	Set(unsigned ways_num, unsigned Bsize)
	{
		//this->ways = vector<Way>(ways_num);
		for (int i = 0; i < ways_num; i++)
		{
			this->ways.push_back(Way(0, Bsize)); // tag is set to 0 
		}
		//cout << "set nnot basic ctor" << endl;

	}
};


class Cache {
public:
	unsigned BlockSize;	// the size of each block in the cache
	unsigned size;		// the size of the entire cache in bits
	unsigned ways_num;	// pow(2,Assoc) is the number of ways
	unsigned Cycles;	// number of cycles required to access the cache
	unsigned WrAlloc;	// whether uses Write allocate or not.
	// a vector which holds all the sets. each set holds ways, 
	// each way holds it's tag and it's block. each block holds valid bit and data.
	vector<Set> sets;	

	int miss_count;
	int hit_count;

	unsigned offset_size_bits;
	unsigned tag_size;
	unsigned set_field_size_bits;


	Cache()
	{
		this->BlockSize = 0;
		this->size = 0;
		this->ways_num = 0;
		this->Cycles = 0;
		this->WrAlloc = 0;
		//this->vector<Set> sets = vector<Set>(1,0);
		this->miss_count = 0;
		this->hit_count = 0;
		this->offset_size_bits = 0;
		this->tag_size = 0;
		this->set_field_size_bits = 0;
	}

	Cache(unsigned BlockSizeBits, unsigned Size, unsigned Assoc, 
			unsigned Cycles, unsigned WrAlloc)
	{
		this->ways_num = 1 << Assoc;
		this->Cycles =	Cycles;
		this->WrAlloc =	WrAlloc;
		this->BlockSize = 1 << BlockSizeBits; // 
		this->size = pow(2, BlockSizeBits);

		this->miss_count = 0;
		this->hit_count = 0;

		this->offset_size_bits = BlockSizeBits;

		this->set_field_size_bits = Size - Assoc - BlockSizeBits; // log(size/(ways*blocks))
		this->tag_size = 32 - offset_size_bits - set_field_size_bits;
		
		// number of the sets - the total size divided by the size of each set.
		unsigned num_of_sets = (unsigned)pow(2, set_field_size_bits);
		//this->sets = vector<Set>(num_of_sets);
		for (int i = 0; i < num_of_sets; i++)
		{
			//this->sets[i] = Set(this->ways_num, this->BlockSize);
			this->sets.push_back(Set(this->ways_num, this->BlockSize));
		}
		//cout << "--------------------------------------------------: " << offset_size_bits << endl;

		//cout << "cache not basic ctor" << endl;

	}

	void parse_address(unsigned long int address, unsigned &tag, 
					unsigned &set, unsigned &offset)
	{
		//cout << "start parse" << endl;
		//cout << this->sets[1].ways[0].tag << endl;
		//cout << "start parse address " << address << endl;
		// 
		// tag is the most left group of bits in the address
		tag = (address >> offset_size_bits) >> set_field_size_bits;

		// set is the middle group of bits. its size is the sum of 
		// the block size and the amount of ways
		set = (address >> offset_size_bits) % (1 << this->set_field_size_bits);

		// the most right group of bits. its size is given.
		offset = address % (1 << offset_size_bits);

	}

	Way* find_way(unsigned long int address)
	{
		unsigned tag = 0;
		unsigned set = 0;
		unsigned offset = 0;
		Way* curr_way = NULL;

		parse_address(address, tag, set, offset);
		
		// search in all this set's ways for the requested tag. 
		for (int i = 0; i < ways_num; i++)
		{
			//cout << "inside for find_way " << i << endl;
			//cout << this->sets.size() << " " << set << " " << tag << " " << offset << endl;
			//cout << &((this->sets[set]).ways[i]) << endl;
			curr_way = &((this->sets[set]).ways[i]);
			// if exists and valid return true and ++ the hit counter
			//cout << "before if find_way " << i << endl;

			if (tag == curr_way->tag)
			{
				if (curr_way->block.valid)
				{
					//hit_count++; // TODO: find good place to update counter
					// write / read
					//return true;
					return curr_way;
				}
				else
				{
					break;
				}
			}
		}
		// if doesnt exist or not valid return false and ++ the miss counter
		//miss_count++; // TODO: find good place to update counter
		//return false;
		return NULL;
	}

	Way* find_oldest_way(unsigned long int address)
	{
		unsigned tag = 0;
		unsigned set = 0;
		unsigned offset = 0;
		Way* curr_way = NULL;

		parse_address(address, tag, set, offset);

		for (int i = 0; i < ways_num; i++)
		{
			curr_way = &((this->sets[set]).ways[i]);
			if (0 == curr_way->lru_counter)
			{
				return curr_way;
			}
		}

		return NULL;
	}

	void invalidate_block(unsigned long int address)
	{
		// find_way returns NULL if way doesnt exist or not valid, in both cases 
		// we wont need to do way.block.valid = false beause valid is already false.
		Way* curr_way = find_way(address);
		if (curr_way)
		{
			curr_way->block.valid = false;
			//curr_way->tag = 0;
			curr_way->lru_counter = 0;
			
		}
		return;
	}

	void write_to_block(unsigned long int address, unsigned long int& removed_address,
						Way &way_out, bool &should_write_to_lower)
	{
		unsigned tag = 0;
		unsigned set = 0;
		unsigned offset = 0;
		parse_address(address, tag, set, offset);
		//cout << "after parse in write_to_block" << endl;

		bool exists = false;
		//int last_idx = 0;
		Way* way_to_remove = NULL;
		
		for (int i = 0; i < ways_num; i++)
		{
			//cout << "inside for in write_to_block "<<i << endl;
			//cout << "set: " << set << "\n   sets size: " << sets.size() << "\n  ways size: " << ways_num << endl;

			// check if tag is here and valid
			if (tag == sets[set].ways[i].tag && sets[set].ways[i].block.valid)
			{
				//cout << "exists!" << endl;
				sets[set].ways[i].block.is_dirty = true;
				exists = true;
				//		update the data (fake)
			}
			else
			{
				//cout << "doesnt exists :(" << endl;
				//		find the least used way
				if (0 == sets[set].ways[i].lru_counter)
				{
					//last_idx = i;
					//cout << "inside if doesnt exists :(" << endl;
					way_to_remove = &(sets[set].ways[i]);
				}
			}
		}

		if (!exists) // if tag not here 
		{
			// removed_address = tmp.tag << (offset_size_bits + set_field_size_bits) | (set) << offset_size_bits;
			removed_address = ((way_to_remove->tag << set_field_size_bits) + set) << offset_size_bits;
			should_write_to_lower = way_to_remove->block.is_dirty && way_to_remove->block.valid;
			way_to_remove->block.is_dirty = false;
			//		return it's data to way_out and set the flag
			way_out = *way_to_remove;

			// overwrite the current way
			way_to_remove->block.valid = true;
			// in real life, also update the data 
			way_to_remove->block.is_dirty = true;
			way_to_remove->tag = tag;
			way_to_remove->lru_counter = 0; // important, should be zero
		}
		
		
		update_lru(address);
		return;
	}

	// after operation, need to update the lru counters.
	void update_lru(unsigned long int address) {

		unsigned tag = 0;
		unsigned set = 0;
		unsigned offset = 0;
		Way* curr_way = NULL;
		parse_address(address, tag, set, offset);

		for (int i = 0; i < ways_num; i++)
		{
			curr_way = &(sets[set].ways[i]);
			if (tag == curr_way->tag)
			{
				int x = curr_way->lru_counter;
				curr_way->lru_counter = ways_num - 1;
				for (int j = 0; j < ways_num; j++)
				{
					if ((j != i) && sets[set].ways[j].lru_counter > x)
					{
						sets[set].ways[j].lru_counter--;
						//cout << "ways_num: " << ways_num << "\n  way: j" << j << "\n  updated counter:" << sets[set].ways[j].lru_counter << endl;
					}
				}
				break;
			}
		}
	}


	/*void basic_operation(unsigned long int address, unsigned long int& removed_address,
		Way& way_out, bool& should_write_to_lower)
	{

	}*/

	void read_block(unsigned long int address, bool dirty, unsigned long int& removed_address,
		Way& way_out, bool& should_write_to_lower)
	{
		unsigned tag = 0;
		unsigned set = 0;
		unsigned offset = 0;
		parse_address(address, tag, set, offset);

		bool exists = false;
		Way* way_to_remove = NULL;

		for (int i = 0; i < ways_num; i++)
		{
			// check if tag is here and valid
			if (tag == sets[set].ways[i].tag && sets[set].ways[i].block.valid)
			{
				exists = true;
				//		update the data (fake)
			}
			else
			{
				//		find the least used way
				if (0 == sets[set].ways[i].lru_counter)
				{
					way_to_remove = &(sets[set].ways[i]);
				}
			}
		}

		if (!exists) // if tag not here 
		{
			// removed_address = tmp.tag << (offset_size_bits + set_field_size_bits) | (set) << offset_size_bits;
			removed_address = ((way_to_remove->tag << set_field_size_bits) + set) << offset_size_bits;
			should_write_to_lower = way_to_remove->block.is_dirty && way_to_remove->block.valid;
			way_to_remove->block.is_dirty = false;
			//		return it's data to way_out and set the flag
			way_out = *way_to_remove;

			// overwrite the current way
			way_to_remove->block.valid = true;
			// in real life, also update the data 
			way_to_remove->tag = tag;
			way_to_remove->block.is_dirty = dirty;
			way_to_remove->lru_counter = 0; // doesnt matter!
		}


		update_lru(address);
		return;
	}

	double calc_miss_rate()
	{
		return ( (double)this->miss_count / ( (double)this->miss_count + (double)this->hit_count ) );
	}


	void add_data_by_lru(unsigned long int address, bool dirty, unsigned long int& removed_address, bool &should_add_to_lower, bool & removed_block)
	{
		unsigned tag = 0;
		unsigned set = 0;
		unsigned offset = 0;
		parse_address(address, tag, set, offset);

		// check if already exists
		//our problem is that this searches by LRU not by tag, so if we delete from L1 we don't need to use this. we only need to find by tag and update the LRU
		Way* curr_way = find_oldest_way(address);
		should_add_to_lower = false;
		removed_block = false;

		if (NULL != curr_way)
		{
			should_add_to_lower = curr_way->block.valid && curr_way->block.is_dirty;
			removed_address = ((curr_way->tag << set_field_size_bits) + set) << offset_size_bits;
			removed_block = curr_way->block.valid;
			
			curr_way->block.valid = true;
			curr_way->tag = tag;
			curr_way->block.is_dirty = dirty;
			curr_way->lru_counter = 0; 

		}
		else
		{
			cout << "shit, we didnt find the oldest way with lru == 0!" << endl;
		}

		return;
	}

	void touch_way(unsigned long int address, bool is_dirty)
	{
		Way* curr_way = this->find_way(address);
		curr_way->block.is_dirty = is_dirty;
		//curr_way->lru_counter = 0;
		//this->update_lru(address);
		return;
	}
	
};



class CacheSim {
public:
	unsigned MemCyc = 0;
	unsigned BSize = 0;
	unsigned L1Size = 0;
	unsigned L2Size = 0;
	unsigned L1Assoc = 0;
	unsigned L2Assoc = 0;
	unsigned L1Cyc = 0;
	unsigned L2Cyc = 0;
	unsigned WrAlloc = 0;

	unsigned mem_total = 0;
	Cache* L1Cache = NULL;
	Cache* L2Cache = NULL;

	//unsigned L1HitsNum;
	//unsigned MissRate;

	CacheSim()
	{
		this->MemCyc = 0;
		this->BSize = 0;
		this->L1Size = 0;
		this->L2Size = 0;
		this->L1Assoc = 0;
		this->L2Assoc = 0;
		this->L1Cyc = 0;
		this->L2Cyc = 0;
		this->WrAlloc = 0;

		this->mem_total = 0;
		this->L1Cache = NULL;
		this->L2Cache = NULL;
		//cout << "call me!!!!!!!!!!!!" << endl;
	}


	CacheSim(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size,
		unsigned L1Assoc, unsigned L2Assoc, unsigned L1Cyc, unsigned L2Cyc, 
		unsigned WrAlloc) 
	{
		this->MemCyc = MemCyc;
		this->BSize = BSize;
		this->L1Size = L1Size;
		this->L2Size = L2Size;
		this->L1Assoc = L1Assoc;
		this->L2Assoc = L2Assoc;
		this->L1Cyc = L1Cyc;
		this->L2Cyc = L2Cyc;
		this->WrAlloc = WrAlloc;
		
		this->mem_total = 0;
		this->L1Cache = new Cache(BSize, L1Size, L1Assoc, L1Cyc, WrAlloc);
		this->L2Cache = new Cache(BSize, L2Size, L2Assoc, L2Cyc, WrAlloc);
		//cout << "sim not basic ctor " << L1Cache->offset_size_bits << " "  << endl;

	}

	~CacheSim()
	{
		delete L1Cache;
		delete L2Cache;
	}


	void write_op(unsigned long int address)
	{
		Way* curr_way = NULL;
		unsigned long int removed_address = 0;
		unsigned long int blank = 0;
		bool should_add_to_lower = false;
		bool removed_block = false;
		unsigned tag = 0;
		unsigned set = 0;
		unsigned offset = 0;
		this->L1Cache->parse_address(address, tag, set, offset);

		// snoop if in L1
		curr_way = this->L1Cache->find_way(address);
		// if here - done! 
		if (NULL != curr_way)
		{
			// update counters - LRU and hit and miss
			this->L1Cache->hit_count++;
			this->L1Cache->update_lru(address);
			curr_way->block.is_dirty = true;
			return;
		}

		this->L1Cache->miss_count++;
		curr_way = this->L2Cache->find_way(address);
		
		// if in L2
		if (NULL != curr_way)
		{
			// update LRU of L2
			this->L2Cache->hit_count++;
			//this->L2Cache->update_lru(address);
			//curr_way->block.is_dirty = true;

			if (this->WrAlloc)
			{
				// add to L1 by LRU
				// find the oldest way, and replce it with the new one
				this->L1Cache->add_data_by_lru(address, true, removed_address, should_add_to_lower, removed_block); // TODO: check if should be dirty
				this->L1Cache->update_lru(address);
				// update L1 LRU

				if (should_add_to_lower) // if removed from L1 and dirty
				{
					this->L2Cache->touch_way(address, true); // true because if should_add_to_lower is true then is_dirty is true
					// add to L2
					//this->L2Cache->add_data_by_lru(removed_address, true, blank, should_add_to_lower, removed_block);
					//this->L2Cache->update_lru(removed_address); // TODO: should we do this????????
					//if (removed_block)
					//{
					//	this->L1Cache->invalidate_block(removed_address);
					//}
				}
			}
			else
			{
				this->L2Cache->update_lru(address);
				curr_way->block.is_dirty = true;
			}
			return;
		}

		// if not in L2 
		this->L2Cache->miss_count++;
		this->mem_total++;

		if (this->WrAlloc)
		{
			// add to L2
			this->L2Cache->add_data_by_lru(address, true, removed_address, should_add_to_lower, removed_block);
			// update L2 LRU, ///dont/// update hit/miss counters
			this->L2Cache->update_lru(address);
			if (removed_block)
			{
				this->L1Cache->invalidate_block(removed_address);
			}

			// add to L1
			this->L1Cache->add_data_by_lru(address, true, removed_address, should_add_to_lower, removed_block); // TODO: check if should be dirty
			// update L1 LRU, ///dont/// update hit/miss counters 
			this->L1Cache->update_lru(address);

			if (should_add_to_lower) // if removed from L1 and dirty
			{
				this->L2Cache->touch_way(address, true); // true because if should_add_to_lower is true then is_dirty is true
				// add to L2
				//this->L2Cache->add_data_by_lru(removed_address, true, blank, should_add_to_lower, removed_block);
				//this->L2Cache->update_lru(removed_address); // TODO: should we do this????????
				//if (removed_block)
				//{
				//	this->L1Cache->invalidate_block(removed_address);
				//}
			}
		}


		return;
	}


	void read_op(unsigned long int address)
	{
		Way* curr_way = NULL;
		unsigned long int removed_address = 0;
		unsigned long int blank = 0;
		bool should_add_to_lower = false;
		bool removed_block = false;
		//unsigned tag = 0;
		//unsigned set = 0;
		//unsigned offset = 0;
		//this->L1Cache->parse_address(address, tag, set, offset);

		// snoop if in L1
		curr_way = this->L1Cache->find_way(address);
			// if here - done! 
		if (NULL != curr_way)
		{
			// update counters - LRU and hit and miss
			this->L1Cache->hit_count++;
			this->L1Cache->update_lru(address);
			return;
		}
			
		// if not in L1 - snoop if in L2
		this->L1Cache->miss_count++;
		curr_way = this->L2Cache->find_way(address);
			// if in L2 
		if (NULL != curr_way)
		{
			// update LRU of L2
			this->L2Cache->hit_count++;
			this->L2Cache->update_lru(address);
			// add to L1 by LRU
				// find the oldest way, and replce it with the new one
			this->L1Cache->add_data_by_lru(address, false, removed_address, should_add_to_lower, removed_block);
			this->L1Cache->update_lru(address);
			// update L1 LRU

			if (should_add_to_lower) // if removed from L1 and dirty
			{
				this->L2Cache->touch_way(address, true); // true because if should_add_to_lower is true then is_dirty is true
				// add to L2
				//this->L2Cache->add_data_by_lru(removed_address, true, blank, should_add_to_lower, removed_block);
				//this->L2Cache->update_lru(removed_address); // TODO: should we do this????????
				//if (removed_block)
				//{
				//	this->L1Cache->invalidate_block(removed_address);
				//}
			}

			return;
		}
		
		// if not in L2 
		this->L2Cache->miss_count++;
		this->mem_total++;

			// add to L2
		this->L2Cache->add_data_by_lru(address, false, removed_address, should_add_to_lower, removed_block);
			// update L2 LRU, ///dont/// update hit/miss counters
		this->L2Cache->update_lru(address);
		if (removed_block)
		{
			this->L1Cache->invalidate_block(removed_address);
		}

			// add to L1
		this->L1Cache->add_data_by_lru(address, false, removed_address, should_add_to_lower, removed_block);
		// update L1 LRU, ///dont/// update hit/miss counters 
		this->L1Cache->update_lru(address);

		if (should_add_to_lower) // if removed from L1 and dirty
		{
			this->L2Cache->touch_way(address, true); // true because if should_add_to_lower is true then is_dirty is true
			// add to L2
			//this->L2Cache->add_data_by_lru(removed_address, true, blank, should_add_to_lower, removed_block);
			//this->L2Cache->update_lru(removed_address); // TODO: should we do this????????
			//if (removed_block)
			//{
			//	this->L1Cache->invalidate_block(removed_address);
			//}
		}
				
		return;
	}


	void perform_operation(char operation, unsigned long int address)
	{
		//cout << " before operation" << endl;
		//cout << L1Cache->offset_size_bits << endl;
		//Way* curr_way = this->L1Cache->find_way(address);
		//cout << "outside find_way "  << endl;

		//unsigned long int removed_address = 0;
		//Way way_out; 
		//bool should_write_to_lower = false;
		
		if (WRITE == operation)
		{
			this->write_op(address);
		}
		else if (READ == operation)
		{
			this->read_op(address);
		}
	}

	double calc_avg_access_time()
	{
		double total_L1 = (double)L1Cache->miss_count + L1Cache->hit_count;
		double total_L2 = (double)L2Cache->miss_count + L2Cache->hit_count;
		return (total_L1 * L1Cyc + total_L2 * L2Cyc + mem_total * this->MemCyc) / (total_L1);
	}

};


int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}
	
	CacheSim mycache = CacheSim(MemCyc, BSize, L1Size, L2Size, 
									L1Assoc, L2Assoc, L1Cyc, L2Cyc, WrAlloc);



	// for each line in trace file, get the line, 
	// and later perform the simulation for the specific line
	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}
		string cutAddress = address.substr(2); // Removing the "0x" part of the address
		unsigned long int address_int = 0;
		address_int = strtoul(cutAddress.c_str(), NULL, 16);
		//cout << "num: " << address_int << "\ncut address: " << cutAddress << "\naddress: " << address << endl;

		mycache.perform_operation(operation, address_int);

		

		//// DEBUG - remove this line
		//cout << "operation: " << operation;
		//// DEBUG - remove this line
		//cout << ", address (hex)" << cutAddress;
		//// DEBUG - remove this line
		//cout << " (dec) " << address_int << endl;
	}

	double L1MissRate = mycache.L1Cache->calc_miss_rate();
	double L2MissRate = mycache.L2Cache->calc_miss_rate();
	double avgAccTime = mycache.calc_avg_access_time();

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	//delete(mycache);
	return 0;
}
