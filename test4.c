int compute_factorial(int n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * compute_factorial(n - 1);
    }
}

int calculate_sum(int limit) {
    int arr[10];
    int i = 0;
    int total = 0;

    /* Testing while loop and array assignment */
    while (i < 10) {
        arr[i] = i * 2;
        i = i + 1;
    }

    /* Testing nested control flow and complex expressions */
    i = 0;
    while (i < 10) {
        if (arr[i] > limit && i != 5) {
            total = total + arr[i];
        } else {
            total = total + 1;
        }
        i = i + 1;
    }

    return total;
}

int main() {
    int x = 5;
    int result;
    int check;

    /* Testing function calls and binary logic */
    result = compute_factorial(x);
    printf(result);

    check = calculate_sum(10);
    printf(check);

    if (result > 100 && !(check == 0)) {
        printf(result + check);
        return result + check;
    }

    return 0;
}