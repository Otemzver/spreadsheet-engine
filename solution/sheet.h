#pragma once

#include "cell.h"
#include "common.h"

#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

class Sheet : public SheetInterface {
public:
    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    struct PositionHasher {
        std::size_t operator()(Position pos) const {
            return static_cast<std::size_t>(pos.row) * Position::MAX_COLS + static_cast<std::size_t>(pos.col);
        }
    };

    using DependencyMap = std::map<Position, std::set<Position>>;

    static void CheckPosition(Position pos);

    void CheckCircularDependency(Position target, const std::vector<Position>& references) const;
    std::vector<Position> CollectAffectedCells(Position start) const;
    DependencyMap PrepareDependencies(Position pos, const std::vector<Position>& old_refs, const std::vector<Position>& new_refs) const;
    void InvalidateCells(const std::vector<Position>& positions);

    DependencyMap dependents_;
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher> cells_;
};