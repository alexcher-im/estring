# Estring
A header-only intrusive C++ compile and run time strings with 
full compile-time interning (including a cross-object file merging). 

Basically, it's a C-string with pre-calculated length and hash, placed before the actual data.

## How to use
```C++
#include <iostream> 
#include "estrings.hpp"


void example() {
    // copying any EstrPtr will not copy string contents
    
    // compile-time string creation
    EstrPtr compile_time_string = ESTR("hello, world!");
    
    // run-time non-owned string creation
    // this string will not be automatically freed/deleted
    EstrPtr runtime_string = estr_new("any std::string_view allowed here");
    
    // run-time owned (unique_ptr-like) string creation
    // this string will be deleted by RAII
    EstrUniquePtr unique_string = estr_unique_new("unique");
    
    // borrowing a unique string
    // should never survive the original string
    EstrPtr borrowed_string = unique_string.get();
    
    // claiming (owning) a usual string
    // this will be deleted by RAII. you can use the old string after claiming, but not after freeing
    // you are free to claim compile-time strings, but will have no effect
    EstrUniquePtr claimed_string = EstrUniquePtr::claim(runtime_string);
    
    // cloning a unique string
    EstrUniquePtr cloned_string = unique_string.clone();
    
    // manually freeing a usual string
    EstrPtr usual_string = estr_new("usual");
    usual_string.destroy();
    
    // printing (std::ostream)
    std::cout << compile_time_string << std::endl;
    
    // getting C string
    std::cout << compile_time_string.data() << std::endl;
    
    // getting std::string_view
    std::cout << compile_time_string.view() << std::endl;
    
    // getting size
    std::cout << compile_time_string.size() << std::endl;
    
    // getting hash. std::hash is implemented as well
    std::cout << compile_time_string.hash() << std::endl;
}
```

## License
Zlib! You are free to it use everywhere. Stating me 
as an author is "appreciated, but not required" (stating Zlib license).
The original license text has a precedence over this note.
