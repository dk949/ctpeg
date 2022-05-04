#include "math_expr.hpp"
#include "math_expr_parser.hpp"

int main() {
 constexpr std::string_view inputExpr = "1337 - 259 * 5";
    constexpr auto res = std::get<Expr>(parser(inputExpr).value().first).run();
    return static_cast<int>(res);
}
