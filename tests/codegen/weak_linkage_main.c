int weak_entry(void);
int strong_entry(void);

int main(void) {
    return weak_entry() + strong_entry();
}
