#pragma once

template <typename T>
class singleton
{
public:
    static T& get_singleton()
    {
        static T instance{};
        return instance;
    }
protected:
    singleton() {}
    ~singleton() {}
public:
    singleton(singleton const&) = delete;
    singleton& operator=(singleton const&) = delete;
};