#ifndef KAWPOW_H
#define KAWPOW_H

#include "json.h"

class YAAMP_CLIENT;
class YAAMP_JOB;
class YAAMP_TEMPLATE;

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

//! from client.cpp
bool valid_string_params(json_value *json_params);
void client_submit_error(YAAMP_CLIENT *client, YAAMP_JOB *job, int id, const char *message, char *extranonce2, char *ntime, char *nonce);

//! kawpow funcs
void kawpow_test();
void get_kawpow_height(int height);
int get_kawpow_height();
bool kawpow_submit(YAAMP_CLIENT* client, json_value* json_params);
void kawpow_hash(const char* input, char* output, uint32_t len);
bool kawpow_authorize(YAAMP_CLIENT* client, json_value* json_params);
void kawpow_target_diff(double difficulty, char* buffer);
void kawpow_block_diff(uint64_t target, char* buffer);
bool kawpow_send_difficulty(YAAMP_CLIENT *client, double difficulty);
void kawpow_job_mining_notify_buffer(YAAMP_JOB *job, YAAMP_CLIENT* client, char *buffer);
ethash::hash256 KAWPOWQuick(std::string& header_hash, std::string& mix_hash, std::string& nonce_str, int override_height = 0);

#endif // KAWPOW_H
