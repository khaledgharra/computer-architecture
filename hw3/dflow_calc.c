/* 046267 Computer Architecture - HW #3 */
/* Implementation for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <stdlib.h>

typedef struct {
    int src1Dep;  /* index of inst that src1 depends on, -1 if none */
    int src2Dep;  /* index of inst that src2 depends on, -1 if none */
    int depth;    /* critical-path depth in clock cycles to this inst */
} InstData;

typedef struct {
    unsigned int  numOfInsts;
    InstData     *instData;
    int           progDepth;
} ProgCtxData;

ProgCtx analyzeProg(const unsigned int opsLatency[],
                    const InstInfo progTrace[],
                    unsigned int numOfInsts)
{
    unsigned int i;

    if (!opsLatency || !progTrace || numOfInsts == 0)
        return PROG_CTX_NULL;

    ProgCtxData *ctx = malloc(sizeof(ProgCtxData));
    if (!ctx) return PROG_CTX_NULL;

    ctx->numOfInsts = numOfInsts;
    ctx->progDepth  = 0;
    ctx->instData   = malloc(numOfInsts * sizeof(InstData));
    if (!ctx->instData) { free(ctx); return PROG_CTX_NULL; }

    /* Find the highest register index so we can allocate lastWriter[] */
    unsigned int maxReg = 0;
    for (i = 0; i < numOfInsts; ++i) {
        if (progTrace[i].dstIdx  > 0 && (unsigned int)progTrace[i].dstIdx > maxReg)
            maxReg = (unsigned int)progTrace[i].dstIdx;
        if (progTrace[i].src1Idx > maxReg) maxReg = progTrace[i].src1Idx;
        if (progTrace[i].src2Idx > maxReg) maxReg = progTrace[i].src2Idx;
    }

    /* lastWriter[reg] = index of most recent instruction that wrote reg, -1 if none */
    int *lastWriter = malloc((maxReg + 1) * sizeof(int));
    if (!lastWriter) { free(ctx->instData); free(ctx); return PROG_CTX_NULL; }
    for (unsigned int r = 0; r <= maxReg; ++r)
        lastWriter[r] = -1;

    for (i = 0; i < numOfInsts; ++i) {
        unsigned int src1 = progTrace[i].src1Idx;
        unsigned int src2 = progTrace[i].src2Idx;
        int          dst  = progTrace[i].dstIdx;

        int dep1 = lastWriter[src1];
        int dep2 = lastWriter[src2];

        ctx->instData[i].src1Dep = dep1;
        ctx->instData[i].src2Dep = dep2;

        /* depth = max contribution from each source dependency */
        int d1 = (dep1 >= 0)
                 ? ctx->instData[dep1].depth + (int)opsLatency[progTrace[dep1].opcode]
                 : 0;
        int d2 = (dep2 >= 0)
                 ? ctx->instData[dep2].depth + (int)opsLatency[progTrace[dep2].opcode]
                 : 0;

        ctx->instData[i].depth = (d1 > d2) ? d1 : d2;

        /* track overall critical path (depth + this instruction's own latency) */
        int fullDepth = ctx->instData[i].depth + (int)opsLatency[progTrace[i].opcode];
        if (fullDepth > ctx->progDepth)
            ctx->progDepth = fullDepth;

        /* update last writer for the destination register */
        if (dst >= 0)
            lastWriter[(unsigned int)dst] = (int)i;
    }

    free(lastWriter);
    return (ProgCtx)ctx;
}

void freeProgCtx(ProgCtx ctx) {
    if (ctx == PROG_CTX_NULL) return;
    ProgCtxData *c = (ProgCtxData *)ctx;
    free(c->instData);
    free(c);
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    if (ctx == PROG_CTX_NULL) return -1;
    ProgCtxData *c = (ProgCtxData *)ctx;
    if (theInst >= c->numOfInsts) return -1;
    return c->instData[theInst].depth;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst,
                int *src1DepInst, int *src2DepInst) {
    if (ctx == PROG_CTX_NULL || !src1DepInst || !src2DepInst) return -1;
    ProgCtxData *c = (ProgCtxData *)ctx;
    if (theInst >= c->numOfInsts) return -1;
    *src1DepInst = c->instData[theInst].src1Dep;
    *src2DepInst = c->instData[theInst].src2Dep;
    return 0;
}

int getProgDepth(ProgCtx ctx) {
    if (ctx == PROG_CTX_NULL) return 0;
    return ((ProgCtxData *)ctx)->progDepth;
}
