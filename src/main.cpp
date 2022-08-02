#include <iostream>
#include <vector>
#include <string>
#include "board.hpp"
#include "util.hpp"

using namespace std;

void solve(Board *board, vector<int> path, const uint64_t puttable, const Board *goal){
    if (board->player == goal->player && board->opponent == goal->opponent){
        cout << "solution found: ";
        for (const int policy: path)
            cout << idx_to_coord(policy);
        cout << endl;
        return;
    }
    uint64_t legal = board->get_legal() & puttable;
    if (legal){
        Flip flip;
        for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)){
            calc_flip(&flip, board, cell);
            board->move_board(&flip);
            path.emplace_back(cell);
                solve(board, path, puttable, goal);
            path.pop_back();
            board->undo_board(&flip);
        }
    }
}

int main(){
    bit_init();
    board_init();
    Board goal = {0x0000000810000000ULL, 0x0000001008000000ULL};
    string in_path;
    cin >> in_path;
    int y, x;
    Flip flip;
    for (int i = 0; i < in_path.length(); i += 2){
        x = in_path[i] - 'a';
        y = in_path[i + 1] - '1';
        y = HW_M1 - y;
        x = HW_M1 - x;
        calc_flip(&flip, &goal, y * HW + x);
        goal.move_board(&flip);
    }
    goal.print();

    uint64_t puttable = goal.player | goal.opponent;
    Board board = {0x0000000810000000ULL, 0x0000001008000000ULL};
    vector<int> path;
    solve(&board, path, puttable, &goal);
    cout << "solved" << endl;
    return 0;
}