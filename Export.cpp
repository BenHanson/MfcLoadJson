#include "pch.h"

#include "enums.hpp"
#include "Export.hpp"
#include "Types.hpp"

#include <boost/json/serialize.hpp>
#include <boost/json/string.hpp>

#include <atlconv.h>

#include <format>
#include <stack>
#include <string>

constexpr int indent_size = 2;

struct SData
{
	std::string json;
	HTREEITEM hCurr{};
	int indent{};
	std::stack<item> stack;
};

static void ExportContainer(const CTreeCtrl& tree, SData& data,
	const json_type type, const whitespace ws)
{
	if (ws == whitespace::yes)
		data.json += '\n';

	data.indent += indent_size;

	if (type & json_type::Document)
		data.hCurr = tree.GetChildItem(data.hCurr);
	else
	{
		if (type & json_type::Key)
			data.stack.push(item(tree.GetParentItem(data.hCurr), type));
		else
			data.stack.push(item(data.hCurr, type));

		if (!(type & json_type::Key))
			data.hCurr = tree.GetChildItem(data.hCurr);
	}
}

static void ExportText(const CTreeCtrl& tree, SData& data,
	const json_type type, const whitespace ws)
{
	const std::string text = static_cast<const char*>
		(CT2A(tree.GetItemText(data.hCurr), CP_UTF8));

	if (tree.GetItemData(data.hCurr) & json_type::String)
	{
		const boost::json::string value(text);

		data.json += boost::json::serialize(value);
	}
	else
		data.json += text;

	if (type & json_type::Key)
		data.hCurr = tree.GetNextSiblingItem(tree.GetParentItem(data.hCurr));
	else
		data.hCurr = tree.GetNextSiblingItem(data.hCurr);

	if (data.hCurr)
		data.json += ',';

	if (ws == whitespace::yes)
		data.json += '\n';
}

static void UnwindStack(const CTreeCtrl& tree, const HTREEITEM hItem,
	SData& data, json_type& type, const whitespace ws)
{
	bool finished = false;

	while (!finished && !data.hCurr)
	{
		type = data.stack.top()._type;
		data.indent -= indent_size;

		if (type & json_type::Object)
			data.json += std::format("{}}}", ws == whitespace::yes ?
				std::string(data.indent, ' ') :
				std::string());
		else if (type & json_type::Array)
			data.json += std::format("{}]", ws == whitespace::yes ?
				std::string(data.indent, ' ') :
				std::string());

		finished = data.stack.top()._tree_item == hItem;

		if (!finished)
		{
			data.hCurr = tree.GetNextSiblingItem(data.stack.top()._tree_item);

			if (data.hCurr)
				data.json += ',';
		}

		if (ws == whitespace::yes)
			data.json += '\n';

		data.stack.pop();
	}
}

std::string Export(const HTREEITEM hItem, const CTreeCtrl& tree,
	const whitespace ws)
{
	if (!hItem)
		return std::string();

	SData data;

	data.hCurr = hItem;
	data.stack.push(item(data.hCurr,
		static_cast<json_type>(tree.GetItemData(data.hCurr))));

	while (data.hCurr && !data.stack.empty())
	{
		auto type = static_cast<json_type>(tree.GetItemData(data.hCurr));

		if (ws == whitespace::yes)
			data.json += std::string(data.indent, ' ');

		if (type & json_type::Key)
		{
			data.json += std::format(R"("{}":)", static_cast<const char*>
				(CT2A(tree.GetItemText(data.hCurr), CP_UTF8)));

			if (ws == whitespace::yes)
				data.json += ' ';

			data.hCurr = tree.GetChildItem(data.hCurr);
		}

		if (type & json_type::Object)
		{
			data.json += '{';
			ExportContainer(tree, data, type, ws);
		}
		else if (type & json_type::Array)
		{
			data.json += '[';
			ExportContainer(tree, data, type, ws);
		}
		else
		{ 
			ExportText(tree, data, type, ws);
		}

		UnwindStack(tree, hItem, data, type, ws);
	}

	return data.json;
}
