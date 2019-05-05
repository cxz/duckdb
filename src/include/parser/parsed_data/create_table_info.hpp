//===----------------------------------------------------------------------===//
//                         DuckDB
//
// parser/parsed_data/create_table_info.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/common.hpp"
#include "common/unordered_set.hpp"
#include "parser/column_definition.hpp"
#include "parser/constraint.hpp"
#include "planner/expression.hpp"

namespace duckdb {
class CatalogEntry;

struct CreateTableInfo {
	//! Schema name to insert to
	string schema;
	//! Table name to insert to
	string table;
	//! List of columns of the table
	vector<ColumnDefinition> columns;
	//! List of constraints on the table
	vector<unique_ptr<Constraint>> constraints;
	//! Bound default values
	vector<unique_ptr<Expression>> bound_defaults;
	//! Dependents of the table (in e.g. default values)
	unordered_set<CatalogEntry *> dependencies;
	//! Ignore if the entry already exists, instead of failing
	bool if_not_exists = false;
	//! Whether or not it is a temporary table
	bool temporary = false;

	CreateTableInfo() : schema(DEFAULT_SCHEMA), if_not_exists(false), temporary(false) {
	}
	CreateTableInfo(string schema, string name)
	    : schema(schema), table(name), if_not_exists(false), temporary(false) {
	}
};

}
