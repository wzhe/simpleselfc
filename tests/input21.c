char c;
char *str;

int main() {
    c = '\n'; printchar(c);
    for (str = "Hello world\n"; *str != 0; str = str + 1) {
        printchar(*str);
    }
    return (0);
}
