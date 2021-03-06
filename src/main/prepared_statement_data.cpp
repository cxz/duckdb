#include "duckdb/main/prepared_statement_data.hpp"
#include "duckdb/execution/physical_operator.hpp"

namespace duckdb {
using namespace std;

PreparedStatementData::PreparedStatementData(StatementType type)
    : statement_type(type), read_only(true), requires_valid_transaction(true) {
}

PreparedStatementData::~PreparedStatementData() {
}

void PreparedStatementData::Bind(vector<Value> values) {
	// set parameters
	if (values.size() != value_map.size()) {
		throw BinderException("Parameter/argument count mismatch for prepared statement");
	}
	// bind the values
	for (idx_t i = 0; i < values.size(); i++) {
		auto it = value_map.find(i + 1);
		if (it == value_map.end()) {
			throw BinderException("Could not find parameter with index %llu", i + 1);
		}
		if (values[i].type() != it->second->type()) {
			throw BinderException(
			    "Type mismatch for binding parameter with index %llu, expected type %s but got type %s", i + 1,
			    values[i].type().ToString().c_str(), it->second->type().ToString().c_str());
		}
		auto &target = it->second;
		*target = values[i];
	}
}

LogicalType PreparedStatementData::GetType(idx_t param_idx) {
	auto it = value_map.find(param_idx);
	if (it == value_map.end()) {
		throw BinderException("Could not find parameter with index %llu", param_idx);
	}
	return it->second->type();
}

} // namespace duckdb
