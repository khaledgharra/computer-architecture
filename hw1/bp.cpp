/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>

#define NOT_USING_SHARE 0
#define USING_SHARE_LSB 1
#define USING_SHARE_MID 2

struct BTBEntry
{
	uint32_t tag;
	uint32_t target;
	uint32_t history;

	std::vector<uint8_t> fsmTable;

	bool valid;
};

static std::vector<BTBEntry> btb;

static unsigned flush_num = 0;
static unsigned br_num = 0;

static unsigned g_btbSize;
static unsigned g_historySize;
static unsigned g_tagSize;
static unsigned g_fsmState;

static bool g_isGlobalHist;
static bool g_isGlobalTable;

static int g_shared;

static uint32_t globalHistory = 0;

static std::vector<uint8_t> globalFSMTable;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared)
{
	g_btbSize = btbSize;
	g_historySize = historySize;
	g_tagSize = tagSize;
	g_fsmState = fsmState;

	g_isGlobalHist = isGlobalHist;
	g_isGlobalTable = isGlobalTable;

	g_shared = Shared;
	btb.resize(btbSize);
	unsigned fsmTableSize = 1 << historySize;

	for (unsigned i = 0; i < btbSize; i++)
	{
		btb[i].valid = false;
		btb[i].tag = 0;
		btb[i].target = 0;
		btb[i].history = 0;

		if (!isGlobalTable)
		{
			btb[i].fsmTable.resize(fsmTableSize);

			for (unsigned j = 0; j < fsmTableSize; j++)
			{
				btb[i].fsmTable[j] = fsmState;
			}
		}
	}
	if (isGlobalTable)
	{
		globalFSMTable.resize(fsmTableSize);

		for (unsigned i = 0; i < fsmTableSize; i++)
		{
			globalFSMTable[i] = fsmState;
		}
	}
	globalHistory = 0;

	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst)
{
	// calculate BTB index
	unsigned index = (pc >> 2) & (g_btbSize - 1);

	// number of bits used for BTB index
	unsigned indexBits = __builtin_ctz(g_btbSize);

	// extract tag
	uint32_t tag =
		(pc >> (2 + indexBits)) &
		((1 << g_tagSize) - 1);

	// get BTB entry
	BTBEntry &entry = btb[index];

	// BTB miss
	if (!entry.valid || entry.tag != tag)
	{
		*dst = pc + 4;
		return false;
	}

	// get history
	uint32_t history;

	if (g_isGlobalHist)
	{
		history = globalHistory;
	}
	else
	{
		history = entry.history;
	}

	// FSM index
	// FSM index
	uint32_t fsmIndex = history;

	// SHARE logic
	if (g_isGlobalTable)
	{
		if (g_shared == USING_SHARE_LSB)
		{
			fsmIndex =
				history ^
				((pc >> 2) & ((1 << g_historySize) - 1));
		}

		else if (g_shared == USING_SHARE_MID)
		{
			fsmIndex =
				history ^
				((pc >> 16) & ((1 << g_historySize) - 1));
		}
	}

	// get FSM state
	uint8_t state;

	if (g_isGlobalTable)
	{
		state = globalFSMTable[fsmIndex];
	}
	else
	{
		state = entry.fsmTable[fsmIndex];
	}

	// predict TAKEN
	if (state >= 2)
	{
		*dst = entry.target;
		return true;
	}

	// predict NOT TAKEN
	*dst = pc + 4;
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	br_num++;
	uint32_t real_dst;

	if (taken)
	{
		real_dst = targetPc;
	}
	else
	{
		real_dst = pc + 4;
	}

	if (pred_dst != real_dst)
	{
		flush_num++;
	}
	// calculate BTB index
	unsigned index = (pc >> 2) & (g_btbSize - 1);

	// number of BTB index bits
	unsigned indexBits = __builtin_ctz(g_btbSize);

	// extract tag
	uint32_t tag =
		(pc >> (2 + indexBits)) &
		((1 << g_tagSize) - 1);

	// get BTB entry
	BTBEntry &entry = btb[index];

	// new entry / replacement
	if (!entry.valid || entry.tag != tag)
	{
		entry.valid = true;

		entry.tag = tag;

		entry.history = 0;

		entry.target = targetPc;

		// reset local FSM table
		if (!g_isGlobalTable)
		{
			unsigned fsmSize = 1 << g_historySize;
			entry.fsmTable.resize(fsmSize);

			for (unsigned i = 0; i < fsmSize; i++)
			{
				
				entry.fsmTable[i] = g_fsmState;
			}
		}
	}

	// choose history
	uint32_t history;

	if (g_isGlobalHist)
	{
		history = globalHistory;
	}
	else
	{
		history = entry.history;
	}

	// FSM index
	uint32_t fsmIndex = history;

	// SHARE logic
	if (g_isGlobalTable)
	{
		if (g_shared == USING_SHARE_LSB)
		{
			fsmIndex =
				history ^
				((pc >> 2) & ((1 << g_historySize) - 1));
		}

		else if (g_shared == USING_SHARE_MID)
		{
			fsmIndex =
				history ^
				((pc >> 16) & ((1 << g_historySize) - 1));
		}
	}

	// get FSM state reference
	uint8_t &state =
		g_isGlobalTable ? globalFSMTable[fsmIndex] : entry.fsmTable[fsmIndex];

	// update FSM
	if (taken)
	{
		if (state < 3)
		{
			state++;
		}
	}
	else
	{
		if (state > 0)
		{
			state--;
		}
	}

	// update history
	if (g_isGlobalHist)
	{
		globalHistory =
			((globalHistory << 1) | taken) &
			((1 << g_historySize) - 1);
	}
	else
	{
		entry.history =
			((entry.history << 1) | taken) &
			((1 << g_historySize) - 1);
	}

	// always update target
	entry.target = targetPc;
	return;
}

void BP_GetStats(SIM_stats *curStats)
{
	curStats->flush_num = flush_num;

	curStats->br_num = br_num;

	unsigned size = 0;

	// BTB entries
	size += g_btbSize * (1 + g_tagSize + 30);

	// histories
	if (g_isGlobalHist)
	{
		size += g_historySize;
	}
	else
	{
		size += g_btbSize * g_historySize;
	}

	// FSM tables
	unsigned fsmBits =
		(1 << g_historySize) * 2;

	if (g_isGlobalTable)
	{
		size += fsmBits;
	}
	else
	{
		size += g_btbSize * fsmBits;
	}

	curStats->size = size;
	return;
}
