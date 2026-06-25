#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <cstdlib>

// алокатор
class Arena {
private:
    struct Block { Block* next; };
    uint8_t* buffer;
    Block* freeList;
    size_t blockSize;
    size_t totalSize;

public:
    int usedBlocks; // Выносим для быстрого доступа

    Arena(size_t aSize, size_t bSize) : totalSize(aSize), usedBlocks(0) {
        blockSize = (bSize < sizeof(Block*)) ? sizeof(Block*) : bSize;
        blockSize = (blockSize + 7) & ~7;
        buffer = static_cast<uint8_t*>(std::malloc(totalSize));
        if (!buffer) {
            std::cerr << "bad alloc" << std::endl;
            exit(1);
        }

        size_t numBlocks = totalSize / blockSize;
        freeList = nullptr;
        for (size_t i = 0; i < numBlocks; ++i) {
            auto b = reinterpret_cast<Block*>(buffer + (i * blockSize));
            b->next = freeList;
            freeList = b;
        }
    }

    ~Arena() { std::free(buffer); }

    inline void* alloc() {
        if (!freeList) return nullptr;
        void* ptr = freeList;
        freeList = freeList->next;
        usedBlocks++;
        return ptr;
    }

    inline void dealloc(void* ptr) {
        auto b = static_cast<Block*>(ptr);
        b->next = freeList;
        freeList = b;
        usedBlocks--;
    }

    inline bool is_owner(void* ptr) const {
        return (ptr >= buffer && ptr < buffer + totalSize);
    }

    inline bool is_full() const { return freeList == nullptr; }
};

class Alocator {
private:
    // Структура для группы арен одного размера
    struct Bin {
        std::vector<std::unique_ptr<Arena>> pools;
        Arena* lastUsed = nullptr;
        size_t blockSize;

        Bin(size_t size) : blockSize(size) {}

        void* alloc(size_t page_size) {
            if (lastUsed && !lastUsed->is_full()) return lastUsed->alloc();
            for (const auto& pool : pools) {
                if (!pool->is_full()) {
                    lastUsed = pool.get();
                    return lastUsed->alloc();
                }
            }
            pools.push_back(std::make_unique<Arena>(page_size, blockSize));
            lastUsed = pools.back().get();
            return lastUsed->alloc();
        }
    };

    static constexpr size_t MAX_SMALL_SIZE = 512;
    static constexpr size_t PAGE_SIZE = 16 * 1024;
    
    // Биннинг: 16, 32, 64, 128, 256, 512 (степени двойки — самый быстрый поиск)
    std::vector<Bin> bins;

    // Быстрый поиск индекса бина через логарифм (C++20)
    // 0-16 -> 0, 17-32 -> 1, 33-64 -> 2 и т.д.
    size_t get_bin_index(size_t size) const {
        if (size <= 16) return 0;
        // Находим ближайшую степень двойки и вычитаем смещение
        return std::bit_width(size - 1) - 3; 
    }

public:
    Alocator() {
        // Инициализируем корзины для размеров 16, 32, 64, 128, 256, 512
        for (size_t s = 16; s <= MAX_SMALL_SIZE; s <<= 1) {
            bins.emplace_back(s);
        }
    }

    void* alloc(size_t size) noexcept {
        if (size > MAX_SMALL_SIZE) {
            return std::malloc(size); // Для крупных объектов лучше юзать системный аллокатор
        }
        
        size_t index = get_bin_index(size);
        return bins[index].alloc(PAGE_SIZE);
    }

    void dealloc(void* ptr, size_t size) noexcept {
        if (!ptr) return;
        
        // Если мы используем Sized Deallocation без Header, 
        // то ptr здесь — это чистый указатель на объект.
        if (size > MAX_SMALL_SIZE) {
            std::free(ptr);
            return;
        }

        size_t binIdx = get_bin_index(size);
        auto& bin = bins[binIdx];

        // 1. Проверяем кэш (lastUsed)
        if (bin.lastUsed && bin.lastUsed->is_owner(ptr)) {
            bin.lastUsed->dealloc(ptr);
            // Если кэшированная арена опустела, проверим, не пора ли её удалить
            if (bin.lastUsed->usedBlocks == 0) {
                try_shrink_bin(binIdx);
            }
            return;
        }

        // 2. Поиск по всем пулам бина
        for (size_t i = 0; i < bin.pools.size(); ++i) {
            if (bin.pools[i]->is_owner(ptr)) {
                bin.pools[i]->dealloc(ptr);

                if (bin.pools[i]->usedBlocks == 0) {
                    // Если арена пуста, проверяем, нужно ли её удалять
                    size_t emptyCount = 0;
                    for (const auto& p : bin.pools) {
                        if (p->usedBlocks == 0) emptyCount++;
                    }

                    if (emptyCount > 1) {
                        // Сбрасываем кэш, если он указывает на удаляемую арену
                        if (bin.lastUsed == bin.pools[i].get()) {
                            bin.lastUsed = nullptr;
                        }
                        // Удаляем за O(1) через swap с концом вектора
                        std::swap(bin.pools[i], bin.pools.back());
                        bin.pools.pop_back();
                    }
                }
                return;
            }
        }
    }

    // Вспомогательная функция для очистки кэша (опционально)
    void try_shrink_bin(size_t binIdx) noexcept {
        auto& bin = bins[binIdx];
        size_t emptyCount = 0;
        int targetIdx = -1;

        for (size_t i = 0; i < bin.pools.size(); ++i) {
            if (bin.pools[i]->usedBlocks == 0) {
                emptyCount++;
                targetIdx = i;
            }
        }

        if (emptyCount > 1 && targetIdx != -1) {
            if (bin.lastUsed == bin.pools[targetIdx].get()) bin.lastUsed = nullptr;
            std::swap(bin.pools[targetIdx], bin.pools.back());
            bin.pools.pop_back();
        }
    }
};
auto memory = Alocator();
template <typename T>
struct LinearPoolAllocator {
    using value_type = T;

    LinearPoolAllocator() = default;

    // Шаблонный конструктор для "перекраски" аллокатора под нужды shared_ptr
    template <typename U>
    LinearPoolAllocator(const LinearPoolAllocator<U>&) noexcept {}

    // Выделение памяти: n — это количество объектов (блоков)
    T* allocate(std::size_t n) {
        // shared_ptr запросит n=1, но sizeof(T) будет равен (счетчик + объект)
        void* ptr = memory.alloc(n * sizeof(T));
        if (!ptr) {
            std::cerr << "bad alloc" << std::endl;
            exit(1);
        }
        return static_cast<T*>(ptr);
    }

    // Освобождение памяти
    void deallocate(T* ptr, std::size_t n) noexcept {
        memory.dealloc(ptr, n*sizeof(T));
    }
};

// Сравнение для совместимости с STL
template <typename T, typename U>
bool operator==(const LinearPoolAllocator<T>&, const LinearPoolAllocator<U>&) { return true; }
template <typename T, typename U>
bool operator!=(const LinearPoolAllocator<T>&, const LinearPoolAllocator<U>&) noexcept { 
    return false; 
}
