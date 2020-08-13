#include "duckdb/parser/expression/comparison_expression.hpp"
#include "duckdb/planner/expression/bound_cast_expression.hpp"
#include "duckdb/planner/expression/bound_comparison_expression.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "duckdb/planner/expression_binder.hpp"
#include "duckdb/catalog/catalog_entry/collate_catalog_entry.hpp"
#include "duckdb/common/string_util.hpp"

#include "duckdb/function/scalar/string_functions.hpp"

#include "duckdb/main/config.hpp"
#include "duckdb/catalog/catalog.hpp"

using namespace std;

namespace duckdb {

unique_ptr<Expression> ExpressionBinder::PushCollation(ClientContext &context, unique_ptr<Expression> source,
                                                       string collation, bool equality_only) {
	// replace default collation with system collation
	if (collation.empty()) {
		collation = DBConfig::GetConfig(context).collation;
	}
	// bind the collation
	if (collation.empty() || collation == "binary" || collation == "c" || collation == "posix") {
		// binary collation: just skip
		return source;
	}
	auto &catalog = Catalog::GetCatalog(context);
	auto splits = StringUtil::Split(StringUtil::Lower(collation), ".");
	vector<CollateCatalogEntry *> entries;
	for (auto &collation_argument : splits) {
		auto collation_entry = catalog.GetEntry<CollateCatalogEntry>(context, DEFAULT_SCHEMA, collation_argument);
		if (collation_entry->combinable) {
			entries.insert(entries.begin(), collation_entry);
		} else {
			if (entries.size() > 0 && !entries.back()->combinable) {
				throw BinderException("Cannot combine collation types \"%s\" and \"%s\"", entries.back()->name,
				                      collation_entry->name);
			}
			entries.push_back(collation_entry);
		}
	}
	for (auto &collation_entry : entries) {
		if (equality_only && collation_entry->not_required_for_equality) {
			continue;
		}
		auto function = make_unique<BoundFunctionExpression>(collation_entry->function.return_type, collation_entry->function);
		function->children.push_back(move(source));
		if (collation_entry->function.bind) {
			function->bind_info = collation_entry->function.bind(*function, context);
		}
		source = move(function);
	}
	return source;
}

BindResult ExpressionBinder::BindExpression(ComparisonExpression &expr, idx_t depth) {
	// first try to bind the children of the case expression
	string error;
	BindChild(expr.left, depth, error);
	BindChild(expr.right, depth, error);
	if (!error.empty()) {
		return BindResult(error);
	}
	// the children have been successfully resolved
	auto &left = (BoundExpression &)*expr.left;
	auto &right = (BoundExpression &)*expr.right;
	auto left_sql_type = left.expr->return_type;
	auto right_sql_type = right.expr->return_type;
	// cast the input types to the same type
	// now obtain the result type of the input types
	auto input_type = MaxLogicalType(left_sql_type, right_sql_type);
	if (input_type.id() == LogicalTypeId::VARCHAR) {
		// for comparison with strings, we prefer to bind to the numeric types
		if (left_sql_type.IsNumeric()) {
			input_type = left_sql_type;
		} else if (right_sql_type.IsNumeric()) {
			input_type = right_sql_type;
		} else {
			// else: check if collations are compatible
			if (!left_sql_type.collation().empty() && !right_sql_type.collation().empty() &&
			    left_sql_type.collation() != right_sql_type.collation()) {
				throw BinderException("Cannot combine types with different collation!");
			}
		}
	}
	if (input_type.id() == LogicalTypeId::UNKNOWN) {
		throw BinderException("Could not determine type of parameters: try adding explicit type casts");
	}
	// add casts (if necessary)
	left.expr = BoundCastExpression::AddCastToType(move(left.expr), input_type);
	right.expr = BoundCastExpression::AddCastToType(move(right.expr), input_type);
	if (input_type.id() == LogicalTypeId::VARCHAR) {
		// handle collation
		left.expr =
		    PushCollation(context, move(left.expr), input_type.collation(), expr.type == ExpressionType::COMPARE_EQUAL);
		right.expr =
		    PushCollation(context, move(right.expr), input_type.collation(), expr.type == ExpressionType::COMPARE_EQUAL);
	}
	// now create the bound comparison expression
	return BindResult(make_unique<BoundComparisonExpression>(expr.type, move(left.expr), move(right.expr)));
}

} // namespace duckdb
