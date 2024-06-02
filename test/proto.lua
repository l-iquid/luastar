const some_constant: i32 = 53;

$macro exmcro 5

struct b {
	*c: i32;
};

local a: b = {
	c = &some_constant, -- throws warning because it's const and the pointer isn't
};

print(a.c);

switch (exmcro) {
  case (5) {
    print("macro is 5!");
  }
  default {
    print("macro is not 5");
  }
}

local d: string = "Hello, world!";
print(d);