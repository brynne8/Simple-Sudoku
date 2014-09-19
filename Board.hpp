#ifndef BOARD_H
#define BOARD_H

#include <list>

class BlankList {
public:
    BlankList() {
        for (unsigned i = 0; i < 9; ++i)
            rows[i] = cols[i] = allSet;
        for (unsigned i = 0; i < 3; ++i)
            for (unsigned j = 0; j < 3; ++j)
                blocks[i][j] = allSet;
    }

    void elim(unsigned i, unsigned j, unsigned n) {
        bitfield bit = bitFor(n);
        rows[i] &= ~bit;
        cols[j] &= ~bit;
        blocks[i/3][j/3] &= ~bit;
    }

    void cancel(unsigned i, unsigned j, unsigned n) {
        bitfield bit = bitFor(n);
        rows[i] |= bit;
        cols[j] |= bit;
        blocks[i/3][j/3] |= bit;
    }

    bitfield possible(unsigned i, unsigned j) {
        return rows[i] & cols[j] & blocks[i/3][j/3];
    }
private:
    bitfield rows[9], cols[9];
    bitfield blocks[3][3];
};

class Board {
public:
    unsigned backtrack_count;
    unsigned one_step;

    Board()
        : remains(81), solutions(0) {
        for (unsigned i = 0; i < 9; ++i)
            for (unsigned j = 0; j < 9; ++j) {
                matrix[i][j] = 0;
                memory[i][j] = allSet;
            }
    }
    Board(unsigned seed)
        : remains(81), solutions(0) {
        for (unsigned i = 0; i < 9; ++i)
            for (unsigned j = 0; j < 9; ++j) {
                matrix[i][j] = 0;
                memory[i][j] = allSet;
            }

        std::srand(seed);
        _random_fill();
    }
    void set(unsigned row, unsigned col, unsigned val, bool advanced = false) {
        matrix[row][col] = val;
        if (matrix[row][col]) {
            Blank.elim(row, col, val);
            --remains;
        }
        if (advanced)
            _update(row, col);
    }
    unsigned unset(unsigned row, unsigned col) {
        unsigned val = matrix[row][col];
        if (val) {
            Blank.cancel(row, col, val);
            ++remains;
        }
        matrix[row][col] = 0;
        return val;
    }
    bitfield mask_check(unsigned i, unsigned j, bitfield mask) {
        return Blank.possible(i, j) & mask & memory[i][j];
    }
    bitfield house_check(unsigned i, unsigned j, bool advanced = false) {
        bitfield house_hidden = allSet;
        unsigned row_base = i / 3 * 3;
        unsigned col_base = j / 3 * 3;
        for (unsigned row = row_base; row < row_base + 3; ++row)
            for (unsigned col = col_base; col < col_base + 3; ++col) {
                if ((row == i && col == j) || matrix[row][col])
                    continue;
                house_hidden &= advanced ? ~memory[row][col] : ~Blank.possible(row, col);
            }
        return house_hidden;
    }
    bitfield row_check(unsigned i, unsigned j, bool advanced = false) {
        bitfield row_hidden = allSet;
        for (unsigned col = 0; col < 9; ++col)
            if (!matrix[i][col] && col != j)
                row_hidden &= advanced ? ~memory[i][col] : ~Blank.possible(i, col);
        return row_hidden;
    }
    bitfield col_check(unsigned i, unsigned j, bool advanced = false) {
        bitfield col_hidden = allSet;
        for (unsigned row = 0; row < 9; ++row)
            if (!matrix[row][j] && row != i)
                col_hidden &= advanced ? ~memory[row][j] : ~Blank.possible(row, j);
        return col_hidden;
    }
    bitfield decide(bitfield & possible, bitfield & house,
                    bitfield & row,      bitfield & col) {
        if (bitCount(possible) == 1)
            return possible;

        bitfield check1 = possible & house;
        if (bitCount(check1) == 1)
            return check1;

        bitfield check2 = possible & row;
        if (bitCount(check2) == 1)
            return check2;

        bitfield check3 = possible & col;
        if (bitCount(check3) == 1)
            return check3;

        bitfield check = check1 & check2;
        if (bitCount(check) == 1)
            return check;

        check = check1 & check3;
        if (bitCount(check) == 1)
            return check;

        check = check2 & check3;
        if (bitCount(check) == 1)
            return check;

        check &= check1;
        if (bitCount(check) == 1)
            return check;

        return 0;
    }
    bool hidden_fill(bool hint = false) {
        bool again;
        output << std::endl << "Simple solving ";
        do {
            again = false;
            output << ".";
            unsigned loc, i, j;
            for (unsigned pos = 0; pos < 81; ++pos) {
                loc = 4 * pos % 81;
                i = loc / 9, j = loc % 9;
                if (matrix[i][j]) continue;
                bitfield possible = Blank.possible(i, j);
                bitfield house = house_check(i, j);
                bitfield row = row_check(i, j);
                bitfield col = col_check(i, j);
                bitfield to_check = decide(possible, house, row, col);
                if (to_check) {
                    set(i, j, numFor(to_check));
                    if (hint) {
                        one_step = 9 * i + j;
                        return true;
                    }
                    else {
                        again = true;
                    }
                }
            }
        } while (again);
        return false;
    }
    void candidate_check(unsigned i, unsigned j) {
        bitfield row_locked(0), col_locked(0), row_i(0), col_j(0);
        unsigned row_base = i / 3 * 3;
        unsigned col_base = j / 3 * 3;

        //Locked Candidate Type 1 (Pointing)
        unsigned total_count = 0;
        for (unsigned row = row_base; row < row_base + 3; ++row) {
            if (row == i) {
                for (unsigned col = col_base; col < col_base + 3; ++col) {
                    if (matrix[row][col])
                        continue;
                    row_i |= memory[row][col];
                }
            } else {
                for (unsigned col = col_base; col < col_base + 3; ++col) {
                    if (matrix[row][col])
                        continue;
                    row_locked |= memory[row][col];
                    ++total_count;
                }
            }
        }
        if (total_count && total_count == bitCount(row_locked))
            memory[i][j] &= ~row_locked;
        bitfield pointing = row_i & ~row_locked;
        if (pointing)
            for (unsigned col = 0; col < 9; ++col) {
                if (matrix[i][col] || (col >= col_base && col < col_base + 3))
                    continue;
                memory[i][col] &= ~pointing;
            }

        total_count = 0;
        for (unsigned col = col_base; col < col_base + 3; ++col) {
            if (col == j) {
                for (unsigned row = row_base; row < row_base + 3; ++row) {
                    if (matrix[row][col])
                        continue;
                    col_j |= memory[row][col];
                }
            } else {
                for (unsigned row = row_base; row < row_base + 3; ++row) {
                    if (matrix[row][col])
                        continue;
                    col_locked |= memory[row][col];
                    ++total_count;
                }
            }
        }
        if (total_count && total_count == bitCount(col_locked))
            memory[i][j] &= ~col_locked;
        pointing = col_j & ~col_locked;
        if (pointing)
            for (unsigned row = 0; row < 9; ++row) {
                if (matrix[row][j] || (row >= row_base && row < row_base + 3))
                    continue;
                memory[row][j] &= ~pointing;
            }

        //Locked Candidate Type 2 (Claiming)
        for (unsigned col = 0; col < 9; ++col) {
            if (matrix[i][col] || (col >= col_base && col < col_base + 3))
                continue;
            row_i &= ~memory[i][col];
        }
        if (row_i) {
            for (unsigned r = row_base; r < row_base + 3; ++r)
                for (unsigned c = col_base; c < col_base + 3; ++c) {
                    if (r == i || matrix[r][c])
                        continue;
                    memory[r][c] &= ~row_i;
                }
        }

        for (unsigned row = 0; row < 9; ++row) {
            if (matrix[row][j] || (row >= row_base && row < row_base + 3))
                continue;
            col_j &= ~memory[row][j];
        }
        if (col_j) {
            for (unsigned r = row_base; r < row_base + 3; ++r)
                for (unsigned c = col_base; c < col_base + 3; ++c) {
                    if (c == j || matrix[r][c])
                        continue;
                    memory[r][c] &= ~col_j;
                }
        }
    }
    void pair_check(unsigned i, unsigned j) {
        bitfield value = memory[i][j];
        if (bitCount(value) == 2) {
            unsigned match_r(9), match_c(9);

            //Naked Pair in Row
            for (unsigned col = 0; col < 9; ++col) {
                if (matrix[i][col] || col == j || memory[i][col] != value)
                    continue;
                match_c = col;
                break;
            }

            if (match_c != 9)
                for (unsigned col = 0; col < 9; ++col) {
                    if (matrix[i][col] || col == j || col == match_c)
                        continue;
                    memory[i][col] &= ~value;
                }

            //Naked Pair in Column
            for (unsigned row = 0; row < 9; ++row) {
                if (matrix[row][j] || row == i || memory[row][j] != value)
                    continue;
                match_r = row;
                break;
            }

            if (match_r != 9)
                for (unsigned row = 0; row < 9; ++row) {
                    if (matrix[row][j] || row == i || row == match_r)
                        continue;
                    memory[row][j] &= ~value;
                }

            match_r = 9, match_c = 9;
            //Naked Pair in House
            unsigned row_base = i / 3 * 3;
            unsigned col_base = j / 3 * 3;
            for (unsigned row = row_base; row < row_base + 3; ++row) {
                if (row == i)
                    continue;
                for (unsigned col = col_base; col < col_base + 3; ++col) {
                    if (col == j || matrix[row][col] || memory[row][col] != value)
                        continue;
                    match_r = row, match_c = col;
                    break;
                }
            }

            if (match_r != 9)
                for (unsigned row = row_base; row < row_base + 3; ++row) {
                    for (unsigned col = col_base; col < col_base + 3; ++col) {
                        if (matrix[row][col] || (row == i && col == j) ||
                                (row == match_r && col == match_c))
                            continue;
                        memory[row][col] &= ~value;
                    }
                }
        }
    }
    bool advanced_fill(bool hint = false) {
        bool again;
        unsigned sum;
        output << std::endl << "Advanced solving ";
        for (unsigned i = 0; i < 9; ++i)
            for (unsigned j = 0; j < 9; ++j)
                if (!matrix[i][j])
                    memory[i][j] &= Blank.possible(i, j);

        do {
            again = false;
            output << ".";
            sum = 0;
            unsigned loc, i, j;
            for (i = 0; i < 9; ++i)
                for (j = 0; j < 9; ++j)
                    sum += memory[i][j];

            for (unsigned pos = 0; pos < 81; ++pos) {
                loc = 4 * pos % 81;
                i = loc / 9, j = loc % 9;
                if (matrix[i][j]) continue;
                candidate_check(i, j);
                pair_check(i, j);
                bitfield possible = memory[i][j];
                bitfield house = house_check(i, j, true);
                bitfield row = row_check(i, j, true);
                bitfield col = col_check(i, j, true);
                bitfield to_check = decide(possible, house, row, col);
                if (to_check) {
                    set(i, j, numFor(to_check), true);
                    if (hint) {
                        one_step = 9 * i + j;
                        return true;
                    }
                }
            }

            for (i = 0; i < 9; ++i)
                for (j = 0; j < 9; ++j)
                    sum -= memory[i][j];
            if (sum) again = true;
        } while (again);
        return false;
    }
    unsigned remaining() {
        return remains;
    }
    unsigned solution_count() {
        return solutions;
    }
    bool backtrack(bool multiple = false) {
        backtrack_count = 0;
        for (unsigned i = 0; i < 9; ++i)
            for (unsigned j = 0; j < 9; ++j) {
                if (!matrix[i][j]) {
                    unsigned count = bitCount(Blank.possible(i, j) & memory[i][j]);
                    if (count < 3) {
                        if (!count) return false;
                        countList.push_front(9 * i + j);
                    } else {
                        countList.push_back(9 * i + j);
                    }
                }
            }
        _btrack(remaining(), multiple);
        output << "\nAnd totally " << backtrack_count
               << " backtracking attempt(s)." << std::endl;
        return solutions != 0;
    }
    bool assert(unsigned i, unsigned j, unsigned val) {
        return matrix[i][j] == val;
    }
    unsigned get_num(unsigned i, unsigned j) {
        return matrix[i][j];
    }
    void print_board(std::ostream & out) {
        for (unsigned i = 0; i < 9; ++i) {
            for (unsigned j = 0; j < 9; ++j)
                out << matrix[i][j] << " ";
            out << std::endl;
        }
        out << std::endl;
    }
private:
    unsigned matrix[9][9];
    bitfield memory[9][9];
    unsigned remains, solutions;
    BlankList Blank;
    std::list<int> countList;

    static unsigned numFor(bitfield bit) {
        unsigned num = 0;
        while (bit) {
            bit >>= 1;
            ++num;
        }
        return num;
    }
    bool findMin(unsigned& row, unsigned& col, bool & unique) {
        unsigned count = 10;

        std::list<int>::iterator it, chosen;
        for (it = countList.begin(); it != countList.end(); ++it) {
            int loc = *it;
            row = loc / 9, col = loc % 9;
            unsigned newCount = bitCount(Blank.possible(row, col) & memory[row][col]);
            if(count > newCount) {
                count = newCount;
                chosen = it;
                if (count == 1) {
                    unique = true;
                    break;
                } else if (count == 0) {
                    return false;
                }
            }
        }

        if (count != 10) {
            int loc = *chosen;
            row = loc / 9, col = loc % 9;
            countList.erase(chosen);
            return true;
        } else {
            return false;
        }
    }
    bool _btrack(unsigned depth, const bool & multiple) {
        unsigned row, col;
        bool unique = false;
        if (!findMin(row, col, unique))
            if (!depth) {
                ++solutions;
                if (multiple) {
                    if (solutions > 1)
                        return true;
                    return false;
                } else {
                    return true;
                }
            } else {
                return false;
            }

        // Iterate through the possible values this cell could have
        unsigned num = 1;
        bitfield mask = bitFor(num);
        ++backtrack_count;

        while (mask != maskMax) {
            if (mask_check(row, col, mask)) {
                set(row, col, num);
                if (_btrack(depth - 1, multiple))
                    return true;
                unset(row, col);
                if (unique)
                    break;
            }

            // Advance to the next number
            mask *= 2;
            num++;
        }
        _enlist(row, col);
        return false;
    }
    void _update(unsigned row, unsigned col) {
        unsigned row_base = row / 3 * 3;
        unsigned col_base = col / 3 * 3;
        unsigned r, c;

        for (unsigned i = 0; i < 9; ++i) {
            if (!matrix[row][i])
                memory[row][i] &= Blank.possible(row, i);
            if (!matrix[i][col])
                memory[i][col] &= Blank.possible(i, col);
            r = row_base + i / 3;
            c = col_base + i % 3;
            if (r != row && c != col && !matrix[r][c])
                memory[r][c] &= Blank.possible(r, c);
        }
    }
    void _enlist(unsigned row, unsigned col) {
        countList.push_front(9 * row + col);
    }
    bool _fill(bool is_big, unsigned block, unsigned num) {
        if (is_big) {
            if (num == 10)
                return true;
            if (block == 9)
                return _fill(is_big, 0, num + 1);
        }
        if (block == 9)
            return true;
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
        unsigned place;
        bitfield available = allSet;
        while (available) {
            while (true) {
                place = std::rand() % 9;
                if (available & bitFor(place + 1))
                    break;
            }
            available &= ~bitFor(place + 1);

            unsigned loc = base[(2 * block) % 9] + look_up[place];
            unsigned row = loc / 9, col = loc % 9;
            if (!matrix[row][col] &&
                    (bitFor(num) & Blank.possible(row, col))) {
                set(row, col, num);
                if (_fill(is_big, block + 1, num))
                    return true;
                unset(row, col);
            }
        }
        return false;
    }

    void _random_fill() {
        for (unsigned i = 1; i <= 5; ++i)
            _fill(false, 0, i);
        if (!_fill(true, 0, 6))
            std::cout << "Error" << std::endl;
    }
};

#endif
