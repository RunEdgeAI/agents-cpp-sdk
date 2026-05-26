/**
 * @file coroutine_utils.h
 * @brief Coroutine Utils Definitions
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2026 Edge AI, LLC. All rights reserved.
 *
 */
#pragma once

#include <agents-cpp/types.h>

#include <condition_variable>
#include <coroutine>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace agents {

/**
 * @brief Standard C++20 coroutine-based Task implementation
 * @tparam T The result type of the task
 */
template <typename T>
class Task {
public:
    /**
     * @brief Promise type for Task
     */
    struct promise_type {
        /**
         * @brief Result for Task
         * @return The result for Task
         */
        std::optional<T> result;
        /**
         * @brief Exception for Task
         * @return The exception for Task
         */
        std::exception_ptr exception;
        /**
         * @brief Continuation for Task
         * @return The continuation for Task
         */
        std::coroutine_handle<> continuation{nullptr};
        /**
         * @brief On completed callbackfor Task
         */
        std::function<void()> on_completed; // used by blockingWait

        /**
         * @brief Get the return object for Task
         * @return The return object for Task
         */
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        /**
         * @brief Initial suspend for Task
         * @return The initial suspend for Task
         */
        std::suspend_always initial_suspend() noexcept { return {}; }
        /**
         * @brief Final awaiter for Task
         */
        struct final_awaiter {
            /**
             * @brief Await ready for Task
             * @return Whether the Task is ready
             */
            bool await_ready() noexcept { return false; }
            /**
             * @brief Await suspend for Task
             * @param h The handle to suspend
             * @return The suspended handle
             */
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                // Notify blocking waiters (if any) BEFORE transferring control,
                // so the promise memory is still valid during callback.
                if (h.promise().on_completed) {
                    h.promise().on_completed();
                }
                // Transfer control to the awaiting coroutine without immediate resume.
                return h.promise().continuation ? h.promise().continuation : std::noop_coroutine();
            }
            /**
             * @brief Await resume for Task
             */
            void await_resume() noexcept {}
        };
        /**
         * @brief Final suspend for Task
         * @return The final suspend for Task
         */
        final_awaiter final_suspend() noexcept { return {}; }
        /**
         * @brief Unhandled exception for Task
         */
        void unhandled_exception() { exception = std::current_exception(); }
        /**
         * @brief Return value for Task
         * @param value The value to return
         */
        template <typename U>
        void return_value(U&& value) { result = std::forward<U>(value); }
    };

    /**
     * @brief Handle type for Task
     */
    using handle_type = std::coroutine_handle<promise_type>;

    /**
     * @brief Constructor with no handle
     */
    Task() noexcept : _coro(nullptr) {}
    /**
     * @brief Constructor with handle
     * @param h The handle to use
     */
    explicit Task(handle_type h) : _coro(h) {}
    /**
     * @brief Constructor with other Task
     * @param other The other Task to use
     */
    Task(Task&& other) noexcept : _coro(std::exchange(other._coro, {})) {}
    /**
     * @brief Operator= for Task
     * @param other The other Task to use
     * @return The Task
     */
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (_coro) _coro.destroy();
            _coro = std::exchange(other._coro, {});
        }
        return *this;
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    ~Task() { if (_coro) _coro.destroy(); }

    /**
     * @brief Valid for Task
     * @return Whether the Task is valid
     */
    bool valid() const noexcept { return _coro != nullptr; }

    // Awaiter interface
    /**
     * @brief Await ready for Task
     * @return Whether the Task is ready
     */
    bool await_ready() const noexcept { return !_coro || _coro.done(); }
    /**
     * @brief Await suspend for Task
     * @param awaiting The awaiting coroutine handle
     */
    void await_suspend(std::coroutine_handle<> awaiting) {
        _coro.promise().continuation = awaiting;
        _coro.resume();
    }
    /**
     * @brief Await resume for Task
     * @return The awaited result if `T` is not `void`.
     */
    T await_resume() {
        if (_coro.promise().exception) {
            std::rethrow_exception(_coro.promise().exception);
        }
        return std::move(*_coro.promise().result);
    }

    /**
     * @brief Synchronously wait and get the value (used by blockingWait)
     * @return The result of the Task
     */
    T get_sync() {
        if (!_coro) {
            throw std::runtime_error("Invalid Task");
        }
        std::mutex m;
        std::condition_variable cv;
        bool done = false;
        _coro.promise().on_completed = [&](){
            std::unique_lock<std::mutex> lk(m);
            done = true;
            lk.unlock();
            cv.notify_one();
        };
        _coro.resume();
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&]{ return done; });
        if (_coro.promise().exception) {
            std::rethrow_exception(_coro.promise().exception);
        }
        return std::move(*_coro.promise().result);
    }

private:
    handle_type _coro;
};

/**
 * @brief Task specialization for void
 */
template <>
class Task<void> {
public:
    /**
     * @brief Promise type for Task
     */
    struct promise_type {
        /**
         * @brief Exception for Task
         * @return The exception for Task
         */
        std::exception_ptr exception;
        /**
         * @brief Continuation for Task
         * @return The continuation for Task
         */
        std::coroutine_handle<> continuation{nullptr};
        /**
         * @brief On completed callback for Task
         * @return The on completed callback for Task
         */
        std::function<void()> on_completed;
        /**
         * @brief Get the return object for Task
         * @return The Task object
         */
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        /**
         * @brief Initial suspend for Task
         * @return The initial suspend for Task
         */
        std::suspend_always initial_suspend() noexcept { return {}; }
        /**
         * @brief Final awaiter for Task
         */
        struct final_awaiter {
            /**
             * @brief Await ready for Task
             * @return Whether the Task is ready
             */
            bool await_ready() noexcept { return false; }
            /**
             * @brief Await suspend for Task
             * @param h The handle to suspend
             * @return The suspended handle
             */
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                if (h.promise().on_completed) {
                    h.promise().on_completed();
                }
                return h.promise().continuation ? h.promise().continuation : std::noop_coroutine();
            }
            /**
             * @brief Await resume for Task
             */
            void await_resume() noexcept {}
        };
        /**
         * @brief Final suspend for Task
         * @return The final awaiter for Task
         */
        final_awaiter final_suspend() noexcept { return {}; }
        /**
         * @brief Unhandled exception for Task
         */
        void unhandled_exception() { exception = std::current_exception(); }
        /**
         * @brief Return void for Task
         */
        void return_void() {}
    };

    /**
     * @brief Handle type for Task
     */
    using handle_type = std::coroutine_handle<promise_type>;

    Task() noexcept : _coro(nullptr) {}
    /**
     * @brief Constructor with handle
     * @param h The handle to use
     */
    explicit Task(handle_type h) : _coro(h) {}
    /**
     * @brief Constructor with other Task
     * @param other The other Task to use
     */
    Task(Task&& other) noexcept : _coro(std::exchange(other._coro, {})) {}
    /**
     * @brief Operator= for Task
     * @param other The other Task to use
     * @return The Task
     */
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (_coro) _coro.destroy();
            _coro = std::exchange(other._coro, {});
        }
        return *this;
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    ~Task() { if (_coro) _coro.destroy(); }

    /**
     * @brief Await ready for Task
     * @return Whether the Task is ready
     */
    bool await_ready() const noexcept { return !_coro || _coro.done(); }
    /**
     * @brief Await suspend for Task
     * @param awaiting The awaiting coroutine handle
     */
    void await_suspend(std::coroutine_handle<> awaiting) {
        _coro.promise().continuation = awaiting;
        _coro.resume();
    }
    /**
     * @brief Await resume for Task
     */
    void await_resume() {
        if (_coro.promise().exception) {
            std::rethrow_exception(_coro.promise().exception);
        }
    }
    /**
     * @brief Get the result of the Task synchronously
     */
    void get_sync() {
        if (!_coro) {
            throw std::runtime_error("Invalid Task");
        }
        std::mutex m;
        std::condition_variable cv;
        bool done = false;
        _coro.promise().on_completed = [&](){
            std::unique_lock<std::mutex> lk(m);
            done = true;
            lk.unlock();
            cv.notify_one();
        };
        _coro.resume();
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&]{ return done; });
        if (_coro.promise().exception) {
            std::rethrow_exception(_coro.promise().exception);
        }
    }

private:
    handle_type _coro;
};

/**
 * @brief AsyncGenerator with a Folly-compatible API: next() returns Task<optional<T>>
 */
template <typename T>
class AsyncGenerator {
public:
    /**
     * @brief Promise type for AsyncGenerator
     */
    struct promise_type {
        /**
         * @brief Current value for AsyncGenerator
         * @return The current value for AsyncGenerator
         */
        std::optional<T> current;
        /**
         * @brief Exception for AsyncGenerator
         * @return The exception for AsyncGenerator
         */
        std::exception_ptr exception;
        /**
         * @brief Get the return object for AsyncGenerator
         * @return The return object for AsyncGenerator
         */
        AsyncGenerator get_return_object() {
            return AsyncGenerator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        /**
         * @brief Initial suspend for AsyncGenerator
         * @return The initial suspend for AsyncGenerator
         */
        std::suspend_always initial_suspend() noexcept { return {}; }
        /**
         * @brief Final suspend for AsyncGenerator
         * @return The final suspend for AsyncGenerator
         */
        std::suspend_always final_suspend() noexcept { return {}; }
        /**
         * @brief Unhandled exception for AsyncGenerator
         */
        void unhandled_exception() { exception = std::current_exception(); }
        /**
         * @brief Yield value for AsyncGenerator
         * @param value The value to yield
         * @return The yield value for AsyncGenerator
         */
        std::suspend_always yield_value(T value) noexcept {
            current = std::move(value);
            return {};
        }
        /**
         * @brief Return void for AsyncGenerator
         */
        void return_void() {}
    };

    /**
     * @brief Handle type for AsyncGenerator
     */
    using handle_type = std::coroutine_handle<promise_type>;

    /**
     * @brief Constructor with no handle
     */
    AsyncGenerator() noexcept : _coro(nullptr) {}
    /**
     * @brief Constructor with handle
     * @param h The handle to use
     */
    explicit AsyncGenerator(handle_type h) : _coro(h) {}
    /**
     * @brief Constructor with other AsyncGenerator
     * @param other The other AsyncGenerator to use
     */
    AsyncGenerator(AsyncGenerator&& other) noexcept : _coro(std::exchange(other._coro, {})) {}
    /**
     * @brief Operator= for AsyncGenerator
     * @param other The other AsyncGenerator to use
     * @return The AsyncGenerator
     */
    AsyncGenerator& operator=(AsyncGenerator&& other) noexcept {
        if (this != &other) {
            if (_coro) _coro.destroy();
            _coro = std::exchange(other._coro, {});
        }
        return *this;
    }

    AsyncGenerator(const AsyncGenerator&) = delete;
    AsyncGenerator& operator=(const AsyncGenerator&) = delete;
    ~AsyncGenerator() { if (_coro) _coro.destroy(); }

    /**
     * @brief Get the next item from the AsyncGenerator
     * @return The next item from the AsyncGenerator
     */
    Task<std::optional<T>> next() {
        if (!_coro || _coro.done()) {
            if (_coro) {
                _coro.destroy();
                _coro = nullptr;
            }
            co_return std::nullopt;
        }
        _coro.resume();
        if (_coro.promise().exception) {
            auto ex_ptr = _coro.promise().exception;
            _coro.destroy();
            _coro = nullptr;
            std::rethrow_exception(ex_ptr);
        }
        if (_coro.done()) {
            _coro.destroy();
            _coro = nullptr;
            co_return std::nullopt;
        }
        co_return std::move(_coro.promise().current);
    }

private:
    handle_type _coro;
};

/**
 * @brief Helper to run a coroutine and get the result synchronously
 * @tparam T The type of the result
 * @param task The task to run
 * @return The result of the task
 */
template <typename T>
T blockingWait(Task<T>&& task) {
    return task.get_sync();
}

/**
 * @brief Helper to run a coroutine and get the result synchronously
 * @param task The task to run
 */
inline void blockingWait(Task<void>&& task) {
    task.get_sync();
}

/**
 * @brief Helper to collect all results from an AsyncGenerator
 * @tparam T The type of the result
 * @param generator The generator to collect results from
 * @return The results of the generator
 */
template <typename T>
Task<std::vector<T>> collectAllFromGenerator(AsyncGenerator<T>&& generator) {
    std::vector<T> results;
    while (true) {
        auto item = co_await generator.next();
        if (!item) {
            break;
        }
        results.push_back(std::move(*item));
    }
    co_return results;
}

/**
 * @brief Helper to run an async generator and collect results
 * @tparam T The type of the result
 * @param generator The generator to collect results from
 * @return The results of the generator
 */
template <typename T>
std::vector<T> collectAll(AsyncGenerator<T>&& generator) {
    return blockingWait(collectAllFromGenerator(std::move(generator)));
}

/**
 * @brief A minimal executor implementation (fire-and-forget)
 */
class Executor {
public:
    /**
     * @brief Add a function to the executor
     * @tparam F The type of the function to run
     * @param f The function to run
     */
    template <typename F>
    void add(F&& f) {
        std::thread(std::forward<F>(f)).detach();
    }
};

/**
 * @brief Get a global executor
 * @return Pointer to global executor
 */
inline Executor* getExecutor() {
    static Executor executor;
    return &executor;
}

} // namespace agents
