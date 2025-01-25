/*
 * Copyright 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "shrwlock.h"
#include <functional>
#include <mutex>

#ifdef WIN32
#include <string>
#include <windows.h>
#endif

#include "offset_ptr.h"

/**
 * @brief Metadata structure for shared memory management.
 *
 * This structure contains information necessary for managing shared memory,
 * including versioning, allocation size, and mutex synchronization.
 */
struct shmem_meta {
    std::size_t version{1}; //!< the version of the shared memory.
    std::size_t alloc_size{1024*1024}; //!< the size of the allocated memory
    std::size_t first_free{0}; //!< position of the first free block
    shrwlock meta_mutex; //!< the lock of the meta mutex
};

/**
 * @brief Metadata structure for persistently saved data.
 *
 * This structure contains information necessary for managing saved data,
 * including versioning, allocation size.
 */
struct dump_meta {
    std::size_t version{1}; //!< the version of the saved data.
    std::size_t alloc_size{1024*1024}; //!< the size of the saved data (including unoccupied memory).
    std::size_t first_free{0}; //!< the first unused block of memory.
};

/**
 * @brief Manages shared memory segments with metadata and data handling.
 *
 * The `shmem` class provides an interface for creating, accessing, and managing
 * shared memory segments. It handles metadata and data segments, allowing for
 * atomic access, resizing, and allocation within shared memory.
 */
class shmem {
    std::size_t local_version{1}; //!< This is the version this process is currently working with on access we check if the version in the metadata segment has changed
    std::size_t local_size{0}; //!< This is the size of the version this process is currently working with

    shmem_meta* metadata{nullptr}; //!< This is one shared memory segment that is fixed in size and contains the metadata
    void* base{nullptr}; //!< This is the second shared memory segment that is variable in size and contains the actual data

#ifdef WIN32
    HANDLE meta_handle; //<! The handle
    HANDLE data_handle;

    void close_meta_handle();
    void close_data_handle();

    std::string get_handle_name() {
        std::string res = "shmem_data_";
        const auto letter_offset = (this->metadata->version-1) % (26 * 2);
        char letter = 65 + letter_offset + (letter_offset > 25 ? + 6 : 0);
        res.push_back(letter);
        return res;
    }
#endif

    void attach(std::function<void(shmem&)> on_create);
    void close();

    void ensure_mounted(); //!< Ensures that the newest version of the shared memory is mounted
    bool can_fit(std::size_t required_memory, std::size_t required_alignment); //!< This function is used to check if the required size can fit in the base segment
    void ensure_enough_space(std::size_t required_memory, std::size_t required_alignment); //!< This function is used to resize the base segment if the required size is larger than the current size
    void atomic_access(offset_t offset, std::function<void(void*)>&& lambda, bool locks_held = false); //!< This function is used to access the data segment in an atomic way
    offset_t allocate(std::size_t size, std::size_t alignment, bool lock_held = false); //!< This function is used to allocate a new block of memory in the base segment
    void deallocate(offset_t ptr, bool lock_held = false); //!< This function is used to allocate a new block of memory in the base segment
public:
    shmem(); //!< The default constructor
    /**
    * A Constructor that takes a callback.
    * @param on_create A Function that is called after successfully constructing the shared memory.
    */
    shmem(std::function<void(shmem&)> on_create);
    ~shmem(); //!< The destructor
    void destroy(); //!< Destroys the shared memory
    /**
     * Be careful with this function, it will destroy the shared memory segment and all data in it
     */
    static void destroy_data();

    /**
    * Used to obtain the underlying lock
    * @return The lock
    */
    shrwlock& get_meta_mutex();

    /**
    * Used to get the offset of the data. I.e. the size of the allocation/management header
    * @return the offset to the data
    */
    offset_t get_management_offset() const;
    /**
    * Used to get the used memory. I.e. the size of the allocated memory minus the size of the free/unused memory.
    * @return the amount of used memory
    */
    offset_t get_used_memory();

    /**
     * @neverInAtomicAccess
     *
     * Constructs an object in shared memory
     * @tparam T the type of the Object to construct
     * @param args the arguments to pass to the Objects constructor
     * @return An offset pointer to the newly constructed object
     */
    template <typename T, typename... Args>
    offset_ptr<T> construct(Args&&... args) {
        auto storage = allocate(sizeof(T), alignof(T));
        offset_ptr<T> ptr{storage};
        atomic_access(storage, [&args...](void *ptr) {
            new (ptr) T(std::forward<Args>(args)...);
        });
        return ptr;
    }

    /**
     * *Should **only** be called while holding a unique lock!*
     *
     * Constructs an object in shared memory in an unsafe way
     * @tparam T the type of the Object to construct
     * @param args the arguments to pass to the Objects constructor
     * @return An offset pointer to the newly constructed object
     */
    template <typename T, typename... Args>
    offset_ptr<T> construct_unsafe(Args&&... args) {
        auto storage = allocate(sizeof(T), alignof(T), true);
        offset_ptr<T> ptr{storage};
        atomic_access(storage, [&args...](void *ptr) {
            new (ptr) T(std::forward<Args>(args)...);
        }, true);
        return ptr;
    }

    /**
     * @neverInAtomicAccess
     *
     * Constructs an array in shared memory
     * @tparam T the type of the Objects to construct. Must have a default constructor.
     * @return An offset pointer to the newly constructed array
     */
    template <typename T>
    offset_ptr<T> construct_array(size_t count) {
        auto storage = allocate(sizeof(T) * count, alignof(T));
        offset_ptr<T> ptr{storage};
        atomic_access(storage, [count](void *ptr) {
            for (size_t i = 0; i < count; i++) {
                new (reinterpret_cast<uint8_t*>(ptr) + i * sizeof(T)) T();
            }
        });
        return ptr;
    }

    /**
    * *Should **only** be called while holding a unique lock!*
     *
     * Constructs an array in shared memory in an unsafe way.
     * @tparam T the type of the Objects to construct. Must have a default constructor.
     * @param count the length of the array
     * @return An offset pointer to the newly constructed array
     */
    template <typename T>
    offset_ptr<T> construct_array_unsafe(size_t count) {
        auto storage = allocate(sizeof(T) * count, alignof(T), true);
        offset_ptr<T> ptr{storage};
        atomic_access(storage, [count](void *ptr) {
            for (size_t i = 0; i < count; i++) {
                new (reinterpret_cast<uint8_t*>(ptr) + i * sizeof(T)) T();
            }
        }, true);
        return ptr;
    }

    /**
     * Destroys an object in shared memory.
     * @tparam the Type of the object
     * @param ptr an offset pointer to the object
     * @param locks_held defaults to false. should be true if the caller already holds the lock (i.e. inside an atomic access block)
     */
    template<typename T>
    void destroy(offset_ptr<T> ptr, bool locks_held = false) { // Using this inside an atomic_access lambda will cause a deadlock
        if (ptr.offset == -1) return;
        atomic_access(ptr.offset, [](void *ptr) {
            reinterpret_cast<T*>(ptr)->~T();
        }, locks_held);
        deallocate(ptr.offset, locks_held);
    }

    /**
     * Destroys an array in shared memory.
     * @tparam the Type of the objects
     * @param ptr an offset pointer to the object
     * @param size the length of the array
     * @param locks_held defaults to false. should be true if the caller already holds the lock (i.e. inside an atomic access block)
    */
    template<typename T>
    void destroy_array(offset_ptr<T> ptr, size_t size, bool locks_held = false) { // Using this inside an atomic_access lambda will cause a deadlock
        if (ptr.offset == -1) return;
        atomic_access(ptr.offset, [](void *ptr) {
            reinterpret_cast<T*>(ptr)->~T();
        }, locks_held);
        deallocate(ptr.offset, locks_held);
    }

    /**
     * Converts a relative offset pointer to an offset pointer
     * @param ptr a reference to a relative offset pointer
     * @return the offset pointer
     */
    template<typename T>
    offset_ptr<T> relative_to_offset(relative_offset_ptr<T> &ptr) const {
        return offset_ptr<T> (reinterpret_cast<offset_t>(ptr.get()) - reinterpret_cast<offset_t>(base));
    }

    template <typename T>
    friend class offset_ptr;

    using dump_t = std::pair<dump_meta, std::vector<uint8_t>>;
    /**
    * Serializes the data
    * @return the serialized data
    */
    dump_t dump_data();
    //! Deserializes the data
    void load_data(const dump_t& data);
};

template <typename T>
template <typename Lambda>
void offset_ptr<T>::atomic_access(shmem& mem, Lambda&& value, bool locks_held) {
    static_assert(std::is_invocable_v<Lambda, T*>, "Lambda must be invocable with T*");
    mem.atomic_access(this->offset, [value](void *ptr) {
        value(reinterpret_cast<T *>(ptr));
    }, locks_held);
}

template<typename T>
T *offset_ptr<T>::get(shmem &mem) const {
    return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(mem.base) + offset);
}