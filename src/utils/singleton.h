#ifndef SINGLETON_H
#define SINGLETON_H

/**
 * @brief Thread-safe(ish) Singleton Template for embedded systems
 * 
 * Usage:
 * class MyClass : public Singleton<MyClass> {
 *     friend class Singleton<MyClass>;
 * private:
 *     MyClass() {} // Private constructor
 * public:
 *     void doSomething();
 * };
 * 
 * // Access:
 * MyClass::getInstance()->doSomething();
 */
template <typename T>
class Singleton {
public:
    // Delete copy constructor and assignment operator
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    /**
     * @brief Get the Singleton instance
     * 
     * Uses lazy initialization. The instance is created on the first call.
     * Note: On standard C++11 compliant compilers, static local variables are thread-safe.
     * On some embedded platforms, this might need explicit locking if used in multi-threaded contexts (FreeRTOS tasks).
     * However, for simplicity and standard Arduino usage, this is sufficient.
     */
    static T* getInstance() {
        static T instance;
        return &instance;
    }

protected:
    Singleton() {}
    virtual ~Singleton() {}
};

#endif // SINGLETON_H
