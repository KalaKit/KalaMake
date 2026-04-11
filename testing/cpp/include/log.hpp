#include <string_view>
#include <iostream>

using std::cout;
using std::string_view;

inline void print(string_view message)
{
    cout << message << "\n";
}