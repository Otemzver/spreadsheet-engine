#include "common.h"

#include <algorithm>

const Position Position::NONE{ -1, -1 };

bool Position::IsValid() const {
    return row >= 0
        && row < MAX_ROWS
        && col >= 0
        && col < MAX_COLS;
}

bool Position::operator==(const Position& other) const {
    return row == other.row && col == other.col;
}

std::string Position::ToString() const {
    if (!IsValid()) {
        return {};
    }

    constexpr int ALPHABET_SIZE = 26;

    std::string column_name;
    int column_number = col + 1;

    while (column_number > 0) {
        const int letter_index =
            (column_number - 1) % ALPHABET_SIZE;

        const char letter =
            static_cast<char>('A' + letter_index);

        column_name.push_back(letter);

        column_number =
            (column_number - 1) / ALPHABET_SIZE;
    }

    std::reverse(column_name.begin(), column_name.end());

    return column_name + std::to_string(row + 1);
}

Position Position::FromString(std::string_view text) {
    if (text.empty()) {
        return NONE;
    }

    constexpr int ALPHABET_SIZE = 26;

    std::size_t index = 0;
    int column_number = 0;

    while (index < text.size()
        && text[index] >= 'A'
        && text[index] <= 'Z') {
        const int letter_number = text[index] - 'A' + 1;

        column_number = column_number * ALPHABET_SIZE + letter_number;

        if (column_number > MAX_COLS) {
            return NONE;
        }

        index++;
    }

    if (index == 0 || index == text.size()) {
        return NONE;
    }

    int row_number = 0;

    while (index < text.size()) {
        const char symbol = text[index];

        if (symbol < '0' || symbol>'9') {
            return NONE;
        }

        const int digit = symbol - '0';

        row_number = row_number * 10 + digit;

        if (row_number > MAX_ROWS) {
            return NONE;
        }

        ++index;
    }

    const Position result{
        row_number - 1,
        column_number - 1
    };

    if (!result.IsValid()) {
        return NONE;
    }

    return result;
}

bool Position::operator<(const Position& other) const {
    if (row != other.row) {
        return row < other.row;
    }

    return col < other.col;
}

bool Size::operator==(const Size& other) const {
    return rows == other.rows && cols == other.cols;
}