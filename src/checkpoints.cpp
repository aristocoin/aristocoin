	// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double fSigcheckVerificationFactor = 5.0;

    struct CCheckpointData {
        const MapCheckpoints *mapCheckpoints;
        int64 nTimeLastCheckpoint;
        int64 nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
     boost::assign::map_list_of
          (   0, uint256("0x4053f27502d05555eab8855c2ee464798c2a4a541a242ea75a52add46471d8ba"))
          (   1, uint256("0x8ff38ae466d640b4c4eb4db2f373c839ff65c863e78b90d5314552d8fbaa8478"))
          (  50, uint256("0xc3988e1913d72ba1a33bdb9cc34f460c5e90e0064199da648089c00a72f64712"))
          ( 100, uint256("0xbe7619443976c18ada6ddc8a7b59a1ee47abee6eb68815b1abda02d981effa07"))
          ( 200, uint256("0x478f4ab5e07c797126a17ea078461c474c5303f331fff3d1da31b44775a6c2f3"))
          ( 300, uint256("0x80b6b17b168443dbb964f6da53d234c6f9e1c120b0f25c27b3d2174b7163c568"))
          ( 400, uint256("0x559f0be318ae6bcab276793cbaba5a5236cee427d03e25919a63a543d7832893"))
          ( 500, uint256("0x4bb42968ed8b5ce1103083300e11116c0a7483c0f8863e5f15c4fbc14bab79c7"))
          ( 600, uint256("0xd55fe90b5d15d686d85cbf20879e8c006f1be80a6a544b72cefaacc305420b32"))
          ( 700, uint256("0xbee62a461a0aa171173b3acf7939951f1abfbb17d51a28f56d25098550fef6ed"))
          ( 800, uint256("0xe772ac92281bffe583383c016194ff252d5f59ef851def642c231d3eea353979"))
          ( 900, uint256("0x396b0644027517ab056d6daa2d15ed3d75370c08e5ee62880c0d2d3e98129bb7"))
          ( 926, uint256("0xca7fb79ddd4f673a1142a8a547428256cb478cacbb3c2c239a6de605542d495c"));
        


    static const CCheckpointData data = {
        &mapCheckpoints,
        1501847291, // * UNIX timestamp of last checkpoint block
        927,    // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        1000.0     // * estimated number of transactions per day after checkpoint
    };

    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        (   0, uint256("0x"))
        
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        //1492691811,
        //547,
        //576
    };

    const CCheckpointData &Checkpoints() {
        if (fTestNet)
            return dataTestnet;
        else
            return data;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (!GetBoolArg("-checkpoints", true))
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex) {
        if (pindex==NULL)
            return 0.0;

        int64 nNow = time(NULL);

        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkoint, and
        // fSigcheckVerificationFactor per transaction after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate()
    {
        if (!GetBoolArg("-checkpoints", true))
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!GetBoolArg("-checkpoints", true))
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
