#include <iostream>
#include <fstream>
#include <cstddef>
#include <initializer_list>
#include <string>
#include "JsonOOLib.h"

using namespace std;
USING_JSON_UTILITIES


int main()
{
    try
    {
        Value v1 = {"a",1};
        cout << v1.Serialize() << endl;

        Value v2 = {{"a",1}};
        cout << v2.Serialize() << endl;

        Value v3 = {{"a", 1}, 1};
        cout << v3.Serialize() << endl;

        Value v4 = {{1,2,3}, {}, {{"a", {{"a1", 1}, {"b1", {}}}}, {"b", 3.1415929}}, 1};
        cout << v4.Serialize() << endl;

        Value v5 = {};
        cout << v5.Serialize() << endl;

        Value v6;
        cout << v6.Serialize() << endl;
    }
    catch(JsonError e)
    {
        cout << e.What() << endl;
        cout << "code: " << e.Code() << endl;
    }

    return 0;
}
