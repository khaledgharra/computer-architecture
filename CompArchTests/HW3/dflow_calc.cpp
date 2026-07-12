/* 046267 Computer Architecture - Winter 20/21 - HW #3               */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <vector>
#include <algorithm>
#include <iostream>

#define REGISTERS   32
#define ENTRY       -1

using namespace std;

class Node
{
public:
    int EndCycle; // the clock cycle that the operation ended in
    int OpTime;   // the run time of the operation
    int OpLine;   // the line of the opcode
    Node* dep1;   // pointer to the first dependency, NULL if points to Entry
    Node* dep2;   // pointer to the second dependency, NULL if points to Entry

    Node() : EndCycle(0), OpTime(0), OpLine(0), dep1(NULL), dep2(NULL) 
    {

    }
    ~Node() 
    {

    }

};


class Program
{
public:
    vector<Node> Data;
    int MostRecentlyUsed[REGISTERS]; //saves which line was the last to write to the register

    Program()
    {
        for (int i = 0; i < REGISTERS; i++)
        {
            this->MostRecentlyUsed[i] = 0;
        }
    }

    Program(int len)
    {
        this->Data = vector<Node>(len);
        for (int i = 0; i < REGISTERS; i++)
        {
            this->MostRecentlyUsed[i] = ENTRY;
        }
    }


    ~Program()
    {
        
    }

};



ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) 
{
    Program* Myprog = new Program(numOfInsts);
    int cycles1 = 0; // counts the cycles of each dependenfy, to later take the max value of them.
    int cycles2 = 0;
    

    for (unsigned int i = 0; i < numOfInsts; i++)
    {
        Myprog->Data[i].OpTime = opsLatency[progTrace[i].opcode]; // store the OpTime value from the user
        Myprog->Data[i].OpLine = i; // store the line number
        cycles1 = 0; 
        cycles2 = 0;
        
        // if the second dependency points to entry
        if (Myprog->MostRecentlyUsed[progTrace[i].src2Idx] == ENTRY)
        {
            // if the other dependency points to entry also, use default values
            if (Myprog->MostRecentlyUsed[progTrace[i].src1Idx] == ENTRY)
            {
                // TODO: point to entry
                //Myprog->Data[i].EndCycle = opsLatency[progTrace[i].opcode];
            }
            else
            {
                // if there is another dependency - store the pointer to it 
                Myprog->Data[i].dep1 = &(Myprog->Data[Myprog->MostRecentlyUsed[progTrace[i].src1Idx]]);
                // get the dependency's running cycles num
                cycles1 = Myprog->Data[Myprog->MostRecentlyUsed[progTrace[i].src1Idx]].EndCycle;
            }
        }
        else
        {
            // if there is a second dependency - store the pointer to it 
            Myprog->Data[i].dep2 = &(Myprog->Data[Myprog->MostRecentlyUsed[progTrace[i].src2Idx]]);
            // get the dependency's running cycles num
            cycles2 = Myprog->Data[Myprog->MostRecentlyUsed[progTrace[i].src2Idx]].EndCycle;

            if (Myprog->MostRecentlyUsed[progTrace[i].src1Idx] == ENTRY)
            {
                // if no dependency for the first one - use default values for it.
            }
            else
            {
                // if there is another dependency  - store the pointer to it 
                Myprog->Data[i].dep1 = &(Myprog->Data[Myprog->MostRecentlyUsed[progTrace[i].src1Idx]]);
                // get the dependency's running cycles num
                cycles1 = Myprog->Data[Myprog->MostRecentlyUsed[progTrace[i].src1Idx]].EndCycle;
            }
        }
        // take the maximum value of the cycles of each dependenfy, and add the run time of the op to it
        Myprog->Data[i].EndCycle = max(cycles1, cycles2) + Myprog->Data[i].OpTime;

        //  mark the reg the operation writes to with it's line number
        Myprog->MostRecentlyUsed[progTrace[i].dstIdx] = Myprog->Data[i].OpLine;
    }

    return Myprog;
}

void freeProgCtx(ProgCtx ctx) 
{
    Program* Myprog = static_cast<Program*>(ctx);
    delete Myprog;

    return;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) 
{
    Program* Myprog = static_cast<Program*>(ctx);
    if (theInst < Myprog->Data.size())
    {
        // return the number of cycles of this 
        return (Myprog->Data[theInst].EndCycle - Myprog->Data[theInst].OpTime);
    }
    return -1;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) 
{
    Program* Myprog = static_cast<Program*>(ctx);
    *src1DepInst = -1;
    *src2DepInst = -1;
    if (theInst < Myprog->Data.size())
    {
        if (NULL != Myprog->Data[theInst].dep1)
        {
            *src1DepInst = Myprog->Data[theInst].dep1->OpLine;
        }

        if (NULL != Myprog->Data[theInst].dep2)
        {
            *src2DepInst = Myprog->Data[theInst].dep2->OpLine;
        }
        return 0;
    }
    return -1;
}

int getProgDepth(ProgCtx ctx) 
{
    Program* Myprog = static_cast<Program*>(ctx);
    int MaxDepth = 0;
    int length = Myprog->Data.size();
    for (int i = 0; i < length; i++)
    {
        /*if (MaxDepth < Myprog->Data[i].EndCycle)
        {
            MaxDepth = Myprog->Data[i].EndCycle;
        }*/
        MaxDepth = max(MaxDepth, Myprog->Data[i].EndCycle);

    }
    return MaxDepth;
}


