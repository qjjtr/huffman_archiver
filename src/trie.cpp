#include "trie.h"

Trie::Trie() : root_(std::make_unique<Trie::Node>()) {
}

Trie::Trie(uint64_t c) : Trie() {
    root_->is_terminal = true;
    root_->symbol = c;
}

Trie::Trie(Trie&& left, Trie&& right) : Trie() {
    root_->left = std::move(left.root_);
    root_->left->parent = root_.get();

    root_->right = std::move(right.root_);
    root_->right->parent = root_.get();
}

Trie::Trie(const Trie& other) : Trie() {
    CopyNodesRecursively(other.root_, root_);
}

void Trie::CopyNodesRecursively(const std::unique_ptr<Node>& original, std::unique_ptr<Node>& node, Node* parent) {
    node->is_terminal = original->is_terminal;
    node->symbol = original->symbol;
    node->parent = parent;
    if (original->left) {
        node->left = std::make_unique<Trie::Node>();
        CopyNodesRecursively(original->left, node->left, node.get());
    }
    if (original->right) {
        node->right = std::make_unique<Trie::Node>();
        CopyNodesRecursively(original->right, node->right, node.get());
    }
}

Trie& Trie::operator=(const Trie& other) {
    Trie new_trie(other);
    std::swap(*this, new_trie);
    return *this;
}

void Trie::AddBranch(uint64_t path, size_t length, uint64_t symbol) {
    AddBranch(path, length, symbol, root_.get());
}

void Trie::AddBranch(uint64_t path, size_t length, uint64_t symbol, Node* node) {
    if (length == 0) {
        node->is_terminal = true;
        node->symbol = symbol;
        return;
    }
    bool son_value = ((path >> (length - 1)) & 1);
    auto& son = son_value ? node->right : node->left;
    if (!son) {
        son = std::make_unique<Node>();
        son->parent = node;
    }
    AddBranch(path, length - 1, symbol, son.get());
}

TrieRunner::TrieRunner(const Trie& trie) : root_(trie.root_.get()), current_(root_) {
}

bool TrieRunner::IsInTerminal() const {
    return current_->is_terminal;
}

uint64_t TrieRunner::GetCurrentSymbol() const {
    return current_->symbol;
}

bool TrieRunner::HasChild(bool value) const {
    return static_cast<bool>(value ? current_->right : current_->left);
}

void TrieRunner::GoToChild(bool value) {
    current_ = (value ? current_->right.get() : current_->left.get());
}

void TrieRunner::GoToParent() {
    if (current_->parent) {
        current_ = current_->parent;
    }
}

void TrieRunner::GoToRoot() {
    current_ = root_;
}
