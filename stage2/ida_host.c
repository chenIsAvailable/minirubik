#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    CUBIES = 7,
    PERMUTATIONS = 5040,
    ORIENTATIONS = 729,
    STATES = PERMUTATIONS * ORIENTATIONS,
    MOVES = 9
};

typedef struct {
    uint8_t p[CUBIES], o[CUBIES];
} state_t;

/*@ predicate valid_state(state_t *state) =
      (\forall integer i; 0 <= i < CUBIES ==>
         state->p[i] < CUBIES && state->o[i] < 3) &&
      (\forall integer i, j; 0 <= i < j < CUBIES ==>
         state->p[i] != state->p[j]) &&
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
 */

static uint16_t permutation[3][PERMUTATIONS], orientation[3][ORIENTATIONS];
static uint8_t pdb2[PERMUTATIONS * 9];                 /* 位置 + 1號、2號角塊的方向 */
static int use_pdb2 = 0;                               /* 1 = 用新牌子 */
static void unrank_state(uint32_t rank, state_t *state);   /* 預告:後面會定義 */
static uint32_t pdb2_index(uint32_t rank);                 /* 預告:後面會定義 */
static int oracle_dist(const uint8_t *table, uint32_t r);  /* 預告:後面會定義 */
static uint8_t perm_dist[PERMUTATIONS];     /* 只看位置:每種排列最少要幾步 */
static uint8_t orient_dist[ORIENTATIONS];   /* 只看方向:每種方向最少要幾步 */

static void bfs_small(int n, const uint16_t *tab, uint8_t *dist)
{
    uint16_t queue[PERMUTATIONS];
    int head = 0, tail = 0;
    memset(dist, 0xFF, (size_t) n);      /* 全部填 255 = 還沒找到 */
    dist[0] = 0;                         /* 已還原是 0 步 */
    queue[tail++] = 0;                   /* 放進待辦清單 */
    while (head < tail) {
        uint16_t x = queue[head++];      /* 從前面拿一個 */
        for (int face = 0; face < 3; ++face) {
            uint16_t y = x;
            for (int turn = 0; turn < 3; ++turn) {
                y = tab[face * n + y];       /* 查第 face 面、第 y 格 */
                if (dist[y] == 0xFF) {       /* 沒見過 */
                    dist[y] = dist[x] + 1;       /* 比 x 多一步 */
                    queue[tail++] = y;       /* 放到後面 */
                }
            }
        }
    }
}

static unsigned long long nodes;   /* 這次搜尋總共檢查了幾個節點 */

/* 牌子:兩張表取最大值 */
/*
static uint8_t h(uint16_t p, uint16_t o)
{
    uint8_t a = perm_dist[p], b = orient_dist[o];
    return a > b ? a : b;
}*/



static uint8_t h(uint16_t p, uint16_t o)
{
    uint8_t a = use_pdb2 ? pdb2[pdb2_index((uint32_t) p * ORIENTATIONS + o)]
                         : perm_dist[p];
    uint8_t b = orient_dist[o];
    return a > b ? a : b;
}

/* 深度限制 DFS:回傳 1 = 找到 */
static int dfs(uint16_t p, uint16_t o, int g, int bound, int last_face)
{
    nodes++;
    if (g + h(p, o) > bound) return 0;            /* ★ 剪枝:g + h > 上限就放棄 */
    if (p == 0 && o == 0) return 1;               /* 已還原,找到了 */
    for (int face = 0; face < 3; ++face) {
        if (face == last_face) continue;          /* ★ 同一面不連轉 */
        uint16_t np = p, no = o;
        for (int turn = 0; turn < 3; ++turn) {    /* 90°、180°、270° */
            np = permutation[face][np];
            no = orientation[face][no];
            if (dfs(np, no, g + 1, bound, face)) return 1;
        }
    }
    return 0;
}

/* IDA*:上限從 h(起點) 開始,一輪一輪加 1 */
static int ida(uint32_t rank)
{
    uint16_t p = (uint16_t) (rank / ORIENTATIONS), o = (uint16_t) (rank % ORIENTATIONS);
    for (int bound = h(p, o); ; ++bound)          /* ★ 從 h(起點) 開始 */
        if (dfs(p, o, 0, bound, -1)) return bound;
}

/* 標準答案:沿著原始 BFS 的路標走,數幾步回到已還原 */
static int oracle_dist(const uint8_t *table, uint32_t r)
{
    int d = 0;
    while (r) {
        uint8_t m = table[r];
        uint16_t p = (uint16_t) (r / ORIENTATIONS), o = (uint16_t) (r % ORIENTATIONS);
        for (int k = 0; k <= m % 3; ++k) {
            p = permutation[m / 3][p];
            o = orientation[m / 3][o];
        }
        r = (uint32_t) p * ORIENTATIONS + o;
        d++;
    }
    return d;
}



/* 算出一個狀態在 pdb2 的哪一格 */
static uint32_t pdb2_index(uint32_t rank)
{
    state_t s;
    unrank_state(rank, &s);                       /* 編號 → 陣列 */
    uint8_t t0 = 0, t1 = 0;
    for (int i = 0; i < CUBIES; ++i) {            /* 找出 1號、2號角塊在哪,讀它們的方向 */
        if (s.p[i] == 0) t0 = s.o[i];
        if (s.p[i] == 1) t1 = s.o[i];
    }
    return (rank / ORIENTATIONS) * 9 + t0 * 3 + t1;   /* 位置編號 × 9 + 兩個方向 */
}

/* 建 pdb2:每格存「落在這格的狀態中,最小的真實步數」 */
static void build_pdb2(const uint8_t *table)
{
    memset(pdb2, 0xFF, sizeof pdb2);
    for (uint32_t r = 0; r < STATES; ++r) {
        uint32_t k = pdb2_index(r);
        uint8_t d = (uint8_t) oracle_dist(table, r);
        if (d < pdb2[k]) pdb2[k] = d;
    }
}


static const char *const move_names[MOVES] = {"R",  "R2", "R'", "B", "B2",
                                              "B'", "D",  "D2", "D'"};
static const uint8_t inverse_move[MOVES] = {2, 1, 0, 5, 4, 3, 8, 7, 6};
/* Each destination takes a cubie from source[face][destination]. */
static const uint8_t source[3][CUBIES] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};
static const uint8_t twist[3][CUBIES] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};

/* The three quarter-turns preserve the fixed front-upper-left corner. */
/*@ requires face < 3;
    assigns \nothing;
    ensures \forall integer i; 0 <= i < CUBIES ==>
              \result.p[i] == state.p[source[face][i]];
    ensures \forall integer i; 0 <= i < CUBIES ==>
              \result.o[i] == (state.o[source[face][i]] + twist[face][i]) % 3;
 */
static state_t quarter_turn(state_t state, uint8_t face)
{
    state_t result;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant \forall integer j; 0 <= j < i ==>
          result.p[j] == state.p[source[face][j]];
        loop invariant \forall integer j; 0 <= j < i ==>
          result.o[j] == (state.o[source[face][j]] + twist[face][j]) % 3;
        loop assigns i, result.p[0..6], result.o[0..6];
        loop variant CUBIES - i;
    */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t from = source[face][i];
        result.p[i] = state.p[from];
        result.o[i] = (uint8_t) ((state.o[from] + twist[face][i]) % 3U);
    }
    return result;
}

static state_t apply_move(state_t state, uint8_t move)
{
    uint8_t turns = (uint8_t) (move % 3U + 1U);
    for (uint8_t i = 0; i < turns; ++i)
        state = quarter_turn(state, (uint8_t) (move / 3U));
    return state;
}

/*@ requires \valid_read(state);
    requires \forall integer i; 0 <= i < CUBIES ==>
      0 <= state->p[i] < CUBIES;
    requires \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    requires \forall integer i; 0 <= i < CUBIES ==>
      0 <= state->o[i] < 3;
    assigns \nothing;
    ensures \result < STATES;
 */
static uint32_t rank_state(const state_t *state)
{
    uint32_t p = 0, o = 0;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant (i == 0 ==> p == 0) && (i == 1 ==> p <= 6) &&
          (i == 2 ==> p <= 41) && (i == 3 ==> p <= 209) &&
          (i == 4 ==> p <= 839) && (i == 5 ==> p <= 2519) &&
          (i >= 6 ==> p <= 5039);
        loop assigns i, p;
        loop variant CUBIES - i;
     */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t smaller = 0;
        /*@ loop invariant i + 1 <= j <= CUBIES;
            loop invariant smaller <= j - i - 1;
            loop assigns j, smaller;
            loop variant CUBIES - j;
         */
        for (uint8_t j = (uint8_t) (i + 1U); j < CUBIES; ++j)
            if (state->p[j] < state->p[i])
                ++smaller;
        p = p * (CUBIES - i) + smaller;
    }
    /*@ loop invariant 0 <= i <= 6;
        loop invariant (i == 0 ==> o == 0) && (i == 1 ==> o < 3) &&
          (i == 2 ==> o < 9) && (i == 3 ==> o < 27) &&
          (i == 4 ==> o < 81) && (i == 5 ==> o < 243) &&
          (i == 6 ==> o < 729);
        loop assigns i, o;
        loop variant 6 - i;
     */
    for (uint8_t i = 0; i < 6; ++i)
        o = o * 3U + state->o[i];
    return p * ORIENTATIONS + o;
}

/*@ requires \valid(state); requires rank < STATES; assigns *state; */
static void unrank_state(uint32_t rank, state_t *state)
{
    uint8_t available[CUBIES] = {0, 1, 2, 3, 4, 5, 6};
    uint32_t p = rank / ORIENTATIONS, o = rank % ORIENTATIONS, f = 720;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t q = (uint8_t) (p / f);
        p %= f;
        state->p[i] = available[q];
        for (uint8_t j = q; j + 1U < CUBIES - i; ++j)
            available[j] = available[j + 1U];
        if (i < 5)
            f /= 6U - i;
    }
    for (uint8_t i = 6; i-- > 0;) {
        state->o[i] = (uint8_t) (o % 3U);
        sum = (uint8_t) (sum + state->o[i]);
        o /= 3U;
    }
    state->o[6] = (uint8_t) ((3U - sum % 3U) % 3U);
}

/*@ requires \valid_read(state);
    requires \initialized(&state->p[0..6]) && \initialized(&state->o[0..6]);
    assigns \nothing;
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] < CUBIES && state->o[i] < 3;
    ensures \result != 0 ==> \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    ensures \result != 0 ==>
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
    ensures complete: valid_state(state) ==> \result != 0;
 */
static int valid(const state_t *state)
{
    uint8_t sum = 0;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant sum <= 2 * i;
        loop invariant sum == (i > 0 ? state->o[0] : 0) +
          (i > 1 ? state->o[1] : 0) + (i > 2 ? state->o[2] : 0) +
          (i > 3 ? state->o[3] : 0) + (i > 4 ? state->o[4] : 0) +
          (i > 5 ? state->o[5] : 0) + (i > 6 ? state->o[6] : 0);
        loop invariant \forall integer j; 0 <= j < i ==>
          state->p[j] < CUBIES && state->o[j] < 3;
        loop invariant \forall integer j, k; 0 <= j < k < i ==>
          state->p[j] != state->p[k];
        loop assigns i, sum;
        loop variant CUBIES - i;
    */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        if (state->p[i] >= CUBIES || state->o[i] >= 3)
            return 0;
        /*@ loop invariant 0 <= j <= i;
            loop invariant \forall integer k; 0 <= k < j ==>
              state->p[k] != state->p[i];
            loop assigns j;
            loop variant i - j;
        */
        for (uint8_t j = 0; j < i; ++j)
            if (state->p[j] == state->p[i])
                return 0;
        sum = (uint8_t) (sum + state->o[i]);
    }
    return sum % 3U == 0;
}



static uint8_t *build_table(uint8_t *diameter)
{
    uint8_t *toward_solved = malloc(STATES);
    uint32_t *queue = malloc((size_t) STATES * sizeof *queue);
    //uint16_t permutation[3][PERMUTATIONS], orientation[3][ORIENTATIONS];
    uint32_t head = 0, tail = 1, level_end = 1;
    state_t state;
    if (!toward_solved || !queue) {
        free(toward_solved);
        free(queue);
        return NULL;
    }
    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {
        unrank_state((uint32_t) rank * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            permutation[face][rank] =
                (uint16_t) (rank_state(&next) / ORIENTATIONS);
        }
    }
    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {
        unrank_state(rank, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            orientation[face][rank] =
                (uint16_t) (rank_state(&next) % ORIENTATIONS);
        }
    }
    memset(toward_solved, UINT8_MAX, STATES);
    queue[0] = 0;
    toward_solved[0] = 0;
    *diameter = 0;
    while (head < tail) {
        if (head == level_end) {
            level_end = tail;
            ++*diameter;
        }
        uint32_t here = queue[head++];
        uint16_t p = (uint16_t) (here / ORIENTATIONS);
        uint16_t o = (uint16_t) (here % ORIENTATIONS);
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next_p = p, next_o = o;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next_p = permutation[face][next_p];
                next_o = orientation[face][next_o];
                uint32_t there = (uint32_t) next_p * ORIENTATIONS + next_o;
                if (toward_solved[there] == UINT8_MAX) {
                    uint8_t move = (uint8_t) (face * 3U + turn);
                    toward_solved[there] = inverse_move[move];
                    queue[tail++] = there;
                }
            }
        }
    }
    free(queue);
    if (tail != STATES) {
        free(toward_solved);
        return NULL;
    }
    return toward_solved;
}

/*@ requires valid_read_string(input);
    requires \valid(state);
    assigns state->p[0..6], state->o[0..6];
    ensures \result != 0 ==> input[14] == '\0';
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] < CUBIES && state->o[i] < 3;
    ensures \result != 0 ==> \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    ensures \result != 0 ==>
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] == input[i] - '1';
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->o[i] == input[i + CUBIES] - '1';
 */
static int parse_state(const char *input, state_t *state)
{
    /*@ loop invariant 0 <= i <= 14;
        loop invariant i <= strlen(input);
        loop invariant i <= 7 ==> \initialized(&state->p[0..i-1]);
        loop invariant i >= 7 ==> \initialized(&state->p[0..6]);
        loop invariant i >= 7 ==> \initialized(&state->o[0..i-8]);
        loop invariant \forall integer j; 0 <= j < i && j < CUBIES ==>
          state->p[j] == input[j] - '1';
        loop invariant \forall integer j; 0 <= j < i - CUBIES ==>
          state->o[j] == input[j + CUBIES] - '1';
        loop assigns i, state->p[0..6], state->o[0..6];
        loop variant 14 - i;
     */
    for (int i = 0; i < 14; ++i) {
        int limit = i < 7 ? 7 : 3;
        if (input[i] < '1' || input[i] > '0' + limit)
            return 0;
        (i < 7 ? state->p : state->o)[i % 7] = (uint8_t) (input[i] - '1');
    }
    return input[14] == '\0' && valid(state);
}

/* stdout is fully buffered off a terminal, so a write error surfaces at the
 * flush, not at the printf that queued the bytes. Every exit path that has
 * produced output goes through here.
 */
static int output_failed(void)
{
    return fflush(stdout) != 0 || ferror(stdout);
}

static int self_test(void)
{
    const state_t solved = {{0, 1, 2, 3, 4, 5, 6}, {0}};
    state_t state;
    for (uint8_t move = 0; move < MOVES; ++move) {
        state = solved;
        state = apply_move(state, move);
        state = apply_move(state, inverse_move[move]);
        if (memcmp(&solved, &state, sizeof solved))
            return 0;
    }
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        unrank_state(rank, &state);
        if (!valid(&state) || rank_state(&state) != rank)
            return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--pdb") == 0) {
        uint8_t diameter;
        uint8_t *table = build_table(&diameter);
        bfs_small(PERMUTATIONS, &permutation[0][0], perm_dist);
        bfs_small(ORIENTATIONS, &orientation[0][0], orient_dist);
        int cp[16] = {0}, co[16] = {0};
        for (int i = 0; i < PERMUTATIONS; ++i) cp[perm_dist[i]]++;
        for (int i = 0; i < ORIENTATIONS; ++i) co[orient_dist[i]]++;
        for (int d = 0; d < 12; ++d)
            printf("%2d: perm %4d   orient %3d\n", d, cp[d], co[d]);
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

    state_t state;
    uint8_t diameter;
    if (argc == 2 && !strcmp(argv[1], "--self-test")) {
        if (!self_test()) {
            fputs("self-test failed\n", stderr);
            return 1;
        }
        uint8_t *table = build_table(&diameter);
        if (!table) {
            fputs("could not build complete state table\n", stderr);
            return 1;
        }
        free(table);
        if (diameter != 11) {
            fputs("BFS check failed\n", stderr);
            return 1;
        }
        puts("3674160 states; diameter 11");
        return output_failed();
    }
    if (argc != 2 || !parse_state(argv[1], &state)) {
        /* C99 5.1.2.2.1 lets argv[0] be null when argc is 0. */
        fprintf(stderr, "usage: %s PPPPPPPOOOOOOO\n",
                argc > 0 && argv[0] ? argv[0] : "solver");
        return 2;
    }
    uint8_t *table = build_table(&diameter);
    if (!table) {
        fputs("could not build complete state table\n", stderr);
        return 1;
    }
    const char *separator = "";
    for (uint32_t rank = rank_state(&state); rank; rank = rank_state(&state)) {
        uint8_t move = table[rank];
        printf("%s%s", separator, move_names[move]);
        separator = " ";
        state = apply_move(state, move);
    }
    putchar('\n');
    free(table);
    return output_failed();
}
