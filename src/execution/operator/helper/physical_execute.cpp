#include "execution/operator/helper/physical_execute.hpp"

using namespace duckdb;
using namespace std;

void PhysicalExecute::GetChunkInternal(ClientContext &context, DataChunk &chunk, PhysicalOperatorState *state_) {
	assert(plan);
	plan->GetChunk(context, chunk, state_);
}

unique_ptr<PhysicalOperatorState> PhysicalExecute::GetOperatorState() {
	return make_unique<PhysicalOperatorState>(plan->children.size() > 0 ? plan->children[0].get() : nullptr);
}
