#include "Ast.h"
#include "ParserFrontend.h"
#include "Semantic.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace
{
void write_file(const std::string &path, const std::string &content)
{
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    out << content;
}

void remove_file(const std::string &path)
{
    std::remove(path.c_str());
}

std::unique_ptr<ast::CompUnit> parse_program(const std::string &filename,
                                             const std::string &content)
{
    write_file(filename, content);

    std::unique_ptr<ast::CompUnit> root;
    std::string error;
    const bool ok = parse_file_to_ast(filename, &root, &error);
    remove_file(filename);

    assert(ok);
    assert(error.empty());
    assert(root.get() != 0);
    return root;
}
} // namespace

int main()
{
    ast::CompUnit unit;
    unit.items.push_back(ast::make_function(
        ast::Type::int_type(),
        "main",
        std::vector<ast::Param>(),
        ast::make_block(std::vector<std::unique_ptr<ast::BlockItem> >())));

    assert(unit.items.size() == 1);
    assert(unit.items[0]->kind() == ast::NodeKind::FunctionDef);
    const ast::FunctionDef *fn = static_cast<const ast::FunctionDef *>(unit.items[0].get());
    assert(fn->name == "main");
    assert(fn->return_type.kind == ast::TypeKind::Int);

    semantic::AnalyzeOptions options;

    std::unique_ptr<ast::CompUnit> valid_root = parse_program(
        "test_ast_semantic_valid.sy",
        "int main(){ putint(1); return 0; }");
    assert(semantic::analyze_program(*valid_root, options).ok);

    std::unique_ptr<ast::CompUnit> timer_root = parse_program(
        "test_ast_semantic_timer.sy",
        "int main(){ starttime(); stoptime(); return 0; }");
    assert(semantic::analyze_program(*timer_root, options).ok);

    std::unique_ptr<ast::CompUnit> bad_main_root = parse_program(
        "test_ast_semantic_bad_main.sy",
        "float main(){ return 0.0; }");
    assert(!semantic::analyze_program(*bad_main_root, options).ok);

    std::unique_ptr<ast::CompUnit> bad_const_root = parse_program(
        "test_ast_semantic_bad_const.sy",
        "int main(){ const int x = 1; x = 2; return x; }");
    assert(!semantic::analyze_program(*bad_const_root, options).ok);

    return 0;
}
