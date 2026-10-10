#include <app.hpp>
#include <command.hpp>

int main(int argc, char** argv) {
    c2::AppContext app;
    int result = 0;
    if (c2::initApp(app)) {
        result = c2::runApp(app);
    }

    c2::shutdownApp(app);
    return result;
}
