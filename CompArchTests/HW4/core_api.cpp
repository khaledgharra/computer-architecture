/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <vector>
#include <iostream>

#define SWITCHED        -3
#define DIDNT_SWITCH    -1
#define ALL_HALTED      -2

using namespace std;


class Thread
{
public:
    
    bool halted;//whether this thread is still alive. if all are halted end program
    int tid;//thread id
    tcontext cxt;//registers for each thread

    int wake_me_on_cycle;//chen to stop being idle
    int inst_num;//which instruction to run

    Thread();
    ~Thread();

};

Thread::Thread() : halted(false), tid(0), wake_me_on_cycle(0), 
inst_num(0)
{
    for (size_t i = 0; i < REGS_COUNT; i++)
    {
        this->cxt.reg[i] = 0;
    }
}



Thread::~Thread()
{
}

//Generic class, base for both types of MT which inherit from it.

class Generic_MT
{
public:
    
    vector<Thread> threads;     //threads for each MT machine
    int cycles_num;             //cycle num for entire program
    int instructions_num;       //inst num for entire program
    int curr_tid;               // the current (or last, in case of idel) 
                                // thread running in te simulation

    int load_lat;               //load latency
    int store_lat;              //store latency
    int threads_num;            //number of threads in program

    bool was_idle;              // if the cpu was idle the last cycle
    bool debug;                 // to diasable debug prints quickly


    Generic_MT();
    virtual ~Generic_MT();

    bool is_idle(int tid);
    int switch_context(int curr_tid);
    virtual void sim_flow();
    void init_sim_vals();

    double calc_cpi();
    bool run_inst(Instruction* inst, int tid);


private:

};

Generic_MT::Generic_MT() : 
    cycles_num(0), instructions_num(0), curr_tid(0), load_lat(0),
    store_lat(0), threads_num(0), was_idle(false), debug(false)
{
    
}

Generic_MT::~Generic_MT()
{
}

//checks if a thread is waiting for memory
bool Generic_MT::is_idle(int tid)
{
    return this->cycles_num < this->threads[tid].wake_me_on_cycle; // TODO: check if < or <=
}

//in charge of switching between threads,from the next in line(according to RR)
//@param: threadid: current thread id
//@output: thread to change to. returns -1 if all threads are idle, and -2 if all threads are halted
int Generic_MT::switch_context(int curr_tid)
{
    bool all_halted = true;
    // if was idle and the last running thread finished running, continue with it and dont switch.
    if (this->was_idle && (!this->is_idle(this->curr_tid)) 
        && (!this->threads[this->curr_tid].halted))
    {
        this->was_idle = false;
        return DIDNT_SWITCH;
    }

    // if first didnt, look for the next thread to switch to
    for (int i = curr_tid + 1; i < this->threads_num; i++)
    {
        if (!this->is_idle(i) && !this->threads[i].halted)
        {
            //return i;
            this->was_idle = false;
            this->curr_tid = i;
            return SWITCHED;
        }
        else if (!this->threads[i].halted)
        {
            all_halted = false;
        }
    }

    for (int i = 0; i < curr_tid + 1; i++)
    {
        if (!this->is_idle(i) && !this->threads[i].halted)
        {
            //return i;
            this->was_idle = false;
            this->curr_tid = i;
            return SWITCHED;
        }
        else if (!this->threads[i].halted)
        {
            all_halted = false;
        }
    }

    // if everyone halted, inidicate it.
    if (all_halted)
    {
        return ALL_HALTED;
    }
    this->was_idle = true;
    return DIDNT_SWITCH;
}


void Generic_MT::sim_flow()
{
    return;
}

//initiate simulation values recieved in file
void Generic_MT::init_sim_vals()
{
    this->load_lat = SIM_GetLoadLat();
    this->store_lat = SIM_GetStoreLat();
    this->threads_num = SIM_GetThreadsNum();

    this->threads = vector<Thread>(this->threads_num);
    return;
}

double Generic_MT::calc_cpi()
{
    if (0 == this->instructions_num)
    {
        return 0;
    }
    return (double)this->cycles_num / (double)this->instructions_num;
}

/*
    *executes the current instruction for thread with id threadid
    *@param:inst - current instruction that needs executing
    *@param:tid - id of active thread
    *output: returns 1 if needs to wait for memory or if halted, 0 otherwise
    */
bool Generic_MT::run_inst(Instruction* inst, int tid)
{
    int src1_val = this->threads[tid].cxt.reg[inst->src1_index];
    int src2_val = 0;
    int * dst = &(this->threads[tid].cxt.reg[inst->dst_index]);
    if (debug)
    {
        // cout << "cycle: " << this->cycles_num << " tid: " << tid << " opcode: " << inst->opcode;
    }

    if (inst->isSrc2Imm)
    {
        src2_val = inst->src2_index_imm;
    }
    else
    {
        src2_val = this->threads[tid].cxt.reg[inst->src2_index_imm];
    }

    bool should_switch = false;
    
    this->cycles_num++;


    switch (inst->opcode)
    {
    case CMD_NOP: // NOP
        // cycles++ but instructions is not changing!
        //should_switch = false;
        //cout << "cycle: " << this->cycles_num - 1 << "       " 
        //    << "          " << " Idle" << endl;
        should_switch = true;
        return should_switch;
        break;
    case CMD_ADDI:
        *dst = src1_val + src2_val;
        break;
    case CMD_SUBI:
        *dst = src1_val - src2_val;
        break;
    case CMD_ADD:
        *dst = src1_val + src2_val;
        break;
    case CMD_SUB:
        *dst = src1_val - src2_val;
        break;
    case CMD_LOAD:
        SIM_MemDataRead(src1_val + src2_val, dst);
        this->threads[tid].wake_me_on_cycle=this->cycles_num + this->load_lat;
        should_switch = true;
        break;
    case CMD_STORE:
        SIM_MemDataWrite(*dst + src2_val, src1_val);
        this->threads[tid].wake_me_on_cycle=this->cycles_num + this->store_lat;
        should_switch = true;
        break;
    case CMD_HALT:
        this->threads[tid].halted = true;
        should_switch = true;
        break;

    default:
        break;
    }

    //cout << " *dst: " << dst << " dst: " << *dst << endl;
    this->instructions_num++; // update only if got inst.opcode != NOP
    this->threads[tid].inst_num++;
    //this->cycles_num++;

    return should_switch;
}


class BlockedMT : public Generic_MT
{
public:
    int switch_num;
    int switch_latency;

    BlockedMT();
    ~BlockedMT();

    void init_sim_vals();
    void sim_flow();

private:

};

BlockedMT::BlockedMT() : 
    switch_num(0), switch_latency(0)
{
    
}

BlockedMT::~BlockedMT()
{
}

//initiate simulation values recieved in file
void BlockedMT::init_sim_vals() 
{
    Generic_MT::init_sim_vals();
    this->switch_latency = SIM_GetSwitchCycles();

    return;
}


// executes the simulation
void BlockedMT::sim_flow()
{
    Instruction inst = {CMD_NOP,0,0,0,false};
    int switch_ret_val = (int)DIDNT_SWITCH;
    bool should_switch = false; 
    bool add_nop = false;



    while (ALL_HALTED != switch_ret_val)
    {
        SIM_MemInstRead(this->threads[this->curr_tid].inst_num, &inst, this->curr_tid);
        if (add_nop) // true when moves to idle mode
        {
            add_nop = false;
            inst.opcode = CMD_NOP;
        }
        if (debug) // print the table
        {
            cout << "cycle: " << this->cycles_num << " tid: " << this->curr_tid
                 << " opcode: " << inst.opcode;
            if (CMD_NOP == inst.opcode)
            {
                cout << " IDLE!";
            }
            cout << endl;
        }
        // returns whether needs to switch context when wait for memory or thread halting
        should_switch = this->run_inst(&inst, this->curr_tid);

        if (should_switch) 
        {
            switch_ret_val = this->switch_context(this->curr_tid);
            if (DIDNT_SWITCH == switch_ret_val && this->was_idle)
            {
                add_nop = true;
            }
            else if (DIDNT_SWITCH == switch_ret_val && !this->was_idle)
            {
                add_nop = false;
                should_switch = false;

            }
            else if (SWITCHED == switch_ret_val)
            {
                if (debug) // print the table
                {
                    cout << "cycle: " << this->cycles_num << "       "
                        << "          " << " Switch Overhead ("
                        << "->" << this->curr_tid << ")" << endl;
                }
                this->cycles_num += this->switch_latency;
                // if triggered switch_contexet once, turn trigger off so it 
                // won't be triggered again.
                should_switch = false;
            }
        }
    }

    return;
}


class FinegrainedMT : public Generic_MT
{
public:
    FinegrainedMT();
    ~FinegrainedMT();

    void sim_flow();

private:

};

FinegrainedMT::FinegrainedMT()
{
    
}

FinegrainedMT::~FinegrainedMT()
{
}


void FinegrainedMT::sim_flow()
{
    Instruction inst = { CMD_NOP,0,0,0,false };
    int switch_ret_val = (int)DIDNT_SWITCH;
    bool add_nop = false;

    while (ALL_HALTED != switch_ret_val)
    {

        SIM_MemInstRead(this->threads[this->curr_tid].inst_num, &inst, this->curr_tid);

        if (add_nop)
        {
            add_nop = false;
            inst.opcode = CMD_NOP;
        }
        if (debug)
        {
            cout << "cycle: " << this->cycles_num << " tid: " << this->curr_tid
                << " opcode: " << inst.opcode;
            if (CMD_NOP == inst.opcode)
            {
                cout << " IDLE!";
            }
            cout << endl;
        }
        
        this->run_inst(&inst, this->curr_tid);

        switch_ret_val = this->switch_context(this->curr_tid);
            if (DIDNT_SWITCH == switch_ret_val && this->was_idle)
            {
                add_nop = true;
            }
            else if (DIDNT_SWITCH == switch_ret_val && !this->was_idle)
            {
                add_nop = false;
                //should_switch = false;

            }
            else if (SWITCHED == switch_ret_val)
            {
                if (debug) // print the table
                {
                    cout << "cycle: " << this->cycles_num << "       "
                        << "          " << " Switch Overhead ("
                        << this->curr_tid << "->" << switch_ret_val << ")" << endl;
                }
            }
        
    }
    return;
}


BlockedMT MyblockMT = BlockedMT();
FinegrainedMT MyfinegrainedMT = FinegrainedMT();

void CORE_BlockedMT() 
{
    MyblockMT.init_sim_vals();
    MyblockMT.sim_flow();
    return;
}

void CORE_FinegrainedMT() 
{
    MyfinegrainedMT.init_sim_vals();
    MyfinegrainedMT.sim_flow();
    return;
}

double CORE_BlockedMT_CPI()
{
    return MyblockMT.calc_cpi();
}

double CORE_FinegrainedMT_CPI()
{
    return MyfinegrainedMT.calc_cpi();
}

void CORE_BlockedMT_CTX(tcontext context[], int threadid)
{
    for (size_t i = 0; i < REGS_COUNT; i++)
    {
        context[threadid].reg[i]= MyblockMT.threads[threadid].cxt.reg[i];
    }
    return;
}

void CORE_FinegrainedMT_CTX(tcontext context[], int threadid)
{
    for (size_t i = 0; i < REGS_COUNT; i++)
    {
        context[threadid].reg[i]= MyfinegrainedMT.threads[threadid].cxt.reg[i];
    }
    return;
}
