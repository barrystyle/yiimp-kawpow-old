#include "stratum.h"
#include <openssl/sha.h>

uint256 GetKAWPOWHeaderHash(const CBlockHeader& blockHeader)
{
    CKAWPOWInput tempHash;

    tempHash.nVersion = blockHeader.nVersion;
    tempHash.hashPrevBlock = blockHeader.hashPrevBlock;
    tempHash.hashMerkleRoot = blockHeader.hashMerkleRoot;
    tempHash.nTime = blockHeader.nTime;
    tempHash.nBits = blockHeader.nBits;
    tempHash.nHeight = blockHeader.nHeight;

    uint256 midState{}, midHash{};
    SHA256((const unsigned char*) &tempHash, 80, (unsigned char*)&midState);
    SHA256((unsigned char*) &midState, 32, (unsigned char*)&midHash);

    return midHash;
}

uint256 KAWPOWHash_OnlyMix(const CBlockHeader& blockHeader)
{
    uint256 raw_header = GetKAWPOWHeaderHash(blockHeader);
    const auto header_hash = to_hash256(raw_header.GetHex());
    const auto result = progpow::hash_no_verify(blockHeader.nHeight,
                                                header_hash,
                                                to_hash256(blockHeader.mix_hash.GetHex()),
                                                blockHeader.nNonce64);

    return uint256S(to_hex(result));
}

uint256 KAWPOWHash(const CBlockHeader& blockHeader, uint256& mix_hash)
{
    static ethash::epoch_context_ptr context {nullptr, nullptr};
    const auto epoch_number = ethash::get_epoch_number(blockHeader.nHeight);

    if (!context || context->epoch_number != epoch_number) {
        context = ethash::create_epoch_context(epoch_number);
    }

    uint256 raw_header = GetKAWPOWHeaderHash(blockHeader);
    const auto header_hash = to_hash256(raw_header.GetHex());
    const auto result = progpow::hash(*context,
                                      blockHeader.nHeight,
                                      header_hash,
                                      blockHeader.nNonce64);

    return uint256S(to_hex(result.final_hash));
}

void kawpow_hash(const char* input, char* output, uint32_t len)
{
        scrypt_1024_1_1_256((unsigned char *)input, (unsigned char *)output);
}

