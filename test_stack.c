#include "minunit.h"
#include "node.h"
#include "stack.h"

MU_TEST(test_1) {
    Stack st = Stack_new(NULL, 0);

    Node x = Node_new_tree(NULL, NULL, NULL);

    for (int i = 0; i < 5; i++)
        Stack_push(st, x, x, x, x, INS_L);

    st = Stack_spush(st, 0);
    st = Stack_spush(st, 0);
    st = Stack_spush(st, 0);
    st = Stack_spush(st, 0);

    Stack_debug(st);
    st = Stack_spop(st, 0);
    st = Stack_spop(st, 0);
    Stack_debug(st);

    for (int i = 0; i < 5; i++)
        Stack_push(st, x, x, x, x, INS_R);

    for (int i = 0; i < 10; i++) {
        struct Frame x = Stack_pop(&st);
        Stack_debug(st);
    }
}


MU_TEST_SUITE(test_suite) {
    MU_RUN_TEST(test_1);
}

int main() {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return 0;
}
