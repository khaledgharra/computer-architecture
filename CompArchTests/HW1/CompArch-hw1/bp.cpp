/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <math.h>

// enum representing the state of the predictor
enum FSMstate { SNT = 0, WNT, WT, ST };


class Row 
{
public:
	unsigned tag;			// the id of the current branch
	unsigned target;		// dst of the current branch
	bool valid;				// if the data represents real info or just initialized to 
	uint8_t local_history;	// if using the local history
	std::vector<FSMstate> local_state; // holds all the State Machines of the branch when not sharing SMs. 
};


// the predictor's class
class branchPredictor {

public:
	unsigned	btbSize;
	unsigned	historySize;
	unsigned	tagSize;
	unsigned	fsmState;
	bool		isGlobalHist;
	bool		isGlobalTable; 
	int			Shared;

	// for stats
	unsigned flushed_count;
	unsigned branch_count;
	unsigned theoretical_predictor_size;

	uint8_t history_mask;
	
	FSMstate default_state;

	uint8_t global_history;
	std::vector<FSMstate> global_predictors;

	std::vector<Row> btb;


	// returns a pointer to the hisotry of a requested entry in the table, depends on isGlobalHist flag of predictor.
	uint8_t* get_history(int entry) 
	{
		if (isGlobalHist) 
		{
			return & global_history;
		}
		else 
		{
			return & ( btb[entry].local_history );
		}
	}

	// resets the history matching to the entry of the table to 0 if using local history.
	void init_history(int entry)
	{
		if (!isGlobalHist)
		{
			btb[entry].local_history = 0;
		}
		return;
	}

	// updates the hisotry of a requested entry, depends on isGlobalHist flag of predictor, and if was taken or not.
	// if should init history before updating, use should_init_history
	void update_history(int entry, bool taken, bool should_init_history) 
	{
		if (isGlobalHist) 
		{
			// move bits of local history left by 1 bit, add to it the taken bit. 
			// then, if history size is less then 8 bits, we will have to get rid of the MSBs, so 
			// we use & with 1'b1*(historySize) to take onle the lower and relevant bits.
			global_history = ( (global_history << 1) + taken ) & ( history_mask );
		}
		else
		{
			/*if (should_init_history) {
				btb[entry].local_history = 0;
			}*/
			// move bits of local history left by 1 bit, add to it the taken bit. 
			// then, if history size is less then 8 bits, we will have to get rid of the MSBs, so 
			// we use & with 1'b1*(historySize) to take onle the lower and relevant bits.
			btb[entry].local_history = ((btb[entry].local_history << 1) + taken) & ( history_mask );
		}
		return;
	}

	// returnes the bytes to be xored with the history bytes, according to predictor.Shared value.
	uint8_t get_shared_bytes(uint32_t pc)
	{
		switch (Shared)
		{
			case (1): // if using_share_lsb
				return (pc >> 2) % ( (int) pow(2, historySize) );
			case (2): // if using_share_mid
				return (pc >> 16) % ( (int) pow(2, historySize) );
			default:  // if not_using_share 
				return 0;
		}
		 
	}


	// returns the relevant state machine depending of the entry, 
	// depending on the usage of global macihnes and if using share.
	FSMstate* get_state(uint32_t pc, int entry)
	{
		if (isGlobalTable)
		{
			uint8_t xor_for_shared = get_shared_bytes(pc);
			return & (global_predictors[ ( * get_history(entry) ) ^ xor_for_shared ]);
		}
		else
		{
			return & (btb[entry].local_state[*get_history(entry)]) ;
		}
	}

	// updates the relevant state machine of the entry, 
	// depends on it's history and if branch was taken or not.
	void update_state(uint32_t pc, int entry, bool taken)
	{
		FSMstate* current_state = get_state(pc, entry);
		if (taken)
		{
			switch (*current_state)
			{
			case (SNT):
				*current_state = WNT;
				break;
			case (WNT):
				*current_state = WT;
				break;
			case (WT):
				*current_state = ST;
				break;
			default:
				*current_state = ST;
				break;
			}

		}
		else
		{
			switch (*current_state)
			{
			
			case (ST):
				*current_state = WT;
				break;
			case (WNT):
				*current_state = SNT;
				break;
			case (WT):
				*current_state = WNT;
				break;
			default:
				*current_state = SNT;
				break;
			}
		}
		return;
	}

	
};

branchPredictor * predictor;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, 
			unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared)
{
	// create new predictor
	predictor = new branchPredictor;

	// put all the relevant parameters in place:
	predictor->btbSize = btbSize;
	predictor->historySize = historySize;
	predictor->tagSize = tagSize;
	predictor->isGlobalHist = isGlobalHist;
	predictor->isGlobalTable = isGlobalTable;
	predictor->Shared = Shared;

	predictor->branch_count =	0;
	predictor->flushed_count = 0;
	
	if (isGlobalHist)
	{
		if (isGlobalTable)
		{
			predictor->theoretical_predictor_size = btbSize * (1 + 30 + tagSize) + (int) pow(2, historySize) * 2 + historySize;
		}
		else
		{
			predictor->theoretical_predictor_size = btbSize * (1 + 30 + tagSize + (int) pow(2, historySize) * 2) + historySize;
		}
	}
	else
	{
		if (isGlobalTable)
		{
			predictor->theoretical_predictor_size = btbSize * (1 + 30 + tagSize + historySize) + (int)pow(2, historySize) * 2;

		}
		else
		{
			predictor->theoretical_predictor_size = btbSize * (1 + 30 + tagSize + historySize + (int)pow(2, historySize) * 2);
		}
	}

	predictor->history_mask = (uint8_t)(pow(2, historySize) - 1);

	predictor->btb = std::vector<Row>(btbSize);

	switch (fsmState)
	{
	case (0):
		predictor->default_state = SNT;
		break;
	case (1):
		predictor->default_state = WNT;
		break;
	case (2):
		predictor->default_state = WT;
		break;
	default:
		predictor->default_state = ST;
		break;
	}

	for (unsigned int i = 0; i < btbSize; i++)
	{
		if (tagSize)
		{
			predictor->btb[i].tag = 0;
		}
		predictor->btb[i].target = 0;
		predictor->btb[i].valid = false;
		predictor->btb[i].local_history = 0;
		if (!isGlobalTable)
		{
			predictor->btb[i].local_state = std::vector<FSMstate>((int)pow(2, historySize), predictor->default_state);
		}
	}
	if (isGlobalTable)
	{
		predictor->global_predictors = std::vector<FSMstate>((int)pow(2, historySize), predictor->default_state);
	}
	
	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst)
{
	uint32_t pc_tag = ((pc >> 2) / predictor->btbSize) % (int)pow(2, predictor->tagSize);
	uint32_t entry = (pc >> 2) % predictor->btbSize;

	// if found pc in table
	//if (predictor->btb[entry].tag == pc_tag)
	if (predictor->btb[entry].valid && (predictor->btb[entry].tag == pc_tag) )
	{
		if ( (*(predictor->get_state(pc, entry)) == ST) || (*(predictor->get_state(pc, entry)) == WT) ) 
		{
			*dst = predictor->btb[entry].target;
			return true;
		}
	}
	
	*dst = pc + 4;
	
	
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	uint32_t pc_tag = ( ( pc >> 2 ) / predictor->btbSize ) % (int) pow(2, predictor->tagSize);
	uint32_t entry = (pc >> 2) % predictor->btbSize ;




	if ( ((predictor->btb[entry].tag) == pc_tag) && (predictor->btb[entry].valid))
	{
		if (targetPc != predictor->btb[entry].target) 
		{
			predictor->btb[entry].target = targetPc;
		}
		predictor->update_state(pc, entry, taken);
		predictor->update_history(entry, taken, false);

	}
	// check if the current branch is in the table
	//if ( ((predictor->btb[entry].tag) == pc_tag) && (predictor->btb[entry].valid))
	//{
	//	predictor->update_state(pc, entry, taken);
	//	predictor->update_history(entry, taken, false);
	//}
	else 
		// replacing the current row or creating a new row
	{
		predictor->btb[entry].tag = pc_tag;
		predictor->btb[entry].target = targetPc;

		if ( ( !(predictor->isGlobalTable) ) && ( (predictor->btb[entry].valid) ) )
		{
			predictor->btb[entry].local_state = std::vector<FSMstate> ( (int)pow(2, predictor->historySize), predictor->default_state );
		}

		predictor->init_history(entry);
		predictor->update_state(pc, entry, taken);
		predictor->update_history(entry, taken, true);
		predictor->btb[entry].valid = true;

	}

	
	predictor->branch_count++;
	
	if (pred_dst != targetPc && taken)
	{
		predictor->flushed_count++;
	}
	else if (pred_dst != (pc + 4) && !taken)
	{
		predictor->flushed_count++;
	}
	
	return;
}

void BP_GetStats(SIM_stats *curStats)
{
	curStats->br_num = predictor->branch_count;
	curStats->flush_num = predictor->flushed_count;

	curStats->size = predictor->theoretical_predictor_size;
	
	delete predictor;
	return;
}
