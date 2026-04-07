#include "IRBuilder.h"

#include <sstream>

namespace ir
{
std::string build_runtime_prelude()
{
	std::ostringstream os;
	os << "declare i32 @getchar()\n";
	os << "declare i32 @printf(ptr, ...)\n";
	os << "declare i32 @scanf(ptr, ...)\n\n";
	return os.str();
}

std::string build_placeholder_module(const semantic::ProgramInfo &program_info)
{
	std::ostringstream os;
	os << "; self-hosted backend placeholder\n";
	os << "; globals=" << program_info.global_item_count << ", functions=" << program_info.function_count << "\n";
	os << build_runtime_prelude();
	return os.str();
}
} // namespace ir
