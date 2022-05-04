#ifndef CTPEG_HPP
#define CTPEG_HPP
#include <array>
#include <concepts>
#include <cstdint>
#include <optional>
#include <string_view>
#include <tl/expected.hpp>
#include <variant>
//#include <variant>

/*
/////////////////////////////////////
//////////// Debug tools ////////////
/////////////////////////////////////
 */

#ifndef CTPEG_TRACE
#define CTPEG_TRACE if (false)
#else
#undef CTPEG_TRACE
#define CTPEG_TRACE if (true)
#endif

#ifndef CTPEG_NO_CONSTEXPR
namespace ctpeg::debug {
inline namespace v0_3_1 {
#define CTPEG_CONSTEXPR constexpr
#define CTPEG_ASSERT(A) static_assert((A))

template <typename Arg, typename... Args>
constexpr void print(Arg, Args...) {}

template <typename T>
std::string_view typeName() {
    return "";
}

template <typename T>
std::string_view typeName(const T &) {
    return "";
}
}  // namespace v0_3_1
}  // namespace ctpeg::debug
#else
#include <cxxabi.h>

#include <cassert>
#include <iostream>
#include <memory>

#define CTPEG_ASSERT(A) assert((A))
#define CTPEG_CONSTEXPR

namespace ctpeg::detail {
inline namespace v0_3_1 {
#ifndef _MSC_VER
// https://stackoverflow.com/questions/4939636/function-to-mangle-demangle-functions
std::string cppDemangle(const char *abiName) {
    int status;
    std::unique_ptr<char, void (*)(void *)> ret{
        abi::__cxa_demangle(abiName, nullptr, nullptr, &status), free};
    if (status) {
        switch (status) {
            case -1:
                return "memory allocation failure";
            case -2:
                return abiName;
            case -3:
                return "internal error: invalid argument";
        }
    }
    return ret.get();
}
#else
std::string cppDemangle(const char *abiName) { return abiName; }
#endif

}  // namespace v0_3_1
}  // namespace ctpeg::detail
namespace ctpeg::debug {
inline namespace v0_3_1 {
template <typename Arg, typename... Args>
void print(Arg arg, Args... args) {
    std::cout << arg;
    if constexpr (sizeof...(Args)) print(args...);
}

template <typename T>
std::string typeName() {
    return detail::cppDemangle(typeid(T).name());
}

template <typename T>
std::string typeName(const T &) {
    return detail::cppDemangle(typeid(T).name());
}
}  // namespace v0_3_1
}  // namespace ctpeg::debug

#endif

/*
/////////////////////////////////////
///// Internal helper functions /////
/////////////////////////////////////
 */

namespace ctpeg::detail {
inline namespace v0_3_1 {

// https://stackoverflow.com/questions/52393831/can-i-extend-variant-in-c
template <typename T, typename New>
struct VariantCat;

template <typename... Old, typename New>
struct VariantCat<std::variant<Old...>, New> {
    using type = std::variant<Old..., New>;
};

template <typename Old, typename New>
using VariantCat_t = typename VariantCat<Old, New>::type;

// https://stackoverflow.com/questions/45892170/how-do-i-check-if-an-stdvariant-can-hold-a-certain-type
template <typename T, typename Variant>
struct IsVariantMember;

template <typename T, typename... All>
struct IsVariantMember<T, std::variant<All...>>
    : public std::disjunction<std::is_same<T, All>...> {};

template <typename T, typename... All>
constexpr bool IsVariantMember_v = IsVariantMember<T, All...>::value;

template <std::integral Base, std::integral Exp>
[[nodiscard]] constexpr auto power(Base base, Exp exp) noexcept {
    if (exp == 0 || base == 1) return 1;
    if (base == 0) return 0;
    if (exp == 1) return base;

    Base out = base;
    for (Exp i = 1; i < exp; i++) out *= base;
    return out;
}
[[nodiscard]] constexpr auto getNumDigits(std::integral auto i) noexcept {
    auto num = 0;
    while (i /= 10) {
        num++;
    }
    return num + 1;
}
[[nodiscard]] constexpr auto nthDigit(std::integral auto i,
                                      std::integral auto n) noexcept {
    return (i / power(10, n)) % 10;
}
[[nodiscard]] constexpr auto charToInt(char c) noexcept { return c - '0'; }
[[nodiscard]] constexpr char digitToChar(std::integral auto i) noexcept {
    return static_cast<char>('0' + i);
}
[[nodiscard]] constexpr bool isdigit(char c) noexcept {
    return charToInt(c) >= 0 && charToInt(c) <= 9;
}

[[nodiscard]] constexpr auto svToInt(std::string_view sv) noexcept {
    auto out = 0;
    for (std::size_t i = sv.size() - 1, j = 0; j < sv.size(); i--, j++) {
        out += (charToInt(sv[j]) * power(10, i));
    }
    return out;
}

}  // namespace v0_3_1
}  // namespace ctpeg::detail

/*
/////////////////////////////////////
//////////////// Setup //////////////
/////////////////////////////////////
 */
namespace ctpeg {
inline namespace v0_3_1 {
struct UninitialisedVariant {};
struct EmptyVariant {};
constexpr bool operator==(const EmptyVariant &, const EmptyVariant &) {
    return true;
}

using ResultVariantSingle = std::variant<UninitialisedVariant, EmptyVariant,
                                         char, std::string_view, int64_t
#ifdef CTPEG_VARIANT
                                         ,
                                         CTPEG_VARIANT
#endif
                                         >;

#ifndef CTPEG_MAX_SEQUENCE_LENGTH
#define CTPEG_MAX_SEQUENCE_LENGTH 100
#endif

using Error_t = std::string_view;
template <typename T>
using ErrorOr = tl::expected<T, Error_t>;

using ResultVariantArray =
    std::array<ResultVariantSingle, CTPEG_MAX_SEQUENCE_LENGTH>;
using ResultVariant =
    detail::VariantCat_t<ResultVariantSingle, ResultVariantArray>;

using Result = ErrorOr<std::pair<ResultVariant, std::string_view>>;

template <typename P>
concept Parser = requires(P p, std::string_view sv) {
                     { p(sv) } -> std::same_as<Result>;
                 };

}  // namespace v0_3_1
}  // namespace ctpeg

/*
/////////////////////////////////////
///////// Internal functions ////////
////////// requiring setup //////////
/////////////////////////////////////
 */
namespace ctpeg::detail {
inline namespace v0_3_1 {

[[nodiscard]] CTPEG_CONSTEXPR
    ErrorOr<std::array<std::pair<ResultVariant, std::string_view>, 1>>
    SequenceImpl(std::string_view sv, Parser auto arg) noexcept {
    if (auto ret = arg(sv)) {
        return std::array{ret.value()};
    }
    return tl::unexpected<Error_t>(Error_t{"Failed to parse Sequence"});
}

template <Parser Arg, Parser... Args>
[[nodiscard]] CTPEG_CONSTEXPR ErrorOr<
    std::array<std::pair<ResultVariant, std::string_view>, sizeof...(Args) + 1>>
SequenceImpl(std::string_view sv, Arg arg, Args... rest) noexcept {
    if (auto ret = arg(sv)) {
        auto r = SequenceImpl(ret.value().second, rest...);
        if (r) {
            std::array<std::pair<ResultVariant, std::string_view>,
                       sizeof...(Args) + 1>
                tmp{ret.value()};
            std::copy(r.value().begin(), r.value().end(),
                      std::next(tmp.begin()));
            return tmp;
        } else {
            return tl::unexpected<Error_t>(r.error());
        }
    } else {
        return tl::unexpected<Error_t>(ret.error());
    }
}

}  // namespace v0_3_1
}  // namespace ctpeg::detail

/*
/////////////////////////////////////
////////////// Main API /////////////
/////////////////////////////////////
 */

namespace ctpeg {
inline namespace v0_3_1 {

[[nodiscard]] CTPEG_CONSTEXPR auto Choice() noexcept {
    return [](std::string_view sv) -> Result {
        CTPEG_TRACE debug::print("Choice: Failed on input \"", sv, "\".\n");
        return tl::unexpected<Error_t>(Error_t{"Failed to parse Choice"});
    };
}

[[nodiscard]] CTPEG_CONSTEXPR auto Choice(Parser auto arg,
                                          Parser auto... rest) noexcept {
    return [arg, rest...](std::string_view sv) -> Result {
        if (auto res = arg(sv)) {
            CTPEG_TRACE debug::print(
                "Choice: Successfully parsed input \"", sv,
                "\". remaining string to parse: ", res.value().second, ".\n");
            return Result{res.value()};
        } else {
            return Choice(rest...)(sv);
        }
    };
}

[[nodiscard]] CTPEG_CONSTEXPR auto Sequence(Parser auto arg,
                                            Parser auto... rest) noexcept {
    return [arg, rest...](std::string_view sv) -> Result {
        auto tmp = detail::SequenceImpl(sv, arg, rest...);
        ResultVariantArray out;
        if (tmp) {
            std::string_view remaining = sv;
            std::size_t i = 0;
            for (const auto &[res, rem] : tmp.value()) {
                remaining = rem;
                auto single = toVariant<ResultVariantSingle>(res);
                if (single) {
                    out[i++] = single.value();
                } else {
                    CTPEG_TRACE debug::print(
                        "Internal Error: Sequence: Failed on input \"", sv,
                        "\". Could not convert variant\n");
                    return tl::unexpected<Error_t>(Error_t{
                        "Internal error: Sequence: Failed to convert variant"});
                }
            }
            CTPEG_TRACE debug::print(
                "Sequence: Successfully parsed input \"", sv,
                "\". remaining string to parse: ", remaining, ".\n");
            return std::make_pair(ResultVariantArray{out}, remaining);
        } else {
            CTPEG_TRACE debug::print("Sequence: Failed on input \"", sv,
                                     "\".\n");
            return tl::unexpected<Error_t>(tmp.error());
        }
    };
}

[[nodiscard]] CTPEG_CONSTEXPR auto Many(Parser auto arg) noexcept {
    return [arg](std::string_view sv) -> Result {
        std::string_view input = sv;
        ResultVariantArray out;
        for (std::size_t i = 0; i < CTPEG_MAX_SEQUENCE_LENGTH; i++) {
            if (auto res = arg(input)) {
                if (auto single =
                        toVariant<ResultVariantSingle>(res.value().first)) {
                    out[i] = single.value();
                } else {
                    CTPEG_TRACE debug::print(
                        "Internal Error: Many: Failed on input \"", sv,
                        "\". Could not convert variant\n");
                    return tl::unexpected<Error_t>(Error_t{
                        "Internal error: Many: Failed to convert variant"});
                }
                input = res.value().second;
                if (input.empty()) {
                    CTPEG_TRACE debug::print(
                        "Many: Successfully parsed input \"", sv,
                        "\". remaining string to parse: ", input, ".\n");
                    return std::make_pair(ResultVariantArray{out}, input);
                }
            } else {
                CTPEG_TRACE debug::print(
                    "Many: Successfully parsed input \"", sv,
                    "\". remaining string to parse: ", input, ".\n");
                return std::make_pair(ResultVariantArray{out}, input);
            }
        }
        CTPEG_TRACE debug::print("Internal Error: Many: Failed on input \"", sv,
                                 "\". Fall through.\n");
        return tl::unexpected<Error_t>(
            Error_t{"Internal error: Many: Fall through"});
    };
}

[[nodiscard]] CTPEG_CONSTEXPR auto Not(Parser auto arg) noexcept {
    return [arg](std::string_view sv) -> Result {
        if (arg(sv)) {
            CTPEG_TRACE debug::print("Not: Failed on input \"", sv, "\".\n");
            return tl::unexpected<Error_t>(Error_t{"Failed to parse Not"});
        }
        CTPEG_TRACE debug::print("Not: Successfully parsed input \"", sv,
                                 "\". remaining string to parse: ", sv, ".\n");
        return {std::make_pair(ResultVariant{EmptyVariant{}}, sv)};
    };
}

[[nodiscard]] CTPEG_CONSTEXPR Result Empty(std::string_view sv) noexcept {
    return {std::make_pair(EmptyVariant{}, sv)};
}

[[nodiscard]] CTPEG_CONSTEXPR auto Skip(Parser auto arg) noexcept {
    return [arg](std::string_view sv) -> Result {
        if (auto ret = arg(sv)) {
            CTPEG_TRACE debug::print(
                "Skip: Successfully parsed input \"", sv,
                "\". remaining string to parse: ", ret.value().second, ".\n");
            return std::make_pair(EmptyVariant{}, ret.value().second);
        } else {
            CTPEG_TRACE debug::print("Skip: Failed on input \"", sv, "\".\n");
            return tl::unexpected<Error_t>(ret.error());
        }
    };
}

[[nodiscard]] CTPEG_CONSTEXPR auto Maybe(Parser auto arg) noexcept {
    return Choice(arg, Empty);
}

struct Char {
    std::optional<char> m_c;
    explicit CTPEG_CONSTEXPR Char(char c) : m_c(c) {}

    explicit CTPEG_CONSTEXPR Char() : m_c() {}

    [[nodiscard]] CTPEG_CONSTEXPR Result
    operator()(std::string_view arg) const noexcept {
        if (arg.empty()) {
            CTPEG_TRACE debug::print("Char: Failed on empty input.\n");
            return tl::unexpected<Error_t>(
                Error_t{"Could not parse Char with empty input"});
        }
        if (!m_c) {
            CTPEG_TRACE debug::print(
                "Char: Successfully parsed input \"", arg,
                "\". remaining string to parse: ", arg.substr(1), ".\n");
            return std::make_pair(ResultVariant{arg[0]}, arg.substr(1));
        }

        if (arg[0] == m_c.value()) {
            CTPEG_TRACE debug::print(
                "Char(", m_c.value(), "): Successfully parsed input \"", arg,
                "\". remaining string to parse: ", arg.substr(1), ".\n");
            return std::make_pair(ResultVariant{m_c.value()}, arg.substr(1));
        } else {
            CTPEG_TRACE debug::print("Char(", m_c.value(),
                                     "): Failed on input \"", arg, "\".\n");
            return tl::unexpected<Error_t>(Error_t{"Failed to parse Char"});
        }
    }
};

[[nodiscard]] CTPEG_CONSTEXPR auto Final(Parser auto arg) noexcept {
    return [arg](std::string_view sv) -> Result {
        if (auto ret = Sequence(arg, Not(Char()))(sv)) {
            CTPEG_TRACE debug::print("Final: Successfully parsed input \"", sv,
                                     "\".\n");
            const auto arr = std::get<ResultVariantArray>(ret.value().first);
            if (auto res = toVariant<ResultVariant>(arr[0])) {
                return std::make_pair(res.value(), ret.value().second);
            } else {
                return tl::unexpected<Error_t>(Error_t{
                    "Internal error: Final: Failed to convert variant"});
            }
        } else {
            CTPEG_TRACE debug::print("Final: Failed on input \"", sv, "\".\n");
            return tl::unexpected<Error_t>(ret.error());
        }
    };
}

struct String {
    std::string_view m_sv;
    explicit CTPEG_CONSTEXPR String(std::string_view sv) : m_sv(sv) {}

    [[nodiscard]] CTPEG_CONSTEXPR Result
    operator()(std::string_view arg) const noexcept {
        if (m_sv.size() > arg.size()) {
            CTPEG_TRACE debug::print("String(", m_sv, "): Failed on input \"",
                                     arg, "\". Input too short.\n");
            return tl::unexpected<Error_t>(
                Error_t{"Unexpected end of input when parsing String"});
        }
        if (arg.substr(0, m_sv.size()) == m_sv) {
            CTPEG_TRACE debug::print(
                "String(", m_sv, "): Successfully parsed input \"", arg,
                "\". remaining string to parse: ", arg.substr(m_sv.size()),
                ".\n");
            return std::make_pair(ResultVariant{m_sv}, arg.substr(m_sv.size()));
        }
        CTPEG_TRACE debug::print("String(", m_sv, "): Failed on input \"", arg,
                                 "\".\n");
        return tl::unexpected<Error_t>(Error_t{"Failed to parse String"});
    }
};

struct Digit {
    std::optional<int64_t> m_i;
    explicit CTPEG_CONSTEXPR Digit(int64_t i) : m_i(i) {}
    explicit CTPEG_CONSTEXPR Digit() : m_i() {}

    [[nodiscard]] CTPEG_CONSTEXPR Result
    operator()(std::string_view arg) const noexcept {
        if (arg.empty())
            return tl::unexpected<Error_t>(
                Error_t{"Unexpected end of input when parsing Digit"});

        if (!m_i) {
            if (detail::isdigit(arg[0])) {
                CTPEG_TRACE debug::print(
                    "Digit: Successfully parsed input \"", arg,
                    "\". remaining string to parse: ", arg.substr(1), ".\n");
                return std::make_pair(ResultVariant{detail::charToInt(arg[0])},
                                      arg.substr(1));
            } else {
                CTPEG_TRACE debug::print("Digit: Failed on input \"", arg,
                                         "\".\n");
                return tl::unexpected<Error_t>(
                    Error_t{"Failed to parse Digit"});
            }
        }

        if (arg[0] == (detail::digitToChar(m_i.value()))) {
            CTPEG_TRACE debug::print(
                "Digit(", m_i.value(), "): Successfully parsed input \"", arg,
                "\". remaining string to parse: ", arg.substr(1), ".\n");
            return std::make_pair(ResultVariant{m_i.value()}, arg.substr(1));
        } else {
            CTPEG_TRACE debug::print("Digit(", m_i.value(),
                                     "): Failed on input \"", arg, "\".\n");
            return tl::unexpected<Error_t>(Error_t{"Failed to parse Digit"});
        }
    }
};

struct Int {
    std::optional<int64_t> m_i;
    explicit CTPEG_CONSTEXPR Int(int64_t i) : m_i(i) {}

    CTPEG_CONSTEXPR Int() : m_i() {}

    [[nodiscard]] CTPEG_CONSTEXPR Result
    operator()(std::string_view arg) const noexcept {
        if (!m_i) {
            std::size_t i = 0;
            for (char ch : arg) {
                if (detail::isdigit(ch))
                    i++;
                else {
                    break;
                }
            }
            if (i == 0) {
                CTPEG_TRACE debug::print("Int: Failed on input \"", arg,
                                         "\".\n");
                return tl::unexpected<Error_t>(Error_t{"Failed to parse Int"});
            }
            CTPEG_TRACE debug::print(
                "Int: Successfully parsed input \"", arg,
                "\". remaining string to parse: ", arg.substr(i), ".\n");
            return std::make_pair(detail::svToInt(arg.substr(0, i)),
                                  arg.substr(i));
        }
        const auto numDigits =
            static_cast<size_t>(detail::getNumDigits(m_i.value()));
        if (arg.size() < numDigits) {
            CTPEG_TRACE debug::print("Int(", m_i.value(),
                                     "): Failed on input \"", arg,
                                     "\". Input too short.\n");
            return tl::unexpected<Error_t>(
                Error_t{"Failed to parse Int: Input is too short"});
        }
        for (std::size_t digitCounter = numDigits - 1, strCounter = 0;
             strCounter < numDigits; digitCounter--, strCounter++) {
            if (detail::digitToChar(detail::nthDigit(
                    m_i.value(), digitCounter)) != arg[strCounter]) {
                CTPEG_TRACE debug::print("Int(", m_i.value(),
                                         "): Failed on input \"", arg, "\".\n");
                return tl::unexpected<Error_t>(Error_t{"Failed to parse Int"});
            }
        }
        CTPEG_TRACE debug::print(
            "Int(", m_i.value(), "): Successfully parsed input \"", arg,
            "\". remaining string to parse: ", arg.substr(numDigits), ".\n");
        return std::make_pair(ResultVariant{m_i.value()},
                              arg.substr(numDigits));
    }
};

[[nodiscard]] constexpr auto nextNonEmpty(
    ResultVariantArray::const_iterator arr,
    ResultVariantArray::const_iterator end) noexcept {
    for (; arr != end; arr++) {
        if (!std::holds_alternative<EmptyVariant>(*arr)) {
            return arr;
        }
        if (std::holds_alternative<UninitialisedVariant>(*arr)) {
            return end;
        }
    }
    return end;
}

constexpr void variantArrayForeach(ResultVariantArray::const_iterator begin,
                                   ResultVariantArray::const_iterator end,
                                   auto &&fn) noexcept {
    for (auto it = begin;
         it != end && !std::holds_alternative<UninitialisedVariant>(*it);
         it++) {
        if (std::holds_alternative<EmptyVariant>(*it)) continue;
        fn(*it);
    }
}

template <typename To, typename From>
[[nodiscard]] constexpr std::optional<To> toVariant(From a) noexcept {
    return std::visit(
        [](auto content) -> std::optional<To> {
            if constexpr (detail::IsVariantMember_v<decltype(content), To>) {
                return To{content};
            }
            return std::nullopt;
        },
        a);
}
}  // namespace v0_3_1
}  // namespace ctpeg
#endif  // CTPEG_HPP
