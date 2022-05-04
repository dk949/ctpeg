#include "../ctpeg.hpp"
template <typename Parser, typename Result>
CTPEG_CONSTEXPR bool testSuccess(std::string_view input, const Parser &parser,
                                 const Result &expectedResult,
                                 std::string_view expectedRemaining) {
    const auto parserRet = parser(input);
    if (!parserRet) return false;
    const auto actualResult = std::get<Result>(parserRet.value().first);
    if (actualResult != expectedResult) return false;
    const auto actualRemaining = parserRet.value().second;
    if (actualRemaining != expectedRemaining) return false;

    return true;
}

template <typename Parser, typename Result>
CTPEG_CONSTEXPR bool testSuccessArray(
    std::string_view input, const Parser &parser,
    std::initializer_list<Result> expectedResult,
    std::string_view expectedRemaining) {
    const auto parserRet = parser(input);
    if (!parserRet) return false;
    const auto array =
        std::get<ctpeg::ResultVariantArray>(parserRet.value().first);
    bool passed = true;
    ctpeg::variantArrayForeach(
        array.cbegin(), array.cend(),
        [it = expectedResult.begin(), &passed](const auto &res) mutable {
            const auto actualResult = std::get<Result>(res);
            if (passed) passed = actualResult == *it;
            it++;
        });

    return passed && parserRet.value().second == expectedRemaining;
}

template <typename Parser>
CTPEG_CONSTEXPR bool testFailure(std::string_view input, const Parser &parser) {
    return !parser(input);
}

int main() {
    using namespace std::literals;
    // Char()
    CTPEG_ASSERT(testSuccess("abc", ctpeg::Char(), 'a', "bc"));
    CTPEG_ASSERT(testSuccess("xyz", ctpeg::Char(), 'x', "yz"));
    CTPEG_ASSERT(testFailure("", ctpeg::Char()));

    // Char(char)
    CTPEG_ASSERT(testSuccess("abc", ctpeg::Char('a'), 'a', "bc"));
    CTPEG_ASSERT(testFailure("xyz", ctpeg::Char('a')));
    CTPEG_ASSERT(testFailure("", ctpeg::Char('a')));

    // Digit()
    CTPEG_ASSERT(testSuccess("1ab", ctpeg::Digit(), std::int64_t(1), "ab"));
    CTPEG_ASSERT(testFailure("abc", ctpeg::Digit()));
    CTPEG_ASSERT(testFailure("", ctpeg::Digit()));

    // Digit(int)
    CTPEG_ASSERT(testSuccess("1ab", ctpeg::Digit(1), std::int64_t(1), "ab"));
    CTPEG_ASSERT(testFailure("2ab", ctpeg::Digit(1)));
    CTPEG_ASSERT(testFailure("abc", ctpeg::Digit(1)));
    CTPEG_ASSERT(testFailure("", ctpeg::Digit(1)));

    // String(std::string_view)
    CTPEG_ASSERT(testSuccess("abcdef", ctpeg::String("abc"), "abc"sv, "def"));
    CTPEG_ASSERT(testFailure("abxyz", ctpeg::String("abc")));
    CTPEG_ASSERT(testFailure("axcxyz", ctpeg::String("abc")));
    CTPEG_ASSERT(testFailure("xbcxyz", ctpeg::String("abc")));
    CTPEG_ASSERT(testFailure("", ctpeg::String("abc")));

    // Int()
    CTPEG_ASSERT(testSuccess("123abc", ctpeg::Int(), std::int64_t(123), "abc"));
    CTPEG_ASSERT(testFailure("abcdef", ctpeg::Int()));
    CTPEG_ASSERT(testFailure("", ctpeg::Int()));

    // Int(int)
    CTPEG_ASSERT(testSuccess("12ab", ctpeg::Int(12), std::int64_t(12), "ab"));
    CTPEG_ASSERT(testFailure("14abc", ctpeg::Int(12)));
    CTPEG_ASSERT(testFailure("143abc", ctpeg::Int(12)));
    CTPEG_ASSERT(testFailure("423abc", ctpeg::Int(12)));
    CTPEG_ASSERT(testFailure("1abc", ctpeg::Int(12)));
    CTPEG_ASSERT(testFailure("1abc", ctpeg::Int(12)));
    CTPEG_ASSERT(testFailure("abc", ctpeg::Int(12)));
    CTPEG_ASSERT(testFailure("", ctpeg::Int(12)));

    // Empty
    CTPEG_ASSERT(testSuccess("ab", ctpeg::Empty, ctpeg::EmptyVariant{}, "ab"));
    CTPEG_ASSERT(testSuccess("", ctpeg::Empty, ctpeg::EmptyVariant{}, ""));

    // Choice
    CTPEG_ASSERT(testSuccess("abcde",
                             ctpeg::Choice(ctpeg::Char('a'), ctpeg::Char('b')),
                             'a', "bcde"));
    CTPEG_ASSERT(testSuccess(
        "acde", ctpeg::Choice(ctpeg::Char('a'), ctpeg::Char('b')), 'a', "cde"));
    CTPEG_ASSERT(testSuccess(
        "bcde", ctpeg::Choice(ctpeg::Char('a'), ctpeg::Char('b')), 'b', "cde"));
    CTPEG_ASSERT(
        testFailure("cde", ctpeg::Choice(ctpeg::Char('a'), ctpeg::Char('b'))));
    CTPEG_ASSERT(
        testFailure("", ctpeg::Choice(ctpeg::Char('a'), ctpeg::Char('b'))));

    // Not
    CTPEG_ASSERT(testSuccess("bcde", Not(ctpeg::Char('a')),
                             ctpeg::EmptyVariant{}, "bcde"));
    CTPEG_ASSERT(testFailure("abcde", Not(ctpeg::Char('a'))));
    CTPEG_ASSERT(
        testSuccess("", Not(ctpeg::Char('a')), ctpeg::EmptyVariant{}, ""));

    // Skip
    CTPEG_ASSERT(testSuccess("abcde", ctpeg::Skip(ctpeg::Char('a')),
                             ctpeg::EmptyVariant{}, "bcde"));
    CTPEG_ASSERT(testFailure("bcde", ctpeg::Skip(ctpeg::Char('a'))));
    CTPEG_ASSERT(testFailure("", ctpeg::Skip(ctpeg::Char('a'))));

    // Maybe
    CTPEG_ASSERT(
        testSuccess("abcde", ctpeg::Maybe(ctpeg::Char('a')), 'a', "bcde"));
    CTPEG_ASSERT(testSuccess("bcde", ctpeg::Maybe(ctpeg::Char('a')),
                             ctpeg::EmptyVariant{}, "bcde"));
    CTPEG_ASSERT(testSuccess("", ctpeg::Maybe(ctpeg::Char('a')),
                             ctpeg::EmptyVariant{}, ""));

    // Final
    CTPEG_ASSERT(testSuccess("a", Final(ctpeg::Char('a')), 'a', ""));
    CTPEG_ASSERT(testFailure("abcde", Final(ctpeg::Char('a'))));
    CTPEG_ASSERT(testFailure("bcde", Final(ctpeg::Char('a'))));
    CTPEG_ASSERT(testFailure("", Final(ctpeg::Char('a'))));

    // Sequence
    CTPEG_ASSERT(testSuccessArray(
        "abcde", ctpeg::Sequence(ctpeg::Char('a'), ctpeg::Char('b')),
        {'a', 'b'}, "cde"));

    CTPEG_ASSERT(testFailure(
        "acde", ctpeg::Sequence(ctpeg::Char('a'), ctpeg::Char('b'))));
    CTPEG_ASSERT(testFailure(
        "cde", ctpeg::Sequence(ctpeg::Char('a'), ctpeg::Char('b'))));
    CTPEG_ASSERT(
        testFailure("", ctpeg::Sequence(ctpeg::Char('a'), ctpeg::Char('b'))));

    // Many
    CTPEG_ASSERT(testSuccessArray("aaabcd", ctpeg::Many(ctpeg::Char('a')),
                                  {'a', 'a', 'a'}, "bcd"));
    CTPEG_ASSERT(
        testSuccessArray("abcd", ctpeg::Many(ctpeg::Char('a')), {'a'}, "bcd"));
    CTPEG_ASSERT(testSuccessArray("bcd", ctpeg::Many(ctpeg::Char('a')),
                                  std::initializer_list<char>{}, "bcd"));
    CTPEG_ASSERT(testSuccessArray("", ctpeg::Many(ctpeg::Char('a')),
                                  std::initializer_list<char>{}, ""));

    static_assert(ctpeg::Parser<ctpeg::Char>);
    static_assert(ctpeg::Parser<ctpeg::Digit>);
    static_assert(ctpeg::Parser<ctpeg::String>);
    static_assert(ctpeg::Parser<ctpeg::Int>);
    static_assert(ctpeg::Parser<decltype(ctpeg::Empty)>);
    static_assert(ctpeg::Parser<decltype(ctpeg::Choice(ctpeg::Char()))>);
    static_assert(ctpeg::Parser<decltype(ctpeg::Not(ctpeg::Char()))>);
    static_assert(ctpeg::Parser<decltype(ctpeg::Skip(ctpeg::Char()))>);
    static_assert(ctpeg::Parser<decltype(ctpeg::Maybe(ctpeg::Char()))>);
    static_assert(ctpeg::Parser<decltype(ctpeg::Final(ctpeg::Char()))>);
    static_assert(ctpeg::Parser<decltype(ctpeg::Sequence(ctpeg::Char()))>);
    static_assert(ctpeg::Parser<decltype(ctpeg::Many(ctpeg::Char()))>);
}
