# c-lifetime
Introducing a lifetime system for the C language

For example:
```c
#include "lifetime.h"

int main() {
        lftm(4, {
                void *mem1 = lftm_malloc(114514);
                void *mem2 = lftm_malloc(114514);
                lftm_export(mem1);
        })
        return 0;
}
```

The memory requested by the `lftm_malloc` function will be automatically released at the end of its lifetime.   
If it is not in a lifetime, the behavior is no different from that of ordinary `malloc`.

Enjoy!
