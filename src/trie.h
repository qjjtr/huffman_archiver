#include <memory>

class Trie {
    friend class TrieRunner;

public:
    Trie();
    explicit Trie(uint64_t c);
    Trie(Trie&& left, Trie&& right);

    Trie(const Trie& other);
    Trie& operator=(const Trie& other);
    Trie(Trie&& other) = default;
    Trie& operator=(Trie&& other) = default;

    void AddBranch(uint64_t path, size_t length, uint64_t symbol);

private:
    struct Node {
        bool is_terminal = false;
        uint64_t symbol;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        Node* parent = nullptr;
    };

    static void CopyNodesRecursively(const std::unique_ptr<Node>& original, std::unique_ptr<Node>& node,
                                     Node* parent = nullptr);
    void AddBranch(uint64_t path, size_t length, uint64_t symbol, Node* node);

private:
    std::unique_ptr<Node> root_;
};

class TrieRunner {
public:
    explicit TrieRunner(const Trie& trie);

    bool IsInTerminal() const;
    uint64_t GetCurrentSymbol() const;

    bool HasChild(bool value) const;
    void GoToChild(bool value);
    void GoToParent();
    void GoToRoot();

private:
    const Trie::Node* root_;
    const Trie::Node* current_;
};
