int add(int x, int y) {
    return x + y;
}

int main() {
    int arr[5];
    int result;

    arr[0] = 10;
    arr[1] = 20;

    result = add(arr[0], arr[1]);

    return result;
}