int main() {
    int a;
    int b;

    a = 5;
    b = 10;

    if (a < b) {
        a = a + b;
    } else {
        a = a * b;
    }

    while (a > 0) {
        a = a - 1;
    }

    return a;
}