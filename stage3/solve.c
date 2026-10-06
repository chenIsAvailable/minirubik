/* Stage 3: target-shaped C version of the stage 2 search.
 * No heap, no recursion, no floating point, no multiply or divide.
 * Build for the host with -DHOST to print results; build for RV32I without it.
 */
#include <stdint.h>
#include "tables.h"

#ifndef STATE
#define STATE "21345671111111"          /* 14-character state, inlined at build time */
#endif

#define MAXD 12

static const uint16_t *const ptab[3] = {perm_R, perm_B, perm_D};
static const uint16_t *const otab[3] = {orient_R, orient_B, orient_D};

/* LEHMER[i][c] = c * (6 - i)!, so ranking needs adds only */
static const uint16_t LEHMER[7][7] = {
    {0, 720, 1440, 2160, 2880, 3600, 4320},
    {0, 120, 240, 360, 480, 600, 0},
    {0, 24, 48, 72, 96, 0, 0},
    {0, 6, 12, 18, 0, 0, 0},
    {0, 2, 4, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0},
};

unsigned long nodes;                    /* calls counted the same way as stage 2 */
uint8_t move_face[MAXD], move_turn[MAXD];
int result_len = -1;

static uint8_t h(uint16_t p, uint16_t o)
{
    uint32_t row = (uint32_t) o << 3;                     /* o * 8: twist_at8 is padded to 8 */
    uint32_t t0 = twist_at8[row + pos0[p]];
    uint32_t t1 = twist_at8[row + pos1[p]];
    uint32_t k = ((uint32_t) p << 3) + p                  /* p * 9 */
               + (t0 << 1) + t0 + t1;                      /* t0 * 3 + t1 */
    uint8_t a = pdb2[k], b = orient_dist[o];
    return a > b ? a : b;
}

/* Returns 0 if the string is not a valid state. */
static int parse(const char *s, uint16_t *pr, uint16_t *or_)
{
    uint8_t p[7], seen = 0;
    uint32_t perm = 0, orient = 0, sum = 0;
    for (int i = 0; i < 7; ++i) {
        uint8_t c = (uint8_t) (s[i] - '1');
        if (c > 6 || (seen >> c) & 1) return 0;
        seen |= (uint8_t) (1u << c);
        p[i] = c;
    }
    for (int i = 0; i < 7; ++i) {
        uint8_t smaller = 0;
        for (int j = i + 1; j < 7; ++j) smaller += p[j] < p[i];
        perm += LEHMER[i][smaller];
    }
    for (int i = 7; i < 14; ++i) {
        uint32_t d = (uint32_t) (s[i] - '1');
        if (d > 2) return 0;
        sum += d;
        if (i < 13) orient = (orient << 1) + orient + d;    /* orient * 3 + d */
    }
    while (sum >= 3) sum -= 3;                              /* sum mod 3 without divide */
    if (sum || s[14]) return 0;
    *pr = (uint16_t) perm;
    *or_ = (uint16_t) orient;
    return 1;
}

/* Iterative IDA*: an explicit per-depth iterator replaces recursion. */
static int search(uint16_t p0, uint16_t o0)
{
    uint16_t P[MAXD], O[MAXD], CP[MAXD], CO[MAXD];
    uint8_t F[MAXD], T[MAXD], LAST[MAXD];
    for (int bound = h(p0, o0); bound < MAXD; ++bound) {
        nodes++;                                            /* the root */
        if (p0 == 0 && o0 == 0) return 0;
        int d = 0;
        P[0] = CP[0] = p0; O[0] = CO[0] = o0;
        F[0] = 0; T[0] = 0; LAST[0] = 3;
        while (d >= 0) {
            uint8_t f = F[d];
            if (f == 3) { d--; continue; }                  /* all faces tried: back up */
            if (f == LAST[d]) { F[d] = (uint8_t) (f + 1); continue; }   /* same face twice */
            uint16_t np = ptab[f][CP[d]], no = otab[f][CO[d]];
            uint8_t t = T[d];
            if (t == 2) { F[d] = (uint8_t) (f + 1); T[d] = 0; CP[d] = P[d]; CO[d] = O[d]; }
            else { T[d] = (uint8_t) (t + 1); CP[d] = np; CO[d] = no; }
            nodes++;
            if (d + 1 + h(np, no) > bound) continue;        /* prune */
            move_face[d] = f; move_turn[d] = t;
            if (np == 0 && no == 0) return d + 1;           /* solved */
            d++;
            P[d] = CP[d] = np; O[d] = CO[d] = no;
            F[d] = 0; T[d] = 0; LAST[d] = f;
        }
    }
    return -1;
}

/* Replays the solution with the tables; 1 if it reaches solved. */
static int verify(uint16_t p, uint16_t o, int len)
{
    for (int k = 0; k < len; ++k)
        for (int t = 0; t <= move_turn[k]; ++t) {
            p = ptab[move_face[k]][p];
            o = otab[move_face[k]][o];
        }
    return p == 0 && o == 0;
}

#ifdef HOST
#include <stdio.h>
int main(int argc, char **argv)
{
    static const char *name[3][3] = {{"R", "R2", "R'"}, {"B", "B2", "B'"}, {"D", "D2", "D'"}};
    const char *s = argc > 1 ? argv[1] : STATE;
    uint16_t p, o;
    if (!parse(s, &p, &o)) { puts("invalid state"); return 2; }
    result_len = search(p, o);
    for (int k = 0; k < result_len; ++k) printf("%s%s", k ? " " : "", name[move_face[k]][move_turn[k]]);
    printf("\n%d moves, %lu nodes, %s\n", result_len, nodes, verify(p, o, result_len) ? "verified" : "FAILED");
    return 0;
}
#else
int main(void)
{
    uint16_t p, o;
    if (!parse(STATE, &p, &o)) return 2;
    result_len = search(p, o);
    return verify(p, o, result_len) ? 0 : 1;
}
#endif
