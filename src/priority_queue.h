#include <functional>
#include <vector>

template <typename T, typename Comparator = std::less<T>>
class PriorityQueue {
public:
    PriorityQueue() = default;
    PriorityQueue(std::initializer_list<T> list);
    explicit PriorityQueue(const std::vector<T>& data);

    PriorityQueue(const PriorityQueue<T, Comparator>& other) = default;
    PriorityQueue(PriorityQueue<T, Comparator>&& other) = default;

    PriorityQueue<T, Comparator>& operator=(const PriorityQueue<T, Comparator>& other) = default;
    PriorityQueue<T, Comparator>& operator=(PriorityQueue<T, Comparator>&& other) = default;

    bool Empty() const;
    size_t Size() const;

    const T& Top() const;

    void Push(const T& value);
    void Pop();

private:
    void ShiftDown(size_t id);

private:
    std::vector<T> data_;
    Comparator comparator_;
};

template <typename T, typename Comparator>
PriorityQueue<T, Comparator>::PriorityQueue(std::initializer_list<T> list) : PriorityQueue(std::vector<T>(list)) {
}

template <typename T, typename Comparator>
PriorityQueue<T, Comparator>::PriorityQueue(const std::vector<T>& data) : data_(data) {
    size_t count = data_.size() / 2;
    for (size_t i = 0; i <= count; ++i) {
        ShiftDown(count - i);
    }
}

template <typename T, typename Comparator>
bool PriorityQueue<T, Comparator>::Empty() const {
    return Size() == 0;
}

template <typename T, typename Comparator>
size_t PriorityQueue<T, Comparator>::Size() const {
    return data_.size();
}

template <typename T, typename Comparator>
const T& PriorityQueue<T, Comparator>::Top() const {
    return data_.at(0);
}

template <typename T, typename Comparator>
void PriorityQueue<T, Comparator>::Push(const T& value) {
    data_.push_back(value);

    size_t current_id = data_.size() - 1;
    while (current_id > 0) {
        size_t parent_id = (current_id - 1) / 2;
        if (!comparator_(data_[parent_id], data_[current_id])) {
            break;
        }
        std::swap(data_[parent_id], data_[current_id]);
        current_id = parent_id;
    }
}

template <typename T, typename Comparator>
void PriorityQueue<T, Comparator>::Pop() {
    std::swap(data_[0], data_.back());
    data_.pop_back();
    ShiftDown(0);
}

template <typename T, typename Comparator>
void PriorityQueue<T, Comparator>::ShiftDown(size_t id) {
    while (2 * id + 1 < data_.size()) {
        size_t max_son = 2 * id + 1;
        if (max_son + 1 < data_.size() && comparator_(data_[max_son], data_[max_son + 1])) {
            ++max_son;
        }
        if (!comparator_(data_[id], data_[max_son])) {
            break;
        }
        std::swap(data_[id], data_[max_son]);
        id = max_son;
    }
}
