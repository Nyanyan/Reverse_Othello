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

bool input_board_line(std::string board_str, Board *board, int *player){
    board_str.erase(std::remove_if(board_str.begin(), board_str.end(), ::isspace), board_str.end());
    if (board_str.length() != HW2 + 1){
        std::cerr << "[ERROR] invalid argument" << std::endl;
        return false;
    }
    board->player = 0ULL;
    board->opponent = 0ULL;
    for (int i = 0; i < HW2; ++i){
        if (board_str[i] == 'B' || board_str[i] == 'b' || board_str[i] == 'X' || board_str[i] == 'x' || board_str[i] == '0' || board_str[i] == '*')
            board->player |= 1ULL << (HW2_M1 - i);
        else if (board_str[i] == 'W' || board_str[i] == 'w' || board_str[i] == 'O' || board_str[i] == 'o' || board_str[i] == '1')
            board->opponent |= 1ULL << (HW2_M1 - i);
    }
    if (board_str[HW2] == 'B' || board_str[HW2] == 'b' || board_str[HW2] == 'X' || board_str[HW2] == 'x' || board_str[HW2] == '0' || board_str[HW2] == '*')
        *player = BLACK;
    else if (board_str[HW2] == 'W' || board_str[HW2] == 'w' || board_str[HW2] == 'O' || board_str[HW2] == 'o' || board_str[HW2] == '1')
        *player = WHITE;
    else{
        std::cerr << "[ERROR] invalid player argument" << std::endl;
        return false;
    }
    if (*player == WHITE)
        std::swap(board->player, board->opponent);
    return true;
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

void find_path(Board *board, std::vector<int> &path, int player, const uint64_t goal_mask, const uint64_t corner_mask, const int goal_n_discs, const Board *goal, const int goal_player, uint64_t *n_nodes, uint64_t *n_solutions){
    ++(*n_nodes);
    if (player == goal_player && board->player == goal->player && board->opponent == goal->opponent){
        output_transcript(path);
        ++(*n_solutions);
        return;
    }
    uint64_t goal_board_player, goal_board_opponent;
    if (player != goal_player){
        goal_board_player = goal->opponent;
        goal_board_opponent = goal->player;
    } else{
        goal_board_player = goal->player;
        goal_board_opponent = goal->opponent;
    }
    uint64_t stable = enhanced_stability(board, goal_mask);
    if ((stable & board->player & goal_board_opponent) || (stable & board->opponent & goal_board_player))
        return;
    uint64_t legal = board->get_legal() & goal_mask & ~(corner_mask & goal_board_opponent);
    if (legal){
        Flip flip;
        for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)){;
            calc_flip(&flip, board, cell);
            board->move_board(&flip);
            path.emplace_back(cell);
                find_path(board, path, player ^ 1, goal_mask, corner_mask, goal_n_discs, goal, goal_player, n_nodes, n_solutions);
            path.pop_back();
            board->undo_board(&flip);
        }
    }
}

int main(int argc, char* argv[]){
    init();
    std::cerr << "please input the board (X: black O: white)" << std::endl;
    std::cerr << "example: ------------------O--X---OOOXXX--OOOXXX---OOXX-----OX----------- X" << std::endl;
    //Board goal = input_board();
    std::string board_str;
    getline(std::cin, board_str);
    std::cout << board_str << std::endl;
    Board goal;
    int goal_player;
    if (!input_board_line(board_str, &goal, &goal_player))
        return 1;
    goal.print();

    uint64_t goal_mask = goal.player | goal.opponent; // legal candidate
    uint64_t corner_mask = 0ULL; // cells that work as corner (non-flippable cells)
    uint64_t empty_mask_r1 = ((~goal_mask & 0xFEFEFEFEFEFEFEFEULL) >> 1) | 0x8080808080808080ULL;
    uint64_t empty_mask_l1 = ((~goal_mask & 0x7F7F7F7F7F7F7F7FULL) << 1) | 0x0101010101010101ULL;
    uint64_t empty_mask_r8 = ((~goal_mask & 0xFFFFFFFFFFFFFF00ULL) >> 8) | 0xFF00000000000000ULL;
    uint64_t empty_mask_l8 = ((~goal_mask & 0x00FFFFFFFFFFFFFFULL) << 8) | 0x00000000000000FFULL;
    uint64_t empty_mask_r7 = ((~goal_mask & 0x7F7F7F7F7F7F7F00ULL) >> 7) | 0xFF01010101010101ULL;
    uint64_t empty_mask_l7 = ((~goal_mask & 0x00FEFEFEFEFEFEFEULL) << 7) | 0x80808080808080FFULL;
    uint64_t empty_mask_r9 = ((~goal_mask & 0xFEFEFEFEFEFEFE00ULL) >> 9) | 0x01010101010101FFULL;
    uint64_t empty_mask_l9 = ((~goal_mask & 0x007F7F7F7F7F7F7FULL) << 9) | 0xFF80808080808080ULL;
    corner_mask |= empty_mask_r1 & empty_mask_r8 & empty_mask_r9 & empty_mask_r7;
    corner_mask |= empty_mask_r1 & empty_mask_r8 & empty_mask_r9 & empty_mask_l7;
    corner_mask |= empty_mask_r1 & empty_mask_r8 & empty_mask_l9 & empty_mask_r7;
    corner_mask |= empty_mask_r1 & empty_mask_r8 & empty_mask_l9 & empty_mask_l7;
    corner_mask |= empty_mask_r1 & empty_mask_l8 & empty_mask_r9 & empty_mask_r7;
    corner_mask |= empty_mask_r1 & empty_mask_l8 & empty_mask_r9 & empty_mask_l7;
    corner_mask |= empty_mask_r1 & empty_mask_l8 & empty_mask_l9 & empty_mask_r7;
    corner_mask |= empty_mask_r1 & empty_mask_l8 & empty_mask_l9 & empty_mask_l7;
    corner_mask |= empty_mask_l1 & empty_mask_r8 & empty_mask_r9 & empty_mask_r7;
    corner_mask |= empty_mask_l1 & empty_mask_r8 & empty_mask_r9 & empty_mask_l7;
    corner_mask |= empty_mask_l1 & empty_mask_r8 & empty_mask_l9 & empty_mask_r7;
    corner_mask |= empty_mask_l1 & empty_mask_r8 & empty_mask_l9 & empty_mask_l7;
    corner_mask |= empty_mask_l1 & empty_mask_l8 & empty_mask_r9 & empty_mask_r7;
    corner_mask |= empty_mask_l1 & empty_mask_l8 & empty_mask_r9 & empty_mask_l7;
    corner_mask |= empty_mask_l1 & empty_mask_l8 & empty_mask_l9 & empty_mask_r7;
    corner_mask |= empty_mask_l1 & empty_mask_l8 & empty_mask_l9 & empty_mask_l7;
    corner_mask &= goal_mask;

    bit_print_board(goal_mask);
    bit_print_board(corner_mask);

    int n_discs = pop_count_ull(goal_mask);
    Board board = {0x0000000810000000ULL, 0x0000001008000000ULL};
    std::vector<int> path;
    uint64_t strt = tim();
    uint64_t n_nodes = 0, n_solutions = 0;
    find_path(&board, path, BLACK, goal_mask, corner_mask, n_discs, &goal, goal_player, &n_nodes, &n_solutions);
    uint64_t elapsed = tim() - strt;
    std::cout << "found " << n_solutions << " solutions in " << elapsed << " ms " << n_nodes << " nodes" << std::endl;
    std::cerr << "found " << n_solutions << " solutions in " << elapsed << " ms " << n_nodes << " nodes" << std::endl;
    return 0;
}
