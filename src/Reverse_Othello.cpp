/*
	Reverse Othello

	@file Shortest_Draw_Othello.cpp
		Main file
	@date 2024
	@author Takuto Yamana
	@license GPL-3.0 license
*/

#include <iostream>
#include "engine/board.hpp"
#include "engine/util.hpp"


void init(){
    bit_init();
    mobility_init();
    flip_init();
}

void output_transcript(std::vector<int> &transcript){
    for (int &move: transcript){
        std::cout << idx_to_coord(move);
    }
    std::cout << std::endl;
}

inline uint64_t full_stability_h(uint64_t full){
    full &= full >> 1;
    full &= full >> 2;
    full &= full >> 4;
    return (full & 0x0101010101010101ULL) * 0xFF;
}

inline uint64_t full_stability_v(uint64_t full){
    full &= (full >> 8) | (full << 56);
    full &= (full >> 16) | (full << 48);
    full &= (full >> 32) | (full << 32);
    return full;
}

inline void full_stability_d(uint64_t full, uint64_t *full_d7, uint64_t *full_d9){
    constexpr uint64_t edge = 0xFF818181818181FFULL;
    uint64_t l7, r7, l9, r9;
    l7 = r7 = full;
    l7 &= edge | (l7 >> 7);		r7 &= edge | (r7 << 7);
    l7 &= 0xFFFF030303030303ULL | (l7 >> 14);	r7 &= 0xC0C0C0C0C0C0FFFFULL | (r7 << 14);
    l7 &= 0xFFFFFFFF0F0F0F0FULL | (l7 >> 28);	r7 &= 0xF0F0F0F0FFFFFFFFULL | (r7 << 28);
    *full_d7 = l7 & r7;

    l9 = r9 = full;
    l9 &= edge | (l9 >> 9);		r9 &= edge | (r9 << 9);
    l9 &= 0xFFFFC0C0C0C0C0C0ULL | (l9 >> 18);	r9 &= 0x030303030303FFFFULL | (r9 << 18);
    *full_d9 = l9 & r9 & (0x0F0F0F0FF0F0F0F0ULL | (l9 >> 36) | (r9 << 36));
}

inline void full_stability(uint64_t discs, uint64_t *h, uint64_t *v, uint64_t *d7, uint64_t *d9){
    *h = full_stability_h(discs);
    *v = full_stability_v(discs);
    full_stability_d(discs, d7, d9);
}

inline uint64_t enhanced_stability(Board *board, const uint64_t goal_mask){
    uint64_t full_h, full_v, full_d7, full_d9;
    uint64_t discs = board->player | board->opponent;
    full_stability(discs | ~goal_mask, &full_h, &full_v, &full_d7, &full_d9);
    full_h &= goal_mask;
    full_v &= goal_mask;
    full_d7 &= goal_mask;
    full_d9 &= goal_mask;
    uint64_t h, v, d7, d9;
    uint64_t stability = 0ULL, n_stability;
    n_stability = discs & (full_h & full_v & full_d7 & full_d9);
    while (n_stability & ~stability){
        stability |= n_stability;
        h = (stability >> 1) | (stability << 1) | full_h;
        v = (stability >> 8) | (stability << 8) | full_v;
        d7 = (stability >> 7) | (stability << 7) | full_d7;
        d9 = (stability >> 9) | (stability << 9) | full_d9;
        n_stability = h & v & d7 & d9;
    }
    return stability;
}

void find_path(Board *board, std::vector<int> &path, int player, const uint64_t goal_mask, const uint64_t corner_mask, const int goal_n_discs, const Board *goal){
    if (player == 0 && board->player == goal->player && board->opponent == goal->opponent){
        for (const int policy: path)
            std::cout << idx_to_coord(policy);
        std::cout << std::endl;
        return;
    }
    uint64_t goal_player, goal_opponent;
    if (player){
        goal_player = goal->opponent;
        goal_opponent = goal->player;
    } else{
        goal_player = goal->player;
        goal_opponent = goal->opponent;
    }
    uint64_t stable = enhanced_stability(board, goal_mask);
    if ((stable & board->player & goal_opponent) || (stable & board->opponent & goal_player))
        return;
    uint64_t legal = board->get_legal() & goal_mask & ~(corner_mask & goal_opponent);
    if (legal){
        Flip flip;
        for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)){;
            calc_flip(&flip, board, cell);
            board->move_board(&flip);
            path.emplace_back(cell);
                find_path(board, path, player ^ 1, goal_mask, corner_mask, goal_n_discs, goal);
            path.pop_back();
            board->undo_board(&flip);
        }
    }
}

int main(int argc, char* argv[]){
    init();
    std::cout << "please input the board" << std::endl;
    Board goal = input_board();
    //goal.print();
    for (int i = HW2_M1; i >= 0; --i){
        if (1 & (goal.player >> i))
            std::cout << "X ";
        else if (1 & (goal.opponent >> i))
            std::cout << "O ";
        else
            std::cout << ". ";
        if (i % HW == 0)
            std::cout << std::endl;
    }

    int player = 0;
    std::swap(goal.player, goal.opponent);

    uint64_t goal_mask = goal.player | goal.opponent; // legal candidate
    uint64_t framed_mask = 0ULL; // wall
    uint64_t goal_mask_h = goal_mask & 0x7E7E7E7E7E7E7E7EULL;
    uint64_t goal_mask_v = goal_mask & 0x00FFFFFFFFFFFF00ULL;
    uint64_t goal_mask_d = goal_mask & 0x007E7E7E7E7E7E00ULL;
    framed_mask |= (goal_mask_h << 1) | (goal_mask_h >> 1);
    framed_mask |= (goal_mask_v << 8) | (goal_mask_v >> 8);
    framed_mask |= (goal_mask_d << 7) | (goal_mask_d >> 7);
    framed_mask |= (goal_mask_d << 9) | (goal_mask_d >> 9);
    framed_mask &= ~goal_mask;
    uint64_t corner_mask = 0ULL; // cells that work as corner (non-flippable cells)
    corner_mask |= (framed_mask >> 1) & (framed_mask >> 8) & (framed_mask >> 9) & (framed_mask >> 7);
    corner_mask |= (framed_mask >> 1) & (framed_mask >> 8) & (framed_mask >> 9) & (framed_mask << 7);
    corner_mask |= (framed_mask >> 1) & (framed_mask >> 8) & (framed_mask << 9) & (framed_mask >> 7);
    corner_mask |= (framed_mask >> 1) & (framed_mask >> 8) & (framed_mask << 9) & (framed_mask << 7);
    corner_mask |= (framed_mask >> 1) & (framed_mask << 8) & (framed_mask >> 9) & (framed_mask >> 7);
    corner_mask |= (framed_mask >> 1) & (framed_mask << 8) & (framed_mask >> 9) & (framed_mask << 7);
    corner_mask |= (framed_mask >> 1) & (framed_mask << 8) & (framed_mask << 9) & (framed_mask >> 7);
    corner_mask |= (framed_mask >> 1) & (framed_mask << 8) & (framed_mask << 9) & (framed_mask << 7);
    corner_mask |= (framed_mask << 1) & (framed_mask >> 8) & (framed_mask >> 9) & (framed_mask >> 7);
    corner_mask |= (framed_mask << 1) & (framed_mask >> 8) & (framed_mask >> 9) & (framed_mask << 7);
    corner_mask |= (framed_mask << 1) & (framed_mask >> 8) & (framed_mask << 9) & (framed_mask >> 7);
    corner_mask |= (framed_mask << 1) & (framed_mask >> 8) & (framed_mask << 9) & (framed_mask << 7);
    corner_mask |= (framed_mask << 1) & (framed_mask << 8) & (framed_mask >> 9) & (framed_mask >> 7);
    corner_mask |= (framed_mask << 1) & (framed_mask << 8) & (framed_mask >> 9) & (framed_mask << 7);
    corner_mask |= (framed_mask << 1) & (framed_mask << 8) & (framed_mask << 9) & (framed_mask >> 7);
    corner_mask |= (framed_mask << 1) & (framed_mask << 8) & (framed_mask << 9) & (framed_mask << 7);
    int n_discs = pop_count_ull(goal_mask);
    Board board = {0x0000000810000000ULL, 0x0000001008000000ULL};
    std::vector<int> path;
    uint64_t strt = tim();
    find_path(&board, path, player, goal_mask, corner_mask, n_discs, &goal);
    uint64_t elapsed = tim() - strt;
    std::cout << "solved in " << elapsed << " ms" << std::endl;
    std::cerr << "solved in " << elapsed << " ms" << std::endl;
    return 0;
}