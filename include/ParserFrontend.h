#pragma once

#include <memory>
#include <string>

#include "Ast.h"

bool parse_file_to_ast(const std::string &input_path,
				   std::unique_ptr<ast::CompUnit> *root,
				   std::string *error_message);
