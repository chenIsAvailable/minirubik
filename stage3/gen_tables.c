/* Stage 3 table generator: builds every table on the host, checks them (--gates),
 * and writes them as C arrays (--emit FILE). Derived from stage2/ida_host.c. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { CUBIES = 7, PERMUTATIONS = 5040, ORIENTATIONS = 729, STATES = PERMUTATIONS * ORIENTATIONS, MOVES = 9 };
typedef struct { uint8_t p[CUBIES], o[CUBIES]; } state_t;
static uint16_t permutation[3][PERMUTATIONS], orientation[3][ORIENTATIONS];
static uint8_t pdb2[PERMUTATIONS * 9];
static int use_pdb2 = 0;
static void unrank_state(uint32_t rank, state_t *state);
static uint32_t pdb2_index(uint32_t rank);
static int oracle_dist(const uint8_t *table, uint32_t r);
static uint8_t perm_dist[PERMUTATIONS];
static uint8_t orient_dist[ORIENTATIONS];
static void bfs_small(int n, const uint16_t *tab, uint8_t *dist)
{
    uint16_t queue[PERMUTATIONS];
    int head = 0, tail = 0;
    memset(dist, 0xFF, (size_t) n);
    dist[0] = 0;
    queue[tail++] = 0;
    while (head < tail) {
        uint16_t x = queue[head++];
        for (int face = 0; face < 3; ++face) {
            uint16_t y = x;
            for (int turn = 0; turn < 3; ++turn) {
                y = tab[face * n + y];
                if (dist[y] == 0xFF) { dist[y] = dist[x] + 1; queue[tail++] = y; }
            }
        }
    }
}
static uint8_t pos0[PERMUTATIONS], pos1[PERMUTATIONS], twist_at[ORIENTATIONS][CUBIES];
static void build_index_tables(void)
{
    state_t s;
    for (uint32_t p = 0; p < PERMUTATIONS; ++p) {
        unrank_state(p * ORIENTATIONS, &s);
        for (uint8_t i = 0; i < CUBIES; ++i) { if (s.p[i] == 0) pos0[p] = i; if (s.p[i] == 1) pos1[p] = i; }
    }
    for (uint32_t o = 0; o < ORIENTATIONS; ++o) {
        unrank_state(o, &s);
        for (uint8_t i = 0; i < CUBIES; ++i) twist_at[o][i] = s.o[i];
    }
}
static uint32_t pdb2_fast(uint16_t p, uint16_t o)
{
    return (uint32_t) p * 9 + twist_at[o][pos0[p]] * 3 + twist_at[o][pos1[p]];
}
static unsigned long long nodes;
static uint8_t h(uint16_t p, uint16_t o)
{
    uint8_t a = use_pdb2 ? pdb2[pdb2_fast(p, o)] : perm_dist[p];
    uint8_t b = orient_dist[o];
    return a > b ? a : b;
}
static uint8_t path[16];
static int dfs(uint16_t p, uint16_t o, int g, int bound, int last_face)
{
    nodes++;
    if (g + h(p, o) > bound) return 0;
    if (p == 0 && o == 0) return 1;
    for (int face = 0; face < 3; ++face) {
        if (face == last_face) continue;
        uint16_t np = p, no = o;
        for (int turn = 0; turn < 3; ++turn) {
            np = permutation[face][np];
            no = orientation[face][no];
            path[g] = (uint8_t) (face * 3 + turn);
            if (dfs(np, no, g + 1, bound, face)) return 1;
        }
    }
    return 0;
}
static int ida(uint32_t rank)
{
    uint16_t p = (uint16_t) (rank / ORIENTATIONS), o = (uint16_t) (rank % ORIENTATIONS);
    for (int bound = h(p, o); ; ++bound)
        if (dfs(p, o, 0, bound, -1)) return bound;
}
static int oracle_dist(const uint8_t *table, uint32_t r)
{
    int d = 0;
    while (r) {
        uint8_t m = table[r];
        uint16_t p = (uint16_t) (r / ORIENTATIONS), o = (uint16_t) (r % ORIENTATIONS);
        for (int k = 0; k <= m % 3; ++k) { p = permutation[m / 3][p]; o = orientation[m / 3][o]; }
        r = (uint32_t) p * ORIENTATIONS + o;
        d++;
    }
    return d;
}
static uint32_t pdb2_index(uint32_t rank)
{
    state_t s;
    unrank_state(rank, &s);
    uint8_t t0 = 0, t1 = 0;
    for (int i = 0; i < CUBIES; ++i) { if (s.p[i] == 0) t0 = s.o[i]; if (s.p[i] == 1) t1 = s.o[i]; }
    return (rank / ORIENTATIONS) * 9 + t0 * 3 + t1;
}
static void build_pdb2(const uint8_t *table)
{
    memset(pdb2, 0xFF, sizeof pdb2);
    for (uint32_t r = 0; r < STATES; ++r) {
        uint32_t k = pdb2_index(r);
        uint8_t d = (uint8_t) oracle_dist(table, r);
        if (d < pdb2[k]) pdb2[k] = d;
    }
}
static const uint8_t inverse_move[MOVES] = {2, 1, 0, 5, 4, 3, 8, 7, 6};
static const uint8_t source[3][CUBIES] = {{1, 4, 2, 0, 3, 5, 6},{0, 1, 2, 4, 5, 6, 3},{0, 2, 5, 3, 1, 4, 6}};
static const uint8_t twist[3][CUBIES] = {{1, 2, 0, 2, 1, 0, 0},{0, 0, 0, 1, 2, 1, 2},{0, 0, 0, 0, 0, 0, 0}};
static state_t quarter_turn(state_t state, uint8_t face)
{
    state_t result;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t from = source[face][i];
        result.p[i] = state.p[from];
        result.o[i] = (uint8_t) ((state.o[from] + twist[face][i]) % 3U);
    }
    return result;
}
static uint32_t rank_state(const state_t *state)
{
    uint32_t p = 0, o = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t smaller = 0;
        for (uint8_t j = (uint8_t) (i + 1U); j < CUBIES; ++j) if (state->p[j] < state->p[i]) ++smaller;
        p = p * (CUBIES - i) + smaller;
    }
    for (uint8_t i = 0; i < 6; ++i) o = o * 3U + state->o[i];
    return p * ORIENTATIONS + o;
}
static void unrank_state(uint32_t rank, state_t *state)
{
    uint8_t available[CUBIES] = {0, 1, 2, 3, 4, 5, 6};
    uint32_t p = rank / ORIENTATIONS, o = rank % ORIENTATIONS, f = 720;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t q = (uint8_t) (p / f);
        p %= f;
        state->p[i] = available[q];
        for (uint8_t j = q; j + 1U < CUBIES - i; ++j) available[j] = available[j + 1U];
        if (i < 5) f /= 6U - i;
    }
    for (uint8_t i = 6; i-- > 0;) { state->o[i] = (uint8_t) (o % 3U); sum = (uint8_t) (sum + state->o[i]); o /= 3U; }
    state->o[6] = (uint8_t) ((3U - sum % 3U) % 3U);
}
static uint8_t *build_table(uint8_t *diameter)
{
    uint8_t *toward_solved = malloc(STATES);
    uint32_t *queue = malloc((size_t) STATES * sizeof *queue);
    uint32_t head = 0, tail = 1, level_end = 1;
    state_t state;
    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {
        unrank_state((uint32_t) rank * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) { state_t next = quarter_turn(state, face); permutation[face][rank] = (uint16_t) (rank_state(&next) / ORIENTATIONS); }
    }
    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {
        unrank_state(rank, &state);
        for (uint8_t face = 0; face < 3; ++face) { state_t next = quarter_turn(state, face); orientation[face][rank] = (uint16_t) (rank_state(&next) % ORIENTATIONS); }
    }
    memset(toward_solved, UINT8_MAX, STATES);
    queue[0] = 0; toward_solved[0] = 0; *diameter = 0;
    while (head < tail) {
        if (head == level_end) { level_end = tail; ++*diameter; }
        uint32_t here = queue[head++];
        uint16_t p = (uint16_t) (here / ORIENTATIONS), o = (uint16_t) (here % ORIENTATIONS);
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next_p = p, next_o = o;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next_p = permutation[face][next_p]; next_o = orientation[face][next_o];
                uint32_t there = (uint32_t) next_p * ORIENTATIONS + next_o;
                if (toward_solved[there] == UINT8_MAX) { toward_solved[there] = inverse_move[face * 3U + turn]; queue[tail++] = there; }
            }
        }
    }
    free(queue);
    return toward_solved;
}

static void emit_u16(FILE *f, const char *name, const uint16_t *a, int n)
{
    fprintf(f, "static const uint16_t %s[%d] = {", name, n);
    for (int i = 0; i < n; ++i) fprintf(f, "%s%u", i == 0 ? "\n    " : (i % 16 ? "," : ",\n    "), a[i]);
    fprintf(f, "\n};\n");
}
static void emit_u8(FILE *f, const char *name, const uint8_t *a, int n)
{
    fprintf(f, "static const uint8_t %s[%d] = {", name, n);
    for (int i = 0; i < n; ++i) fprintf(f, "%s%u", i == 0 ? "\n    " : (i % 32 ? "," : ",\n    "), a[i]);
    fprintf(f, "\n};\n");
}

static void emit_asm(FILE *f, const char *dir, const char *name, const void *a, int n, int is16)
{
    fprintf(f, "%s:\n", name);
    for (int i = 0; i < n; ++i) {
        unsigned v = is16 ? ((const uint16_t *) a)[i] : ((const uint8_t *) a)[i];
        if (i % 16 == 0) fprintf(f, "%s    %s %u", i ? "\n" : "", dir, v);
        else fprintf(f, ", %u", v);
    }
    fprintf(f, "\n");
}
int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "--emit-asm") == 0) {
        uint8_t diameter;
        uint8_t *table = build_table(&diameter);
        bfs_small(PERMUTATIONS, &permutation[0][0], perm_dist);
        bfs_small(ORIENTATIONS, &orientation[0][0], orient_dist);
        build_index_tables();
        build_pdb2(table);
        static uint8_t twist_at8[ORIENTATIONS * 8];
        for (int o = 0; o < ORIENTATIONS; ++o)
            for (int i = 0; i < CUBIES; ++i) twist_at8[o * 8 + i] = twist_at[o][i];
        FILE *f = fopen(argv[2], "w");
        if (!f) return 1;
        fprintf(f, "# Generated by stage3/gen_tables.c --emit-asm. Do not edit.\n");
        fprintf(f, ".data\n");
        fprintf(f, "# 16-bit tables first, so every .half is 2-byte aligned.\n");
        fprintf(f, "# perm_R, perm_B, perm_D are contiguous (10080 bytes each); so are orient_R/B/D (1458 bytes each).\n");
        emit_asm(f, ".half", "perm_R", permutation[0], PERMUTATIONS, 1);
        emit_asm(f, ".half", "perm_B", permutation[1], PERMUTATIONS, 1);
        emit_asm(f, ".half", "perm_D", permutation[2], PERMUTATIONS, 1);
        emit_asm(f, ".half", "orient_R", orientation[0], ORIENTATIONS, 1);
        emit_asm(f, ".half", "orient_B", orientation[1], ORIENTATIONS, 1);
        emit_asm(f, ".half", "orient_D", orientation[2], ORIENTATIONS, 1);
        emit_asm(f, ".byte", "pdb2", pdb2, PERMUTATIONS * 9, 0);
        emit_asm(f, ".byte", "orient_dist", orient_dist, ORIENTATIONS, 0);
        emit_asm(f, ".byte", "pos0", pos0, PERMUTATIONS, 0);
        emit_asm(f, ".byte", "pos1", pos1, PERMUTATIONS, 0);
        emit_asm(f, ".byte", "twist_at8", twist_at8, ORIENTATIONS * 8, 0);
        fclose(f);
        free(table);
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--emit") == 0) {
        uint8_t diameter;
        uint8_t *table = build_table(&diameter);
        bfs_small(PERMUTATIONS, &permutation[0][0], perm_dist);
        bfs_small(ORIENTATIONS, &orientation[0][0], orient_dist);
        build_index_tables();
        build_pdb2(table);
        static uint8_t twist_at8[ORIENTATIONS * 8];
        for (int o = 0; o < ORIENTATIONS; ++o)
            for (int i = 0; i < CUBIES; ++i) twist_at8[o * 8 + i] = twist_at[o][i];
        FILE *f = fopen(argv[2], "w");
        if (!f) return 1;
        fprintf(f, "/* Generated by stage3/gen_tables.c --emit. Do not edit. */\n");
        emit_u16(f, "perm_R", permutation[0], PERMUTATIONS);
        emit_u16(f, "perm_B", permutation[1], PERMUTATIONS);
        emit_u16(f, "perm_D", permutation[2], PERMUTATIONS);
        emit_u16(f, "orient_R", orientation[0], ORIENTATIONS);
        emit_u16(f, "orient_B", orientation[1], ORIENTATIONS);
        emit_u16(f, "orient_D", orientation[2], ORIENTATIONS);
        emit_u8(f, "pdb2", pdb2, PERMUTATIONS * 9);
        emit_u8(f, "orient_dist", orient_dist, ORIENTATIONS);
        emit_u8(f, "pos0", pos0, PERMUTATIONS);
        emit_u8(f, "pos1", pos1, PERMUTATIONS);
        emit_u8(f, "twist_at8", twist_at8, ORIENTATIONS * 8);
        fclose(f);
        free(table);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--pdb") == 0) {
        uint8_t diameter;
        uint8_t *table = build_table(&diameter);
        bfs_small(PERMUTATIONS, &permutation[0][0], perm_dist);
        bfs_small(ORIENTATIONS, &orientation[0][0], orient_dist);
        int cp[16] = {0}, co[16] = {0};
        for (int i = 0; i < PERMUTATIONS; ++i) cp[perm_dist[i]]++;
        for (int i = 0; i < ORIENTATIONS; ++i) co[orient_dist[i]]++;
        for (int d = 0; d < 12; ++d) printf("%2d: perm %4d   orient %3d\n", d, cp[d], co[d]);
        free(table);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "--ida") == 0 || strcmp(argv[1], "--ida2") == 0)) {
        uint8_t diameter;
        uint8_t *table = build_table(&diameter);
        bfs_small(PERMUTATIONS, &permutation[0][0], perm_dist);
        bfs_small(ORIENTATIONS, &orientation[0][0], orient_dist);
        if (strcmp(argv[1], "--ida2") == 0) {
            build_pdb2(table);
            use_pdb2 = 1;
        }
        unsigned long long worst = 0, total = 0;
        uint32_t worst_rank = 0;
        int count = 0, bad = 0;
        for (uint32_t r = 0; r < STATES; ++r) {
            if (oracle_dist(table, r) != 11) continue;
            nodes = 0;
            if (ida(r) != 11) bad++;
            total += nodes;
            count++;
            if (nodes > worst) { worst = nodes; worst_rank = r; }
        }
        nodes = 0;
        int len = ida(524880);
        printf("distance-11 states: %d, wrong length: %d\n", count, bad);
        printf("nodes: max %llu (rank %u), avg %.0f\n", worst, (unsigned) worst_rank, (double) total / count);
        printf("21345671111111: %d moves, %llu nodes\n", len, nodes);
        free(table);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--gates") == 0) {
        uint8_t diameter;
        uint8_t *table = build_table(&diameter);
        bfs_small(PERMUTATIONS, &permutation[0][0], perm_dist);
        bfs_small(ORIENTATIONS, &orientation[0][0], orient_dist);
        build_index_tables();
        build_pdb2(table);
        use_pdb2 = 1;
        uint8_t *dist = malloc(STATES);
        for (uint32_t r = 0; r < STATES; ++r) dist[r] = (uint8_t) oracle_dist(table, r);
        /* H2 */
        int h2ok = 1; uint8_t mp = 0, mo = 0, m2 = 0;
        for (int i = 0; i < PERMUTATIONS; ++i) { if (perm_dist[i] == 0xFF) h2ok = 0; if (perm_dist[i] > mp) mp = perm_dist[i]; }
        for (int i = 0; i < ORIENTATIONS; ++i) { if (orient_dist[i] == 0xFF) h2ok = 0; if (orient_dist[i] > mo) mo = orient_dist[i]; }
        for (int i = 0; i < PERMUTATIONS * 9; ++i) { if (pdb2[i] == 0xFF) h2ok = 0; if (pdb2[i] > m2) m2 = pdb2[i]; }
        if (perm_dist[0] || orient_dist[0] || pdb2[0]) h2ok = 0;
        printf("H2 %s: max perm %u, orient %u, pdb2 %u; solved entries 0\n", h2ok ? "PASS" : "FAIL", mp, mo, m2);
        /* H1 */
        uint32_t bad1 = 0;
        for (uint32_t r = 0; r < STATES; ++r) if (h((uint16_t)(r / ORIENTATIONS), (uint16_t)(r % ORIENTATIONS)) > dist[r]) bad1++;
        printf("H1 %s: %u states with h > d\n", bad1 ? "FAIL" : "PASS", bad1);
        /* H3 */
        uint32_t bad3 = 0;
        for (uint32_t r = 0; r < STATES; ++r) {
            int len = ida(r);
            uint16_t p = (uint16_t)(r / ORIENTATIONS), o = (uint16_t)(r % ORIENTATIONS);
            for (int k = 0; k < len; ++k) { int f = path[k] / 3; for (int t = 0; t <= path[k] % 3; ++t) { p = permutation[f][p]; o = orientation[f][o]; } }
            if (len != dist[r] || p || o) bad3++;
        }
        printf("H3 %s: %u states wrong\n", bad3 ? "FAIL" : "PASS", bad3);
        return 0;
    }
    return 2;
}
