/**
 * @file Singleton.h
 * @author KevinGlaser
 * @brief Base class for singleton pattern
 * @version 0.1
 * @date 2025-03-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __SINGLETON_H__
#define __SINGLETON_H__

template<typename T>
class Singleton {
public:
    template<typename... Args>
    static T& GetInstance(Args&&... args) {
        static T instance(std::forward<Args>(args)...);
        return instance;
    }
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&) = delete;

protected:
    Singleton() { std::cout <<"Singleton" << std::endl; }
    virtual ~Singleton() { std::cout <<"~Singleton" << std::endl; }
};

#endif