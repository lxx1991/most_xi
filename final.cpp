#include <iostream>
#include <cstdint>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <atomic>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <fcntl.h>
#include <boost/multiprecision/cpp_int.hpp>

#include "rt_assert.h"
#include "prepare_socket.h"
#include "utils.h"

using namespace std;
using namespace boost::multiprecision;

#define USER "xi"
#define PASSWORD "vtmvhy5e"

#define __M1_HASH__
#define __M2_HASH__
#define __M3_HASH__
#define __M4_HASH__
#define __M2_GUESS__
#define __M3_GUESS__
#define __M4_GUESS__

const int N = 256;

char headers[N + 10][1024];
int headers_n[N + 10];

const int BUFFER_SIZE = 36000000; // 10000 * 3600
uint8_t buffer[BUFFER_SIZE];

#ifdef __M1__
const int64_t M1 = 20220217214410LL;
const int M1_len = 13;
#endif

#ifdef __M1_HASH__
const uint64_t M1 = 2022021721441LL; // 20220217214410
const __uint128_t M1_inv10 = 1819819549297LL;
int M1_hash_map[33554432];
uint64_t M1_num[BUFFER_SIZE];
uint64_t M1_inv10_power_k[2048][60];
#endif

#ifdef __M2__
const __int128_t M2 = __int128_t(1046482571183LL) * __int128_t(100000000000000LL) + __int128_t(48370704723119LL);
const int M2_len = 26;
#endif

#ifdef __M2_HASH__
const __uint128_t M2 = __uint128_t(1046482571183LL) * __uint128_t(100000000000000LL) + __uint128_t(48370704723119LL);
const int256_t M2_inv10 = __uint128_t(104648257118LL) * __uint128_t(100000000000000LL) + __uint128_t(34837070472312LL);
int M2_hash_map[33554432];
__uint128_t M2_num[BUFFER_SIZE];
__uint128_t M2_inv10_power_k[2048][60];
#endif

#ifdef __M2_GUESS__
const int128_t M2G("104648257118348370704723119");
const int M2G_len = 26;
const int M2G_guess_len = 26 - 8;
#endif

#ifdef __M3__
const int256_t M3("125000000000000140750000000000052207500000000006359661");
const int M3_len = 54;
#endif

#ifdef __M3_HASH__
const uint64_t M3 = 500000000000000209LL;
const __uint128_t M3_inv10 = 50000000000000021LL;
const int M3_len = 54;
int M3_hash_map[33554432];
uint64_t M3_num[BUFFER_SIZE];
uint64_t M3_inv10_power_k[2048][60];
#endif

#ifdef __M3_GUESS__
const int256_t M3G("125000000000000140750000000000052207500000000006359661");
const int M3G_len = 54;
const int M3G_guess_len = 54 - 8;
#endif

#ifdef __M4__
const int256_t M4("10885732038215355481752285039386319187390558900925053798518152998488201"); // 3**50 * 7**30 * 11**20
const int M4_len = 71;
#endif

#ifdef __M4_HASH__
const uint64_t M4 = 244210137401840841LL;
const __uint128_t M4_inv10 = 219789123661656757LL;
const int M4_len = 71;
int M4_hash_map[33554432];
uint64_t M4_num[BUFFER_SIZE];
uint64_t M4_inv10_power_k[2048][60];
#endif

#ifdef __M4_GUESS__
const int256_t M4G("10885732038215355481752285039386319187390558900925053798518152998488201"); // 3**50 * 7**30 * 11**20
const int M4G_len = 71;
const int M4G_guess_len = 71 - 8;
#endif

int last_M2_ans = 0;
int last_M3_ans = 0;
int last_M4_ans = 0;

void submit_ans(uint8_t *pos, int k)
{
#ifdef DEBUG
    printf("%.*s\n", k, pos);
#else
    memcpy(headers[k] + headers_n[k], pos, k);
    int ret = write(submit_fds[next_submit_fd_pos], headers[k], headers_n[k] + k);
    if (ret != headers_n[k] + k)
    {
        close(submit_fds[next_submit_fd_pos]);
        submit_fds[next_submit_fd_pos] = get_send_socket();
        ret = write(submit_fds[next_submit_fd_pos], headers[k], headers_n[k] + k);
        if (ret != headers_n[k] + k)
        {
            printf("-----------------------------re-connect failed--------------------------------\n");
            exit(-1);
        }
        printf("---------------------------------re-connect-----------------------------------\n");
    }
    next_submit_fd_pos = next_submit_fd_pos < SUBMIT_FD_N - 1 ? next_submit_fd_pos + 1 : 0;
#endif
}

int main()
{

#ifndef DEBUG
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    rt_assert_eq(sched_setaffinity(0, sizeof(mask), &mask), 0);
#endif

    for (int i = 1; i <= N; i++)
        headers_n[i] = sprintf(headers[i], "POST /submit?user=%s&passwd=%s HTTP/1.1\r\nContent-Length: %d\r\n\r\n", USER, PASSWORD, i);

    double sent_time[64];
    double total_sent_time = 0;
    int total_sent_time_cnt = 0;

    int pos = N + 10;
    memset(buffer, '0', N + 10);

#ifdef __M1__
    int64_t M1_pow10[N + 10][60];
    for (int i = 0; i < 10; i++)
    {
        char ch = char(i + '0');
        M1_pow10[0][ch] = i % M1;
        for (int j = 1; j <= N; j++)
        {
            M1_pow10[j][ch] = (M1_pow10[j - 1][ch] * 10) % M1;
        }
    }
    int64_t M1_cur_suffix = 0;
#endif

#ifdef __M1_HASH__
    memset(M1_hash_map, 0, sizeof(M1_hash_map));
    memset(M1_num, 0, sizeof(M1_num));
    for (int i = 0; i < 10; i++)
    {
        M1_inv10_power_k[0][i + '0'] = i;
        for (int j = 1; j < 2048; j++)
            M1_inv10_power_k[j][i + '0'] = int64_t((M1_inv10_power_k[j - 1][i + '0'] * M1_inv10) % M1);
    }
#endif

#ifdef __M2__
    __int128_t M2_pow10[N + 10][60];
    for (int i = 0; i < 10; i++)
    {
        char ch = char(i + '0');
        M2_pow10[0][ch] = i % M2;
        for (int j = 1; j <= N; j++)
        {
            M2_pow10[j][ch] = (M2_pow10[j - 1][ch] * 10) % M2;
        }
    }
    __int128_t M2_cur_suffix = 0;
#endif

#ifdef __M2_HASH__
    memset(M2_hash_map, 0, sizeof(M2_hash_map));
    memset(M2_num, 0, sizeof(M2_num));
    for (int i = 0; i < 10; i++)
    {
        M2_inv10_power_k[0][i + '0'] = i;
        for (int j = 1; j < 2048; j++)
            M2_inv10_power_k[j][i + '0'] = ((M2_inv10_power_k[j - 1][i + '0'] * M2_inv10) % M2).convert_to<__uint128_t>();
    }
#endif

#ifdef __M2_GUESS__
    int128_t M2_pow10[N + 10][60];
    for (int i = 0; i < 10; i++)
    {
        char ch = char(i + '0');
        M2_pow10[0][ch] = i % M2G;
        for (int j = 1; j <= N; j++)
        {
            M2_pow10[j][ch] = (M2_pow10[j - 1][ch] * 10) % M2G;
        }
    }
#endif

#ifdef __M3__
    int256_t M3_pow10[N + 10][60];
    for (int i = 0; i < 10; i++)
    {
        char ch = char(i + '0');
        M3_pow10[0][ch] = i % M3;
        for (int j = 1; j <= N; j++)
        {
            M3_pow10[j][ch] = (M3_pow10[j - 1][ch] * 10) % M3;
        }
    }
    int256_t M3_cur_suffix = 0;
#endif

#ifdef __M3_HASH__
    memset(M3_hash_map, 0, sizeof(M3_hash_map));
    memset(M3_num, 0, sizeof(M3_num));
    for (int i = 0; i < 10; i++)
    {
        M3_inv10_power_k[0][i + '0'] = i;
        for (int j = 1; j < 2048; j++)
            M3_inv10_power_k[j][i + '0'] = int64_t((M3_inv10_power_k[j - 1][i + '0'] * M3_inv10) % M3);
    }
#endif

#ifdef __M3_GUESS__
    int256_t M3_pow10[N + 10][60];
    for (int i = 0; i < 10; i++)
    {
        char ch = char(i + '0');
        M3_pow10[0][ch] = i % M3G;
        for (int j = 1; j <= N; j++)
        {
            M3_pow10[j][ch] = (M3_pow10[j - 1][ch] * 10) % M3G;
        }
    }
#endif

#ifdef __M4__
    int256_t M4_pow10[N + 10][60];
    for (int i = 0; i < 10; i++)
    {
        char ch = char(i + '0');
        M4_pow10[0][ch] = i % M4;
        for (int j = 1; j <= N; j++)
        {
            M4_pow10[j][ch] = (M4_pow10[j - 1][ch] * 10) % M4;
        }
    }
    int256_t M4_cur_suffix = 0;
#endif

#ifdef __M4_HASH__
    memset(M4_hash_map, 0, sizeof(M4_hash_map));
    memset(M4_num, 0, sizeof(M4_num));
    for (int i = 0; i < 10; i++)
    {
        M4_inv10_power_k[0][i + '0'] = i;
        for (int j = 1; j < 2048; j++)
            M4_inv10_power_k[j][i + '0'] = int64_t((M4_inv10_power_k[j - 1][i + '0'] * M4_inv10) % M4);
    }
#endif

#ifdef __M4_GUESS__
    int256_t M4_pow10[N + 10][60];
    for (int i = 0; i < 10; i++)
    {
        char ch = char(i + '0');
        M4_pow10[0][ch] = i % M4G;
        for (int j = 1; j <= N; j++)
        {
            M4_pow10[j][ch] = (M4_pow10[j - 1][ch] * 10) % M4G;
        }
    }

#endif
    prepare_socket();
    ssize_t n;
    int recv_fd_idx = 0;

    while (pos + 1024 < BUFFER_SIZE)
    {
        // prepare
#ifdef __M1_HASH__
        for (int i = N; i > 0; i--)
            M1_hash_map[M1_num[pos - i] & 33554431LL] = 0;
        M1_num[pos - N] = M1_inv10_power_k[0][buffer[pos - N]];
        M1_hash_map[M1_num[pos - N] & 33554431LL] = pos - N;
        for (int i = N - 1; i > 0; i--)
        {
            M1_num[pos - i] = M1_num[pos - i - 1] + M1_inv10_power_k[N - i][buffer[pos - i]];
            M1_num[pos - i] = M1_num[pos - i] >= M1 ? M1_num[pos - i] - M1 : M1_num[pos - i];
            M1_hash_map[M1_num[pos - i] & 33554431LL] = pos - i;
        }
#endif

#ifdef __M3_HASH__
        for (int i = N; i > 0; i--)
            M3_hash_map[M3_num[pos - i] & 33554431LL] = 0;
        M3_num[pos - N] = M3_inv10_power_k[0][buffer[pos - N]];
        M3_hash_map[M3_num[pos - N] & 33554431LL] = pos - N;
        for (int i = N - 1; i > 0; i--)
        {
            M3_num[pos - i] = M3_num[pos - i - 1] + M3_inv10_power_k[N - i][buffer[pos - i]];
            M3_num[pos - i] = M3_num[pos - i] >= M3 ? M3_num[pos - i] - M3 : M3_num[pos - i];
            M3_hash_map[M3_num[pos - i] & 33554431LL] = pos - i;
        }
#endif

#ifdef __M4_HASH__
        for (int i = N; i > 0; i--)
            M4_hash_map[M4_num[pos - i] & 33554431LL] = 0;
        M4_num[pos - N] = M4_inv10_power_k[0][buffer[pos - N]];
        M4_hash_map[M4_num[pos - N] & 33554431LL] = pos - N;
        for (int i = N - 1; i > 0; i--)
        {
            M4_num[pos - i] = M4_num[pos - i - 1] + M4_inv10_power_k[N - i][buffer[pos - i]];
            M4_num[pos - i] = M4_num[pos - i] >= M4 ? M4_num[pos - i] - M4 : M4_num[pos - i];
            M4_hash_map[M4_num[pos - i] & 33554431LL] = pos - i;
        }
#endif

#ifdef __M2_HASH__
        for (int i = N; i > 0; i--)
            M2_hash_map[M2_num[pos - i] & 33554431LL] = 0;
        M2_num[pos - N] = M2_inv10_power_k[0][buffer[pos - N]];
        M2_hash_map[M2_num[pos - N] & 33554431LL] = pos - N;
        for (int i = N - 1; i > 0; i--)
        {
            M2_num[pos - i] = M2_num[pos - i - 1] + M2_inv10_power_k[N - i][buffer[pos - i]];
            M2_num[pos - i] = M2_num[pos - i] >= M2 ? M2_num[pos - i] - M2 : M2_num[pos - i];
            M2_hash_map[M2_num[pos - i] & 33554431LL] = pos - i;
        }
#endif
        while (true)
        {
            n = read(recv_fds[recv_fd_idx], buffer + pos, 1024);
            if (n > 0)
                break;
            if (unlikely(n == 0 || (n == -1 && errno != EAGAIN)))
            {
                printf("Read Error\n");
                exit(-1);
            }
            recv_fd_idx = (recv_fd_idx < RECV_FD_N - 1) ? recv_fd_idx + 1 : 0;
        }

        // initial
        double received_time = get_timestamp();
        int sent_time_cnt = 0;

        // work
        for (int i = pos; i < pos + n; i++)
        {
            int digit = buffer[i] - '0';
#ifdef __M1__
            M1_cur_suffix = (M1_cur_suffix - M1_pow10[M1_len - 1][buffer[i - M1_len]]) * 10 + digit;
            if (buffer[i] == '0')
            {
                int64_t cur_sum = M1_cur_suffix;
                for (int k = M1_len; k <= N; k++)
                {
                    if (unlikely(cur_sum == 0 && buffer[i - k + 1] != '0'))
                    {
                        sent_time[sent_time_cnt++] = get_timestamp();
                        submit_ans(buffer + (i - k + 1), k);
                    }
                    cur_sum += M1_pow10[k][buffer[i - k]];
                    cur_sum = (cur_sum >= M1) ? cur_sum - M1 : cur_sum;
                }
            }
#endif

#ifdef __M1_HASH__
            M1_num[i] = M1_num[i - 1] + M1_inv10_power_k[i - pos + N][buffer[i]];
            M1_num[i] = M1_num[i] >= M1 ? M1_num[i] - M1 : M1_num[i];
            uint64_t M1_hash_value = (M1_num[i] & 33554431LL);
            {
                int prev_loc = M1_hash_map[M1_hash_value];
                if (unlikely(i - prev_loc <= N && buffer[prev_loc + 1] != '0' && M1_num[prev_loc] == M1_num[i]))
                {
                    int k = i - prev_loc;
                    int suffix_zeros = 0;
                    while (true)
                    {
                        if (buffer[i + suffix_zeros] == '0')
                        {
                            sent_time[sent_time_cnt++] = get_timestamp();
                            submit_ans(buffer + prev_loc + 1, k);
                        }
                        suffix_zeros++;
                        if (buffer[i + suffix_zeros] != '0')
                            break;
                        k++;
                    }
                }
            }
            M1_hash_map[M1_hash_value] = i;
#endif
        }

        for (int i = pos; i < pos + n; i++)
        {
            int digit = buffer[i] - '0';
#ifdef __M3__
            M3_cur_suffix = (M3_cur_suffix - M3_pow10[M3_len - 1][buffer[i - M3_len]]) * 10 + digit;
            int256_t cur_sum = M3_cur_suffix;
            for (int k = M3_len; k <= N; k++)
            {
                if (unlikely(cur_sum == 0 && buffer[i - k + 1] != '0'))
                {
                    sent_time[sent_time_cnt++] = get_timestamp();
                    submit_ans(buffer + (i - k + 1), k);
                }
                cur_sum += M3_pow10[k][buffer[i - k]];
                cur_sum = (cur_sum >= M3) ? cur_sum - M3 : cur_sum;
            }
#endif

#ifdef __M3_HASH__
            M3_num[i] = M3_num[i - 1] + M3_inv10_power_k[i - pos + N][buffer[i]];
            M3_num[i] = M3_num[i] >= M3 ? M3_num[i] - M3 : M3_num[i];
            uint64_t M3_hash_value = (M3_num[i] & 33554431LL);
            {
                int prev_loc = M3_hash_map[M3_hash_value];
                if (unlikely(i - prev_loc <= N && i - prev_loc >= M3_len && buffer[prev_loc + 1] != '0' && M3_num[prev_loc] == M3_num[i]))
                {
                    int k = i - prev_loc;
                    int suffix_zeros = 0;
                    while (true)
                    {
                        if (likely(i + suffix_zeros > last_M3_ans))
                        {
                            sent_time[sent_time_cnt++] = get_timestamp();
                            submit_ans(buffer + prev_loc + 1, k);
                        }
                        suffix_zeros++;
                        if (buffer[i + suffix_zeros] != '0')
                            break;
                        k++;
                    }
                }
            }
            M3_hash_map[M3_hash_value] = i;
#endif
        }

        for (int i = pos; i < pos + n; i++)
        {
            int digit = buffer[i] - '0';
#ifdef __M4__
            M4_cur_suffix = (M4_cur_suffix - M4_pow10[M4_len - 1][buffer[i - M4_len]]) * 10 + digit;
            int256_t cur_sum = M4_cur_suffix;
            for (int k = M4_len; k <= N; k++)
            {
                if (unlikely(cur_sum == 0 && buffer[i - k + 1] != '0'))
                {
                    sent_time[sent_time_cnt++] = get_timestamp();
                    submit_ans(buffer + (i - k + 1), k);
                }
                cur_sum += M4_pow10[k][buffer[i - k]];
                cur_sum = (cur_sum >= M4) ? cur_sum - M4 : cur_sum;
            }
#endif

#ifdef __M4_HASH__
            M4_num[i] = M4_num[i - 1] + M4_inv10_power_k[i - pos + N][buffer[i]];
            M4_num[i] = M4_num[i] >= M4 ? M4_num[i] - M4 : M4_num[i];
            uint64_t M4_hash_value = (M4_num[i] & 33554431LL);
            {
                int prev_loc = M4_hash_map[M4_hash_value];
                if (unlikely(i - prev_loc <= N && i - prev_loc >= M4_len && buffer[prev_loc + 1] != '0' && M4_num[prev_loc] == M4_num[i]))
                {
                    int k = i - prev_loc;
                    int suffix_zeros = 0;
                    while (true)
                    {
                        if (likely(i + suffix_zeros > last_M4_ans))
                        {
                            sent_time[sent_time_cnt++] = get_timestamp();
                            submit_ans(buffer + prev_loc + 1, k);
                        }
                        suffix_zeros++;
                        if (buffer[i + suffix_zeros] != '0')
                            break;
                        k++;
                    }
                }
            }
            M4_hash_map[M4_hash_value] = i;
#endif
        }

        for (int i = pos; i < pos + n; i++)
        {
            int digit = buffer[i] - '0';
#ifdef __M2__
            M2_cur_suffix = (M2_cur_suffix - M2_pow10[M2_len - 1][buffer[i - M2_len]]) * 10 + digit;
            __int128_t cur_sum = M2_cur_suffix;
            for (int k = M2_len; k <= N; k++)
            {
                if (unlikely(cur_sum == 0 && buffer[i - k + 1] != '0'))
                {
                    sent_time[sent_time_cnt++] = get_timestamp();
                    submit_ans(buffer + (i - k + 1), k);
                }
                cur_sum += M2_pow10[k][buffer[i - k]];
                cur_sum = (cur_sum >= M2) ? cur_sum - M2 : cur_sum;
            }
#endif

#ifdef __M2_HASH__
            M2_num[i] = M2_num[i - 1] + M2_inv10_power_k[i - pos + N][buffer[i]];
            M2_num[i] = M2_num[i] >= M2 ? M2_num[i] - M2 : M2_num[i];

            __uint128_t M2_hash_value = (M2_num[i] & 33554431LL);
            {
                int prev_loc = M2_hash_map[M2_hash_value];
                if (unlikely(i - prev_loc <= N && buffer[prev_loc + 1] != '0' && M2_num[prev_loc] == M2_num[i]))
                {
                    int k = i - prev_loc;
                    int suffix_zeros = 0;
                    while (true)
                    {
                        if (likely(i + suffix_zeros > last_M2_ans))
                        {
                            sent_time[sent_time_cnt++] = get_timestamp();
                            submit_ans(buffer + prev_loc + 1, k);
                        }
                        suffix_zeros++;
                        if (buffer[i + suffix_zeros] != '0')
                            break;
                        k++;
                    }
                }
            }
            M2_hash_map[M2_hash_value] = i;
#endif
        }
        // guess

#ifdef __M4_GUESS__
        for (int guess_len = 1; guess_len <= M4G_guess_len; guess_len++)
        {
            int256_t cur_sum = M4_pow10[guess_len][buffer[pos + n - 1]];
            for (int k = 1; k <= N - guess_len; k++)
            {
                if (unlikely(M4G - cur_sum > 0 && M4G - cur_sum < M4_pow10[guess_len]['1'] && buffer[pos + n - k] != '0'))
                {
                    const char *append = (M4G - cur_sum).str().c_str();
                    if (guess_len > strlen(append))
                        memset(buffer + pos + n, '0', guess_len - strlen(append));
                    sprintf((char *)buffer + pos + n + guess_len - strlen(append), "%.*s", strlen(append), append);
                    if (pos + n + guess_len - 1 > last_M4_ans)
                    {
                        sent_time[sent_time_cnt++] = received_time;
                        submit_ans(buffer + pos + n - k, k + guess_len);
                        printf("GUESS!\n");
                        last_M4_ans = pos + n + guess_len - 1;
                    }
                    break;
                }
                cur_sum = cur_sum + M4_pow10[guess_len + k][buffer[pos + n - 1 - k]];
                cur_sum = cur_sum >= M4G ? cur_sum - M4G : cur_sum;
            }
            if (last_M4_ans >= pos + n + guess_len - 1)
                break;
        }
#endif

#ifdef __M3_GUESS__
        for (int guess_len = 1; guess_len <= M3G_guess_len; guess_len++)
        {
            int256_t cur_sum = M3_pow10[guess_len][buffer[pos + n - 1]];
            for (int k = 1; k <= N - guess_len; k++)
            {
                if (unlikely(M3G - cur_sum > 0 && M3G - cur_sum < M3_pow10[guess_len]['1'] && buffer[pos + n - k] != '0'))
                {
                    const char *append = (M3G - cur_sum).str().c_str();
                    if (guess_len > strlen(append))
                        memset(buffer + pos + n, '0', guess_len - strlen(append));
                    sprintf((char *)buffer + pos + n + guess_len - strlen(append), "%.*s", strlen(append), append);
                    if (pos + n + guess_len - 1 > last_M3_ans)
                    {
                        sent_time[sent_time_cnt++] = received_time;
                        submit_ans(buffer + pos + n - k, k + guess_len);
                        printf("GUESS!\n");
                        last_M3_ans = pos + n + guess_len - 1;
                    }
                    break;
                }
                cur_sum = cur_sum + M3_pow10[guess_len + k][buffer[pos + n - 1 - k]];
                cur_sum = cur_sum >= M3G ? cur_sum - M3G : cur_sum;
            }
            if (last_M3_ans >= pos + n + guess_len - 1)
                break;
        }
#endif

#ifdef __M2_GUESS__
        for (int guess_len = 1; guess_len <= M2G_guess_len; guess_len++)
        {
            int128_t cur_sum = M2_pow10[guess_len][buffer[pos + n - 1]];
            for (int k = 1; k <= N - guess_len; k++)
            {
                if (unlikely(M2G - cur_sum > 0 && M2G - cur_sum < M2_pow10[guess_len]['1'] && buffer[pos + n - k] != '0'))
                {
                    const char *append = (M2G - cur_sum).str().c_str();
                    if (guess_len > strlen(append))
                        memset(buffer + pos + n, '0', guess_len - strlen(append));
                    sprintf((char *)buffer + pos + n + guess_len - strlen(append), "%.*s", strlen(append), append);
                    if (pos + n + guess_len - 1 > last_M2_ans)
                    {
                        sent_time[sent_time_cnt++] = received_time;
                        submit_ans(buffer + pos + n - k, k + guess_len);
                        printf("GUESS!\n");
                        last_M2_ans = pos + n + guess_len - 1;
                    }
                    break;
                }
                cur_sum = cur_sum + M2_pow10[guess_len + k][buffer[pos + n - 1 - k]];
                cur_sum = cur_sum >= M2G ? cur_sum - M2G : cur_sum;
            }
            if (last_M2_ans >= pos + n + guess_len - 1)
                break;
        }
#endif

        // finalize
        if (sent_time_cnt > 0)
        {
            for (int i = 0; i < sent_time_cnt; i++)
            {
                printf("%.6lf %.6lf %.6lf\n", sent_time[i] - received_time, sent_time[i], received_time);
                total_sent_time += sent_time[i] - received_time;
            }
            total_sent_time_cnt += sent_time_cnt;
            printf("%.6lf\n", total_sent_time / total_sent_time_cnt);
        }

        for (int i = 0; i < RECV_FD_N; i++)
            if (i != recv_fd_idx)
            {
                int cum_n = 0;
                while (cum_n < n)
                {
                    int ni = read(recv_fds[i], tmp_buffer, n - cum_n);
                    if (ni > 0)
                        cum_n += ni;
                    else if (ni != -1 || errno != EAGAIN)
                    {
                        printf("Read Error %d %d\n", n, errno);
                        exit(-1);
                    }
                }
            }
        pos += n;
    }
    printf("Full\n");
    return 0;
}