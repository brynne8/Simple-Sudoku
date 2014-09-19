#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <algorithm>

std::ofstream output("Sudoku.log");

typedef unsigned bitfield;

static const bitfield maskMax = 512;
static const bitfield allSet = 511;

enum difficulty { DEFAULT, EASY, MEDIUM, DIFFICULT, EVIL };

// Returns the size of the set
static unsigned bitCount(bitfield bits) {
    unsigned result = 0;
    bitfield mask = 1;

    while(mask != maskMax) {
        if (bits & mask)
            result++;
        mask *= 2;
    }

    return result;
}

// Returns a bitfield representing {num}
static inline bitfield bitFor(unsigned num) {
    return 1 << (num - 1);
}

#include "Board.h"

class Holes {
public:
    Holes(Board & board)
        : puzzle(board) {
    }
    void digHoles(difficulty level) {
        static unsigned look_up[] = {
            0,  1,  2,
            9,  10, 11,
            18, 19, 20
        };
        static unsigned base[] = {
            0,  3,  6,
            27, 30, 33,
            54, 57, 60
        };
        static unsigned array[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        for (unsigned i = 0; i < 9; ++i) {
            unsigned diff = _level_min(level);
            std::random_shuffle(array, array + 9);
            for (unsigned j = 0; j < diff; ++j) {
                unsigned loc = base[(4 * i) % 9] + look_up[array[j]];
                unsigned row = loc / 9, col = loc % 9;
                _valid_dig(row, col, level);
            }
        }

        unsigned control = 81;
        if (level == MEDIUM) {
            control = 45;
        } else if (level == DIFFICULT) {
            control = 65;
        } else if (level == EVIL) {
            control = 105;
        }
        for (unsigned k = 0; k < control; ++k) {
            unsigned loc = std::rand() % 81;
            unsigned row = loc / 9, col = loc % 9;
            if (!puzzle.assert(row, col, 0)) {
                _valid_dig(row, col, level);
            }
        }
    }
    Board& to_play() {
        return puzzle;
    }
private:
    Board puzzle;

    unsigned _level_min(difficulty level) {
        unsigned limit;
        unsigned rand = std::rand() % 4;

        switch (level) {
        case EASY:
            limit = 2;
            break;
        case MEDIUM:
            limit = 2;
            break;
        case DIFFICULT:
            limit = 4 + rand;
            break;
        case EVIL:
            limit = 6 + rand;
            break;
        default:
            limit = 0;
        }
        return limit;
    }

    void _valid_dig(unsigned i, unsigned j, difficulty level) {
        unsigned val = puzzle.unset(i, j);
        if (level == EASY) {
            Board bd = puzzle;
            bd.hidden_fill();
            if (bd.remaining()) {
                puzzle.set(i, j, val);
                return;
            }
        } else {
            for (unsigned num = 1; num <= 9; ++num) {
                if (num != val &&
                        puzzle.mask_check(i, j, bitFor(num))) {
                    Board bd = puzzle;
                    bd.set(i, j, num);
                    bd.hidden_fill();
                    bd.advanced_fill();
                    if (bd.backtrack()) {
                        puzzle.set(i, j, val);
                        return;
                    }
                }
            }
        }
    }
};

class Sudoku {
public:
    bool unique_solution;

    Sudoku(const char * name) {
        std::ifstream in(name);
        unsigned num = 0;
        for (unsigned i = 0; i < 9; ++i) {
            for (unsigned j = 0; j < 9; ++j) {
                in >> num;
                _board.set(i, j, num);
            }
        }
        _board.print_board(std::cout);
        _answer = _board;
        unique_solution = false;
        if (_has_solution()) {
            unsigned solutions = _answer.solution_count();
            if (solutions == 1) {
                unique_solution = true;
            } else {
                std::cout << "Multiple solutions!" << std::endl;
                std::cout << "One solution is: " << std::endl;
                _answer.print_board(std::cout);
            }
        } else {
            std::cout << "The sudoku is not solvable!" << std::endl;
        }
    }
    Sudoku(difficulty level, std::ostream & out) {
        output << "Generating new puzzle:" << std::endl;
        generate(level, out);
    }
    void generate(difficulty level, std::ostream & out) {
        _answer = Board(static_cast<unsigned>(std::time(0)));
        while (true) {
            Holes game(_answer);
            game.digHoles(level);
            Board bd = game.to_play();
            if (level > EASY) {
                bd.hidden_fill();
                if (!bd.remaining())
                    continue;
                bd.advanced_fill();
                if (level > MEDIUM && !bd.remaining())
                    continue;
            }
            _board = game.to_play();
            break;
        }
        _board.print_board(out);
    }
    void play(unsigned& row, unsigned& col, unsigned& val) {
        if (_board.mask_check(row, col, bitFor(val)))
            if (_answer.assert(row, col, val)) {
                _board.set(row, col, val);
                system("cls");
                _board.print_board(std::cout);
            } else
                std::cout << "You've chosen the wrong number.\n";
        else
            std::cout << "Your play violated the rules.\n";
    }
    bool is_complete() {
        return _board.remaining() == 0;
    }
    void solve(bool verbose = true) {
        _answer = _board;
        output << "Solving puzzle:" << std::endl;
        _answer.hidden_fill();
        if (_answer.remaining())
            _answer.advanced_fill();
        if (_answer.remaining())
            _answer.backtrack();
        if (verbose) {
            std::cout << "The answer is:" << std::endl;
            _answer.print_board(std::cout);
        }
    }
    void partial_solve() {
        _answer = _board;
        output << "Solving puzzle:" << std::endl;
        _answer.hidden_fill();
        if (_answer.remaining())
            _answer.advanced_fill();
        std::cout << "Logic solver gives:" << std::endl;
        _answer.print_board(std::cout);
        std::cout << _answer.remaining() << " cell(s) are left blank." << std::endl;
    }
    void next_step() {
        Board ans = _board;
        output << "\nFetching a hint:" << std::endl;
        unsigned hint;
        if (ans.hidden_fill(true)) {
            hint = ans.one_step;
            unsigned row = hint / 9, col = hint % 9;
            std::cout << "SINGLE(NAKED SINGLE, HIDDGE SINGLE, FULL HOUSE): "
                      << 'R' << row + 1 << 'C' << col + 1 << '='
                      << ans.get_num(row, col) << std::endl;
        } else if (ans.advanced_fill(true)) {
            hint = ans.one_step;
            unsigned row = hint / 9, col = hint % 9;
            std::cout << "LOCKED CANDIDATE AND PAIR CHECK(2-(2+) ONLY): "
                      << 'R' << row + 1 << 'C' << col + 1 << '='
                      << ans.get_num(row, col) << std::endl;
        } else {
            do {
                hint = std::rand() % 81;
            } while (!ans.assert(hint / 9, hint % 9, 0));
            unsigned row = hint / 9, col = hint % 9;
            std::cout << "RANDOM HINT: "
                      << 'R' << row + 1 << 'C' << col + 1 << '='
                      << _answer.get_num(row, col) << std::endl;
        }
    }
private:
    Board _board;
    Board _answer;

    bool _has_solution() {
        _answer.hidden_fill();
        _answer.advanced_fill();
        if (_answer.backtrack(true))
            return true;
        else
            return false;
    }
};

int main() {
BG:
    char mode = 0;
    std::cout << "Import puzzle file (I) or Generate one (G): ";
    std::cin >> mode;
    if (mode == 'I' || mode == 'i') {
        char file[100];
        std::cout << "Please specify the file: ";
        std::cin >> file;
        system("cls");
        clock_t b = clock();
        Sudoku puzzle(file);
        std::cout << clock() - b << std::endl;
        if (puzzle.unique_solution) {
CPM:
            std::cout << "Solve it Completely (C) or Partially (P) or Manually (M): ";
            std::cin >> mode;
            if (mode == 'C' || mode == 'c') {
                puzzle.solve();
            } else if (mode == 'P' || mode == 'p') {
                puzzle.partial_solve();
            } else if (mode == 'M' || mode == 'm') {
                puzzle.solve(false);
                unsigned row, col, val;
                while (!puzzle.is_complete()) {
                    std::cout << "Continue(C) or get a Hint(H)?" << std::endl;
                    std::cin >> mode;
                    if (mode == 'C' || mode == 'c') {
                        std::cout << "Enter a position and a number (Row, Column, Number): ";
                        std::cin >> row >> col >> val;
                        puzzle.play(--row, --col, val);
                    } else {
                        puzzle.next_step();
                    }
                }
                std::cout << "You have completed the puzzle." << std::endl;
            } else {
                std::cout << "Invalid Input!\n";
                goto CPM;
            }
        }
    } else if (mode == 'G' || mode == 'g') {
        std::cout << "Which Level: Easy (E) Medium (M) Difficult(D) Evil(U) ";
        std::cin >> mode;
        difficulty level;
        switch (mode) {
        case 'E':
        case 'e':
            level = EASY;
            break;
        case 'M':
        case 'm':
            level = MEDIUM;
            break;
        case 'D':
        case 'd':
            level = DIFFICULT;
            break;
        case 'U':
        case 'u':
            level = EVIL;
            break;
        default:
            level = DEFAULT;
            break;
        }
SP:
        std::cout << "Save it to a file (S) or Play now (P): ";
        std::cin >> mode;
        if (mode == 'S' || mode == 's') {
            std::ofstream ofs("Sudoku.out");
            Sudoku puzzle(level, ofs);
            std::cout << "Saved as Sudoku.out" << std::endl;
        } else if (mode == 'P' || mode == 'p') {
            system("cls");
            Sudoku puzzle(level, std::cout);
            puzzle.solve(false);
            unsigned row, col, val;
            while (!puzzle.is_complete()) {
                std::cout << "Continue(C) or get a Hint(H)? ";
                std::cin >> mode;
                if (mode == 'C' || mode == 'c') {
                    std::cout << "Enter a position and a number (Row, Column, Number): ";
                    std::cin >> row >> col >> val;
                    puzzle.play(--row, --col, val);
                } else {
                    puzzle.next_step();
                }
            }
            std::cout << "You have completed the puzzle." << std::endl;
        } else {
            std::cout << "Invalid Input!\n";
            goto SP;
        }
    }
    std::cout << "Quit(Q) or Run Again(R)?" << std::endl;
    std::cin >> mode;
    if (mode == 'R' || mode == 'r') {
        system("cls");
        goto BG;
    }
}

