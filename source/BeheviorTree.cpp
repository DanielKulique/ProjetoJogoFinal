#include "BehaviorTree.hpp"

ActionNode::ActionNode(std::string command)
    : m_command(std::move(command)) {}

BTResult ActionNode::tick(const BTContext&) const {
    return {true, m_command};
}

ConditionNode::ConditionNode(std::string name)
    : m_name(std::move(name)) {}

BTResult ConditionNode::tick(const BTContext& context) const {
    for (const auto& item : context) {
        if (item.first == m_name && item.second != 0) {
            return {true, ""};
        }
    }
    return {false, ""};
}

SequenceNode::SequenceNode(std::vector<BTNodePtr> children)
    : m_children(std::move(children)) {}

BTResult SequenceNode::tick(const BTContext& context) const {
    BTResult result{true, ""};
    for (const auto& child : m_children) {
        BTResult current = child->tick(context);
        if (!current.success) {
            return {false, ""};
        }
        if (!current.command.empty()) {
            result.command = std::move(current.command);
        }
    }
    return result;
}

SelectorNode::SelectorNode(std::vector<BTNodePtr> children)
    : m_children(std::move(children)) {}

BTResult SelectorNode::tick(const BTContext& context) const {
    for (const auto& child : m_children) {
        BTResult result = child->tick(context);
        if (result.success) {
            return result;
        }
    }
    return {false, ""};
}

BehaviorTree::BehaviorTree(BTNodePtr root)
    : m_root(std::move(root)) {}

std::string BehaviorTree::tick(const BTContext& context) const {
    if (!m_root) {
        return "";
    }
    BTResult result = m_root->tick(context);
    return result.success ? result.command : "";
}
