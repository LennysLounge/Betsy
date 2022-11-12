# Language design
These are my ideas for how the language can look.
My goal is to create a syntax that is consistent throughout the language and does not rely on syntacical sugar to create a good syntax.

I also want to make the syntax as easy to parse as possible.
For that reason, almost no symbols exist and every token is seperated by a space.
To further reduce the complexity of the syntax i use polish notation for expressions.

== THESE IDEAS ARE NOT FINAL AND CAN CHANGE AT ANY TIME ==

##           Compiler options
Compiler options are set in the main betsy file.
This reduces the need for dedicated build tools since the langue itself it its own build too.
```
compiler output_c_file true
compiler optimisation_level 3

compiler search_path ./modules
compiler search_path ../dependencies/someLib/modules

compiler lib kernel32.lib
```

Include other betsy files using `using`
```
using std.betsy
using lists.betsy
using concurrency.betsy
using ./test-package/greeter.betsy
```

##           Declaring variables
Declare a variable using `var`.
A variable has a name, a type and optionally a value.
```
var a int
var b int 12
var c int * b 2             # expressions are evaluated
```
Assign a value to a variable using `set`.
```
set a 4
set b c
set c * * a b c
```
The word `const` makes a variable constant and cannot be changed.
```
const ARRAY_SIZE int 14
set ARRAY_SIZE 100          # ERROR: cannot assgin a value to a constant
```


##          Calling functions
Functions are called in reverse polish notation.
```
print 144                   # prints 144
```
Functions with no parameters are called by just their name
```
...
do_the_thing                # Calls function 'do_the_thing'
...
```
Mathematical operations are just builtin functions.
```
print / 12 + 96 * 6 8       # prints 12
```

##          Control flow
Branching with `if`, `elif`, and `else`.
The words `do` and `end` create a block with a new scope (more in section Blocks).
```
if < n 16 do
    "n is smaller than 16" print
elif > n 16 do
    "n is greater than 16" print
else
    "n is equal to 16" print
end
```
Loop are done using 'while'.
```
var n int 0
while < n 20 do
    print n
    set n + n 1
end
```
== MAYBE ==  
For while loops, the iterator variable exists usually in the outer scope.
When multiple loops exist in the same scope, this can pollute the scope with left over iterator variables.
It would be nice to declare an interator variable for the looping scope only

```
while n int 0 < n 20 do
    print n
    set n + n 1
end
```
Or using keyword `loop`.
```
loop n int 0 while < n 20 do
    print n
    set n + n 1
end
```


##           Blocks
The words `do` and `end` create a block with a new scope.
```
var a int 12
if true do
    set a 144
    var b int 3
end
print a                     # 144
print b                     # ERROR: b not defined in this scope
```

Blocks can be assigned to a variable and can be executed later.
```
var saved_block fn end do          
    "Hello from inside a block" print
end
saved_block                 # Executes the block
```
The type of a block is `fn<list of input types> out < list of output types>end`.
Here the variable `saved_block` has the type `fn out end`.
If no output types exist, the `out` word can be omitted.
The variable can be declared first, and the assigned a block later.
```
var saved_block fn end      
set saved_block do          
    print "Hello from inside a block"
end
```

To abbreviate the type declaration, the word `do` automatically infers the type `fn out end`.
```
var say_hello do
    print "I say hello to you"
end
```

##           Functions
Functions are blocks that can take inputs and produce outputs.
The block is prepended with its type information and additional parameter names.
```
var greet fn string end
set greet fn name string do         # the 'string' input is named 'name'
    print & "Hello " name
end
greet "tom"                 # 'Hello tom'
```

Functions can produce zero or multiple outputs.
The `out` keyword in the type definition signals the output types.
```
var get_day_and_month fn out int int do
    return 11 12
end
print + get_day_and_month   # 23
```
Naked values at the end of a function are automatically returned
```
var square fn x int out int do
    * x x
end
print square 12             # prints '144'
```

Functions can return other functions
The function `get_greeter` takes no inputs and produces a function as its output.
The output function takes no inputs and produces no outputs.
```
var get_greeter fn out fn end do
    return do
        print "I greet you"
    end
end
var greeter fn end get_greeter
greeter                     # 'I greet you'
```

## Closure
The scope in which a function is defined is preserved for the livetime of the function.
```
var create_counter fn out fn out int end do
    var count int 0        # count is enclosed in the returned function
    return fn out int do
        set count + count 1
        return count
    end
end

var counterA auto create_counter
print counterA              # 1
print counterA              # 2
print counterA              # 3

var counterB auto create_counter
print counterB              # 1
print counterB              # 2
print counterB              # 3
```

#           Others
== THIS STUFF IS NOT VERY THOUGHT OUT ==

Run a function for each element in an iterable.
```
foreach list fn n int do
    print n
end
```

Foreach knows the type of the block and can act accordingly.
For a map, the function gets passed either the key, or a key/value pair.
```
var print_key fn key string do
    print key
end
foreach map print_key       # Only prints the keys of the map

print_entries fn key string value string do
    print & & key ": " value
end
foreach map print_entries   # Prints key and value pairs
```

Map function.  
Apply a transform function to each element in an iterable
```
transform_function fn n int out string do
    return & to_string n ", "
end
list = map list transform_function
```
Or more compact.
```
list = map list fn n int out string do & to_string n ", " end
```

## Types
```
char        # 1 byte
int         # 4 bytes
float       # 4 byte IEEE
double      # 8 byte IEEE
string      # buffer & size
```

Pointer to a different type.
```
PTR<T>      # 64bit
```

Structs can be defined.
```
struct Person start
    int age
    string name
    Date birthday
end

struct Date start
    int day
    int month
    int year
end
```

Structs can be accessed
```
Person p
p.name = "Steve"
print p.name
```

Assign multiple variable at once using `unpack` and `from`.
```
var get_ABCD fn out int int int int do
    return 10 11 12 13
end
unpack a b c d from get_ABCD
```
