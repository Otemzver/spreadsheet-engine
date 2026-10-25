#pragma once

#include "FormulaAST.h"

#include <memory>
#include <string>

std::unique_ptr<ASTImpl::Expr> BuildExpression(const std::string& expression);