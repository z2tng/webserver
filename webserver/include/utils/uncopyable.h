#pragma once

namespace utils {

class Uncopyable {
public:
    Uncopyable(const Uncopyable&) = delete;
    Uncopyable& operator=(const Uncopyable&) = delete;

    Uncopyable(Uncopyable&&) = default;
    Uncopyable& operator=(Uncopyable&&) = default;

protected:
    Uncopyable() = default;
    ~Uncopyable() = default;
};

} // namespace utils