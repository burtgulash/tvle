#include "minunit.h"
#include "vec.h"

VEC_OF(int);

MU_TEST(test_vec) {
    struct Vec_int v;
    VEC_INIT(&v, 2);

    for (int i = 0; i < 104; i++) {
        VEC_PUSH(&v, i);
        VEC_DEBUG(&v, "%d");
    }
    printf("\n\n");
    for (int i = 0; i < 100; i++) {
        VEC_POP(&v);
        VEC_DEBUG(&v, "%d");
    }

    int last = VEC_POP(&v);

    printf("LAST: %d, LEN: %d\n", last, v.cap);
}


MU_TEST_SUITE(test_suite) {
    MU_RUN_TEST(test_vec);
}

int main() {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return 0;
}
