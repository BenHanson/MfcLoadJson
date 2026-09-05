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
		// Start of JSON, so advance to children
		data.hCurr = tree.GetChildItem(data.hCurr);
	else
	{
		if (type & json_type::Key)
		{
			const auto hParent = tree.GetParentItem(data.hCurr);

			// Record parent container
			data.stack.push(item(hParent, type));
		}
		else
		{
			// Record anonymous container
			data.stack.push(item(data.hCurr, type));
			// Advance to children of anonymous container
			data.hCurr = tree.GetChildItem(data.hCurr);
		}
	}
}

static void ExportText(const CTreeCtrl& tree, SData& data,
	const json_type type, const whitespace ws)
{
	const std::string text = static_cast<const char*>
		(CT2A(tree.GetItemText(data.hCurr), CP_UTF8));

	// Check the type for the value
	if (tree.GetItemData(data.hCurr) & json_type::String)
	{ 
		// Strings need to be JSON conformant
		const boost::json::string value(text);

		data.json += boost::json::serialize(value);
	}
	else
		// Otherwise record data as-is
		data.json += text;

	if (type & json_type::Key)
	{
		const auto hParent = tree.GetParentItem(data.hCurr);

		// Advance to next key (a scalar value is a child of its key)
		data.hCurr = tree.GetNextSiblingItem(hParent);
	}
	else
		// Advance to next value in an Array
		data.hCurr = tree.GetNextSiblingItem(data.hCurr);

	if (data.hCurr)
		// A sibling exists
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
		// Reset type to container whose children have been iterated over
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

		// We have finished if we are back where we started
		finished = data.stack.top()._tree_item == hItem;

		if (!finished)
		{
			// Move to next sibling
			data.hCurr = tree.GetNextSiblingItem(data.stack.top()._tree_item);

			if (data.hCurr)
				// A sibling exists
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

			// - A value for a key is always a child
			// - A key also has a flag set for Object or Array
			//   (there is no need to record the type at key level
			//   for scalars)
			// - Therefore always advance to the value but maintain the type
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
