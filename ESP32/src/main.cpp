
void my_main() {}

// The runtime environment expects a "C" main.
extern "C" {
void app_main() { my_main(); }
}