#include "ParserFrontend.h"

#include <exception>

#include "Lexer.h"

extern int yyparse();
extern void set_lexer(Lexer *lexer);
extern void reset_parser_error();
extern std::string take_parser_error();

bool parse_file_to_ast(const std::string &input_path,
				   std::unique_ptr<ast::CompUnit> *root,
				   std::string *error_message)
{
	if (root == nullptr)
	{
		if (error_message != nullptr)
		{
			*error_message = "AST output pointer is null.";
		}
		return false;
	}

	root->reset();
	reset_parser_error();

	try
	{
		Lexer lexer(input_path);
		set_lexer(&lexer);
		if (yyparse() != 0)
		{
			if (error_message != nullptr)
			{
				*error_message = take_parser_error();
				if (error_message->empty())
				{
					*error_message = "Parser failed without diagnostic.";
				}
			}
			return false;
		}

		*root = ast::take_ast_root();
		if ((*root).get() == nullptr)
		{
			if (error_message != nullptr)
			{
				*error_message = "Parser succeeded but AST root is missing.";
			}
			return false;
		}
	}
	catch (const std::exception &e)
	{
		if (error_message != nullptr)
		{
			*error_message = e.what();
		}
		return false;
	}

	return true;
}
