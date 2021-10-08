#include "stratum.h"
#include <openssl/sha.h>

#include <sstream>

#define KAWPOW_NONCE_SIZE 8

int kawpow_height;
YAAMP_COIND* kawpow_coind;

void kawpow_test()
{
    int height = 49;
    std::string headerstr = "63155f732f2bf556967f906155b510c917e48e99685ead76ea83f4eca03ab12b";
    std::string nonce = "0000000007073c07";
    std::string mixhash_test;

    const auto pow_out = KAWPOWQuick(headerstr, mixhash_test, nonce, height);

    stratumlog("%s\n", to_hex(pow_out).c_str());
    stratumlog("%s\n", mixhash_test.c_str());
}

ethash::hash256 KAWPOWQuick(std::string& header_hash, std::string& mix_hash, std::string& nonce_str, int override_height)
{
    //! for testing purposes
    int height;
    if (override_height)
        height = override_height;
    else
        height = get_kawpow_height();

    uint64_t nonce64 = std::stoull(nonce_str, NULL, 16);
    const auto epoch_number = ethash::get_epoch_number(height);
    ethash::hash256 header_bin = to_hash256(header_hash.c_str());
    ethash::hash256 hashmix_bin = to_hash256(mix_hash.c_str());
    const auto result = progpow::hash_no_verify(height, header_bin, hashmix_bin, nonce64);

    return result;
}

std::string get_kawpow_seed(int height)
{
    std::string seed_hex = to_hex(ethash::calculate_epoch_seed(ethash::get_epoch_number(height)));
    return seed_hex;
}

void set_kawpow_height(int height)
{
    kawpow_height = height;
}

void set_kawpow_coind(YAAMP_COIND* coind)
{
    kawpow_coind = coind;
}

YAAMP_COIND* get_kawpow_coind()
{
    if (kawpow_coind)
        return kawpow_coind;
    return NULL;
}

int get_kawpow_height()
{
    return kawpow_height;
}

void kawpow_hash(const char* input, char* output, uint32_t len)
{
    //! dummy function
}

bool kawpow_send_extranonce(YAAMP_CLIENT* client)
{
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "{\"id\":1,\"result\":[null,\"%s\"],\"error\":null}\n", client->extranonce1);

    return socket_send(client->sock, buffer);
}

bool kawpow_authorize(YAAMP_CLIENT* client, json_value* json_params)
{
    if (json_params->u.array.length > 1 && json_params->u.array.values[1]->u.string.ptr)
        strncpy(client->password, json_params->u.array.values[1]->u.string.ptr, 1023);

    if (json_params->u.array.length > 0 && json_params->u.array.values[0]->u.string.ptr) {
        strncpy(client->username, json_params->u.array.values[0]->u.string.ptr, 1023);

        db_check_user_input(client->username);
        int len = strlen(client->username);
        if (!len)
            return false;

        char* sep = strpbrk(client->username, ".,;:");
        if (sep) {
            *sep = '\0';
            strncpy(client->worker, sep + 1, 1023 - len);
            if (strlen(client->username) > MAX_ADDRESS_LEN)
                return false;
        } else if (len > MAX_ADDRESS_LEN) {
            return false;
        }
    }

    if (!is_base58(client->username)) {
        clientlog(client, "bad mining address %s", client->username);
        return false;
    }

    if (g_debuglog_client) {
        debuglog("new client %s, %s, %s\n", client->username, client->password, client->version);
    }

    if (!client->userid || !client->workerid) {
        CommonLock(&g_db_mutex);
        db_add_user(g_db, client);

        if (client->userid == -1) {
            CommonUnlock(&g_db_mutex);
            client_block_ip(client, "account locked");
            clientlog(client, "account locked");

            return false;
        }

        db_add_worker(g_db, client);
        CommonUnlock(&g_db_mutex);
    }

    get_random_key(client->notify_id);

    // if we're gotten this far, assign an extranonce
    get_next_extraonce1(client->extranonce1_default);
    client->difficulty_actual = g_stratum_difficulty;
    strcpy(client->extranonce1, client->extranonce1_default);

    kawpow_send_extranonce(client);
    client_send_result(client, "true");
    kawpow_send_difficulty(client, client->difficulty_actual);
    job_send_last(client);

    g_list_client.AddTail(client);
    return true;
}

void kawpow_target_diff(double difficulty, char* buffer)
{
    sprintf(buffer, "0000%016llx00000000000000000000000000000000000000000000", diff_to_target(difficulty));
}

void kawpow_block_diff(uint64_t target, char* buffer)
{
    sprintf(buffer, "0000%016llx00000000000000000000000000000000000000000000", target);
}

void kawpow_set_difficulty(YAAMP_CLIENT* client, double difficulty)
{
    client->difficulty_actual = difficulty;
}

bool kawpow_send_difficulty(YAAMP_CLIENT* client, double difficulty)
{
    char target[128];
    memset(target, 0, sizeof(target));
    kawpow_target_diff(difficulty, target);
    client->last_target = uint256S(target);
    kawpow_set_difficulty(client, difficulty);

    return client_call(client, "mining.set_target", "[\"%s\"]", target);
}

void kawpow_job_mining_notify_buffer(YAAMP_JOB* job, YAAMP_CLIENT* client, char* buffer)
{
    YAAMP_JOB_TEMPLATE* templ = job->templ;

    // kawpow stratum
    sprintf(buffer, "{\"id\":null,\"method\":\"mining.notify\",\"params\":[\"%x\",\"%s\",\"%s\",\"%s\",true,%d,\"%s\"]}\n",
        job->id,
        templ->header_hash,
        get_kawpow_seed(templ->height).c_str(),
        client->last_target.ToString().c_str(),
        templ->height,
        templ->nbits);
}

void kawpow_submit_ok(YAAMP_CLIENT* client)
{
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "{\"id\":%d,\"jsonrpc\":\"2.0\",\"error\":null,\"result\":{\"status\":\"OK\"}}\n", client->id_int);
    socket_send_raw(client->sock, buffer, strlen(buffer));
}

bool kawpow_submit(YAAMP_CLIENT* client, json_value* json_params)
{
    // submit(worker_name, jobid, nonce, header, mixhash):
    if (json_params->u.array.length < 5 || !valid_string_params(json_params)) {
        debuglog("%s - %s bad message\n", client->username, client->sock->ip);
        client->submit_bad++;
        return false;
    }

    char nonce[128], header[128], mixhash[128];
    memset(nonce, 0, sizeof(nonce));
    memset(header, 0, sizeof(header));
    memset(mixhash, 0, sizeof(mixhash));

    if (strlen(json_params->u.array.values[1]->u.string.ptr) > 32) {
        clientlog(client, "bad json, wrong jobid len");
        client->submit_bad++;
        return false;
    }

    int jobid = htoi(json_params->u.array.values[1]->u.string.ptr);

    strncpy(nonce, json_params->u.array.values[2]->u.string.ptr + 2, 16);
    strncpy(header, json_params->u.array.values[3]->u.string.ptr + 2, 64);
    strncpy(mixhash, json_params->u.array.values[4]->u.string.ptr + 2, 64);

    string_lower(nonce);
    string_lower(header);
    string_lower(mixhash);

    YAAMP_JOB* job = (YAAMP_JOB*)object_find(&g_list_job, jobid, true);
    if (!job) {
        client_submit_error(client, NULL, 21, "Invalid job id", header, mixhash, nonce);
        return true;
    } else if (job->deleted) {
        client_send_result(client, "true");
        object_unlock(job);
        return true;
    }

    //! sanity check nonce
    if (strlen(nonce) != KAWPOW_NONCE_SIZE * 2 || !ishexa(nonce, KAWPOW_NONCE_SIZE * 2)) {
        client_submit_error(client, job, 20, "Invalid nonce size", header, mixhash, nonce);
        return true;
    }

    //! sanity check header/mixhash
    if ((strlen(header) != 64 || !ishexa(header, 64)) || (strlen(mixhash) != 64 || !ishexa(mixhash, 64))) {
        client_submit_error(client, job, 20, "Invalid header/mixhash", header, mixhash, nonce);
        return true;
    }

    YAAMP_SHARE* share = share_find(job->id, header, mixhash, nonce, client->username);
    if (share) {
        client_submit_error(client, job, 22, "Duplicate share", header, mixhash, nonce);
        return true;
    }

    std::string header_str = string(header);
    std::string mixhash_str = string(mixhash);
    std::string nonce_str = string(nonce);

    ethash::hash256 powHash = KAWPOWQuick(header_str, mixhash_str, nonce_str);
    ethash::hash256 targetHash = to_hash256(client->last_target.GetHex());

    //! intermediate
    uint256 hashbin = uint256S(to_hex(powHash));
    stratumlog("%s\n", hashbin.ToString().c_str());

    uint64_t hash_int = get_hash_difficulty((uint8_t*)&hashbin);
    double share_diff = diff_to_target(hash_int);

    if (uint256S(to_hex(powHash)) > uint256S(to_hex(targetHash))) {
        client_submit_error(client, job, 26, "Low difficulty share", header, mixhash, nonce);
        return true;
    }

    stratumlog("client %d sent powhash %s\n", client->id, to_hex(powHash).c_str());

    kawpow_submit_ok(client);
    client_record_difficulty(client);
    client->submit_bad = 0;
    client->shares++;

    share_add(client, job, true, header, mixhash, nonce, share_diff, 0);
    object_unlock(job);

    return true;
}

bool kawpow_submitblock(std::string& header_str, std::string& mixhash_str, std::string& nonce_str)
{
    YAAMP_COIND *coind = get_kawpow_coind();
    if (!coind) {
        return false;
    }

    char params[1024];
    memset(params, 0, sizeof(params));
    sprintf(params, "[\"%s\",\"%s\",\"%s\"]", header_str.c_str(), mixhash_str.c_str(), nonce_str.c_str());

    json_value *json = rpc_call(&coind->rpc, "pprpcsb", params);
    if(!json) {
       usleep(500*YAAMP_MS);
       json = rpc_call(&coind->rpc, "pprpcsb", params);
    }
    free(json);

    return true;
}
