#include "sheet.h"

#include <algorithm>
#include <ostream>
#include <utility>

void Sheet::CheckPosition(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position");
    }
}

void Sheet::CheckCircularDependency(Position target, const std::vector<Position>& references) const {
    std::vector<Position> pending = references;
    std::set<Position> visited;

    while (!pending.empty()) {
        const Position current = pending.back();
        pending.pop_back();

        if (current == target) {
            throw CircularDependencyException("Circular dependency");
        }

        if (!visited.insert(current).second) {
            continue;
        }

        const CellInterface* cell = GetCell(current);

        if (cell != nullptr) {
            const auto next = cell->GetReferencedCells();
            pending.insert(pending.end(), next.begin(), next.end());
        }
    }
}

std::vector<Position> Sheet::CollectAffectedCells(Position start) const {
    std::vector<Position> pending = { start };
    std::vector<Position> result;
    std::set<Position> visited;

    while (!pending.empty()) {
        const Position current = pending.back();
        pending.pop_back();

        if (!visited.insert(current).second) {
            continue;
        }

        result.push_back(current);

        const auto it = dependents_.find(current);

        if (it != dependents_.end()) {
            pending.insert(pending.end(), it->second.begin(), it->second.end());
        }
    }

    return result;
}

Sheet::DependencyMap Sheet::PrepareDependencies(Position pos, const std::vector<Position>& old_refs, const std::vector<Position>& new_refs) const {
    DependencyMap updated = dependents_;

    for (Position reference : old_refs) {
        auto it = updated.find(reference);

        if (it != updated.end()) {
            it->second.erase(pos);

            if (it->second.empty()) {
                updated.erase(it);
            }
        }
    }

    for (Position reference : new_refs) {
        updated[reference].insert(pos);
    }

    return updated;
}

void Sheet::InvalidateCells(const std::vector<Position>& positions) {
    for (Position pos : positions) {
        const auto it = cells_.find(pos);

        if (it != cells_.end()) {
            it->second->InvalidateCache();
        }
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    CheckPosition(pos);

    auto candidate = std::make_unique<Cell>(*this);
    candidate->Set(std::move(text));

    const auto new_refs = candidate->GetReferencedCells();
    CheckCircularDependency(pos, new_refs);

    const auto it = cells_.find(pos);
    std::vector<Position> old_refs;

    if (it != cells_.end()) {
        old_refs = it->second->GetReferencedCells();
    }

    const auto affected = CollectAffectedCells(pos);
    auto updated = PrepareDependencies(pos, old_refs, new_refs);

    if (it != cells_.end()) {
        it->second->SwapContent(*candidate);
    }
    else {
        cells_.emplace(pos, std::move(candidate));
    }

    dependents_.swap(updated);
    InvalidateCells(affected);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    CheckPosition(pos);

    const auto it = cells_.find(pos);

    if (it == cells_.end()) {
        return nullptr;
    }

    return it->second.get();
}

CellInterface* Sheet::GetCell(Position pos) {
    CheckPosition(pos);

    const auto it = cells_.find(pos);

    if (it == cells_.end()) {
        return nullptr;
    }

    return it->second.get();
}

void Sheet::ClearCell(Position pos) {
    CheckPosition(pos);

    const auto it = cells_.find(pos);

    if (it == cells_.end()) {
        return;
    }

    const auto affected = CollectAffectedCells(pos);
    auto updated = PrepareDependencies(pos, it->second->GetReferencedCells(), {});

    cells_.erase(it);
    dependents_.swap(updated);
    InvalidateCells(affected);
}

Size Sheet::GetPrintableSize() const {
    Size size;

    for (const auto& [pos, cell] : cells_) {
        if (!cell->GetText().empty()) {
            size.rows = std::max(size.rows, pos.row + 1);
            size.cols = std::max(size.cols, pos.col + 1);
        }
    }

    return size;
}

void Sheet::PrintTexts(std::ostream& output) const {
    const Size size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }

            const CellInterface* cell = GetCell({ row, col });

            if (cell != nullptr) {
                output << cell->GetText();
            }
        }

        output << '\n';
    }
}

void Sheet::PrintValues(std::ostream& output) const {
    const Size size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }

            const CellInterface* cell = GetCell({ row, col });

            if (cell != nullptr) {
                const CellInterface::Value value = cell->GetValue();

                if (const auto* text = std::get_if<std::string>(&value)) {
                    output << *text;
                }
                else if (const auto* number = std::get_if<double>(&value)) {
                    output << *number;
                }
                else {
                    output << std::get<FormulaError>(value).ToString();
                }
            }
        }

        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}