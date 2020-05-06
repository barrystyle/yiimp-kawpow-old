#ifndef KAWPOW_H
#define KAWPOW_H

struct CBlockHeader
{
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nHeight;
    uint64_t nNonce64;
    uint256 mix_hash;
};

struct CKAWPOWInput
{
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nHeight;
};

void kawpow_hash(const char* input, char* output, uint32_t len);

#endif // KAWPOW_H
