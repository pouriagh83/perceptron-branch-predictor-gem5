/*
 * Copyright (c) 2022-2023 The University of Edinburgh
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2004-2006 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cpu/pred/perceptron.hh"

#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/Fetch.hh"

namespace gem5
{

namespace branch_prediction
{

PerceptronBP::PerceptronBP(const PerceptronBPParams &params)
    : BPredUnit(params),
      n_perceptron(params.n_perceptron),
      history_length(params.history_length),
      perceptronTable(n_perceptron, std::vector<int>(history_length + 1, 0)),
      indexMask(n_perceptron - 1),
      threshold(params.threshold),
      globalHistory(0)
{
    if (!isPowerOf2(n_perceptron)) {
        fatal("Invalid number of perceptrons!\n");
    }

    if (!isPowerOf2(history_length+1)) {
        fatal("Invalid HistorySize!\n");
    }

    DPRINTF(Fetch, "index mask: %#x\n", indexMask);

    DPRINTF(Fetch, "number of perceptron: %i\n",
            n_perceptron);

    DPRINTF(Fetch, "global History size: %i\n", history_length);

    DPRINTF(Fetch, "instruction shift amount: %i\n",
            instShiftAmt);
}

void
PerceptronBP::updateHistories(ThreadID tid, Addr pc, bool uncond,
                         bool taken, Addr target, void * &bp_history)
{
// Place holder for a function that is called to update predictor history
    globalHistory = (globalHistory << 1) | (taken ? 1 : 0);
}


bool
PerceptronBP::lookup(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    bool taken;
    unsigned local_predictor_idx = getLocalIndex(branch_addr);

    DPRINTF(Fetch, "Looking up index %#x\n",
            local_predictor_idx);

    int y = perceptronTable[local_predictor_idx][0];

    for (int i = 0; i < history_length; i++) {
        int bit = (globalHistory >> i) & 1;
        if (bit)
            y += perceptronTable[local_predictor_idx][i + 1];
        else
            y -= perceptronTable[local_predictor_idx][i + 1];
    }    

    DPRINTF(Fetch, "prediction is %i.\n",
            y);

    taken = y >= 0;
    return taken;
}

void
PerceptronBP::update(ThreadID tid, Addr branch_addr, bool taken, void *&bp_history,
                bool squashed, const StaticInstPtr & inst, Addr target)
{
    // No state to restore, and we do not update on the wrong
    // path.
    if (squashed) {
        return;
    }

    // Update the local predictor.
    unsigned local_predictor_idx = getLocalIndex(branch_addr);

    int t = taken ? 1 : -1;
    // t is the actual value
    // y is the value that was predicted
    int y = perceptronTable[local_predictor_idx][0]; // start with the bias
    for (int i = 0; i < history_length; ++i) {
        int bit = (globalHistory >> i) & 1;
        if (bit)
            y += perceptronTable[local_predictor_idx][i + 1];
        else
            y -= perceptronTable[local_predictor_idx][i + 1];
    }
    int y_pred = y >= 0 ? 1 : -1;
    
    if (y_pred == t && std::abs(y) >= threshold){
        updateHistories(tid, branch_addr, false, taken, target, bp_history);
        return; // no update needed 
    }
    perceptronTable[local_predictor_idx][0] += t;
    DPRINTF(Fetch, "Looking up index %#x\n", local_predictor_idx);

    if (taken) {
        DPRINTF(Fetch, "Branch updated as taken.\n");
    } else {
        DPRINTF(Fetch, "Branch updated as not taken.\n");
    }
    for (int i = 0; i < history_length; ++i) {
        int bit = (globalHistory >> i) & 1;
        if (bit)
            perceptronTable[local_predictor_idx][i + 1] += (taken ? 1 : -1);
        else
            perceptronTable[local_predictor_idx][i + 1] -= (taken ? 1 : -1);
    }
    updateHistories(tid, branch_addr, false, taken, target, bp_history);
}

inline
bool
PerceptronBP::getPrediction(uint8_t &count)
{
    // Get the MSB of the count
    return (count >> (history_length - 1));
}

inline
unsigned
PerceptronBP::getLocalIndex(Addr &branch_addr)
{
    return (branch_addr >> instShiftAmt) & indexMask;
}

inline
unsigned
PerceptronBP::getThreshold(unsigned history_length)
{
    return std::floor(1.93 * history_length + 14);
}

} // namespace branch_prediction
} // namespace gem5
