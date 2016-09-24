# JsonOOLib
An Object-Oriented Lib to Parse/Generate Json Literal Strings
```cpp
// using json like using javascript
Value obj = {
  {"pi", 3.14},
  {"list", {0, 1, 2}},
  {"object", {
    {"key1", "value1"},
    {"key2", "value2"}
  }};
  
// using json like using a map
Object m;
m["pi"] = 3.14;
m["list"] = {0, 1, 2};
m["object"] = {{"key1", 1}, {"key2", 2}};
m.insert({"bool", false});
m.erase("list");

// using json like using stl
Array scores = {0, 1, 3, 2, 4, 5};
std::sort(scores.begin(), scores.end());
auto it = std::find(scores.cbegin(), scores.cend(), 1);

// or using json like using a vector,
// and all with Error handling capability,
// for more infomation, read the following manual or refer to ReadMe.pdf.

```

![Image](https://github.com/apreak/JsonOOLib/blob/master/README.jpg)
