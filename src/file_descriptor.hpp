#pragma once

class file_descriptor final {
public:
    file_descriptor() {};
    file_descriptor(int);
    ~file_descriptor();

    file_descriptor(file_descriptor&& other) : fd(other.fd) { other.fd = -1; }

    file_descriptor& operator=(file_descriptor&& other);

    void close();
    bool valid() const { return fd >= 0; }

    int get_raw() const { return fd; }
    int operator*() const { return fd; }

private:
    file_descriptor(const file_descriptor&) = delete;

    int fd = -1;
};
