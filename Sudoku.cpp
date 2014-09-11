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
            unsigned diff = _random_by_level(level);
            std::random_shuffle(array, array + 9);
            for (unsigned j = 0; j < diff; ++j) {
                unsigned loc = base[i] + look_up[array[j]];
                unsigned row = loc / 9, col = loc % 9;
                if (!_valid_dig(row, col, level)) {
                    ++diff;
                    if (diff == 10)
                        break;
                }
            }
        }
    }
    Board& to_play() {
        return puzzle;
    }
private:
    Board puzzle;

    unsigned _random_by_level(difficulty level) {
        unsigned random = 5;
        int rnd = std::rand();

        switch (level) {
        case EASY:
            random = rnd % 7;
            if (random < 4 && rnd % 2)
                random += std::rand() % 4;
            random += 3;
            break;
        case MEDIUM:
            random = rnd % 5 + 3;
            break;
        case DIFFICULT:
            random = rnd % 5 + 4;
            break;
        case EVIL:
            random = rnd % 5 + 5;
            break;
        }
        return random;
    }

    bool _valid_dig(unsigned i, unsigned j, difficulty level) {
        unsigned val = puzzle.unset(i, j);
        if (level == EASY) {
            Board bd = puzzle;
            bd.hidden_fill();
            if (bd.remaining()) {
                puzzle.set(i, j, val);
                return false;
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
                        return false;
                    }
                }
            }
        }

        return true;
    }
};

class Sudoku {
public:
    Sudoku(const char * name, bool play = false) {
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
        if (_has_solution()) {
            unsigned solutions = _answer.solution_count();
            if (solutions > 1)
                std::cout << "Multiple solutions!" << std::endl;
            if (!play) {
                _answer = _board;
                output << "Solving puzzle:" << std::endl;
                _answer.hidden_fill();
                if (_answer.remaining())
                    _answer.advanced_fill();
                if (_answer.remaining())
                    _answer.backtrack();
                std::cout << "The answer is:" << std::endl;
                _answer.print_board(std::cout);
            }
        } else
            std::cout << "The sudoku is not solvable!" << std::endl;
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
    char mode = 0;
    std::cout << "Input puzzle file (I) or Generate one (G): ";
    std::cin >> mode;
    if (mode == 'I') {
        char file[100];
        bool flag;
        std::cout << "Please specify the file: ";
        std::cin >> file;
AM:
        std::cout << "Automatically (A) solve it or Manually (M): ";
        std::cin >> mode;
        if (mode == 'A')
            flag = false;
        else if (mode == 'M')
            flag = true;
        else {
            std::cout << "Invalid Input!\n";
            goto AM;
        }

        system("cls");
        Sudoku puzzle(file, flag);
        if (flag) {
            unsigned row, col, val;
            while (!puzzle.is_complete()) {
                std::cout << "Enter a position and a number (i, j, num): ";
                std::cin >> row >> col >> val;
                puzzle.play(row, col, val);
            }
            std::cout << "You have completed the puzzle." << std::endl;
        }
    } else if (mode == 'G') {
        std::cout << "Which Level: Easy (E) Medium (M) Difficult(D) Evil(U) ";
        std::cin >> mode;
        difficulty level;
        bool flag;
        switch (mode) {
        case 'E':
            level = EASY;
            break;
        case 'M':
            level = MEDIUM;
            break;
        case 'D':
            level = DIFFICULT;
            break;
        case 'U':
            level = EVIL;
            break;
        default:
            level = DEFAULT;
            break;
        }
SP:
        std::cout << "Save it to a file (S) or Play now (P): ";
        std::cin >> mode;
        if (mode == 'S')
            flag = false;
        else if (mode == 'P')
            flag = true;
        else {
            std::cout << "Invalid Input!\n";
            goto SP;
        }

        if (flag) {
            system("cls");
            Sudoku puzzle(level, std::cout);
            unsigned row, col, val;
            while (!puzzle.is_complete()) {
                std::cout << "Enter a position and a number (i, j, num): ";
                std::cin >> row >> col >> val;
                puzzle.play(row, col, val);
            }
            std::cout << "You have completed the puzzle." << std::endl;
        } else {
            std::ofstream ofs("Sudoku.out");
            Sudoku puzzle(level, ofs);
            std::cout << "Saved as Sudoku.out" << std::endl;
        }
    }
    system("pause");
}
