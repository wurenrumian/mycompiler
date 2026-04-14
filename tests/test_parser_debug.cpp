#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
bool run_command(const std::string &command)
{
	return std::system(command.c_str()) == 0;
}

void write_text(const std::string &path, const std::string &content)
{
	std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
	out << content;
}

bool file_exists(const std::string &path)
{
	std::ifstream in(path.c_str(), std::ios::binary);
	return in.good();
}

void remove_if_exists(const std::string &path)
{
	std::remove(path.c_str());
}

bool run_parser()
{
#ifdef _WIN32
	const std::string run_parser = "parser.exe";
#else
	const std::string run_parser = "./parser";
#endif
	return run_command(run_parser);
}

bool test_case(const std::string &name, const std::string &source)
{
	write_text("testfile.txt", source);
	remove_if_exists("output.ll");
	
	if (!run_parser())
	{
		std::cerr << "[FAIL] " << name << ": parser execution failed" << std::endl;
		return false;
	}
	
	if (!file_exists("output.ll"))
	{
		std::cerr << "[FAIL] " << name << ": output.ll not generated" << std::endl;
		return false;
	}
	
	std::cout << "[PASS] " << name << std::endl;
	return true;
}

} // namespace

int main()
{
	bool all_pass = true;
	
	all_pass &= test_case("1. simple_main", R"(
int main() {
    return 0;
}
)");
	
	all_pass &= test_case("2. const_global", R"(
const int G_CONST_A = 10;
int main() {
    return 0;
}
)");
	
	all_pass &= test_case("3. const_expr", R"(
const int G_CONST_A = 10;
const int G_CONST_B = G_CONST_A * 2 + 5;
int main() {
    return 0;
}
)");
	
	all_pass &= test_case("4. const_unary", R"(
const int G_CONST_A = 10;
const int G_CONST_C = - + - G_CONST_A;
int main() {
    return 0;
}
)");
	
	all_pass &= test_case("5. global_var", R"(
int global_var = 100;
int main() {
    return global_var;
}
)");
	
	all_pass &= test_case("6. global_arr", R"(
int global_arr[2][3] = {{1, 2, 3}, {4, 5, 6}};
int main() {
    return global_arr[0][0];
}
)");
	
	all_pass &= test_case("7. func_with_arr_param", R"(
int test_func(int arr[][3]) {
    return arr[0][0];
}
int main() {
    int a[1][3] = {{1,2,3}};
    return test_func(a);
}
)");
	
	all_pass &= test_case("8. simple_arith", R"(
int main() {
    int a = 10;
    int b = 3;
    int c = a + b * 2 - a / b % 2;
    return c;
}
)");
	
	all_pass &= test_case("9. unary_ops", R"(
int main() {
    int x = 10;
    int y = 5;
    int a = - + - x;
    int b = -(-y);
    return a + b;
}
)");
	
	all_pass &= test_case("10. complex_expr", R"(
int main() {
    int a = 1;
    int b = 2;
    int x = 3;
    int y = 4;
    int result = (a + b * 2 - x / y % 2) + (- + - x) - (-(-y)) + a * (b - x) / (y + 1);
    return result;
}
)");
	
	all_pass &= test_case("11. simple_while", R"(
int main() {
    int i = 0;
    int sum = 0;
    while (i < 5) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
)");
	
	all_pass &= test_case("12. while_with_mod", R"(
int main() {
    int i = 0;
    int sum = 0;
    while (i < 5) {
        sum = sum + i * 2 - (sum % (i + 1));
        i = i + 1;
    }
    return sum;
}
)");
	
	all_pass &= test_case("13. simple_if", R"(
int main() {
    int val = 1;
    if (!(val == 1)) {
        val = 2;
    }
    return val;
}
)");
	
	all_pass &= test_case("14. if_else", R"(
int main() {
    int val = 1;
    if (!(val == 1)) {
        val = 2;
    } else {
        val = 3;
    }
    return val;
}
)");
	
	all_pass &= test_case("15. if_else_chain", R"(
int main() {
    int step = 0;
    int sum = 0;
    if (step == 0) {
        sum = 100;
    } else if (step == 1) {
        sum = sum + 1;
    } else {
        sum = sum + 2;
    }
    return sum;
}
)");
	
	all_pass &= test_case("16. continue_stmt", R"(
int main() {
    int i = 0;
    int sum = 0;
    while (i < 5) {
        if (i == 2) {
            i = i + 1;
            continue;
        }
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
)");
	
	all_pass &= test_case("17. break_stmt", R"(
int main() {
    int i = 0;
    int sum = 0;
    while (i < 10) {
        if (i == 5) {
            break;
        }
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
)");
	
	all_pass &= test_case("18. block_scope", R"(
int main() {
    int val = 1;
    {
        int val = 2;
    }
    return val;
}
)");
	
	all_pass &= test_case("19. nested_blocks", R"(
int main() {
    int val = 1;
    int shadow_var = 10;
    {
        int val = 2;
        shadow_var = 20;
        {
            int val = 3;
        }
    }
    return val + shadow_var;
}
)");
	
	all_pass &= test_case("20. printf_call", R"(
int main() {
    printf("hello\n");
    return 0;
}
)");
	
	all_pass &= test_case("21. printf_with_vals", R"(
int main() {
    int a = 1;
    int b = 2;
    printf("a=%d, b=%d\n", a, b);
    return 0;
}
)");
	
	all_pass &= test_case("22. getint_call", R"(
int main() {
    int x = getint();
    return x;
}
)");
	
	all_pass &= test_case("23. getint_putint", R"(
int main() {
    int x = getint();
    putint(x);
    return 0;
}
)");
	
	all_pass &= test_case("24. test_complex_math_func", R"(
const int G_CONST_A = 10;
const int G_CONST_B = G_CONST_A * 2 + 5;

int test_complex_math(int arr[][3], int x, int y) {
    int a = arr[x % 2][y % 3];
    int b = arr[(x + 1) % 2][(y + 2) % 3];
    int result = (a + b * G_CONST_B - x / y % 2) + (- + - x) - (-(-y)) + a * (b - x) / (y + 1);
    int i = 0;
    while (i < 5) {
        result = result + i * 2 - (result % (i + 1));
        i = i + 1;
    }
    return result; 
}

int main() {
    int arr[1][3] = {{1, 2, 3}};
    return test_complex_math(arr, 1, 1);
}
)");
	
	all_pass &= test_case("25. test_deep_scopes_func", R"(
void test_deep_scopes() {
    int val = 1;
    int shadow_var = 10;
    
    printf("Scope 0: val=%d, shadow_var=%d\n", val, shadow_var);
    
    {
        int val = 2;
        shadow_var = 20;
        int hidden = 100;
        printf("Scope 1: val=%d, shadow_var=%d, hidden=%d\n", val, shadow_var, hidden);
        
        if (!(val == 1)) {
            int shadow_var = 30;
            int hidden = 200;
            val = val + 1;
            
            printf("Scope 2 (if): val=%d, shadow_var=%d, hidden=%d\n", val, shadow_var, hidden);
            
            {
                int val = shadow_var + hidden;
                printf("Scope 3 (block): val=%d\n", val);
            }
            
            printf("Scope 2 (if) after: val=%d\n", val);
        }
        
        printf("Scope 1 after: val=%d, shadow_var=%d, hidden=%d\n", val, shadow_var, hidden);
    }
    
    printf("Scope 0 after: val=%d, shadow_var=%d\n", val, shadow_var);
}

int main() {
    test_deep_scopes();
    return 0;
}
)");
	
	all_pass &= test_case("26. test_loops_func", R"(
int test_loops_and_control() {
    int sum = 0;
    int i = 0;
    
    while (!(i >= 10)) {
        int step = i % 3;
        
        if (step == 0) {
            int sum = 100;
            sum = sum + i;
        } else if (step == 1) {
            sum = sum + i;
            i = i + 1;
            continue;
        } else {
            {
                int shadow = i * i;
                sum = sum + shadow - (- + - step);
                if (sum > 50) {
                    break;
                }
            }
        }
        i = i + 1;
    }
    
    return sum;
}

int main() {
    return test_loops_and_control();
}
)");
	
	all_pass &= test_case("27. full_test", R"(
const int G_CONST_A = 10;
const int G_CONST_B = G_CONST_A * 2 + 5;
const int G_CONST_C = - + - G_CONST_A;

int global_var = 100;
int global_arr[2][3] = {{1, 2, 3}, {4, 5, 6}};

int test_complex_math(int arr[][3], int x, int y) {
    int a = arr[x % 2][y % 3];
    int b = arr[(x + 1) % 2][(y + 2) % 3];
    int result = (a + b * G_CONST_B - x / y % 2) + (- + - x) - (-(-y)) + a * (b - x) / (y + 1);
    int i = 0;
    while (i < 5) {
        result = result + i * 2 - (result % (i + 1));
        i = i + 1;
    }
    return result; 
}

void test_deep_scopes() {
    int val = 1;
    int shadow_var = 10;
    
    printf("Scope 0: val=%d, shadow_var=%d\n", val, shadow_var);
    
    {
        int val = 2;
        shadow_var = 20;
        int hidden = 100;
        printf("Scope 1: val=%d, shadow_var=%d, hidden=%d\n", val, shadow_var, hidden);
        
        if (!(val == 1)) {
            int shadow_var = 30;
            int hidden = 200;
            val = val + 1;
            
            printf("Scope 2 (if): val=%d, shadow_var=%d, hidden=%d\n", val, shadow_var, hidden);
            
            {
                int val = shadow_var + hidden;
                printf("Scope 3 (block): val=%d\n", val);
            }
            
            printf("Scope 2 (if) after: val=%d\n", val);
        }
        
        printf("Scope 1 after: val=%d, shadow_var=%d, hidden=%d\n", val, shadow_var, hidden);
    }
    
    printf("Scope 0 after: val=%d, shadow_var=%d\n", val, shadow_var);
}

int test_loops_and_control() {
    int sum = 0;
    int i = 0;
    
    while (!(i >= 10)) {
        int step = i % 3;
        
        if (step == 0) {
            int sum = 100;
            sum = sum + i;
        } else if (step == 1) {
            sum = sum + i;
            i = i + 1;
            continue;
        } else {
            {
                int shadow = i * i;
                sum = sum + shadow - (- + - step);
                if (sum > 50) {
                    break;
                }
            }
        }
        i = i + 1;
    }
    
    return sum;
}

int main() {
    int in_x;
    int in_y;
    
    in_x = getint();
    in_y = getint();
    
    if (in_y == 0) {
        in_y = 1;
    }

    int res1 = test_complex_math(global_arr, in_x, in_y);
    printf("Math Result: %d\n", res1);
    
    test_deep_scopes();
    
    int res2 = test_loops_and_control();
    printf("Loop Result: %d\n", res2);
    
    return 0;
}
)");
	
	remove_if_exists("testfile.txt");
	
	if (all_pass)
	{
		std::cout << "\n[PASS] All tests passed!" << std::endl;
		return 0;
	}
	else
	{
		std::cout << "\n[FAIL] Some tests failed!" << std::endl;
		return 1;
	}
}
