#include "pch.h"

#include "enums.hpp"
#include "JsonParser.hpp"
#include "Types.hpp"

// This file must be manually included when
// using basic_parser to implement a parser.
#include <boost/json/basic_parser_impl.hpp>
#include <boost/json/error.hpp>
#include <boost/json/parse_options.hpp>
#include <boost/system/detail/error_code.hpp>

#include <cstdint>
#include <string_view>

bool handler::on_document_begin(boost::system::error_code&) const
{
    return true;
}

bool handler::on_document_end(boost::system::error_code&) const
{
    return true;
}

bool handler::on_object_begin(boost::system::error_code&)
{
    insert_container(L"{}", json_type::Object);
    return true;
}

bool handler::on_object_end(std::size_t, boost::system::error_code&)
{
    _stack.pop();
    return true;
}

bool handler::on_array_begin(boost::system::error_code&)
{
    insert_container(L"[]", json_type::Array);
    return true;
}

bool handler::on_array_end(std::size_t, boost::system::error_code&)
{
    _stack.pop();
    return true;
}

bool handler::on_key_part(const std::string_view&,
    std::size_t, boost::system::error_code&) const
{
    return true;
}

bool handler::on_key(const std::string_view& text, std::size_t,
    boost::system::error_code&)
{
    const CString key(text.data(), static_cast<int>(text.size()));
    const HTREEITEM hItem = _tree.InsertItem(key, _stack.top()._tree_item);

    _tree.SetItemData(hItem, json_type::Key);
    _stack.push(item(hItem, json_type::Key));
    return true;
}

bool handler::on_string_part(const std::string_view& text, std::size_t,
    boost::system::error_code&)
{
    _string_part += text;
    return true;
}

bool handler::on_string(const std::string_view& text, std::size_t,
    boost::system::error_code&)
{
    _string_part += text;
    insert_scalar(_string_part, json_type::String);
    _string_part.clear();
    return true;
}

bool handler::on_number_part(const std::string_view&,
    boost::system::error_code&) const
{
    return true;
}

bool handler::on_int64(std::int64_t, const std::string_view& text,
    boost::system::error_code&)
{
    insert_scalar(text, json_type::Int64);
    return true;
}

bool handler::on_uint64(std::uint64_t, const std::string_view& text,
    boost::system::error_code&)
{
    insert_scalar(text, json_type::UInt64);
    return true;
}

bool handler::on_double(double, const std::string_view& text,
    boost::system::error_code&)
{
    insert_scalar(text, json_type::Double);
    return true;
}

bool handler::on_bool(const bool value, boost::system::error_code&)
{
    insert_scalar(value ? "true" : "false", json_type::Boolean);
    return true;
}

bool handler::on_null(boost::system::error_code&)
{
    insert_scalar("null", json_type::Null);
    return true;
}

bool handler::on_comment_part(const std::string_view&,
    boost::system::error_code&) const
{
    return true;
}

bool handler::on_comment(const std::string_view&,
    boost::system::error_code&) const
{
    return true;
}

void handler::insert_container(const wchar_t* str, const json_type type)
{
    const auto full_type = _stack.empty() ?
        static_cast<json_type>(json_type::Document | type) :
        type;

    if (full_type & json_type::Document || _stack.top()._type & json_type::Array)
    {
        // Add keyless container
        const HTREEITEM hItem = _tree.InsertItem(str,
            full_type & json_type::Document ?
            TVI_ROOT :
            _stack.top()._tree_item);

        _tree.SetItemData(hItem, full_type);
        _stack.push(item(hItem, full_type));
    }
    else
    {
        // Container has key, so include type
        _tree.SetItemData(_stack.top()._tree_item,
            _tree.GetItemData(_stack.top()._tree_item) | full_type);
        _stack.top()._type = static_cast<json_type>(_stack.top()._type | full_type);
    }
}

void handler::insert_scalar(const std::string_view& text, const json_type type)
{
    const CString str(text.data(), static_cast<int>(text.size()));
    const HTREEITEM hItem = _tree.InsertItem(str, _stack.top()._tree_item);

    _tree.SetItemData(hItem, type);

    // Only pop scalars
    if ((_stack.top()._type &
        (json_type::Array | json_type::Key | json_type::Object)) ==
        json_type::Key)
    {
        _stack.pop();
    }
}

json_parser::json_parser(CTreeCtrl& tree) :
    _p(boost::json::parse_options(), tree)
{
}

std::size_t json_parser::write(char const* data, std::size_t size,
    boost::system::error_code& ec)
{
    auto const n = _p.write_some(false, data, size, ec);

    if (!ec && n < size)
        ec = boost::json::error::extra_data;

    return n;
}
