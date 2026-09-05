#pragma once

#include "enums.hpp"
#include "Types.hpp"

#include <boost/system/detail/error_code.hpp>
#include <boost/json/basic_parser.hpp>

#include <cstdint>
#include <stack>
#include <string>
#include <string_view>

struct handler
{
    CTreeCtrl& _tree;
    std::stack<item> _stack;
    std::string _string_part;

    constexpr static std::size_t max_object_size = std::size_t(~0);
    constexpr static std::size_t max_array_size = std::size_t(~0);
    constexpr static std::size_t max_key_size = std::size_t(~0);
    constexpr static std::size_t max_string_size = std::size_t(~0);

    bool on_document_begin(boost::system::error_code&) const;
    bool on_document_end(boost::system::error_code&) const;
    bool on_object_begin(boost::system::error_code&);
    bool on_object_end(std::size_t, boost::system::error_code&);
    bool on_array_begin(boost::system::error_code&);
    bool on_array_end(std::size_t, boost::system::error_code&);
    bool on_key_part(const std::string_view&, std::size_t,
        boost::system::error_code&) const;
    bool on_key(const std::string_view& text, std::size_t,
        boost::system::error_code&);
    bool on_string_part(const std::string_view& text, std::size_t,
        boost::system::error_code&);
    bool on_string(const std::string_view& text, std::size_t,
        boost::system::error_code&);
    bool on_number_part(const std::string_view&,
        boost::system::error_code&) const;
    bool on_int64(std::int64_t, const std::string_view& text,
        boost::system::error_code&);
    bool on_uint64(std::uint64_t, const std::string_view& text,
        boost::system::error_code&);
    bool on_double(double, const std::string_view& text,
        boost::system::error_code&);
    bool on_bool(const bool value, boost::system::error_code&);
    bool on_null(boost::system::error_code&);
    bool on_comment_part(const std::string_view& text,
        boost::system::error_code&) const;
    bool on_comment(const std::string_view& text,
        boost::system::error_code&) const;

    void insert_container(const wchar_t* str, const json_type type);
    void insert_scalar(const std::string_view& text, const json_type type);
};

class json_parser
{
public:
    explicit json_parser(CTreeCtrl& tree);

    std::size_t write(char const* data, std::size_t size,
        boost::system::error_code& ec);

private:
    boost::json::basic_parser<handler> _p;
};
