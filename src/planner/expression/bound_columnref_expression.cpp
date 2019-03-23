#include "planner/expression/bound_columnref_expression.hpp"

using namespace duckdb;
using namespace std;

BoundColumnRefExpression::BoundColumnRefExpression(string alias, TypeId type, SQLType sql_type, ColumnBinding binding,
                                                   uint32_t depth)
    : Expression(ExpressionType::BOUND_COLUMN_REF, ExpressionClass::BOUND_COLUMN_REF, type, sql_type), binding(binding),
      depth(depth) {
	this->alias = alias;
}

BoundColumnRefExpression::BoundColumnRefExpression(TypeId type, SQLType sql_type, ColumnBinding binding, uint32_t depth)
    : BoundColumnRefExpression(string(), type, sql_type, binding, depth) {
}

unique_ptr<Expression> BoundColumnRefExpression::Copy() {
	return make_unique<BoundColumnRefExpression>(alias, return_type, sql_type, binding, depth);
}

uint64_t BoundColumnRefExpression::Hash() const {
	auto result = Expression::Hash();
	result = CombineHash(result, duckdb::Hash<uint32_t>(binding.column_index));
	result = CombineHash(result, duckdb::Hash<uint32_t>(binding.table_index));
	return CombineHash(result, duckdb::Hash<uint32_t>(depth));
}

bool BoundColumnRefExpression::Equals(const BaseExpression *other_) const {
	if (!BaseExpression::Equals(other_)) {
		return false;
	}
	auto other = (BoundColumnRefExpression *)other_;
	return other->binding == binding && other->depth == depth;
}

string BoundColumnRefExpression::ToString() const {
	return "#[" + to_string(binding.table_index) + "." + to_string(binding.column_index) + "]";
}